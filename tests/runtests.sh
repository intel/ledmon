#!/usr/bin/bash
#
SCRIPT_DIR=$( cd -- "$( dirname -- "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )

#Put us in a consistent spot
if [ ! -e "./tests" ];then
    echo "We are running from test folder directly."
    cd ..
fi

echo "exercising ledctl"
pytest tests --ledctl-binary=src/ledctl/ledctl --slot-filters="$LEDMONTEST_SLOT_FILTER" || exit 1

echo "running library unit test"
./tests/lib_unit_test || exit 1

