#!/bin/bash

# Script to automate bug 8

function usage {
    echo "Usage: $0 [command modifier]"
    echo
    echo "Runs the benchmark.  If a command modifier is given, " 
    echo "then runs the benchmark under the command. Returns 0 if "
    echo "the bug is not found and 1 if the bug is found"
    echo
    echo "e.g. $0 \"pin --\" will result in the benchmark being run "
    echo "under pin (with an empty tool)"
    echo
}

if [ "$1" == "-h" ] || 
    [ "$1" == "--h" ] || 
    [ "$1" == "-help" ] || 
    [ "$1" == "--help" ]; 
then
    usage
    exit 0
fi


RUN_DIR=`pwd`

DIR=`dirname $0`
cd $DIR
SCRIPT_DIR=/home/elmas/radbench/Benchmarks/bug8/scripts

COMMON_SH=$SCRIPT_DIR/../../scripts/common.sh
if [ ! -e "$COMMON_SH" ]; then
    echo "ERROR: no common functions script"
    exit 0
fi

source $COMMON_SH


EXECUTABLE=$SCRIPT_DIR/../bin/test-memcached
if [ ! -e "$EXECUTABLE" ]; then
    echo "ERROR: Executable not found at $EXECUTABLE"
    exit 1
fi
TEST_CALL="{ $EXECUTABLE; } & "

SERVER_ALIVE=$SCRIPT_DIR/../bin/server_alive
if [ ! -e "$SERVER_ALIVE" ]; then
    echo "ERROR: server checker not found at $SERVER_ALIVE"
    exit 1
fi
TEST_CALL="{ $EXECUTABLE; } & "

MEMCACHED_SERVER=$SCRIPT_DIR/../bin/install/bin/memcached
if [ ! -e "$MEMCACHED_SERVER" ]; then
    echo "ERROR: server not found at $MEMCACHED_SERVER"
    exit 1
fi

SERVER_CALL="{ $1 $MEMCACHED_SERVER -U 0 -p 11211 -r 4; } &"

set -u

cd $RUN_DIR

function setup {
    killAllWithProcName memcached 10
    echo "[$0] Starting Server: $SERVER_CALL"
    
    LD_PRELOAD="$CONCURRIT_HOME/lib/libclient.so:$BENCHDIR/lib/lib$BENCH.so:$LD_PRELOAD" \
	PATH="$BENCHDIR/bin:$PATH" \
	LD_LIBRARY_PATH="$BENCHDIR/lib:$LD_LIBRARY_PATH" \
	DYLD_LIBRARY_PATH="$BENCHDIR/lib:$DYLD_LIBRARY_PATH" \
   		$MEMCACHED_SERVER -U 0 -p 11211 -r 4 &
    
    export LD_LIBRARY_PATH=$SCRIPT_DIR/../bin/install/lib 
    eval $SERVER_ALIVE
    RET=$?
    while [ "$RET" != "0" ]; do
        sleep .1
        eval $SERVER_ALIVE
        RET=$?
    done
    return 0
}

function runtest {
    eval $TEST_CALL
    PID=$!
    wait $PID
    return $?
}

function cleanup {
    killAllWithProcName memcached
    return 0
}

#setup
#runtest
#RET=$?
#cleanup

#echo "[$0] Return value is $RET"
#exit $RET



################################################################################
################################################################################


ARGS=( $@ )
# get and remove name of the benchmark from arguments
BENCH=radbench_bug8
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

echo "Running concurrit in server mode"

LD_PRELOAD="$CONCURRIT_HOME/lib/libconcurrit.so:$LD_PRELOAD" \
PATH="$BENCHDIR/bin:$PATH" \
LD_LIBRARY_PATH="$BENCHDIR/lib:$LD_LIBRARY_PATH" \
DYLD_LIBRARY_PATH="$BENCHDIR/lib:$DYLD_LIBRARY_PATH" \
	$BENCHDIR/bin/$BENCH ${ARGS[*]} &

TESTPID=$!

echo "Running memcached server"

# run server
setup

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
	
	echo "Runnning request process."
	
	# run loader
	# add bench's library path to the end of arguments
	LD_PRELOAD="$CONCURRIT_HOME/lib/libconcurrit.so:$LD_PRELOAD" \
	PATH="$BENCHDIR/bin:$PATH" \
	LD_LIBRARY_PATH="$BENCHDIR/lib:$LD_LIBRARY_PATH" \
	DYLD_LIBRARY_PATH="$BENCHDIR/lib:$DYLD_LIBRARY_PATH" \
		$SCRIPT_DIR/../bin/test-memcached
		
	if [ "$?" -ne "0" ];
	then
		echo "SUT aborted. Killing Concurrit."
		kill -9 $TESTPID
		break
	fi
		
	echo "SUT ended."
done

cleanup

echo "SEARCH ENDED."
