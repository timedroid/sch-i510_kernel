#!/bin/bash

checkfail(){ 
	if [[ ! ${PIPESTATUS[0]} ]] 
	then 
		echo $1 failed
		mv ../irs.git ../charge_initramfs/.git
		exit 1
	else
		echo $1 succeeded
	fi
}

# setup
DATE=$(date +%m%d)
HOSTCC="ccache gcc"
CROSS_COMPILE="ccache /opt/toolchains/arm-2010q1/bin/arm-none-eabi-"
KERNEL_MODULES="crypto/ansi_cprng.ko drivers/bluetooth/bthid/bthid.ko drivers/cdma_dpram/dpram.ko drivers/ltespi/ltespi.ko drivers/misc/samsung_modemctl/modemctl/modemctl.ko drivers/misc/samsung_modemctl/onedram/onedram.ko drivers/misc/samsung_modemctl/svnet/svnet.ko drivers/misc/vibetonz/vibrator.ko drivers/scsi/scsi_wait_scan.ko drivers/wtlfota_dpram/wtlfota_dpram.ko drivers/net/wireless/bcm4329/dhd.ko drivers/staging/android/logger.ko"

# execution!
# Move .git in initramfs so it doesn't get included in the zImage
mv ../charge_initramfs/.git ../irs.git

# build the kernel
exec 2>&1
# make clean mrproper > /dev/null 2>&1
rm -f update/*.zip update/kernel_update/zImage

make ARCH=arm charge_defconfig
checkfail Configuration

make -j6 HOSTCC="$HOSTCC" CROSS_COMPILE="$CROSS_COMPILE" ARCH=arm TARGET_PRODUCT=SCH-I510 modules | tee ../moduleBuild.txt
checkfail "Module compilation"

cp $KERNEL_MODULES ../charge_initramfs/lib/modules
checkfail "Module copy"

make -j6 HOSTCC="$HOSTCC" CROSS_COMPILE="$CROSS_COMPILE" ARCH=arm TARGET_PRODUCT=SCH-I510 zImage  | tee ../kernelBuild.txt
checkfail "zImage compilation"

cp arch/arm/boot/zImage update/kernel_update/zImage

cp arch/arm/boot/zImage recovery.bin
tar -c recovery.bin > "$DATE"_charge_recovery.tar
md5sum -t "$DATE"_charge_recovery.tar >> "$DATE"_charge_recovery.tar
mv "$DATE"_charge_recovery.tar "$DATE"_charge_recovery.tar.md5

pushd update
zip -r -q kernel_update.zip .
mv kernel_update.zip ../"$DATE"_charge_mtd.zip
popd

#Move .git directory in iniramfs back
mv ../irs.git ../charge_initramfs/.git
echo -e "***** Successfully compiled *****\n"
