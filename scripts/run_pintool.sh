
PIN_ARCH=ia32
# obj-intel64
PIN_ARGS=-inline 1 -mt 1 -slow_asserts
# -filter_rtn <name>
# -filter_no_shared_libs

$CONCURRIT_HOME/pin/pin $PIN_ARGS -t $CONCURRIT_HOME/pin/source/tools/concurrit/$PIN_ARCH/instrumenter.so -- $@
