#!/bin/bash

MAKEBENCH() {
	make -C $BENCHDIR driver
}

ARGS=( $@ )
# get and remove name of the benchmark from arguments
BENCH=${ARGS[0]}
echo "Compiling $BENCH"

BENCHDIR=$CONCURRIT_HOME/bench/$BENCH
MAKEBENCH
