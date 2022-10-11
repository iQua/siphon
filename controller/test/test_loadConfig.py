import redis
import json
import time

if __name__ == '__main__':

    test_num = 10
    redis_conn = redis.StrictRedis()

    # Firstly, clean up all the data stored in the database
    redis_conn.flushdb()

    time.sleep(10)

    for sessionID in range(1, test_num+1):
        session_info_key = 'SessionID:' + str(sessionID)
        session_info_str = redis_conn.get(session_info_key)
        if not session_info_str:
            print ('ERROR: The Session', sessionID, 'does not exist.')
        else:
            session_info_json = session_info_str.decode('utf-8')
            session_info = json.loads(session_info_json)
            print ('Session ', sessionID, 'info:', session_info)
