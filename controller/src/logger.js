/**
 * @file logger.js
 * @brief A helper utility to print inter-process messages.
 * @author Shuhao Liu <shuhao@ece.toronto.edu>
 *
 * @date 2016-03-22
 */

'use strict';

const format = require('util').format;
var Redis = require('ioredis');

// A helper function to pad after the string
String.prototype.paddingRight = function(length, char) {
  char = (typeof char == 'undefined') ? ' ': char;

  if (this.length >= length) {
    return this;
  }
  else {
    var numRepeat = Math.ceil((length - this.length) / char.length);
    return String(this + char.repeat(numRepeat)).slice(0, length);
  }
};

// The logger class
var Logger = function (name) {
  if (!(this instanceof Logger)) {
    return new Logger(name);
  }

  this._name = '[' + name.toString().paddingRight(7) + '|' +
      (process.pid % 100000).toString().paddingRight(5) + '|';
  this._redis = new Redis({
    "connectTimeout": 1000
  });
};

Logger.prototype._prefix = function(level) {
  return (this._name + level.paddingRight(5) + '] ');
};

Logger.prototype.info = function() {
  this._redis.publish('INFO',
      this._prefix('INFO') + format.apply(this, arguments)
  );
};

Logger.prototype.debug = function() {
  this._redis.publish('DEBUG',
      this._prefix('DEBUG') + format.apply(this, arguments)
  );
};

Logger.prototype.warn = function() {
  this._redis.publish('WARN',
      this._prefix('WARN') + format.apply(this, arguments)
  );
};

Logger.prototype.error = function() {
  this._redis.publish('ERROR',
      this._prefix('ERROR') + format.apply(this, arguments)
  );
};

// The LogPrinter Class
var LogPrinter = function() {
  if (!(this instanceof LogPrinter)) {
    return new LogPrinter();
  }

  this._redis = new Redis({
    "connectTimeout": 1000
  });

  this._redis.subscribe('INFO', 'DEBUG', 'WARN', 'ERROR', (err, count) => {
    if (err || count !== 4) {
      console.error('[LogPrinter|ERROR] Subscription Failed' + err.toString());
      process.exit(1);
    }

    this._redis.on('message', (channel, msg) => {
      switch(channel) {
        case 'INFO':
          console.info(msg);
          break;

        case 'DEBUG':
          console.log(msg);
          break;

        case 'WARN':
          console.warn(msg);
          break;

        default:
          console.error(msg);
          break;
      }
    })
  })
};

module.exports = {
  Logger: Logger,
  LogPrinter: LogPrinter
};
