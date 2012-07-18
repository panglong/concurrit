#!/bin/bash

export CONCURRIT_HOME=$HOME/concurrit
export CONCURRIT_TPDIR=$CONCURRIT_HOME/third_party

export LD_LIBRARY_PATH=$CONCURRIT_HOME/lib/:$LD_LIBRARY_PATH
export DYLD_LIBRARY_PATH=$CONCURRIT_HOME/lib/:$DYLD_LIBRARY_PATH

# glog
export LD_LIBRARY_PATH=$CONCURRIT_TPDIR/glog/lib/:$LD_LIBRARY_PATH
export DYLD_LIBRARY_PATH=$CONCURRIT_TPDIR/glog/lib/:$DYLD_LIBRARY_PATH

# tbb
export LD_LIBRARY_PATH=$CONCURRIT_TPDIR/tbb/lib/intel64/cc4.1.0_libc2.4_kernel2.6.16.21/:$LD_LIBRARY_PATH
export DYLD_LIBRARY_PATH=$CONCURRIT_TPDIR/tbb/lib/intel64/cc4.1.0_libc2.4_kernel2.6.16.21/:$DYLD_LIBRARY_PATH

# radbench
export RADBENCH_HOME=$HOME/radbench/Benchmarks

# boost
export BOOST_ROOT=$HOME/boost
export BOOST_PATH=$BOOST_ROOT

# for glog
export GLOG_logtostderr=1
export GLOG_v=0
