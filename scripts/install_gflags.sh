PWD=`pwd`

cd $CONCURRIT_HOME

CHECKOUT_DIR=google-gflags-read-only
INSTALL_DIR=gflags

rm -rf $CHECKOUT_DIR
rm -rf $INSTALL_DIR

mkdir $CHECKOUT_DIR
mkdir $INSTALL_DIR

svn checkout http://google-gflags.googlecode.com/svn/trunk/ google-gflags-read-only

cd $CHECKOUT_DIR
./configure --prefix=$CONCURRIT_HOME/$INSTALL_DIR
make
make install
cd ..
rm -rf $CHECKOUT_DIR

cd $PWD

