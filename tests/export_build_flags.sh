#!/usr/bin/bash

# Set compiler flags
export CFLAGS=$1
if [ -z $2 ]
then
	set CC=$2
fi

# Configure
./autogen.sh && ./configure --enable-test --enable-library
