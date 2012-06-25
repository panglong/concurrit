#!/bin/bash

ARGS=( $@ )
# get and remove name of the benchmark from arguments
BENCH=${ARGS[0]}
echo "Running $BENCH"
unset ARGS[0]

echo "Benchmark is $BENCH"
echo "Arguments are ${ARGS[*]}"

BENCHDIR=$CONCURRIT_HOME/bench/$BENCH

# copy configuration files (if exist)
rm -rf $CONCURRIT_HOME/work/*

BENCHARGS=
if [ -f "$BENCHDIR/bench_args.txt" ];
then
	BENCHARGS=`cat $BENCHDIR/bench_args.txt`
fi

#create fifo directory
rm -rf /tmp/concurrit/*
mkdir -p /tmp/concurrit/pipe

# run concurrit in server mode
ARGS=( "${ARGS[@]}" "-l" "$CONCURRIT_HOME/lib/libserver.so" "-p0" "-m1" "-r")

LD_PRELOAD="$CONCURRIT_HOME/lib/libconcurrit.so:$BENCHDIR/lib/lib$BENCH.so:$LD_PRELOAD" \
PATH="$BENCHDIR/bin:$PATH" \
LD_LIBRARY_PATH="$BENCHDIR/lib:$LD_LIBRARY_PATH" \
DYLD_LIBRARY_PATH="$BENCHDIR/lib:$DYLD_LIBRARY_PATH" \
	$BENCHDIR/bin/$BENCH ${ARGS[*]} &

TESTPID=$!

cd $BENCHDIR

# run in loop for loader
I=1
while true; do
	echo "ITERATION $I"
	(( I=$I+1 ))
	
	# check if test is still running
	kill -0 $TESTPID
	if [ "$?" -ne "0" ];
	then
		break
	fi
	
	echo "Runnning loader with SUT."
	
	# run loader
	# add bench's library path to the end of arguments
	LD_PRELOAD="$CONCURRIT_HOME/lib/libclient.so:$BENCHDIR/lib/lib$BENCH.so:$LD_PRELOAD" \
	PATH="$BENCHDIR/bin:$PATH" \
	LD_LIBRARY_PATH="$BENCHDIR/lib:$LD_LIBRARY_PATH" \
	DYLD_LIBRARY_PATH="$BENCHDIR/lib:$DYLD_LIBRARY_PATH" \
		$CONCURRIT_HOME/bin/testloader $BENCHARGS
		
	if [ "$?" -ne "0" ];
	then
		echo "SUT aborted. Killing Concurrit."
		kill -9 $TESTPID
		break
	fi
		
	echo "SUT ended."
done

cd $CONCURRIT_HOME
echo "SEARCH ENDED."
