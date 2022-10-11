# Siphon Controller

This project is an implementation of Siphon Controller, which interacts with the Siphon Datapath aggregators.

## Prerequisite

The Controller is build with Node.js, which uses [Redis](http://redis.io) key-value store to manage data and handle inter-process communications. To install Node.js and Redis server

on macOS, run

```
brew install node redis
```

on Ubuntu Linux, run

```
sudo apt install nodejs redis-server
sudo service redis-server stop  # Stop the global redis service.
```

## Run the Controller

+ Update npm to the latest version if it has previously been installed

```
npm i -g npm 
```

+ Review current vulnerabilities in npm packages

```
npm audit 
```

+ Fix the current vulnerabilities as suggested by the audit. For example:

```
npm install ioredis@4.1.0
```

+ Install dependencies

```
cd src
npm install  # Read package.json and install dependencies locally
```

+ Run the controller server

```
cd src
node server <directory of the config file, e.g. ./static_routes.json> # Run the server process
```

## Runing the Controller in a Docker Container

Install the latest version of Docker for macOS first. Then, within the main `siphon_controller` directory, run the following command to build the Docker image from scratch:

```
docker build -t siphon_controller .
```

This will take about one or two minutes to finish.

To run the Siphon controller within a Docker container, run the command:

```
docker run -p 6699:6699 siphon_controller
```

The command-line option `-p` is used to map your machine’s port 6699 to the container’s published port 6699.

