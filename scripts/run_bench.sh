#!/bin/bash

ARGS=( $@ )
# get and remove name of the benchmark from arguments
BENCH=${ARGS[0]}
echo "Running $BENCH"
unset ARGS[0]

BENCHDIR=$CONCURRIT_HOME/bench/$BENCH

# copy configuration files (if exist)
rm -rf $CONCURRIT_HOME/work/*

if [ -f "$BENCHDIR/finfile.txt" ];
then
    cp -f $BENCHDIR/finfile.txt $CONCURRIT_HOME/work/finfile.txt
fi

make -C $BENCHDIR clean
make -C $BENCHDIR 
make ARGS="${ARGS[*]}" -C $BENCHDIR pin
