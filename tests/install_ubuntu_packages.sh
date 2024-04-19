#!/usr/bin/bash

VERSION_CODENAME=$(grep -oP '(?<=^VERSION_CODENAME=).+' /etc/os-release | tr -d '"')
echo "Detected VERSION_CODENAME: $VERSION_CODENAME"

# Add ubuntu repository
sudo add-apt-repository "deb [arch=amd64] http://archive.ubuntu.com/ubuntu $VERSION_CODENAME \
        main universe"
# Install gcc
sudo apt-get update && sudo apt-get install gcc-$1 g++-$1
# Install dependencies
sudo apt-get install pkg-config automake autoconf autoconf-archive make libsgutils2-dev \
        libudev-dev libpci-dev check devscripts
