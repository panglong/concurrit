#!/bin/bash

set -e

DIR=`dirname $0`
cd $DIR
cd ..
BUG_ROOT=`pwd`
if [ ! -d "$BUG_ROOT" ]; then
    echo "ERROR: unable to find bug8 directory"
    exit 1
fi;

SRC_DIR=$BUG_ROOT/src
if [ ! -d "$SRC_DIR" ]; then
    echo "ERROR: unable to find bug8 src directory"
    exit 1
fi;

LIBEVENT_ZIP=$SRC_DIR/libevent-1.4.13-stable.tar.gz
if [ ! -e "$LIBEVENT_ZIP" ]; then
    echo "ERROR: unable to find zipped libevent source at $LIBEVENT_ZIP"
    exit 1
fi;

LIBMEMCACHED_ZIP=$SRC_DIR/libmemcached-0.43.tar.gz
if [ ! -e "$LIBMEMCACHED_ZIP" ]; then
    echo "ERROR: unable to find libmemcached source at $LIBMEMCACHED_ZIP"
    exit 1
fi;

MEMCACHED_ZIP=$SRC_DIR/memcached-1.4.5.tar.gz
if [ ! -e "$MEMCACHED_ZIP" ]; then
    echo "ERROR: unable to find zipped memcached source at $MEMCACHED_ZIP"
    exit 1
fi;

TEST_SRC=$SRC_DIR/test-harness.cpp
if [ ! -e "$TEST_SRC" ]; then
    echo "ERROR: unable to find test harness $TEST_SRC"
    exit 1
fi;

LIBEVENT_SRC=$SRC_DIR/libevent-1.4.13-stable
LIBMEMCACHED_SRC=$SRC_DIR/libmemcached-0.43
MEMCACHED_SRC=$SRC_DIR/memcached-1.4.5

BIN_DIR=$BUG_ROOT/bin
mkdir -p $BIN_DIR
if [ ! -d "$BIN_DIR" ]; then
    echo "ERROR: Unable to Create binary directory $BIN_DIR"
    exit 1
fi;

INSTALL_PREFIX=$BIN_DIR/install
INSTALL_LIB_PATH=$BIN_DIR/install/lib
INSTALL_INCLUDE_PATH=$BIN_DIR/install/include
INSTALL_BIN_PATH=$BIN_DIR/install/bin

BIN_DIR_README=$BIN_DIR/run

