#!/bin/bash

checkfail(){ 
	if [[ ! ${PIPESTATUS[0]} ]] 
	then 
		echo $1 failed
		exit 1
	else
		echo $1 succeeded
	fi
}

# setup
DATE=$(date +%m%d)
HOSTCC="ccache gcc"
CROSS_COMPILE="ccache /opt/toolchains/arm-2010q1/bin/arm-none-eabi-"
KERNEL_MODULES="crypto/ansi_cprng.ko drivers/bluetooth/bthid/bthid.ko drivers/cdma_dpram/dpram.ko drivers/ltespi/ltespi.ko drivers/misc/samsung_modemctl/modemctl/modemctl.ko drivers/misc/samsung_modemctl/onedram/onedram.ko drivers/misc/samsung_modemctl/svnet/svnet.ko drivers/misc/vibetonz/vibrator.ko drivers/scsi/scsi_wait_scan.ko drivers/wtlfota_dpram/wtlfota_dpram.ko drivers/net/wireless/bcm4329/bcm4329.ko drivers/staging/android/logger.ko"

# execution!

# build the kernel
exec 2>&1

make ARCH=arm charge_defconfig
checkfail Configuration

make -j6 HOSTCC="$HOSTCC" CROSS_COMPILE="$CROSS_COMPILE" ARCH=arm TARGET_PRODUCT=SCH-I510 modules | tee ../moduleBuild.txt
checkfail "Module compilation"

cp $KERNEL_MODULES usr/chargemtd_initramfs_files/lib/modules
checkfail "Module copy"

make -j6 HOSTCC="$HOSTCC" CROSS_COMPILE="$CROSS_COMPILE" ARCH=arm TARGET_PRODUCT=SCH-I510 zImage  | tee ../kernelBuild.txt
checkfail "zImage compilation"

echo -e "***** Successfully compiled *****\n"
