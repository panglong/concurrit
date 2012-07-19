#!/bin/bash

PIN_ARGS="-mt 1 -inline -follow_execv -separate_memory -slow_asserts"

PINTOOL_ARGS="-skip_int3"
PINTOOL_ARGS="$PINTOOL_ARGS -finfile $CONCURRIT_HOME/work/finfile.txt"
PINTOOL_ARGS="$PINTOOL_ARGS -log_file $CONCURRIT_HOME/work/pinlogfile.txt"
PINTOOL_ARGS="$PINTOOL_ARGS -filtered_images_file $CONCURRIT_HOME/work/filtered_images.txt"
PINTOOL_ARGS="$PINTOOL_ARGS -track_func_calls 0"
PINTOOL_ARGS="$PINTOOL_ARGS -inst_top_level 0"

PROGRAM_ARGS="$@"

# -filter_rtn <name>
# -filter_no_shared_libs

$CONCURRIT_TPDIR/pin/pin $PIN_ARGS -t $CONCURRIT_HOME/lib/instrumenter.so $PINTOOL_ARGS -- $PROGRAM_ARGS
