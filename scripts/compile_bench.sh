#!/bin/bash

ARGS=( $@ )
# get and remove name of the benchmark from arguments
BENCH=${ARGS[0]}
echo "Compiling $BENCH"

BENCHDIR=$CONCURRIT_HOME/bench/$BENCH

make -C $BENCHDIR clean
make -C $BENCHDIR shared
make -C $BENCHDIR all
