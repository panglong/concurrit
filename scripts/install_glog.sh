
cd $CONCURRIT_HOME
rm -rf glog

tar -xvf glog.tar
mv glog-0.3.1 glog

cd glog
./configure --prefix=$CONCURRIT_HOME/glog
make
make install
cd $CONCURRIT_HOME
