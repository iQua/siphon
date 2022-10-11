FROM ubuntu:20.04

LABEL "maintainer"="Baochun Li <bli@ece.toronto.edu>" \
      "version"="2.0"

# Expose port for connection from peers
EXPOSE 6868/udp
EXPOSE 6868/tcp

# Expose port for the gRPC service
EXPOSE 6869

# Configuring the timezone
ENV TZ=America/Toronto
RUN ln -snf /usr/share/zoneinfo/$TZ /etc/localtime && echo $TZ > /etc/timezone

# Retrieving dependencies
RUN apt-get update && apt-get install -y \
  build-essential autoconf libtool vim sudo \
  curl \
  git \
  cmake \
  g++ \
  wget \
  net-tools \
  inetutils-ping \
  mahimahi \
  iperf3 \
  protobuf-compiler-grpc libprotobuf-dev libprotoc-dev \
  libgrpc-dev libgrpc++-dev \
  && apt-get clean


# Building Boost 1.67.0
RUN wget https://boostorg.jfrog.io/artifactory/main/release/1.67.0/source/boost_1_67_0.tar.gz \
  && tar zxvf boost_1_67_0.tar.gz && cd boost_1_67_0 && ./bootstrap.sh && \
  ./b2 && sudo ./b2 install

# Build Siphon
COPY . /var/siphon_datapath
RUN cd /var/siphon_datapath \
    && git submodule update --init --recursive \
    && ./dependencies.sh \
    && ./build.sh \
    && ldconfig

# Add a regular user 'admin'
RUN useradd -ms /bin/bash admin
RUN sudo chown -R admin:admin /var/siphon_datapath

# Running options
WORKDIR /var/siphon_datapath/
# ENTRYPOINT ["bin/siphon_datapath", "controller", "examples/udp_config.json"]
