/**
 * @file config.js
 * @brief A helper utility to load/read the configuration file.
 * @author Shuhao Liu <shuhao@ece.toronto.edu>
 *
 * @date 2020-01-06
 */

'use strict';

const Logger = require('./logger').Logger;
const MsgFactory = require('./controlmsg.js');
const fs = require('fs');
let Redis = require('ioredis');

// The Redis key that will be associated with the JSON configuration.
const CONFIG_IN_REDIS = "Configuration";

// Read the content in the configuration file and store it into Redis.
// If the configuration is not valid, nothing will happen.
exports.readConfig = function(configFile) {
  fs.readFile(configFile, (err, data) => {
    // Initialize the logger.
    let logger = new Logger("ConfigFile Loader");
    if (err) {
      logger.error("Cannot load file: " + err.message);
      return;
    }

    try {
      // Try parsing the configuration file. If the configuration file is not
      // valid, print out the error.
      let config = JSON.parse(data);
      let db = new Redis({
        "connectTimeout": 1000
      });
      db.set(CONFIG_IN_REDIS, JSON.stringify(config));
      logger.info("Successfully load config " + configFile + " to DB.")
    }
    catch (e) {
      logger.error("Failed to store configurations in Redis: " + e.message())
    }
  })
};

// Get configuration from the Redis Database.
// The only parameter is a callback function, which accepts the stringified
// configuration as its parameter.
function getConfig(callback) {
  let logger = new Logger("Config Getter");
  let db = new Redis({
    "connectTimeout": 1000
  });
  db.get(CONFIG_IN_REDIS, function(err, result) {
    if (err) {
      logger.error("Cannot read configuration in Redis: " + e.message);
      result = "{}";
    }
    callback(JSON.parse(result));
  });
}
exports.getConfig = getConfig;


// The callback function has a single parameter, which is a list of
// installation-ready paths.
// Sample config:
//   {
//     ...,
//     "PseudoSessions" : [
//   		 {
// 	  	 	 "SessionID" : "2-1-3",
//         "Path": [2, 1, 3],
//         ...
// 	  	 }
// 	   ]
//   }
//
// Sample callback parameter (if all involved nodes are online):
//  [
//    {"Node": 2, "SessionID": "session", "NextHop": 1},
//    {"Node": 1, "SessionID": "session", "NextHop": 3},
//    {"Node": 3, "SessionID": "session", "NextHop": 3}
//  ]
exports.installAvailableSessionPaths = function(newNodeID) {
  let logger = new Logger("Config Getter");
  let db = new Redis({
    "connectTimeout": 1000
  });
  getConfig(function(config) {
    let result = [];
    if ("PseudoSessions" in config) {
      for (let i = 0; i < config["PseudoSessions"].length; i++) {
        let session = config["PseudoSessions"][i];
        if (!"Path" in session) continue;
        if (!session['Path'].includes(newNodeID)) continue;

        let shouldInstall = true;
        for (let j = 0; j < session["Path"].length; j++) {
          if (session['Path'][j] > newNodeID) {
            shouldInstall = false;
            break;
          }
        }
        if (shouldInstall) {
          let pathLength = session["Path"].length;
          for (let j = 0; j < pathLength - 1; j++) {
            let rule = {
              'SessionID': session['SessionID'],
              'Node': session['Path'][j],
              'NextHop': session['Path'][j+1]
            };
            result.push(rule);
          }
          // Last hop.
          result.push({
            'SessionID': session['SessionID'],
            'Node': session['Path'][pathLength - 1],
            'NextHop': session['Path'][pathLength - 1]
          });
        }
      }
    }

    logger.debug('Installing routes: ' + JSON.stringify(result));

    // Install all forwarding rules within `result`.
    for (let i = 0; i < result.length; i++) {
      let msg = MsgFactory.generateRoutingInfo(
          result[i]['SessionID'], [result[i]['NextHop']]);
      db.publish(result[i]['Node'].toString(), msg);
    }
  });
};
