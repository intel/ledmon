#!/usr/bin/bash

# Install gcc
if [ ! -z "$1" ]; then
       sudo apt-get -y install gcc-$1 g++-$1 --no-upgrade --no-install-recommends --no-install-suggests
fi
# Install dependencies
sudo apt-get -y install pkg-config automake autoconf autoconf-archive make libsgutils2-dev \
        libudev-dev libpci-dev check devscripts
