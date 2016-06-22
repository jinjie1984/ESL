#! /bin/bash

if [ -z $1 ]
then
    echo "Please input ddr freq !"
    exit
fi 

clearcache="echo 3> /proc/sys/vm/drop_caches"
cmd="./jmb-test -n5 -c2"
cpuname=CNR-EB1-singleDie

echo "++++++++++++++++++++++++++++++++++++"
echo "clear caches"
echo 3> /proc/sys/vm/drop_caches
echo "jmb-test running"
echo "$cmd"
$cmd
echo "$cpuname"
echo "DDR-4G-2R  Freq $1 MHz"
echo "++++++++++++++++++++++++++++++++++++"
