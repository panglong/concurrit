
cd $COUNIT_HOME
rm -rf glog

tar -xvf glog.tar
mv glog-0.3.1 glog

cd glog
./configure --prefix=$COUNIT_HOME/glog
make
make install
cd $COUNIT_HOME
