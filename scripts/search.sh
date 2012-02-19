
cd $CONCURRIT_HOME
make -C tests/search/ clean
make -C tests/search/ 

cd $CONCURRIT_HOME
#scripts/run_pintool.sh tests/search/search
make -C tests/search/ test
cd $CONCURRIT_HOME