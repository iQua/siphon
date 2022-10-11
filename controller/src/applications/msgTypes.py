from enum import Enum

class ControlMsgTypes(Enum):
    ''' A class to enumerate control message types '''

    NodeOnline = 1
    NodeOffline = 2
    NewSession = 3
    QuerySessionID = 4
    RoutingInfo = 5
    ReportRTT = 6