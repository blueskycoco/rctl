#! /bin/bash -e

make APPNAME=$1 ID=$2 TIME=$3 clean
make APPNAME=$1 ID=$2 TIME=$3
