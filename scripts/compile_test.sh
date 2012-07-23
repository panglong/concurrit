#!/bin/bash

TESTFILE=$1
if [ -ne "$TESTFILE" ];
then
    echo "Test file $TESTFILE does not exist!"
    exit 1
fi


make -C $CONCURRIT_HOME script
