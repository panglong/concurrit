#!/bin/bash

MAKEBENCH() {
	for CMD in ${ARGS[@]}
	do
		CFLAGS="$CFLAGS -g -gdwarf-2" \
			CXXFLAGS="$CXXFLAGS -g -gdwarf-2" \
				make -C $BENCHDIR $CMD
	done
}

#========================================

ARGS=( $@ )
# get and remove name of the benchmark from arguments
BENCH=${ARGS[0]}
echo "Compiling $BENCH"

#========================================

unset ARGS[0]
if [ ${#ARGS[@]} == 0 ];
then
	ARGS=( "clean" "all" )
fi

#========================================

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
