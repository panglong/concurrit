#!/bin/bash


echo 
echo "This script will create the table used in the test harness"
echo

RUN_DIR=`pwd`

DIR=`dirname $0`
cd $DIR
SRC_DIR=`pwd`
cd ..
BUG_ROOT=`pwd`

set -u


bin/libexec/mysqld --skip-grant --basedir=`pwd`/bin/ --datadir=`pwd`/bin/var --pid-file=`pwd`/bin/var/bugfifteen.pid --skip-locking --log-bin=`pwd`/bin/binlogdir/mysql_bin_log &
sleep 5

# Can't get table creation to not segfault unless 
# table name is one character in length :(
bin/bin/mysql -u $USER -D test -e 'create table a (id int);'

sleep 1
killall mysqld

