#!/bin/bash

cd $CONCURRIT_HOME

make -C bench/nbincrement/ clean
make -C bench/nbincrement/ 
make ARGS="$@" -C bench/nbincrement/ pin

cd $CONCURRIT_HOME