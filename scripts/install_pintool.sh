
cd $COUNIT_HOME

rm -rf pin

tar -xvf pin.tar
mv pin-2.10-43611-gcc.3.4.6-ia32_intel64-linux pin

# create symbolic links
cd pin/source/tools
rm -rf Counit
mkdir Counit
cd Counit
ln -s $COUNIT_HOME/pintool/makefile makefile
ln -s $COUNIT_HOME/pintool/counit.cpp counit.cpp
ln -s $COUNIT_HOME/pintool/pin_base.cpp pin_base.cpp
ln -s $COUNIT_HOME/pintool/vc.h vc.h

cd $COUNIT_HOME
./scripts/compile_pintool.sh

cd $COUNIT_HOME
