#!/bin/bash

#script to automate experiments

function usage {
    echo "Usage: $0 [command modifier]"
    echo
    echo "Runs the benchmark.  If a command modifier is given, " 
    echo "then runs the benchmark under the command. Returns 0 if "
    echo "the bug is not found and 1 if the bug is found"
    echo
    echo "e.g. $0 \"pin --\" will result in the benchmark being run "
    echo "under pin (with an empty tool"
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

RUN_DIR=/home/elmas/radbench/Benchmarks/bug8

BIN_DIR=$RUN_DIR/bin

MEMSERVER=$BIN_DIR/install/bin/memcached
if [ ! -e "$MEMSERVER" ]; then
    echo "ERROR: Memcached server not found at $MEMSERVER"
    exit 1
fi

LIBPATH=$BIN_DIR/install/lib
if [ ! -d "$LIBPATH" ]; then
    echo "ERROR: Libraries not found at $LIBPATH"
    exit 1
fi

EXECUTABLE=$BIN_DIR/test-memcached
if [ ! -e "$EXECUTABLE" ]; then
    echo "ERROR: Executable not found at $EXECUTABLE"
    exit 1
fi
TEST_CALL="$1 $EXECUTABLE"

cd $RUN_DIR

function waitUntilTimeout {
    POLLS=20
    COUNT=0
    while [ "$POLLS" -gt "$COUNT" ]; 
    do
        kill -0 $1 
        if [ "$?" != "0" ]; then
            wait $1
            return $?
        fi
        sleep .1
        let "COUNT = $COUNT + 1"
    done
    kill $1
    sleep .2
    kill $1
    sleep .2
    kill -9 $1
    return 1
}

function setup {
	killall memcached
	
	echo "Launching memcached server"
	LD_PRELOAD="$CONCURRIT_HOME/lib/libconcurrit.so:$CONCURRIT_HOME/lib/libclient.so:$LD_PRELOAD" \
	PATH="$BENCHDIR/bin:$PATH" \
	LD_LIBRARY_PATH="$BENCHDIR/lib:$LD_LIBRARY_PATH" \
	DYLD_LIBRARY_PATH="$BENCHDIR/lib:$DYLD_LIBRARY_PATH" \
    	$MEMSERVER -U 0 -p 11211 -r 4 -d -t 10
    sleep .5
    return 0
}

function runtest {
    echo "Calling: $TEST_CALL"
    $TEST_CALL
    return $?
}

function cleanup {
    killall memcached
    sleep .2
    killall memcached
}

#setup
#runtest
#RESULT=$?
#cleanup

#exit $RESULT


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
ARGS=( "${ARGS[@]}" "-l" "$CONCURRIT_HOME/lib/libserver.so" "-p0" "-m1")

echo "Running concurrit in server mode"

LD_PRELOAD="$CONCURRIT_HOME/lib/libconcurrit.so:$LD_PRELOAD" \
PATH="$BENCHDIR/bin:$PATH" \
LD_LIBRARY_PATH="$BENCHDIR/lib:$LD_LIBRARY_PATH" \
DYLD_LIBRARY_PATH="$BENCHDIR/lib:$DYLD_LIBRARY_PATH" \
	$BENCHDIR/bin/$BENCH ${ARGS[*]} &

TESTPID=$!

sleep 1

echo "Running memcached server"
# run server
setup


cd $BENCHDIR

# run in loop for loader
I=1
#while true; do
	echo "ITERATION $I"
	(( I=$I+1 ))
	
	# check if test is still running
	kill -0 $TESTPID
	if [ "$?" -ne "0" ];
	then
		break
	fi
	
	echo "Running request process."
	
	# run harness
	# add bench's library path to the end of arguments
	LD_PRELOAD="$CONCURRIT_HOME/lib/libconcurrit.so:$LD_PRELOAD" \
	PATH="$BENCHDIR/bin:$PATH" \
	LD_LIBRARY_PATH="$BENCHDIR/lib:$LIBPATH:$LD_LIBRARY_PATH" \
	DYLD_LIBRARY_PATH="$BENCHDIR/lib:$DYLD_LIBRARY_PATH" \
		$BIN_DIR/test-memcached
		
	if [ "$?" -ne "0" ];
	then
		echo "SUT aborted. Killing Concurrit."
		kill -9 $TESTPID
		break
	fi
		
	echo "SUT ended."
#done

cleanup

echo "SEARCH ENDED."

