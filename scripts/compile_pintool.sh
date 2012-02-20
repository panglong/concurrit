
mkdir -p $CONCURRIT_HOME/pin/source/tools/concurrit
cd $CONCURRIT_HOME/pin/source/tools/concurrit
cp -f $CONCURRIT_HOME/pintool/makefile .
cp -f $CONCURRIT_HOME/pintool/instrumenter.cpp .

make clean
make dir instrumenter.test

cd $CONCURRIT_HOME
