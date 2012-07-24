#!/bin/bash

MYPWD=`pwd`

PIN_ARCH="obj-intel64"
PINTOOL_DIR=$CONCURRIT_TPDIR/pin/source/tools/concurrit

mkdir -p $PINTOOL_DIR
cd $PINTOOL_DIR
rm -rf *

cp -f $CONCURRIT_HOME/pintool/makefile .
cp -f $CONCURRIT_HOME/pintool/instrumenter.cpp .

make clean
make $@ dir instrumenter.test

SOURCE="$PINTOOL_DIR/$PIN_ARCH/instrumenter.so"
if [ -f "$SOURCE" ]
then
	cp -f $SOURCE $CONCURRIT_HOME/lib/
else
	echo "Pintool does not exist!"
fi

cd $MYPWD
