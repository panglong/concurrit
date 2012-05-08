#!/bin/bash

set -e
set -u
DIR=`dirname $0`
COMMON_SH=$DIR/common.sh
if [ ! -e "$COMMON_SH" ]; then
    echo "ERROR: unable to find common.sh at $COMMON_SH"
    exit 1
fi

source $COMMON_SH

MY_SRC=$TEST_SRC 

OUTNAME=test-memcached
OUTFILE="$BIN_DIR/$OUTNAME"

echo
echo "Building Test Harness..."
g++ $MY_SRC -o $OUTFILE -I$INSTALL_INCLUDE_PATH -L$INSTALL_LIB_PATH -lmemcached -lpthread
g++ $BIN_DIR/../src/server-alive.cpp -o $BIN_DIR/server_alive -I$INSTALL_INCLUDE_PATH -L$INSTALL_LIB_PATH -lmemcached 

EXECUTE_CMD="$INSTALL_BIN_PATH/memcached -U 0 -p 11211 -r 4 -d; sleep .5; LD_LIBRARY_PATH=$INSTALL_LIB_PATH $OUTFILE; killall memcached"

echo "Test Harness Built!"
echo "To Execute, you must start memcached, execute the harness while specifying the shared library path, and shutdown memcached:"
echo "  $EXECUTE_CMD"
echo "$EXECUTE_CMD" &> $BIN_DIR_README

