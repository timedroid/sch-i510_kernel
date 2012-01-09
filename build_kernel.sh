#!/bin/bash

echo "***** Building : $CONFIG *****"
make clean mrproper

make ARCH=arm charge_defconfig
make -j8 ARCH=arm HOSTCFLAGS="-g -O3"

cp arch/arm/boot/zImage ../../../device/samsung/charge/kernel
cp drivers/net/wireless/bcm4329/bcm4329.ko ../../../device/samsung/charge

echo -e "***** Successfully compiled: $CONFIG *****\n"

