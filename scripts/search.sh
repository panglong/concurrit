
cd $COUNIT_HOME
make -C tests/search/ clean
make -C tests/search/ 

cd $COUNIT_HOME
#scripts/run_pintool.sh tests/search/search
make -C tests/search/ test
cd $COUNIT_HOME