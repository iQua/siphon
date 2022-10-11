/**
 * @file test_logger.js
 * @brief A test fir logger.js
 * @author Shuhao Liu <shuhao@ece.toronto.edu>
 *
 * @date 2016-03-22
 */

'use strict';

const spawn = require('child_process').spawn;
var logger = require('../src/logger').Logger('TestLog');
var printer = require('../src/logger').LogPrinter();

// Start Redis at start up
var redisServerProcess = spawn('redis-server', ['--appendonly', 'no'], {
  cwd: '.',
  env: process.env,
  stdio: 'ignore'
});

redisServerProcess.on('error', (err) => {
  console.error('redis-server: ' + err.toString());
  process.exit(1);
});

// Tear down everything on exit
process.on('exit', () => {
  redisServerProcess.kill('SIGINT');
});

setTimeout(() =>{
  logger.info('info Message');
  logger.debug('debug Message');
  logger.warn('warn Message');
  logger.error('error Message');
  logger.info(' ');
  logger.info('1 + 2 = %d', (1+2));
  logger.info('"1" + "2" = "%s"', ('1'+'2'));
}, 1000);
