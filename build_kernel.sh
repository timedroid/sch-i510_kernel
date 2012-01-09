#!/bin/bash

# setup
DATE=$(date +%m%d)
HOSTCC="ccache gcc"
CROSS_COMPILE="ccache arm-linux-gnueabi-"
MAKE_FLAGS=(-j6 HOSTCC="$HOSTCC" CROSS_COMPILE="$CROSS_COMPILE" ARCH=arm TARGET_PRODUCT=SCH-I510)
KERNEL_MODULES="crypto/ansi_cprng.ko drivers/bluetooth/bthid/bthid.ko drivers/cdma_dpram/dpram.ko drivers/ltespi/ltespi.ko drivers/misc/samsung_modemctl/modemctl/modemctl.ko drivers/misc/samsung_modemctl/onedram/onedram.ko drivers/misc/samsung_modemctl/svnet/svnet.ko drivers/misc/vibetonz/vibrator.ko drivers/scsi/scsi_wait_scan.ko drivers/wtlfota_dpram/wtlfota_dpram.ko drivers/net/wireless/bcm4329/dhd.ko"

# execution!

# build the kernel
make clean mrproper > /dev/null 2>&1
rm -f update/*.zip update/kernel_update/zImage

make ARCH=arm charge_defconfig || exit 1
make -j6 HOSTCC="$HOSTCC" CROSS_COMPILE="$CROSS_COMPILE" ARCH=arm TARGET_PRODUCT=SCH-I510 modules || exit 1
cp $KERNEL_MODULES ../charge_initramfs/lib/modules || exit 1
make -j6 HOSTCC="$HOSTCC" CROSS_COMPILE="$CROSS_COMPILE" ARCH=arm TARGET_PRODUCT=SCH-I510 zImage  || exit 1

cp arch/arm/boot/zImage update/kernel_update/zImage

cp arch/arm/boot/zImage recovery.bin
tar -c recovery.bin > "$DATE"_charge_recovery.tar
md5sum -t "$DATE"_charge_recovery.tar >> "$DATE"_charge_recovery.tar
mv "$DATE"_charge_recovery.tar "$DATE"_charge_recovery.tar.md5

cd update
zip -r -q kernel_update.zip .
mv kernel_update.zip ../"$DATE"_charge_mtd.zip
echo -e "***** Successfully compiled *****\n"
