
cd $COUNIT_HOME

# install google logging
scripts/install_glog.sh

# compile core
cd $COUNIT_HOME
make clean
make

# install and compile pintool
cd $COUNIT_HOME
scripts/install_pintool.sh

cd $COUNIT_HOME
