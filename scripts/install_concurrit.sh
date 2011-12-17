
cd $CONCURRIT_HOME

# install google logging
scripts/install_glog.sh

# compile core
cd $CONCURRIT_HOME
make clean
make

# install and compile pintool
cd $CONCURRIT_HOME
scripts/install_pintool.sh

cd $CONCURRIT_HOME
