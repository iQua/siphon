/**
 * @file connection.js
 * @brief Defining the Connection to the Siphon Switch nodes.
 * @author Shuhao Liu <shuhao@ece.toronto.edu>
 *
 * @date 2016-02-04
 */

'use strict';

const util = require('util');
const EventEmitter = require('events');
const MsgFactory = require('./controlmsg.js');
const MsgTypeEnum = require('./controlmsg').MsgTypeEnum;
const Logger = require('./logger').Logger;
const _ = require('underscore');
const config = require('./config.js');
var net = require('net');
var Redis = require('ioredis');

// Class: Connection
// The class modeling the TCP connection to an individual node
var Connection = function(socket, logger) {
  if (!(socket instanceof net.Socket)) {
    throw {msg: 'Connection should be initiated out of a net.Socket!'};
  }
  if (!(logger instanceof Logger)) {
    logger = new Logger('Connect');
  }

  if (!(this instanceof Connection)) {
    return new Connection(socket, logger);
  }
  EventEmitter.call(this);

  var self = this;

  this._nodeID = 0;
  this._nodeIDStr = '0';
  this._hostName = "";
  this._socket = socket;
  this._inputBuf = '';
  this.logger = logger;

  // Create a connection to Redis for message subscribing
  this._dbSubscriber = new Redis({
    "connectTimeout": 1000
  });
  // Create another connection to Redis for tasks other than subscribing
  this._dbConn = new Redis({
    "connectTimeout": 1000
  });

  // Exception handling
  this._socket.on('error', self.onError.bind(this));
  this._socket.on('close', self.onError.bind(this));

  // Get the nodeID of the connection, then start working
  this._socket.once('readable', () => {
    var data = this._socket.read(4);
    var hostNameLength = data.readUInt32LE(0);
    data = this._socket.read(hostNameLength);
    this._hostName = data.toString('ascii');

    // Push hostname to the list

    this._dbConn.incr('CurrentNodeID').bind(this).then(
        function newConn(nodeID) {
          self._nodeID = nodeID;
          self._nodeIDStr = nodeID.toString();

          var buf = new Buffer(4);
          buf.writeUInt32LE(nodeID, 0);
          self._socket.write(buf);

          this._dbConn.hset('NodeID2Hostname', self._nodeIDStr, self._hostName);
          this._dbConn.hset('Hostname2NodeID', self._hostName, self._nodeIDStr);

          // Emit 'newConnection' event and start working
          this.emit('newConnection', nodeID);
          self._startWorking();
        });

  });
};
// Complete inheritance from the EventEmitter
util.inherits(Connection, EventEmitter);

// Member function to call right after the nodeID message is received
Connection.prototype._startWorking = function() {
  // Subscribe to the corresponding message channel on Redis
  this._dbSubscriber.subscribe(this._nodeID.toString(), () => {
    this.logger.info('Connection to Node ' + this._nodeID + ' on host '
        + this._hostName + ' established.');
  });
  this._dbSubscriber.on('message', this.sendControlMsg.bind(this));

  // Retrieve online node set, add self to the online node set
  // and send messages.
  this._dbConn.hgetall('NodeID2Hostname')
      .bind(this)
      .then(this._onlineDeclaration);

  // Configure the socket
  this._socket.on('data', this.onDataRecved.bind(this));
};

// The procedure to send new node messages to online nodes
// 1. Notify all existing nodes `this` node is online;
// 2. Notify `this` what the existing nodes are
Connection.prototype._onlineDeclaration = function(result) {
  // Parse the online nodes from the DB query
  var onlineNodes = Object.keys(result);
  var myIndex = onlineNodes.indexOf(this._nodeIDStr);
  if (myIndex > -1) {
    onlineNodes.splice(myIndex, 1);
  }
  if (onlineNodes.length == 0) return;

  var onlineNodeMaps = [];
  for (var i = 0; i < onlineNodes.length; i++) {
    onlineNodeMaps.push({
      'NodeID': Number(onlineNodes[i]),
      'Hostname': result[onlineNodes[i]]
    });
  }

  // 1
  var myOnlineMsg = MsgFactory.generateNodeOnlineMsg([{
    'NodeID': this._nodeID,
    'Hostname': this._hostName
  }]);
  var myRoutingInfo = MsgFactory.generateRoutingInfo(this._nodeIDStr, [this._nodeID]);
  for (var i = 0; i < onlineNodes.length; i++) {
    this._dbConn.publish(onlineNodes[i], myOnlineMsg);
    this._dbConn.publish(onlineNodes[i], myRoutingInfo);
  }
  // 2
  this._socket.write(MsgFactory.generateNodeOnlineMsg(onlineNodeMaps) + '\0');
  for (var i = 0; i < onlineNodes.length; i++) {
    var msg = MsgFactory.generateRoutingInfo(onlineNodes[i].toString(), [onlineNodes[i]]);
    this._socket.write(msg + '\0');
  }

  // Sending out the existing registered sessions.
  config.installAvailableSessionPaths(this._nodeID);
};

