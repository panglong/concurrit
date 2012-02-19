
cd $CONCURRIT_HOME
#make -C tests/nbincrement/ clean
make -C tests/nbincrement/ 

cd $CONCURRIT_HOME
#scripts/run_pintool.sh tests/nbincrement/nbincrement
make -C tests/nbincrement/ test
cd $CONCURRIT_HOME