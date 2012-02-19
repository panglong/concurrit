
cd $CONCURRIT_HOME

./pin/pin -slow_asserts -t ./pin/source/tools/Counit/obj-intel64/counit.so -filter_no_shared_libs -- $1

cd $CONCURRIT_HOME

# -filter_rtn <name>
