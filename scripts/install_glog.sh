#!/bin/bash

MYPWD=`pwd`

CHECKOUT_DIR=$CONCURRIT_TPDIR/google-glog-read-only
INSTALL_DIR=$CONCURRIT_TPDIR/glog

rm -rf $CHECKOUT_DIR
rm -rf $INSTALL_DIR

mkdir -p $CHECKOUT_DIR
mkdir -p $INSTALL_DIR

cd $CONCURRIT_TPDIR
svn checkout http://google-glog.googlecode.com/svn/trunk/ google-glog-read-only

cd $CHECKOUT_DIR
./configure --prefix=$INSTALL_DIR
make
make install

rm -rf $CHECKOUT_DIR

cd $MYPWD

