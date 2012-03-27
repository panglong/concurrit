#!/bin/bash

cd $CONCURRIT_HOME

make -C bench/bbuf/ clean
make -C bench/bbuf/ 
make ARGS="$@" -C bench/bbuf/ pin

cd $CONCURRIT_HOME