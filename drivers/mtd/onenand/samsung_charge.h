/* linux/drivers/mtd/onenand/charge.h
*
* Partition Layout for Samsung Droid Charge
*
*/

struct mtd_partition s3c_partition_info[] = {

/* bmlinfo from running kernel:
FSR VERSION: FSR_1.2.1p1_b139_RTM
minor       position           size     units       id
   1: 0x00000000-0x00040000 0x00040000      1        0
   2: 0x00040000-0x00080000 0x00040000      1        1
   3: 0x00080000-0x00a80000 0x00a00000     40       20
   4: 0x00a80000-0x00d80000 0x00300000     12       13
   5: 0x00d80000-0x00ec0000 0x00140000      5        3
   6: 0x00ec0000-0x01000000 0x00140000      5        4
   7: 0x01000000-0x01500000 0x00500000     20       21
   8: 0x01500000-0x01c80000 0x00780000     30        6
   9: 0x01c80000-0x02400000 0x00780000     30        7
  10: 0x02400000-0x17d00000 0x15900000   1380       22
  11: 0x17d00000-0x1e880000 0x06b80000    430       23
  12: 0x1e880000-0x1f480000 0x00c00000     48       11
  13: 0x1f480000-0x1f500000 0x00080000      2       12
*/

/*This is partition layout from the oneNAND it SHOULD match the pitfile on the second page of the NAND.
It will work if it doesn't but beware to write below the adress 0x01600000 there are the bootloaders.
Currently we won't map them, but we should keep that in mind for later things like flashing bootloader
from Linux. There is a partition 'efs' starting @ 0x00080000 40 256K pages long, it contains data for
the modem like IMSI we don't touch it for now, but we need the data from it, we create a partition
for that and copy the data from it. For this you need a image from it and mount it as vfat or copy
it on a kernel with rfs support on the phone.
Partitions on the lower NAND adresses:
0x00000000 - 0x0003FFFF = first stage bootloader (BML1)
0x00040000 - 0x0007FFFF = PIT for second stage bootloader (BML2)
0x00080000 - 0x00A7FFFF = EFS(kind of): What's mounted on /efs on a stock ROM (BML3)
0x00A80000 - 0x00D7FFFF = WTF is this? Maybe LTE related? (BML4)
0x00D80000 - 0x00EBFFFF = second stage bootloader (BML5)
0x00EC0000 - 0x00FFFFFF = backup of the second stage bootloader (should be loaded if the other fails, unconfirmed!) (BML6)
0x01000000 - 0x015FFFFF = PARAM.lfs config the bootloader (BML7)
#########################################################################################
#########################################################################################
###### NEVER TOUCH THE FIRST 2 256k PAGES! THEY CONTAIN THE FIRST STAGE BOOTLOADER ######
#########################################################################################
#########################################################################################*/
                                                                   
{
	.name = "boot", // BML8
	.offset = 0x01500000,
	.size = 0x00780000,
},
{
	.name = "recovery", // BML9
	.offset = 0x01c80000,
	.size = 0x00780000,
},
{
	.name = "system", // BML10
	.offset = 0x02400000,
	.size = 0x14900000,
},
{
	.name = "efs",
	.offset = 0x16d00000,
	.size = 0x01000000,
},
{
	.name = "datadata", // BML11
	.offset = 0x17d00000,
	.size = 0x06b80000,
},
{
	/* Apparently the RIL is responsible for booting the modem */
	.name = "radio", // BML12
	.offset = 0x1e880000,
	.size = 0x00c00000,
},
/* Cache is on eMMC */
{
	/* This is read/written by nvstorage.ko on a stock kernel It
	 * seems to be the equivalent to nv_data.bin on other Galaxy S
	 * devices. */
	.name = "nvdata", // BML13
	.offset = 0x1f480000,
	.size = 0x00080000,
},
{ 
	/* The reservoir area is used by Samsung's Block Management Layer (BML)
	to map good blocks from this reservoir to bad blocks in user
	partitions. A special tool (bml_over_mtd) is needed to write
	partition images using bad block mapping.
	Currently, this is required for flashing the "boot" partition,
	as Samsung's stock bootloader expects BML partitions.*/
	.name = "reservoir",
	.offset = (2004*SZ_256K),
	.size = (44*SZ_256K), //2047
}
};
