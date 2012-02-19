
cd $CONCURRIT_HOME
#make -C tests/increment/ clean
make -C tests/increment/ 

cd $CONCURRIT_HOME
#scripts/run_pintool.sh tests/increment/increment
tests/increment/increment
cd $CONCURRIT_HOME