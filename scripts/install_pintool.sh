#!/bin/bash

PWD=`pwd`

CHECKOUT_DIR=$CONCURRIT_TPDIR/pin-2.10-45467-gcc.3.4.6-ia32_intel64-linux
INSTALL_DIR=$CONCURRIT_TPDIR/pin

cd $CONCURRIT_TPDIR

rm -rf $CONCURRIT_TPDIR/pin

tar -xvf $CONCURRIT_TPDIR/pin-2.10-45467-gcc.3.4.6-ia32_intel64-linux.tar

mv $CHECKOUT_DIR $INSTALL_DIR

$CONCURRIT_HOME/scripts/compile_pintool.sh

cd $PWD
