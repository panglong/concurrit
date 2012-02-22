#!/bin/bash

PWD=`pwd`

CHECKOUT_DIR=$CONCURRIT_TPDIR/pth-2.0.7
INSTALL_DIR=$CONCURRIT_TPDIR/pth

rm -rf $CHECKOUT_DIR
rm -rf $INSTALL_DIR

mkdir -p $CHECKOUT_DIR
mkdir -p $INSTALL_DIR

cd $CONCURRIT_TPDIR
tar -zxvf pth-2.0.7.tar.gz

cd $CHECKOUT_DIR
./configure --prefix=$INSTALL_DIR
make
make install

rm -rf $CHECKOUT_DIR

cd $PWD

