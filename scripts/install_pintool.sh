
cd $CONCURRIT_HOME

rm -rf pin

tar -xvf pin.tar
mv pin-2.10-43611-gcc.3.4.6-ia32_intel64-linux pin

cd $CONCURRIT_HOME
./scripts/compile_pintool.sh

cd $CONCURRIT_HOME
