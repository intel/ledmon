#!/usr/bin/bash

# Set compiler flags
if ! [[ -z $1 ]];
then
	export CFLAGS=$1
else
	echo "There is no CFLAGS set. Please check your configuration."
	exit 1
fi

if [ -z $2 ]
then
	set CC=$2
fi

# Configure
./autogen.sh && ./configure --enable-test --enable-library
