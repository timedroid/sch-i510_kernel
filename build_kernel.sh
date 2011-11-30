#!/bin/bash

echo "***** Building : $CONFIG *****"
make clean mrproper > /dev/null 2>&1

make ARCH=arm charge_defconfig 1>/dev/null 2>"$WORK"/errlog.txt
make -j8 ARCH=arm HOSTCFLAGS="-g -O3" #1>"$WORK"/stdlog.txt 2>>"$WORK"/errlog.txt

cp arch/arm/boot/zImage ../../../device/samsung/charge/kernel

echo -e "***** Successfully compiled: $CONFIG *****\n"

