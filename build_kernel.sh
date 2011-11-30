#!/bin/bash

echo "***** Building : $CONFIG *****"
make clean mrproper

make ARCH=arm charge_defconfig
make -j8 ARCH=arm HOSTCFLAGS="-g -O3"

cp arch/arm/boot/zImage ../../../device/samsung/charge/kernel

echo -e "***** Successfully compiled: $CONFIG *****\n"

