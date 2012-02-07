PWD=`pwd`

cd $CONCURRIT_HOME

CHECKOUT_DIR=pth-2.0.7
INSTALL_DIR=pth

rm -rf $CHECKOUT_DIR
rm -rf $INSTALL_DIR

mkdir $CHECKOUT_DIR
mkdir $INSTALL_DIR

sudo ftp ftp://ftp.gnu.org/gnu/pth/$CHECKOUT_DIR.tar.gz
tar -zxvf $CHECKOUT_DIR.tar.gz

cd $CHECKOUT_DIR
./configure --prefix=$CONCURRIT_HOME/$INSTALL_DIR
make
make test
make install
cd ..
rm -rf $CHECKOUT_DIR
rm -f $CHECKOUT_DIR.tar.gz

cd $PWD

