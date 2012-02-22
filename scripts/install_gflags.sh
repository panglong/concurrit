#!/bin/bash

PWD=`pwd`

CHECKOUT_DIR=$CONCURRIT_TPDIR/google-gflags-read-only
INSTALL_DIR=$CONCURRIT_TPDIR/gflags

rm -rf $CHECKOUT_DIR
rm -rf $INSTALL_DIR

mkdir -p $CHECKOUT_DIR
mkdir -p $INSTALL_DIR

cd $CONCURRIT_TPDIR
svn checkout http://google-gflags.googlecode.com/svn/trunk/ google-gflags-read-only

cd $CHECKOUT_DIR
./configure --prefix=$INSTALL_DIR
make
make install

rm -rf $CHECKOUT_DIR

cd $PWD

