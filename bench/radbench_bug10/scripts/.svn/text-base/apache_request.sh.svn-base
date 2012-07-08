#!/bin/bash


function testApache {
    i=$1 
    let "max = $i + 100"
    while [ $i -lt $max ]; do
        wget http://localhost:8090/ -o /dev/null -O /dev/null
        let "i = $i + 1"
    done
}

testApache 13 &
testApache 3000 &
testApache 9000 &
testApache 1234 &
testApache 1235 &
testApache 1233 &
wait
exit 0
