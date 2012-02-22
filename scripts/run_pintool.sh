#!/bin/bash

PWD=`pwd`

PIN_ARGS="-inline 1 -mt 1 -slow_asserts -follow_execv"
PINTOOL_ARGS="-skip_int3"
# -filter_rtn <name>
# -filter_no_shared_libs

$CONCURRIT_TPDIR/pin/pin $PIN_ARGS -t $CONCURRIT_HOME/lib/instrumenter.so $PINTOOL_ARGS -- $@

cd $PWD
