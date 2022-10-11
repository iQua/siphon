#!/bin/bash

CDIR="$(pwd)"
echo "Current Directory "
echo $CDIR

echo "Dependency Packages"
case "$OSTYPE" in
  darwin*)
    brew install cmake boost
    echo "Installing gRPC runtime"
    brew install protobuf grpc
    ;;
  linux*)
    sudo apt-get install -y g++ curl git cmake build-essential autoconf libtool yasm pkg-config protobuf-compiler-grpc libprotobuf-dev libprotoc-dev libgrpc-dev libgrpc++-dev
    ;;
esac

echo "Installing Isa-l"
mkdir -p /tmp/siphon-deps
cd   /tmp/siphon-deps
git clone https://github.com/01org/isa-l.git
cd isa-l
./autogen.sh
./configure --prefix=/usr/local --libdir=/usr/local/lib 
make
sudo make install

echo "Updating Submodules"
cd $CDIR
git submodule update --init --recursive

