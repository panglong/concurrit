#!/bin/bash

function is_running(pid) {
	kill -0 $pid
	return $?
}

ARGS=( $@ )
# get and remove name of the benchmark from arguments
BENCH=${ARGS[0]}
echo "Running $BENCH"
unset ARGS[0]

BENCHDIR=$CONCURRIT_HOME/bench/$BENCH

# copy configuration files (if exist)
rm -rf $CONCURRIT_HOME/work/*

BENCHARGS=
if [ -f "$BENCHDIR/bench_args.txt" ];
then
	BENCHARGS=`cat $BENCHDIR/bench_args.txt`
fi

# run concurrit in server mode
# -e server makes concurrit load libserver.so
LD_PRELOAD=$CONCURRIT_HOME/lib/libconcurrit.so:$LD_PRELOAD \
	$BENCHDIR/bin/$BENCH "$ARGS -e server" &
TESTPID=$!

# run in loop for loader
while true; do
	# check if test is still running
	if [ is_running($TESTPID) -ne "0" ];
	then
		echo "REMOTE: DSL ended. Breaking the loop."
		break
	fi
	
	echo "REMOTE: SUT starting."
	
	# run loader
	# add bench's library path to the end of arguments
	LD_PRELOAD=$CONCURRIT_HOME/lib/libclient.so:$LD_PRELOAD \
		$CONCURRIT_HOME/bin/testloader $BENCHARGS $BENCHDIR/lib/lib$BENCH.so
		
	echo "REMOTE: SUT ended."
done

echo "REMOTE: Search ended."
