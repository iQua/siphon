/**
 * @file server.js
 * @brief The master server which creates a bunch of server worker processes.
 * @author Shuhao Liu <shuhao@ece.toronto.edu>
 *
 * @date 2016-02-09
 */

const spawn = require('child_process').spawn;
const config = require('./config.js');


// Start Redis at start up
console.log("Starting the Redis server.\n")

var redisServerProcess = spawn('redis-server', ['--appendonly', 'no'], {
  cwd: '.',
  env: process.env,
  stdio: 'ignore'
});

redisServerProcess.on('exit', (code, signal) => {
  console.error('*** redis-server exit with code %d sig %s ***',
      code, signal);
  process.exit(code);
});

const cluster = require('cluster');
const numCPUs = require('os').cpus().length;
const fs = require('fs');
var logger = require('./logger').Logger('ServerM');
var logPrinter = require('./logger').LogPrinter();
var Scheduler = require('./siphonflowscheduler').FlowScheduler;

const controllerPortNum = '6699';

var childProcessesList = [];

var flowScheduler;

// Make sure a redis-server dump file does not exist
try {
  fs.unlinkSync('./dump.rdb');
}
catch (err) {
}

// Keyboard Interrupt Event Handler
process.on('SIGINT', () => {
  logger.warn('*** Keyboard interrupt, exiting... ***');
});

// Tear down everything on exit
process.on('exit', () => {
  for (var i = 0; i < childProcessesList.length; i++) {
    childProcessesList[i].kill('SIGINT');
  }
  redisServerProcess.kill('SIGINT');
});

// Spawning Executors (Python)
function spawnExecutorMaster() {
  // Spawn the process and push into the list
  var executorObj = spawn('python3', ['executor.py', '-d'], {
    cwd: '.',
    env: process.env,
    stdio: 'ignore'
  });
  childProcessesList.unshift(executorObj);

  // Close the Controller in case of executor failure
  executorObj.on('exit', (code, signal) => {
    console.error('*** Executors exited with code %d signal %s ***',
        code, signal);
    console.error('*** Controller exiting because no executor remains. ***');
    process.exit(code);
  });
}

// Spawning ServerWorkers (Nodejs)
function spawnServerWorkers() {
  // Set up worker process settings
  cluster.setupMaster({
    exec: './serverworker.js',
    args: [controllerPortNum],
    silent: false
  });

  // Fork server workers
  var numThreads = 2;
  if (numThreads > numCPUs) {
    numThreads = numCPUs;
  }
  for (var i = 0; i < numThreads; i++) {
    childProcessesList.push(cluster.fork());
  }

  // In case the worker exits unexpectedly, restart it if possible
  cluster.on('exit', (worker, code, signal) => {
    // Remove from the serverWorker array
    var index = childProcessesList.indexOf(worker);
    if (index > -1) {
      childProcessesList.splice(index, 1);
    }
    else {
      logger.error('Cannot find the exited worker in serverWorkers!');
      return;
    }

    if (code !== 0) {
      // Accidentally terminated
      logger.error('*** [ServerW %d] died (%s), restarting... ***',
          worker.process.pid, signal || code);

      // Restart the serverWorker
      var newWorker = cluster.fork();
      childProcessesList.push(newWorker);
      newWorker.once('listening', () => {
        logger.debug('Restarted worker is now listening...');
      });
    }
  });
}

// Wait until redis server starts

setTimeout(() => {
  //spawnExecutorMaster();
  console.log("Starting the Siphon controller.\n")
  spawnServerWorkers();
  if (process.argv.length === 3) {
    console.log("Loading config from file: " + process.argv[2]);
    config.readConfig(process.argv[2]);
  }
}, 1000);

