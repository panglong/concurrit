#!/bin/bash

#script to automate experiments

function usage {
    echo "Usage: $0 [command modifier]"
    echo
    echo "Runs the benchmark.  If a command modifier is given, " 
    echo "then runs the benchmark under the command. Returns 0 if "
    echo "the bug is not found and 1 if the bug is found"
    echo
    echo "e.g. $0 \"pin --\" will result in the benchmark being run "
    echo "under pin (with an empty tool"
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
BIN_DIR=`pwd`

MEMSERVER=$BIN_DIR/install/bin/memcached
if [ ! -e "$MEMSERVER" ]; then
    echo "ERROR: Memcached server not found at $MEMSERVER"
    exit 1
fi

LIBPATH=$BIN_DIR/install/lib
if [ ! -d "$LIBPATH" ]; then
    echo "ERROR: Libraries not found at $LIBPATH"
    exit 1
fi

EXECUTABLE=$BIN_DIR/test-memcached
if [ ! -e "$EXECUTABLE" ]; then
    echo "ERROR: Executable not found at $EXECUTABLE"
    exit 1
fi
TEST_CALL="$1 $EXECUTABLE"

cd $RUN_DIR

function waitUntilTimeout {
    POLLS=20
    COUNT=0
    while [ "$POLLS" -gt "$COUNT" ]; 
    do
        kill -0 $1 
        if [ "$?" != "0" ]; then
            wait $1
            return $?
        fi
        sleep .1
        let "COUNT = $COUNT + 1"
    done
    kill $1
    sleep .2
    kill $1
    sleep .2
    kill -9 $1
    return 1
}

function setup {
    $MEMSERVER -U 0 -p 11211 -r 4 -d
    sleep .5
    export LD_LIBRARY_PATH=$LIBPATH
    return 0
}

function runtest {
    echo "Calling: $TEST_CALL"
    $TEST_CALL
    return $?
}

function cleanup {
    killall memcached
    sleep .2
    killall memcached
}

setup
runtest
RESULT=$?
cleanup

exit $RESULT
