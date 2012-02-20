
cd $CONCURRIT_HOME

mkdir -p pin/source/tools/concurrit
cd pin/source/tools/concurrit
cp -f $CONCURRIT_HOME/pintool/makefile .
cp -f $CONCURRIT_HOME/pintool/instrumenter.cpp .

make clean
make dir instrumenter.test

cd $CONCURRIT_HOME
