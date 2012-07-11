#!/bin/bash


RUN_DIR=`pwd`

DIR=`dirname $0`
cd $DIR
BIN_DIR=`pwd`

set -u

BIN_LOG_DIR=$BIN_DIR/binlogdir/
if [ ! -d $BIN_LOG_DIR ]; 
then
    echo "Error: bin log directory not found: $BIN_LOG_DIR"
    exit 0
fi

MYSQL_BIN=$BIN_DIR/bin/mysql
if [ ! -e $MYSQL_BIN ]; 
then
    echo "Error: mysql client binary not found: $MYSQL_BIN"
    exit 0
fi

LOG_OUTPUT_NAME=$BIN_LOG_DIR/mysql_bin_log.0*

function executeTest {
    rm -f $BIN_LOG_DIR/*
    $MYSQL_BIN -D test -e 'flush logs;' 
    $MYSQL_BIN -D test -e 'flush logs;' &
    WAIT_PID=$!

    $MYSQL_BIN -D test -e 'insert into a values (1337);'

    if [ $? -ne 0 ]
    then
        echo "Test Aborted--is server up (1)?"
        return 1
    fi

    wait $WAIT_PID
    $MYSQL_BIN -D test -e 'flush logs;'

    if [ $? -ne 0 ]
    then
        echo "Test Aborted--is server up (2)?"
        return 1
    fi

    $MYSQL_BIN -D test -e 'truncate a;'

    if [ $? -ne 0 ]
    then
        echo "Test Aborted--is server up (3)?"
        return 1
    fi

    echo "Test completed..."
    return 0
}

function checkTestResults {
    grep insert $LOG_OUTPUT_NAME

    RETURN_VAL=$?
    if [ $RETURN_VAL -eq 0 ] 
    then
        echo "Test Succeeded"
        echo
        return 0
    elif [ $RETURN_VAL -eq 1 ]; then
        echo "***TEST FAILED***"
        echo
        return 1
    else 
        echo "Test returned unexpected value: $RETURN_VALUE"
        echo
        return 2
    fi
}

executeTest
if [ "$?" != "0" ]; then
    exit 2
fi

checkTestResults
exit $?


