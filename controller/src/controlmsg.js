/**
 * @file msgfactory.js
 * @brief The Message Factory.
 * @author Shuhao Liu <shuhao@ece.toronto.edu>
 *
 * @date 2016-02-05
 */

'use strict';

// Enum: MsgTypeEnum
// Listing the control message types
const MsgTypeEnum = {
  kControlMsgNodeOnline: 1,
  kControlMsgNodeOffline: 2,
  kControlMsgNewSession: 3,
  kControlMsgQuerySessionID: 4,
  kControlMsgRoutingInfo: 5,
  kControlMsgReportRTT: 6,
  kControlMsgReportBandwidth: 7,
  kControlMsgSessionSubscribed: 8,
  kControlMsgSetSessionWeight: 9
};

exports.MsgTypeEnum = MsgTypeEnum;

// Function: stringifyMsg
// A utility function used to build stringified control messages easily
function stringifyMsg(type, msg) {
  var json = {
    "Type" : type,
    "Msg" : msg
  };
  return (JSON.stringify(json));
}

exports.generateNodeOnlineMsg = function(nodeList) {
  return stringifyMsg(MsgTypeEnum.kControlMsgNodeOnline, {
    'NewNode': nodeList
  });
};

exports.recordRTTs = function(redis, nodeIDStr, rtts) {
  for (var i = 0; i < rtts.length; i++) {
    redis.hmset(nodeIDStr, rtts[i]['To'], Number(rtts[i]['OneWay']));
  }
};

exports.generateRoutingInfo = function(sessionID, entryList) {
  return stringifyMsg(MsgTypeEnum.kControlMsgRoutingInfo, {
    'SessionID': sessionID,
    'Entry': entryList.map(Number),
    'Timeout': 0
  });
};

exports.generateSessionWeight = function(algorithm, sessionID, weight) {
  return stringifyMsg(MsgTypeEnum.kControlMsgSetSessionWeight, {
    'Algorithm': algorithm,
    'SessionID': sessionID,
    'Weight': weight
  });
};

