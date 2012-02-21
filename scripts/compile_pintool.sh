PWD=`pwd`

PIN_ARCH="obj-ia32"

mkdir -p $CONCURRIT_HOME/pin/source/tools/concurrit
cd $CONCURRIT_HOME/pin/source/tools/concurrit

cp -f $CONCURRIT_HOME/pintool/makefile .
cp -f $CONCURRIT_HOME/pintool/instrumenter.cpp .

make clean
make dir instrumenter.test

SOURCE="$CONCURRIT_HOME/pin/source/tools/concurrit/$PIN_ARCH/instrumenter.so"
if [ -f "$SOURCE" ]
then
	cp -f $SOURCE $CONCURRIT_HOME/lib/
else
	echo "Pintool does not exist!"
fi

cd $PWD
