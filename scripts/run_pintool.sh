#!/bin/bash

PIN_ARGS="-mt 1 -inline -slow_asserts -follow_execv -separate_memory"
PINTOOL_ARGS="-skip_int3 -finfile $CONCURRIT_HOME/work/finfile.txt -log_file $CONCURRIT_HOME/work/pinlogfile.txt -filtered_images_file $CONCURRIT_HOME/work/filtered_images.txt -track_func_calls 0 -inst_top_level 1"
PROGRAM_ARGS="$@"

# -filter_rtn <name>
# -filter_no_shared_libs

$CONCURRIT_TPDIR/pin/pin $PIN_ARGS -t $CONCURRIT_HOME/lib/instrumenter.so $PINTOOL_ARGS -- $PROGRAM_ARGS
