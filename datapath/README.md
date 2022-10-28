# Siphon Datapath Aggregator

This repository hosts the source code for the Siphon datapath aggregator. It works as a application-layer message switch.

## Preparing a Suitable Build Environment for running Siphon

This document describes how to prepare a suitable build environment to compile Siphon in Ubuntu Linux 20.04. The steps have been tested in a Docker container running Ubuntu Linux 20.04.

## Installing GCC 9, G++ 9, and Cmake

```shell
$ sudo apt install gcc-9
$ sudo apt install g++-9
$ dpkg -l | grep gcc | awk '{print $2}'
$ sudo update-alternatives --install /usr/bin/gcc gcc /usr/bin/gcc-9 10
$ sudo update-alternatives --config gcc
$ sudo update-alternatives --install /usr/bin/g++ g++ /usr/bin/g++-9 10
$ sudo update-alternatives --config g++
$ gcc —v
$ g++ —v
$ sudo apt install cmake
$ cmake --version
```

## Installing Boost 1.67

You will need to download version 1.67 of the Boost library from [Boost](https://www.boost.org/users/history/version_1_67_0.html) first. Other versions of boost have not been tested.

### Removing the existing Boost installation

```shell
$ sudo apt-get -y --purge remove libboost-dev libboost-all-dev libboost-doc
$ sudo apt autoremove
$ sudo rm -f /usr/lib/libboost_*
$ sudo rm -f /usr/local/lib/libboost_*
$ sudo rm -f /usr/bin/lib/libboost_*
$ sudo rm -rf /usr/local/include/boost
$ sudo rm -f /usr/local/lib/cmake/*
```

### Installing Boost 1.67

Here is the [direct link to the .tar.gz archive](https://dl.bintray.com/boostorg/release/1.67.0/source/boost_1_67_0.tar.gz).

```shell
$ wget https://dl.bintray.com/boostorg/release/1.67.0/source/boost_1_67_0.tar.gz
$ tar zxvf boost_1_67_0.tar.gz
$ cd boost_1_67_0
$ ./bootstrap.sh
$ ./b2
$ sudo ./b2 install
$ cat /usr/local/include/boost/version.hpp | grep "BOOST_LIB_VERSION"
```

## Installing Protocol Buffers 3.7.x

```shell
$ sudo apt-get install libtool
$ git clone https://github.com/google/protobuf.git
$ cd protobuf
$ git checkout 3.7.x
$ git submodule update --init --recursive
$ ./autogen.sh
$ ./configure
$ make
$ make check
$ sudo make install
$ sudo ldconfig
```

Compiling and testing Protocol Buffers from source will take quite a long time. Other versions of Protocol Buffers have not been tested.

## Building the Project

To build Siphon datapath aggregator on either macOS and Linux, run

```shell
$ git clone https://github.com/iqua/siphon
$ cd siphon/datapath
$ ./dependencies.sh
$ ./build.sh
```

The first script will install all dependencies to the local machine. These include:

+ `cmake`: The building system used by this project.
+ `g++`: GNU Compiler.
+ `protobuf` and `gRPC`: Library used to build user interfaces. Compiling and installing them will take some time.
+ Download `siphon/third_party` dependencies from GitHub.

Note that The second script will run `cmake` and `make` to build the executable. The final executable will be found at `bin/siphon_datapath`.

## Build the Project with Xcode

Run the command:

`cmake -G Xcode .`

which generates the Xcode project `siphon.xcodeproj`. Open the project with Xcode and build. Make sure all required brew packages are installed, by running `brew install cmake protobuf grpc boost` before generating the Xcode project and building the project using Xcode.

## Run as Local Processes

An aggregator is an application-layer software switch, and we can run it on a few machines to create an overlay network. To do this, make sure the following prerequisites are met:

+ At least two (virtual) machines are available. Each machine has an IP/URL that can be directly addressed by all other machines. In other words, machines should be in the same subnet; otherwise, each machines should be associated with a public IP address.
+ Each machine has a copy of the same configuration file, e.g., `example/udp_config.json`.
+ Each machine has all dependencies installed.

To run the executable on a machine as a local process, follow these steps:

1. Make sure the controller is running and get its URL/IP address.

2. Set the machine's hostname to its URL or public IP address. It can be changed temporarily with command `sudo hostname [machine_IP_address]`. To verify whether the hostname is successfully set, run command `hostname` and check its output.

3. Create a configuration file, if you want to generate some traffic for testing purpose. An example of the configuration file can be found at `example/udp_config.json`. Notes on the configuration file:

    + "PseudoSessions" is a list. Each element defines a transfer session. A session generally refers to a multicast session, where one source is multicasting data to one or more receivers. The current code supports **unicast only**.
        + "SessionID" is a string that uniquely identifies a session. To send data on the default direct path between the source and the destination, it should be set to the NodeID of the receiver. For example, if a session is sent to Node 2, its SessionID should be "2".
        + "Src" and "Dst" specify the source and destination of the session.
        + "Rate" specifies the data generation rate for this pseudo transfer session, in **packet/s**.
        + "MessageSize" specifies the size of a packet in this session, in **bytes**.
    + (Optional) "MaxMessageSize" specifies the max size of a message.
    + "UDP" settings are all optional.
    + "CoderName" now supports "test" or "DirectPass".

4. Run the executable. Note: 6699 is the default port of Siphon controller.
    ```shell
    bin/siphon_datapath [controller_IP_address] [path_to_config_file]
    ```
    
5. To stop the program, use `Ctrl-C`.

6. To run three node setup
    * Change the configuration file by changing `"SessionID": "3",`, `"Src": 1,` and `"Dst": 3,`. This sets node with ID = 1 as the source, and node with ID = 3 as the destination.
    * To let the controller forward packets from node with ID = 1 to node with ID = 2 then to node with ID = 3, in siphon_controller/src/connection.js change line 216 to 220 (after `var nodeList = Object.keys(nodeIDHostnameMap);`) to the following
    ```
    if (sessionID == “3”) {
    var msg = MsgFactory.generateRoutingInfo(“3", [3]);
    this._dbConn.publish(3, msg);   // (2)
    this._dbConn.publish(2, msg);   // (2)
    msg = MsgFactory.generateRoutingInfo(“3", [2]);
    this._dbConn.publish(1, msg);   // (1)
    }
    ```
    * If you want to run siphon with three nodes with FEC encoding, make sure to setup the sender and receiver with the FEC_files branch (with coding), but the relay won't encode or decode.

## Running Siphon in Docker Containers

The preferred way of testing and running Siphon is to use Docker containers.

### Step 1: Building the Docker image from scratch for the first time for the Siphon datapath.

> Note: You can optionally load the Docker images you have previously build - see Step 2 for instructions. If you have previously built docker images already, you may wish to remove these images so that you can build everything from scratch. See Step 2 for instructions on removing Docker images.

Install the latest version of Docker for Mac first. Then, within the main `siphon_datapath` directory, run the following command to build the Docker image from scratch:

```
docker build -t siphon_datapath .
```

This will use Ubuntu's latest 20.04 release as the foundation, install the latest version of the required libraries, such as Google's GRPC and Protocol Buffers. It will then compile the Siphon source code from scratch within the Docker container.

This process will take about 20-25 minutes on a typical computer.

### Step 2: Saving the Docker image as a compressed file for later use:

```
docker save siphon_datapath | gzip > docker_images/siphon_datapath.tar.gz
```

`docker-images/` has been added to the `.gitignore` file so that these images saved locally will not be committed to the git repository.

**Optionally,** if you have previously saved a Docker image, you can load the image rather than rebuilding from scratch using the Dockerfile provided. Use any one of the following commands:

```
gzcat docker_images/siphon_datapath.tar.gz | docker load
```
or:

```
docker load -i docker_images/siphon_datapath.tar.gz
```

or:

```
docker load < docker-images/siphon_datapath.tar.gz
```

The `Dockerfile` image file is about 665 MB, and it will take a few minutes to save and to load the image.

#### Helpful commands working with Docker containers and images:

Removing all stopped containers:

```
docker rm $(docker ps -aq)
```

Removing the image `siphon_datapath`:

```
docker rmi siphon_datapath
```

Removing all images that are not being used:

```
docker rmi $(docker images -q)
```

#### Re-entering a Docker container

To re-enter the Docker container after you type `exit` at the command line inside the container, first obtain the container ID by using the `docker ps -a` command:

```
$ docker ps -a
CONTAINER ID        IMAGE               COMMAND             CREATED             STATUS                     PORTS               NAMES
CONTAINER ID        IMAGE               COMMAND             CREATED             STATUS                          PORTS               NAMES
e30baac0a506        siphon_datapath     "/bin/bash"         22 minutes ago      Exited (0) About a minute ago                       flamboyant_wing
```

Then use the `docker start` and the `docker exec` command to run it again:

```
$ docker start e30baac0a506
e30baac0a506
```

```
$ docker exec -it e30baac0a506 /bin/bash
root@e30baac0a506:~#
```

Type `exit` at the command line within the Docker container to leave.

You will see that changes you have previously made within that container have not been lost.

**Alternatively**, you can also use the Docker Dashboard to reenter any Docker container. After you enter the container, type `bash` to start a `bash` shell.

### Step 3: Preparing two (virtual?) physical machines with public IP addresses

> It also works if both machines are in the same subnet. The bottom line: the sender machine can set up a direct TCP/UDP connection to the receiver machine via its IP address.

For example, we have a sender machine with IP address `xx.xx.xx.xx`, and a receiver machine with IP address `yy.yy.yy.yy` (must be directly reachable by the sender).

Install Docker on both machines. Copy the docker image you created at Step 2 to both machines.

### Step 4: Creating a configuration file

A sample configuration file can be found in `deploy/docker/testconfig.json`. Copy the configuration file to both machines as `/path/to/config.json` for later use.

### Step 5: Running Docker containers

On each machine, run the Docker image for the first time as a Docker container:

```shell
docker run -p 6868-6869:6868-6869 -p 6868:6868/udp -h {ip_address} -v /path/to/config.json:/var/config.json -it siphon_datapath
```

+ `-p 6868-6869:6868-6869` and `-p 6868:6868/udp` map the exposed container TCP ports 6868-6869 and UDP port 6868 to the host machine, so that peer can connect to host from another machine through the mapped ports.
+ Replace `{ip_address}` with either `xx.xx.xx.xx` or `yy.yy.yy.yy` (can be double quoted). It determines the hostname of each container. `siphon_datapath` uses the hostname as its own public IP address.
+ `-v /path/to/config.json:/var/config.json` option will attach the configuration file into the container, under `/var/config.json`.

> To exit the Docker container, type `exit` at the command line. It's feasible to re-enter the Docker container at any time. See Step 2 for more instructions on this.

### Step 6: Building and running the Docker container for the Siphon Controller

Follow `siphon_controller/README.md` to build and run a Docker Container for the Siphon Controller on the receiver machine (with IP `yy.yy.yy.yy`).

### Step 7: Running `siphon_datapath` on both machines

> As is defined by the sample configuration file, receiver is the machine with NodeID 1, which is the first machine that connects to the controller.

Within the container on the receiver machine, run

```shell
./bin/siphon_test_exec yy.yy.yy.yy 6699 /var/testconfig.json
```
where the IP address `yy.yy.yy.yy` is the IP address of the machine that runs the controller.

After the receiver is up and running, run the same command within the container on the sender machine.
