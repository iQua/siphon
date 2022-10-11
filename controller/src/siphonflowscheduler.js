/**
 * @file logger.js
 * @brief A helper utility to print inter-process messages.
 * @author Shuhao Liu <shuhao@ece.toronto.edu>
 *
 * @date 2016-03-22
 */

'use strict';

var Redis = require('ioredis');
const MsgFactory = require('./controlmsg.js');

// The SparkFlowScheduler Class
var SparkFlowScheduler = function(algorithm) {
  if (!(this instanceof SparkFlowScheduler)) {
    return new SparkFlowScheduler(algorithm);
  }

  this._redis = new Redis({
    "connectTimeout": 1000
  });
  this._dbConn = new Redis({
    "connectTimeout": 1000
  });

  this._algorithm = algorithm;

  this._redis.subscribe('newShuffleDetails', (err, count) => {
    if (err || count !== 1) {
      console.error('[SparkFlowScheduler|ERROR] Subscription Failed' + err.toString());
      process.exit(1);
    }

    this._redis.on('message', (channel, msg) => {
      console.log(msg);
      return;

      switch(this._algorithm) {
        case 'fair':
        case 'maxFirst':
        case 'minFirst':
          this._dbConn.hgetall('NodeID2Hostname')
              .bind(this)
              .then(function(result) {
                var onlineNodes = Object.keys(result);
                for (var fetch in msg) {
                  var reply = this.getReplyMessage(fetch);
                  for (var node in onlineNodes) {
                    this._dbConn.publish(node, reply);
                  }
                }
              });
          break;

        default:
          console.error(msg);
          break;
      }
    });
  });
};

SparkFlowScheduler.prototype.getReplyMessage = function (msg) {
  var sessionID = msg['fetcher'];
  var weight = 0.0;
  for (var flow in msg["report"]) {
    weight = weight + flow["size"];
  }
  weight = weight / 1024 / 1024;
  return MsgFactory.generateSessionWeight(this._algorithm, sessionID, weight);
};

SparkFlowScheduler.prototype.close = function() {
  this._dbConn.shutdown();
  this._redis.shutdown();
};

module.exports = {
  FlowScheduler: SparkFlowScheduler
};
