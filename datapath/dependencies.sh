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

echo "Updating Submodules"
cd $CDIR
git submodule update --init --recursive
