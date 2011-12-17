
cd $COUNIT_HOME
make -C tests/linkedlist/ clean
make -C tests/linkedlist/ 

cd $COUNIT_HOME
#scripts/run_pintool.sh tests/linkedlist/linkedlist
make -C tests/linkedlist/ test
cd $COUNIT_HOME