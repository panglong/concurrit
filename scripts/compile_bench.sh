#!/bin/bash

MAKEBENCH() {
	make -C $BENCHDIR clean
	make -C $BENCHDIR shared
	make -C $BENCHDIR all
}

ARGS=( $@ )
# get and remove name of the benchmark from arguments
BENCH=${ARGS[0]}
echo "Compiling $BENCH"

if [ $BENCH != "all" ];
then
	BENCHDIR=$CONCURRIT_HOME/bench/$BENCH
	MAKEBENCH
else
	BENCHROOTS=$CONCURRIT_HOME/bench/*
	for BENCHDIR in $BENCHROOTS
	do
		echo "Compiling $BENCHDIR"
		MAKEBENCH
	done
fi