// A helper function to send control message to nodes
// Note: used as a callback of DB subscriber, `channel` is mandatory
Connection.prototype.sendControlMsg = function(channel, msg) {
  this.logger.info('"%s" => %s', msg, channel);
  this._socket.write(msg + '\0');
};

// Process the data received from the datapath node
// Note: used as a callback
Connection.prototype.onDataRecved = function(chunk) {
  this._inputBuf += chunk;
  while (1) {
    var index = this._inputBuf.indexOf('\0');
    if (index >= 0) {
      // A complete JSON exists
      var json_buf = this._inputBuf.slice(0, index);
      this._inputBuf = this._inputBuf.slice(index + 1);
      this.logger.debug('Node ' + this._nodeIDStr + ' Recv: '
                  + json_buf.toString('utf8'));

      // Parse
      var msg = JSON.parse(json_buf);
      var msgType = msg['Type'];
      var msgContent = msg['Msg'];

      switch (msgType) {
      case MsgTypeEnum.kControlMsgNewSession:
        this._dbConn.publish("querySessionID", JSON.stringify(msgContent));
        break;

      case MsgTypeEnum.kControlMsgRoutingInfo:
        this._dbConn.publish("routingInfo", JSON.stringify(msgContent));
        break;

      case MsgTypeEnum.kControlMsgReportRTT:
        MsgFactory.recordRTTs(this._dbConn, this._nodeIDStr, msgContent);
        break;

      case MsgTypeEnum.kControlMsgReportBandwidth:
        var fromNodeStr = msgContent["From"];
        var bandwidth = msgContent["Bandwidth"];
        this._dbConn.hmset("BandwidthTo" + this._nodeIDStr, fromNodeStr, Number(bandwidth));
        this.logger.info("Bandwidth measured: " + fromNodeStr + " -> " + this._nodeIDStr
            + " : " + bandwidth);
        break;

      case MsgTypeEnum.kControlMsgSessionSubscribed:
        this.logger.info("Session " + msgContent + " subscribed from Node "
            + this._nodeIDStr);
        this.onSessionSubscribed(msgContent);
        break;

      default:
        this.logger.warn("Unknown message type received: " + msgType.toString());
        break;
      }
    }
    else break;
  }
};

// Called when a session is subscribed to the connection
Connection.prototype.onSessionSubscribed = function(sessionID) {
  this._dbConn.hset('SessionID2Destination', sessionID, this._nodeIDStr);
  this._dbConn.hset('Region2NodeID', sessionID.split('.')[1], this._nodeIDStr);
  this._dbConn.hset('NodeID2Region', this._nodeIDStr, sessionID.split('.')[1]);
  this._dbConn.hgetall('NodeID2Hostname')
      .bind(this)
      .then(function(nodeIDHostnameMap) {
        var nodeList = Object.keys(nodeIDHostnameMap);
        var msg = MsgFactory.generateRoutingInfo(sessionID, [this._nodeID]);
        for (var i = 0; i < nodeList.length; i++) {
          this._dbConn.publish(nodeList[i], msg);
        }
      });
};

// Error or connection lost handling
Connection.prototype.onError = function(err) {
  this.logger.error('Connection to Node %d lost, Error: %s',
      this._nodeID, err.toString());
  this.close();
  // Emit 'connectionLost' event
  this.emit('connectionLost', this._nodeID);
};

// Close the connection and cleaning up
Connection.prototype.close = function() {
  // Close socket
  this._socket.end();
  this._socket.unref();
  // Delete self from the database
  this._dbConn.srem('NodeSet', this._nodeIDStr);
  // Disconnect from the database
  this._dbConn.disconnect();
  this._dbSubscriber.disconnect();
};

module.exports = Connection;

