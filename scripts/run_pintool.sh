PWD=`pwd`

PIN_ARGS="-inline 1 -mt 1 -slow_asserts"
# -filter_rtn <name>
# -filter_no_shared_libs

$CONCURRIT_HOME/pin/pin $PIN_ARGS -t $CONCURRIT_HOME/lib/instrumenter.so -- $@

cd $PWD
