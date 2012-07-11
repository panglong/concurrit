#!/bin/bash

# Script to automate bug15

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

COMMON_SH=$BIN_DIR/../../scripts/common.sh
if [ ! -e "$COMMON_SH" ]; then
    echo "ERROR: no common functions script"
    exit 0
fi

source $COMMON_SH

MYSQL_SERVER=$BIN_DIR/libexec/mysqld
if [ ! -e "$MYSQL_SERVER" ]; then
    echo "ERROR: cannot find mysql server at: $MYSQL_SERVER"
    exit 0
fi

MYSQL_CLIENT=$BIN_DIR/bin/mysql
if [ ! -e "$MYSQL_CLIENT" ]; then
    echo "ERROR: cannot find mysql client at: $MYSQL_CLIENT"
    exit 0
fi

TEST_HARNESS=$BIN_DIR/testharness.sh
if [ ! -e "$TEST_HARNESS" ]; then
    echo "ERROR: cannot find server test harness at: $TEST_HARNESS"
    exit 0
fi

MYSQL_SERVER_ARGS="--skip-grant --basedir=$BIN_DIR/ --datadir=$BIN_DIR/var/ --pid-file=$BIN_DIR/var/bugonefive.pid --skip-locking --log-bin=$BIN_DIR/binlogdir/mysql_bin_log"

MYSQL_BINLOGS=$BIN_DIR/binlogdir/

SERVER_CALL="{ $1 $MYSQL_SERVER $MYSQL_SERVER_ARGS; } &"

set -u

function setup {
    killAllCmdContainingName mysqld 50
    echo "running mysql as follows: $SERVER_CALL"
    eval $SERVER_CALL
    mysqlWaitUntilAvailable
}

function runtest {
    echo "Calling: $TEST_HARNESS"
    $TEST_HARNESS
    return $?
}

function cleanup {
    killAllWithProcName mysqld
    wait
    return 0
}

RET=2
while [ "$RET" -eq "2" ]; do
    setup 
    runtest
    RET=$?
done
cleanup

if [ "$RET" == "1" ]; then
    echo "[$0] Return value is $RET"
    exit $RET
else
    echo "[$0] Return value is 0"
    exit 0
fi

