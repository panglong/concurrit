#!/bin/bash

# Script to automate bug15

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

RUN_DIR=/home/elmas/radbench/Benchmarks/bug15

BIN_DIR=$RUN_DIR/bin

cd $BIN_DIR

COMMON_SH=$BIN_DIR/../../scripts/common.sh
if [ ! -e "$COMMON_SH" ]; then
    echo "ERROR: no common functions script"
    exit 0
fi

source $COMMON_SH

MYSQL_SERVER=$BIN_DIR/libexec/mysqld
if [ ! -e "$MYSQL_SERVER" ]; then
    echo "ERROR: cannot find mysql server at: $MYSQL_SERVER"
    exit 0
fi

MYSQL_CLIENT=$BIN_DIR/bin/mysql
if [ ! -e "$MYSQL_CLIENT" ]; then
    echo "ERROR: cannot find mysql client at: $MYSQL_CLIENT"
    exit 0
fi

TEST_HARNESS=$BIN_DIR/testharness.sh
if [ ! -e "$TEST_HARNESS" ]; then
    echo "ERROR: cannot find server test harness at: $TEST_HARNESS"
    exit 0
fi

MYSQL_SERVER_ARGS="--skip-grant --basedir=$BIN_DIR/ --datadir=$BIN_DIR/var/ --pid-file=$BIN_DIR/var/bugonefive.pid --skip-locking --log-bin=$BIN_DIR/binlogdir/mysql_bin_log"

MYSQL_BINLOGS=$BIN_DIR/binlogdir/

SERVER_CALL="{ $1 $MYSQL_SERVER $MYSQL_SERVER_ARGS; } &"

set -u

function setup {
    killAllCmdContainingName mysqld 50
    rm -f /home/elmas/radbench/Benchmarks/bug15/bin/binlogdir/*
    echo "running mysql as follows: $SERVER_CALL"
    LD_PRELOAD=
    LD_PRELOAD="$CONCURRIT_HOME/lib/libconcurrit.so:$CONCURRIT_HOME/lib/libclient.so:$LD_PRELOAD" \
	PATH="$BENCHDIR/bin:$PATH" \
	LD_LIBRARY_PATH="$BENCHDIR/lib:$LD_LIBRARY_PATH" \
	DYLD_LIBRARY_PATH="$BENCHDIR/lib:$DYLD_LIBRARY_PATH" \
    	$MYSQL_SERVER $MYSQL_SERVER_ARGS &
    #eval $SERVER_CALL
    mysqlWaitUntilAvailable
}

function runtest {
    echo "Calling: $TEST_HARNESS"
    $TEST_HARNESS
    return $?
}

function cleanup {
    killAllWithProcName mysqld
    wait
    return 0
}

#RET=2
#while [ "$RET" -eq "2" ]; do
#    setup 
#    runtest
#    RET=$?
#done
#cleanup
#
#if [ "$RET" == "1" ]; then
#    echo "[$0] Return value is $RET"
#    exit $RET
#else
#    echo "[$0] Return value is 0"
#    exit 0
#fi


################################################################################
################################################################################

LD_PRELOAD=

# get and remove name of the benchmark from arguments
BENCH=radbench_bug15
echo "Running $BENCH"

echo "Benchmark is $BENCH"

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
ARGS=( "-l" "$CONCURRIT_HOME/lib/libserver.so" "-p0" "-m1")

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
		$BIN_DIR/test-mysql
		
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

