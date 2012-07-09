#!/bin/bash

# Script to automate bug10

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

RUN_DIR=/home/elmas/radbench/Benchmarks/bug10

BIN_DIR=$RUN_DIR/bin

EXECUTABLE=$BIN_DIR/../scripts/apache_request.sh
if [ ! -e "$EXECUTABLE" ]; then
    echo "ERROR: Executable not found at $EXECUTABLE"
    exit 1
fi

HTTPD_SERVER=$BIN_DIR/install/bin/httpd
if [ ! -e "$HTTPD_SERVER" ]; then
    echo "ERROR: httpd not found at $HTTPD_SERVER"
    exit 1
fi

ERROR_LOG=$BIN_DIR/install/logs/error_log

COMMON_SH=$BIN_DIR/../../scripts/common.sh
if [ ! -e "$COMMON_SH" ]; then
    echo "ERROR: no common functions script"
    exit 0
fi

source $COMMON_SH

cd $RUN_DIR

TEST_CALL="{ $1 $HTTPD_SERVER -DNO_DETACH -DFOREGROUND; } & "

function setup {
    killAllCmdContainingName httpd 50
    ipcs -s | xargs ipcrm sem
    rm -f $ERROR_LOG
    echo "Launching server as follows: $TEST_CALL"
    LD_PRELOAD=
    LD_PRELOAD="$CONCURRIT_HOME/lib/libconcurrit.so:$CONCURRIT_HOME/lib/libclient.so:$LD_PRELOAD" \
	PATH="$BENCHDIR/bin:$PATH" \
	LD_LIBRARY_PATH="$BENCHDIR/lib:$LD_LIBRARY_PATH" \
	DYLD_LIBRARY_PATH="$BENCHDIR/lib:$DYLD_LIBRARY_PATH" \
    	MALLOC_CHECK_=3 /home/elmas/radbench/Benchmarks/bug10/bin/install/bin/httpd -X -DNO_DETACH -DFOREGROUND &
    #eval $TEST_CALL
    #serverWaitUntilAvailable "8090"
    return 0
}

function runtest {
    echo "Calling: $EXECUTABLE"
    $EXECUTABLE
    return 0
}

function cleanup {
    killAllWithProcName httpd 50
    wait
    echo
    echo "ERROR LOG:"
    cat $ERROR_LOG
    echo
    grep "Segmentation fault" $ERROR_LOG &> /dev/null
    if [ "$?" == "0" ]; then
        return 1
    else 
        grep "assertion" $ERROR_LOG &> /dev/null
        if [ "$?" == "0" ]; then
            return 1
        else
            return 0
        fi
    fi
}

#setup
#runtest
#cleanup
#RESULT=$?

#echo "[$0] Return value is $RESULT"
#exit $RESULT


################################################################################
################################################################################


ARGS=( $@ )
# get and remove name of the benchmark from arguments
BENCH=radbench_bug10
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

echo "Running apache server"
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
	LD_LIBRARY_PATH="$BENCHDIR/lib:$LD_LIBRARY_PATH" \
	DYLD_LIBRARY_PATH="$BENCHDIR/lib:$DYLD_LIBRARY_PATH" \
		$BIN_DIR/test-apache
		
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

