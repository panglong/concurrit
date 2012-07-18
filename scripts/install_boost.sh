#!/bin/bash

PWD=`pwd`

CHECKOUT_DIR=$HOME/boost_1_50_0
INSTALL_DIR=$HOME/boost

rm -rf $CHECKOUT_DIR
rm -rf $INSTALL_DIR

mkdir -p $CHECKOUT_DIR

cd $HOME
wget http://downloads.sourceforge.net/project/boost/boost/1.50.0/boost_1_50_0.tar.gz
tar -zxvf boost_1_50_0.tar.gz

rm -f boost_1_50_0.tar.gz

mv $CHECKOUT_DIR $INSTALL_DIR

cd $PWD

