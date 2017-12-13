#! /bin/bash -e
make
MSP430Flasher.exe -w rctl.hex -v -g -z [VCC]
