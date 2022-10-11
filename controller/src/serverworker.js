/**
 * @file serverworker.js
 * @brief The server worker which listens on a port, handling connections to the
 *        datapath nodes.
 * @author Shuhao Liu <shuhao@ece.toronto.edu>
 *
 * @date 2016-02-09
 */

const net = require('net');
const Connection = require('./connection.js');
var logger = require('./logger').Logger('ServerW');

// Read the controller port number from the arguments
const controllerPortNum = Number(process.argv[2]);

// The map between nodeID and the connection instance
var connDict = {};

// Start the server instance. When new connection established,
// 1. Create a Connection instance out of the socket; and
// 2. Configure the connection by adding listeners
const server = net.createServer(function onConnectionEstablished(socket) {
  // 1
  var connection = new Connection(socket, logger);

  // 2.1 `newConnection` listener
  connection.on('newConnection', function connNodeIDPaired(nodeID) {
    connDict[nodeID] = this;
  });

  // 2.2 `connectionLost` listener
  connection.on('connectionLost', function connLost(nodeID) {
    logger.warn('Connection Node# %d is lost!', nodeID);
    connDict[nodeID] = undefined;
  });
});

// Let the server listen on the controller port
server.listen(controllerPortNum, (err) => {
  // 'listening' listener
  if (err) throw err;
  logger.info('Server has started listening...');
});

// Exception handling
server.on('error', (err) => {
  // Disconnect all responsible connections
  for (var node in connDict) {
    if (!connDict.hasOwnProperty(node)) continue;

    var conn = connDict[node];
    if (conn) {
      conn.close();
    }
  }

  // Exit with code 1, so that it can be restart by the master
  logger.error('*** Server error: %s ***', err);
  process.exit(1);
});

// Keyboard Interrupt Event Handler
process.on('SIGINT', () => {
  logger.warn('*** Keyboard interrupt ***');
  process.exit(0);
});

process.on('exit', () => {
  // Unref all connections
  for (var key in connDict) {
    if (connDict[key]) connDict[key].close();
  }
  logger.warn('*** Cleaned up, exiting... ***')
});

