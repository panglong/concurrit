#!/bin/bash

PWD=`pwd`

CHECKOUT_DIR=$CONCURRIT_TPDIR/tbb40_20120613oss
INSTALL_DIR=$CONCURRIT_TPDIR/tbb

rm -rf $CHECKOUT_DIR
rm -rf $INSTALL_DIR

mkdir -p $CHECKOUT_DIR

cd $CONCURRIT_TPDIR
tar -xvf tbb40_20120613oss_lin.tar

mv $CHECKOUT_DIR $INSTALL_DIR

cd $PWD

