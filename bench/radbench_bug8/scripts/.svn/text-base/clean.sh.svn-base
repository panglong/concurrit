#!/bin/bash

set -e
set -u
DIR=`dirname $0`
COMMON_SH=$DIR/common.sh
if [ ! -e "$COMMON_SH" ]; then
    echo "ERROR: unable to find common.sh at $COMMON_SH"
    exit 1
fi

source $COMMON_SH

echo "Removing Bin Dir..."
rm -rf $BIN_DIR
echo "Removing libevent source..."
rm -rf $LIBEVENT_SRC
echo "Removing libmemcached source..."
rm -rf $LIBMEMCACHED_SRC
echo "Removing memcached source..."
rm -rf $MEMCACHED_SRC

