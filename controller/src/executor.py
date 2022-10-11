import multiprocessing as mp
import redis
import argparse
import logging
import sys
import json
import os
import importlib

from workers import NewSessionWorker, RoutingInfoWorker
from configLoader import ConfigLoader

def run_worker(q, routing_scheme):

    log_info = '[EMaster|{0:<5}|{1:<5}] Initialize Workers for ' \
               'Events'.format(os.getpid(), 'DEBUG')
    redis_conn.publish('DEBUG', log_info)
    newSessionWorker = NewSessionWorker()
    routingInfoWorker = RoutingInfoWorker(routing_scheme)

    while 1:
        # q.get() will block if necessary until an item is available
        msg = q.get()
        if msg:
            # msg is a tuple (channel, data)
            # Process the message
            data = json.loads(msg[1])
            if msg[0] == 'querySessionID':
                # Trigger newSession handler to process
                log_info = '[EMaster|{0:<5}|{1:<5}] handles ' \
                           '*querySessionID* event {2}'.format(os.getpid(),
                                                               'DEBUG', data)
                redis_conn.publish('DEBUG', log_info)
                newSessionWorker.process(data)
            elif msg[0] == 'routingInfo':
                # Trigger routingInfo handler to process
                log_info = '[EMaster|{0:<5}|{1:<5}] handles ' \
                           '*routingInfo* event {2}'.format(os.getpid(),
                                                            'DEBUG', data)
                redis_conn.publish('DEBUG', log_info)
                routingInfoWorker.process(data)
            else:
                # Leave space for new event
                pass

if __name__ == '__main__':
    # Use argparse module to parse arguments from command line.
    parser = argparse.ArgumentParser(description='Showing debug info.')
    parser.add_argument('-d', '--debug', action='store_const',
                        dest='loglevel', const=logging.DEBUG,
                        default=logging.INFO)
    args = parser.parse_args()

    try:
        # Create connection to Redis and subscribe
        redis_conn = redis.StrictRedis()
        log_info = '[EMaster|{0:<5}|{1:<5}] Connects to Redis '.format(
            os.getpid(), 'INFO')
        redis_conn.publish('INFO', log_info)
        redis_pubsub = redis_conn.pubsub()
        redis_pubsub.subscribe('querySessionID', 'routingInfo')

        # Set SessionID as a global variable and all processes can track it
        # The SessionID number is started from 1001, thus we can distinguish
        # SessionID allocated in the configuration file from SessionID
        # assigned by the controller
        redis_conn.set('SessionID', 1000)

        # Load configuration file first, if it exists
        # Check RoutingScheme specified in configuration file
        default_routing_module = importlib.import_module(
            'applications.baseRouting')
        config_loader = ConfigLoader(redis_conn)
        routing_module = config_loader.load_config()
        if routing_module:
            log_info = '[EMaster|{0:<5}|{1:<5}] Configuration file loaded ' \
                       'successfully'.format(os.getpid(), 'INFO')
            redis_conn.publish('INFO', log_info)
        else:
            routing_module = default_routing_module
            log_info = '[EMaster|{0:<5}|{1:<5}] Configuration file does not ' \
                       'exist or load failed'.format(os.getpid(), 'INFO')
            redis_conn.publish('INFO', log_info)
        routing_scheme = routing_module.Routing(redis_conn)

        # Start processes
        log_info = '[EMaster|{0:<5}|{1:<5}] Starts Processes '.format(
            os.getpid(), 'DEBUG')
        redis_conn.publish('DEBUG', log_info)
        msg_queue = mp.Queue()
        process_list = []
        for i in range(mp.cpu_count()):
            p = mp.Process(target=run_worker, args=(msg_queue, routing_scheme))
            process_list.append(p)
            p.start()

        while 1:
            # ***BLOCKING*** read from the subscription
            # Push to the queue
            for msg in redis_pubsub.listen():
                # msg format: (dictionary)
                # {'channel': 'channel-name', 'data': 'some data', 'pattern':
                #  None, 'type': 'message'}
                if msg['type'] == 'message':
                    try:
                        # put a modified message ('channel-name', 'some data')
                        # since we use python3, the type of channel and data
                        # is bytes, we need to decode them to utf-8
                        msg_queue.put((msg['channel'].decode('utf-8'),
                                       msg['data'].decode('utf-8')))
                    except:
                        log_info = '[EMaster|{0:<5}|{1:<5}] Unexpected Error: '\
                                   '{2}'.format(os.getpid(), 'ERROR',
                                                sys.exc_info()[0])
                        redis_conn.publish('ERROR', log_info)
                        raise

    except KeyboardInterrupt:
        log_info = '\n[EMaster|{0:<5}|{1:<5}] *** Unsubscribe all channels ' \
                   '*** \n'.format(os.getpid(), 'INFO')
        redis_conn.publish('INFO', log_info)
        redis_pubsub.unsubscribe()
        redis_pubsub.close()

        log_info = '\n[EMaster|{0:<5}|{1:<5}] *** Shutting down processes ' \
                   '***\n'.format(os.getpid(), 'INFO')
        redis_conn.publish('INFO', log_info)
        for process in process_list:
            process.terminate()
            process.join()

    except:
        log_info = '[EMaster|{0:<5}|{1:<5}] Error: {2}'.format(os.getpid(),
                   'ERROR', sys.exc_info()[0])
        print (log_info)
        log_info = '[EMaster|{0:<5}|{1:<5}] If the error is ' \
                   'redis.exceptions.ConnectionError, please connect to ' \
                   'redis-server first.'.format(os.getpid(), 'ERROR')
        print (log_info)

        log_info = '\n[EMaster|{0:<5}|{1:<5}] *** Shutting down processes ' \
                   '***\n'.format(os.getpid(), 'INFO')
        print (log_info)
        for process in process_list:
            process.terminate()
            process.join()
        # Since there is an exception, the exit code should not be 0
        sys.exit(1)