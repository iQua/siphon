import redis
import json
import os

from applications import ControlMsgTypes

class WorkerBase(object):
    ''' A base worker class for each event '''

    def __init__(self):
        self.redis_conn = redis.StrictRedis()
        self.pipe = self.redis_conn.pipeline()

    def process(self):
        # do something
        self.submit_orders()

    def submit_orders(self):
        pass


class NewSessionWorker(WorkerBase):
    ''' A class to handle newSession event '''

    def __init__(self):
        WorkerBase.__init__(self)

    def process(self, msg):
        # Get QuerySessionID msg and process it
        src = msg['src']
        dsts = msg['dsts']

        # Get SessionID from db, increment SessionID, store session info (
        # including src and dsts info, which are contained in the msg)
        sessionID = self.redis_conn.incr('SessionID')
        self.pipe.multi()
        sessionInfo_json = json.dumps(msg, separators=(',',':'))
        sessionInfo_key = 'SessionID:' + str(sessionID)
        self.pipe.set(sessionInfo_key, sessionInfo_json)
        self.pipe.execute()

        log_info = '[EWorker|{0:<5}|{1:<5}] SessionID {2} is assigned for ' \
                   'src {3} dsts {4}'.format(os.getpid(), 'DEBUG', sessionID,
                                             src, dsts)
        self.redis_conn.publish('DEBUG', log_info)

        # Generate corresponding newSession msg and publish to the db
        msg['SessionID'] = sessionID
        newSessionMsg = {'Type': ControlMsgTypes.NewSession.value, 'Msg': msg}
        log_info = '[EWorker|{0:<5}|{1:<5}] For session {2}, the ' \
                   '*newSession* msg is {3}'.format(os.getpid(),
                                                    'DEBUG', sessionID,
                                                    newSessionMsg)
        self.redis_conn.publish('DEBUG', log_info)
        newSessionMsg_json = json.dumps(newSessionMsg, separators=(',',':'))

        # Publish newSession msg to the db
        self.submit_orders(sessionID, newSessionMsg_json, src, dsts)

    def submit_orders(self, sessionID, msg, src, dsts):
        # Publish the msg to corresponding channels: src and all dsts
        # src:
        channel_src = str(src)
        self.redis_conn.publish(channel_src, msg)
        # dsts:
        channel_dsts = []
        for dst in dsts:
            channel_dsts.append(str(dst))
        for channel_dst in channel_dsts:
            self.redis_conn.publish(channel_dst, msg)

class RoutingInfoWorker(WorkerBase):
    ''' A class to handle newSession event '''

    def __init__(self, routing_scheme):
        WorkerBase.__init__(self)
        self.routing_scheme = routing_scheme

    def process(self, msg):
        # Extract sessionID from msg, and get session info from the db
        sessionID = msg['SessionID']

        routingInfoMsg = {'Type': ControlMsgTypes.RoutingInfo.value,
                          'Msg': {'SessionID': sessionID,
                          'Entry': [1]}}
        routingInfoMsg_json = json.dumps(routingInfoMsg,
                separators=(',',':'))

        # Publish
        self.submit_orders(routingInfoMsg_json, 2, [1], sessionID,
                           sessionInfo)
        return

        # Check whether the RoutingInfo for this session has already
        # published to all involved nodes
        has_already_sent_key = 'SessionID:' + str(sessionID) + 'RoutingInfoSent'
        if not self.redis_conn.get(has_already_sent_key):
            sessionInfo_key = 'SessionID:' + str(sessionID)
            sessionInfo_str = self.redis_conn.get(sessionInfo_key)
            if not sessionInfo_str:
                sessionInfo_str = self.redis_conn.get(
                        sessionInfo_key.split('@')[0])

            if not sessionInfo_str:
                log_info = '[EWorker|{0:<5}|{1:<5}] SessionID {2} does not ' \
                           'exist'.format(os.getpid(), 'ERROR', sessionID)
                self.redis_conn.publish('ERROR', log_info)
            else:
                sessionInfo_json = sessionInfo_str.decode('utf-8')
                sessionInfo = json.loads(sessionInfo_json)
                src = sessionInfo['src']
                dsts = sessionInfo['dsts']
                log_info = '[EWorker|{0:<5}|{1:<5}] Session {2} Info: src {3} ' \
                           'dsts {4}'.format(os.getpid(), 'DEBUG', sessionID,
                                             src, dsts)
                self.redis_conn.publish('DEBUG', log_info)

                # RoutingScheme will find the path, and return entry directly
                entry = self.routing_scheme.routing_algorithm(src, dsts)

                # Pack routingInfoMsg in JSON form
                routingInfoMsg = {'Type': ControlMsgTypes.RoutingInfo.value,
                                  'Msg': {'SessionID': sessionID,
                                          'Entry': entry}}
                log_info = '[EWorker|{0:<5}|{1:<5}] For session {2}, ' \
                           'RoutingInfo is {3}'.format(os.getpid(), 'DEBUG',
                                                       sessionID,
                                                       routingInfoMsg)
                self.redis_conn.publish('DEBUG', log_info)
                routingInfoMsg_json = json.dumps(routingInfoMsg,
                                                 separators=(',',':'))

                # Publish
                self.submit_orders(routingInfoMsg_json, src, dsts, sessionID,
                                   sessionInfo)
        else:
            log_info = '[EWorker|{0:<5}|{1:<5}] The RoutingInfo for Session {' \
                       '2} has already published to all involved ' \
                       'nodes'.format(os.getpid(), 'DEBUG', sessionID)
            self.redis_conn.publish('DEBUG', log_info)
            return

    def submit_orders(self, msg, src, dsts, sessionID, sessionInfo):
        # Publish routingInfo to the channel of all involved nodes
        # src
        channel_src = str(src)
        self.redis_conn.publish(channel_src, msg)

        # dsts
        # Get existing node info from database
        existing_node_set = self.redis_conn.hkeys('NodeID2Hostname')
        existing_node_list = []
        for nodeID in existing_node_set:
            existing_node_list.append(int(nodeID.decode('utf-8')))

        log_info = '[EWorker|{0:<5}|{1:<5}] Existing node: {2}'.format(
            os.getpid(), 'DEBUG', existing_node_list)
        self.redis_conn.publish('DEBUG', log_info)

        existing_dsts_list = [nodeID for nodeID in dsts if nodeID in
                              existing_node_list]

        log_info = '[EWorker|{0:<5}|{1:<5}] Existing dsts: {2}'.format(
            os.getpid(), 'DEBUG', existing_dsts_list)
        self.redis_conn.publish('DEBUG', log_info)

        # publish RoutingInfo to the channel of existing dsts
        for nodeID in existing_dsts_list:
            channel_dst = str(nodeID)
            self.redis_conn.publish(channel_dst, msg)

        # If all involved dsts are online, set expire time for this session to
        # avoid publishing the same session info repeatedly
        if existing_dsts_list == dsts:
            log_info = '[EWorker|{0:<5}|{1:<5}] For Session {2}, all involved' \
                       ' nodes are online'.format(os.getpid(), 'DEBUG',
                                                  sessionID)
            self.redis_conn.publish('DEBUG', log_info)
            if 'Timeout' in sessionInfo.keys():
                timeout = sessionInfo['Timeout']
                if timeout == 0:
                    timeout = 15
            else:
                timeout = 15
            self.pipe.multi()
            has_already_sent_key = 'SessionID:' + str(sessionID) + \
                                   'RoutingInfoSent'
            self.pipe.set(has_already_sent_key, 1)
            self.pipe.expire(has_already_sent_key, timeout)
            self.pipe.execute()


