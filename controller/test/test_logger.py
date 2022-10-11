# /**
#  * @file test_logger.py
#  * @brief Test multiprocessiong logger
#  * @author Yinan Liu <yinan@ece.toronto.edu>
#  *
#  * @date 2016-03-23
#  */

import redis
import time
import json
import threading
import sys

# def receiveSessionID(data):
#     print ('Received *newSession* msg:', data)
#     msg = {'SessionID': data['SessionID']}
#     msg_json = json.dumps(msg, sort_keys=True, indent=4, separators=(',', ':'))
#     redis_conn.publish('routingInfo', msg_json)
#
# def receiveRoutingInfo(data):
#     print ('Received *RoutingInfo* msg:', data)

def listening(redis_pubsub):
    print ('Listening Thread Working ...')
    while 1:
        for msg in redis_pubsub.listen():
            if msg:
                # print ('Message received:', msg)
                if msg['type'] == 'message':
                    if msg['channel'].decode('utf-8') == 'DEBUG':
                        print (msg['data'].decode('utf-8'))
                    elif msg['channel'].decode('utf-8') == 'INFO':
                        print (msg['data'].decode('utf-8'))
                    elif msg['channel'].decode('utf-8') == 'WARN':
                        print (msg['data'].decode('utf-8'))
                    elif msg['channel'].decode('utf-8') == 'ERROR':
                        print (msg['data'].decode('utf-8'))
                    else:
                        data = json.loads(msg['data'].decode('utf-8'))
                        if data['Type'] == 'NewSession':
                            # receiveSessionID(data['Msg'])
                            print (data['Msg'])
                        elif data['Type'] == 'RoutingInfo':
                            # receiveRoutingInfo(data['Msg'])
                            print (data['Msg'])

if __name__ == '__main__':

    try:
        test_num = 10

        # Connect to the redis db
        redis_conn = redis.StrictRedis()

        # Subscribe to 4 logger channels
        redis_pubsub = redis_conn.pubsub()
        redis_pubsub.subscribe('DEBUG', 'INFO', 'WARN', 'ERROR')

        # Allocates a thread to listen
        listen_thread = threading.Thread(target=listening, args=(redis_pubsub, ))
        listen_thread.start()

        # Generate some fake entries
        time.sleep(10)
        query_sessionID_list = []
        for i in range(test_num):
            query_sessionID_msg = {'src': i+1, 'dsts': [i+2]}
            query_sessionID_msg_json = json.dumps(query_sessionID_msg,
                                                  sort_keys=True, indent=4,
                                                  separators=(',',':'))
            query_sessionID_list.append(query_sessionID_msg_json)

        # Publish msgs to querySessionID channel
        for msg in query_sessionID_list:
            redis_conn.publish('querySessionID', msg)
            time.sleep(1)

        # Publish msgs to routingInfo channel
        time.sleep(30)
        for sessionID in range(1, test_num + 1):
            routingInfo_msg = {'SessionID': sessionID}
            routingInfo_msg_json = json.dumps(routingInfo_msg,
                                              sort_keys=True, indent=4,
                                              separators=(',',':'))
            redis_conn.publish('routingInfo', routingInfo_msg_json)
            time.sleep(1)

        time.sleep(10)
        redis_pubsub.unsubscribe()
        listen_thread.join()

    except KeyboardInterrupt:
        redis_pubsub.unsubscribe()
        listen_thread.join()
        sys.exit()