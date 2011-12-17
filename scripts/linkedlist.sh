
cd $CONCURRIT_HOME
make -C tests/linkedlist/ clean
make -C tests/linkedlist/ 

cd $CONCURRIT_HOME
#scripts/run_pintool.sh tests/linkedlist/linkedlist
make -C tests/linkedlist/ test
cd $CONCURRIT_HOME