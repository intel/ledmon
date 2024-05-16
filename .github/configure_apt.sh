#!/usr/bin/bash

VERSION_CODENAME=$(grep -oP '(?<=^VERSION_CODENAME=).+' /etc/os-release | tr -d '"')
echo "Detected VERSION_CODENAME: $VERSION_CODENAME"

# Add ubuntu repository
sudo add-apt-repository "deb [arch=amd64] http://archive.ubuntu.com/ubuntu $VERSION_CODENAME \
        main universe"
