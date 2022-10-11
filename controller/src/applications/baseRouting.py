class Routing(object):
    ''' A base class for Routing Scheme '''

    def __init__(self, redis_conn):
        self.redis_conn = redis_conn

    def routing_algorithm(self, src, dsts):

        # The basic routing algorithm, return the direct path
        entry = []
        for dst in dsts:
            entry.append(int(dst))
        return entry
