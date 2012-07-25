#!/bin/bash

MYPWD=`pwd`

if [ ! -d "$CONCURRIT_HOME" ] || [ ! "$(ls -A $CONCURRIT_HOME)" ]; then
	echo "CONCURRIT_HOME is not set! Aborting installation."
	exit 1
fi

cd $CONCURRIT_HOME

# install google logging
if [ ! -d "$GLOG_ROOT" ] || [ ! "$(ls -A $GLOG_ROOT)" ]; then
	$CONCURRIT_HOME/scripts/install_glog.sh
fi

# install boost
if [ ! -d "$BOOST_ROOT" ] || [ ! "$(ls -A $BOOST_ROOT)" ]; then
	$CONCURRIT_HOME/scripts/install_boost.sh
fi

# compile core library
cd $CONCURRIT_HOME
make clean
make all

# install and compile pintool
$CONCURRIT_HOME/scripts/install_pintool.sh

cd $MYPWD
