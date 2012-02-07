
cd $CONCURRIT_HOME
make -C tests/modular/ clean
make -C tests/modular/ 

cd $CONCURRIT_HOME
#scripts/run_pintool.sh tests/modular/modular
make -C tests/modular/ test
cd $CONCURRIT_HOME