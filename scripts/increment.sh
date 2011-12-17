
cd $COUNIT_HOME
make -C tests/increment/ clean
make -C tests/increment/ 

cd $COUNIT_HOME
#scripts/run_pintool.sh tests/increment/increment
tests/increment/increment
cd $COUNIT_HOME