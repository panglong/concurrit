PWD=`pwd`

rm -rf $CONCURRIT_HOME/pin

tar -xvf $CONCURRIT_HOME/pin.tar
mv $CONCURRIT_HOME/pin-2.10-45467-gcc.3.4.6-ia32_intel64-linux $CONCURRIT_HOME/pin
#mv $CONCURRIT_HOME/pin-2.10-43611-gcc.3.4.6-ia32_intel64-linux $CONCURRIT_HOME/pin

$CONCURRIT_HOME/scripts/compile_pintool.sh

cd $PWD
