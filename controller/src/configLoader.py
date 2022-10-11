import os.path
import json
import importlib
import redis

class ConfigLoader(object):
    ''' A class to load configuration file info (especially session info) to
    Redis database, for the convenience of doing experiments '''

    def __init__(self, redis_conn):
        self.redis_conn = redis_conn
        self.pipe = self.redis_conn.pipeline()

    def load_config(self):
        if (os.path.exists('./testconfig.json')):
            try:
                # Open configuration file and converts it from json to directory
                with open('./testconfig.json') as json_file:
                    config_data = json.load(json_file)
                    print (config_data)

                # Store session info to Redis database
                # The session info format: (string)
                # 'SessionID:ID': {'src': src, 'dsts': dsts}
                session_info_list = config_data['PseudoSessions']
                for session in session_info_list:
                    session_info = {'src': session['src'],
                                    'dsts': session['dsts']}
                    self.pipe.multi()
                    session_info_json = json.dumps(session_info,
                                                   separators=(',', ':'))
                    session_info_key = 'SessionID:' + str(session['SessionID'])
                    self.pipe.set(session_info_key, session_info_json)
                    self.pipe.execute()

                # Check to use which RoutingScheme, and
                # Import corresponding routing module
                routing_scheme = config_data['RoutingScheme']
                routing_module = importlib.import_module(
                    'applications.'+routing_scheme)
                return routing_module

            except IOError as e:
                print ("Unable to open file")

        else:
            return False

if __name__ == '__main__':
    conn = redis.StrictRedis()
    test = ConfigLoader(conn)
    test.load_config()