# /**
#  * @file test_multiworkers.py
#  * @brief Test whether python part of the controller can work properly.
#  *        Generate some fake datapath workers.
#  * @author Yinan Liu <yinan@ece.toronto.edu>
#  *
#  * @date 2016-03-07
#  */

import redis
import time
import multiprocessing as mp
import threading
import logging
import json

logger = mp.log_to_stderr()
logger.setLevel(logging.DEBUG)

class DatapathNode(object):

    def __init__(self, nodeID, node_list):
        self.nodeID = nodeID
        self.node_list = list(node_list)
        self.session_list = []
        self.node_list.remove(self.nodeID)
        logger.debug('node * %s * is up!', self.nodeID)
        logger.debug('node list: %s', self.node_list)
        self.num = 1
        self.redis_conn = redis.StrictRedis()
        self.redis_pubsub = self.redis_conn.pubsub()
        self.channel = str(self.nodeID)
        self.redis_pubsub.subscribe(self.channel)

    def querySessionID(self):
        if self.num <= 10:
            msg = {'src': self.nodeID, 'dsts': [self.node_list[self.num % 2]]}
            msg_json = json.dumps(msg, sort_keys=True, indent=4, separators=(
                ',',':'))
            self.redis_conn.publish('querySessionID', msg_json)
            logger.debug('node * %s * publish a querySessionID msg: %s',
                         self.nodeID, msg)
            self.num += 1
            time.sleep(3)
            self.querySessionID()

    def routingInfoUp(self, sessionID):
        msg = {'SessionID': sessionID}
        msg_json = json.dumps(msg, sort_keys=True, indent=4, separators=(',',
                                                                         ':'))
        self.redis_conn.publish('routingInfo', msg_json)
        logger.debug('node * %s * publish a routingInfo_up msg: %s',
                     self.nodeID, msg)

    def receive(self):
        while 1:
            # for msg in self.redis_pubsub.listen():
            msg = self.redis_pubsub.get_message()
            if msg:
                if msg['type'] == 'message':
                    data = json.loads(msg['data'].decode('utf-8'))
                    if data['Type'] == 'NewSession':
                        self.receiveSessionID(data['Msg'])
                    elif data['Type'] == 'RoutingInfo':
                        self.receiveRouting(data['Msg'])

    def receiveSessionID(self, msg):
        src = msg['src']
        sessionID = msg['SessionID']

        if self.nodeID == src:
            self.session_list.append(sessionID)
            logger.debug('For src * %s * received newSession msg: %s', src, msg)
            self.routingInfoUp(sessionID)
            logger.debug('Raise routingInfo event for session %s', sessionID)
        else:
            logger.debug('As a dst * %s * received newSession msg: %s',
                         self.nodeID, msg)

    def receiveRouting(self, msg):
        sessionID = msg['SessionID']
        if sessionID in self.session_list:
            logger.debug('src * %s * received routingInfo: %s for Session %s ',
                         self.nodeID, msg, sessionID)
        else:
            logger.debug('node * %s * received wrong routingInfo: %s',
                         self.nodeID, msg)

def run_worker(worker):
    datapathNode = worker
    logger.debug('Worker starts to run')

    publish_thread = threading.Thread(target=datapathNode.querySessionID)
    receive_thread = threading.Thread(target=datapathNode.receive)
    publish_thread.start()
    receive_thread.start()
    publish_thread.join()
    receive_thread.join()

if __name__ == '__main__':
    try:
        num_dapapathNode = 3
        node_list = []
        for i in range(num_dapapathNode):
            node_list.append(i+1)
        datapathNodeWorker = []
        process_list = []
        for i in range(num_dapapathNode):
            datapathNodeWorker_ = DatapathNode(i+1, node_list)
            datapathNodeWorker.append(datapathNodeWorker_)
            p = mp.Process(target=run_worker, args=(datapathNodeWorker[i], ))
            process_list.append(p)
            p.start()

        for process in process_list:
            process.join()

    except KeyboardInterrupt:
        logger.debug('Unsubscribe channels ......')
        for datapathNode in datapathNodeWorker:
            datapathNode.redis_pubsub.unsubscribe()
            while datapathNode.redis_pubsub.get_message():
                pass
        time.sleep(3)

        logger.debug('Shutting down processes ......')
        for process in process_list:
            process.terminate()
            process.join()
