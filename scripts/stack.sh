
cd $CONCURRIT_HOME
#make -C tests/stack/ clean
make -C tests/stack/ 

cd $CONCURRIT_HOME
#scripts/run_pintool.sh tests/stack/stack
make -C tests/stack/ test
cd $CONCURRIT_HOME