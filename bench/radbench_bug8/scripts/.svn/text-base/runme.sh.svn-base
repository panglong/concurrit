#!/bin/bash

# Script to automate bug 8

function usage {
    echo "Usage: $0 [command modifier]"
    echo
    echo "Runs the benchmark.  If a command modifier is given, " 
    echo "then runs the benchmark under the command. Returns 0 if "
    echo "the bug is not found and 1 if the bug is found"
    echo
    echo "e.g. $0 \"pin --\" will result in the benchmark being run "
    echo "under pin (with an empty tool)"
    echo
}

if [ "$1" == "-h" ] || 
    [ "$1" == "--h" ] || 
    [ "$1" == "-help" ] || 
    [ "$1" == "--help" ]; 
then
    usage
    exit 0
fi


RUN_DIR=`pwd`

DIR=`dirname $0`
cd $DIR
SCRIPT_DIR=`pwd`

COMMON_SH=$SCRIPT_DIR/../../scripts/common.sh
if [ ! -e "$COMMON_SH" ]; then
    echo "ERROR: no common functions script"
    exit 0
fi

source $COMMON_SH


EXECUTABLE=$SCRIPT_DIR/../bin/test-memcached
if [ ! -e "$EXECUTABLE" ]; then
    echo "ERROR: Executable not found at $EXECUTABLE"
    exit 1
fi
TEST_CALL="{ $EXECUTABLE; } & "

SERVER_ALIVE=$SCRIPT_DIR/../bin/server_alive
if [ ! -e "$SERVER_ALIVE" ]; then
    echo "ERROR: server checker not found at $SERVER_ALIVE"
    exit 1
fi
TEST_CALL="{ $EXECUTABLE; } & "

MEMCACHED_SERVER=$SCRIPT_DIR/../bin/install/bin/memcached
if [ ! -e "$MEMCACHED_SERVER" ]; then
    echo "ERROR: server not found at $MEMCACHED_SERVER"
    exit 1
fi

SERVER_CALL="{ $1 $MEMCACHED_SERVER -U 0 -p 11211 -r 4; } &"

set -u

cd $RUN_DIR

function setup {
    killAllWithProcName memcached 10
    echo "[$0] Starting Server: $SERVER_CALL"
    eval $SERVER_CALL
    export LD_LIBRARY_PATH=$SCRIPT_DIR/../bin/install/lib 
    eval $SERVER_ALIVE
    RET=$?
    while [ "$RET" != "0" ]; do
        sleep .1
        eval $SERVER_ALIVE
        RET=$?
    done
    return 0
}

function runtest {
    eval $TEST_CALL
    PID=$!
    wait $PID
    return $?
}

function cleanup {
    killAllWithProcName memcached
    return 0
}

setup
runtest
RET=$?
cleanup

echo "[$0] Return value is $RET"
exit $RET
