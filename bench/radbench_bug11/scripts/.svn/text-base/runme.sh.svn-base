#!/bin/bash

# Script to automate bug11

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
cd ../bin
BIN_DIR=`pwd`
EXECUTABLE=$BIN_DIR/install/bin/ab
if [ ! -e "$EXECUTABLE" ]; then
    echo "ERROR: Executable not found at $EXECUTABLE"
    exit 1
fi
TEST_HARNESS="$EXECUTABLE -c 100 -t 10 http://localhost:8091/"

HTTPD_SERVER=$BIN_DIR/install/bin/httpd
if [ ! -e "$HTTPD_SERVER" ]; then
    echo "ERROR: httpd not found at $HTTPD_SERVER"
    exit 1
fi

ERROR_LOG=$BIN_DIR/install/logs/error_log

COMMON_SH=$BIN_DIR/../../scripts/common.sh
if [ ! -e "$COMMON_SH" ]; then
    echo "ERROR: no common functions script"
    exit 0
fi

source $COMMON_SH

cd $RUN_DIR

TEST_CALL="{ $1 $HTTPD_SERVER -DNO_DETACH -DFOREGROUND; } &"

function setup {
    killAllCmdContainingName httpd 50
    rm -f $ERROR_LOG
    echo "Launching server as follows: $TEST_CALL"
    eval $TEST_CALL
    serverWaitUntilAvailable "8091"
    return 0
}

function runtest {
    echo "Calling: $TEST_HARNESS"
    $TEST_HARNESS
    return 0
}

function cleanup {
    killAllWithProcName httpd
    wait
    echo
    echo "ERROR LOG:"
    cat $ERROR_LOG
    echo
    egrep "Segmentation fault|ap_queue_info_set_idle failed|SIGKILL" $ERROR_LOG &> /dev/null
    if [ "$?" == "0" ]; then
        return 1
    else 
        return 0
    fi
}

setup
runtest
cleanup
RESULT=$?

echo "[$0] Return value is $RESULT"
exit $RESULT
