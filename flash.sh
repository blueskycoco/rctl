#! /bin/bash -e
make APPNAME=$1 clean
make APPNAME=$1
MSP430Flasher.exe -w bin/$1.hex -s -v -g -z [VCC]
