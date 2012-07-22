#!/bin/bash

PWD=`pwd`

cd $CONCURRIT_HOME

# install google logging
$CONCURRIT_HOME/scripts/install_glog.sh

# install boost
$CONCURRIT_HOME/scripts/install_boost.sh

# compile core library
cd $CONCURRIT_HOME
make clean
make all

# install and compile pintool
$CONCURRIT_HOME/scripts/install_pintool.sh

cd $PWD
