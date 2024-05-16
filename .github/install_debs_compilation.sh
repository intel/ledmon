#!/usr/bin/bash

# Install gcc
if [ -z "$1" ]; then
       sudo apt-get update && sudo apt-get install gcc-$1 g++-$1
fi
# Install dependencies
sudo apt-get install pkg-config automake autoconf autoconf-archive make libsgutils2-dev \
        libudev-dev libpci-dev check devscripts
