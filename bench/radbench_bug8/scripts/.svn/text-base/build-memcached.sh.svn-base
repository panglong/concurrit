#!/bin/bash

set -e
DIR=`dirname $0`
COMMON_SH=$DIR/common.sh
if [ ! -e "$COMMON_SH" ]; then
    echo "ERROR: unable to find common.sh at $COMMON_SH"
    exit 1
fi

source $COMMON_SH

set -u

if [ ! -d "$LIBEVENT_SRC" ]; then 
    echo
    echo "Unzipping Libevent Source..."
    cd $SRC_DIR
    tar xzf $LIBEVENT_ZIP
    if [ ! -d "$LIBEVENT_SRC" ]; then
        echo "ERROR: unzip failed to create $LIBEVENT_SRC"
        exit 1
    fi
fi

if [ ! -d "$MEMCACHED_SRC" ]; then 
    echo
    echo "Unzipping Memcached Source..."
    cd $SRC_DIR
    tar xzf $MEMCACHED_ZIP
    if [ ! -d "$MEMCACHED_SRC" ]; then
        echo "ERROR: unzip failed to create $MEMCACHED_SRC"
        exit 1
    fi
fi

if [ ! -d "$LIBMEMCACHED_SRC" ]; then 
    echo
    echo "Unzipping Memcached Source..."
    cd $SRC_DIR
    tar xzf $LIBMEMCACHED_ZIP
    if [ ! -d "$LIBMEMCACHED_SRC" ]; then
        echo "ERROR: unzip failed to create $LIBMEMCACHED_SRC"
        exit 1
    fi
fi

echo
echo "Starting Building Libevent..."
cd $LIBEVENT_SRC
./configure --prefix=$INSTALL_PREFIX
make
make install
echo "Libevent installed to $INSTALL_PREFIX"

echo
echo "Starting Building Memcached..."
cd $MEMCACHED_SRC
./configure --prefix=$INSTALL_PREFIX
make
make install
echo "Memcached installed to $INSTALL_PREFIX"

echo
echo "Starting Building Libmemcached..."
cd $LIBMEMCACHED_SRC
PATH=$INSTALL_BIN_PATH:$PATH ./configure --prefix=$INSTALL_PREFIX
make
make install
echo "Libmemcached installed to $INSTALL_PREFIX"

