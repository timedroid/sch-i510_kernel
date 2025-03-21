/* linux/arch/arm/plat-s5p/devs.c
 *
 * Copyright (c) 2010 Samsung Electronics Co., Ltd.
 *		http://www.samsung.com/
 *
 * Base S5P platform device definitions
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
*/

#include <linux/kernel.h>
#include <linux/types.h>
#include <linux/interrupt.h>
#include <linux/list.h>
#include <linux/timer.h>
#include <linux/init.h>
#include <linux/gpio.h>
#include <linux/platform_device.h>
#include <linux/io.h>
#include <linux/slab.h>

#include <asm/mach/arch.h>
#include <asm/mach/map.h>
#include <asm/mach/irq.h>
#include <mach/hardware.h>
#include <mach/map.h>
#include <mach/dma.h>
#include <mach/adc.h>


#include <plat/devs.h>
#include <plat/gpio-cfg.h>
#include <plat/irqs.h>
#include <plat/fb.h>
#include <plat/fimc.h>
#include <plat/csis.h>
#include <plat/media.h>
#include <plat/jpeg.h>
#include <mach/media.h>

#ifdef CONFIG_USB_ANDROID_ACCESSORY
#include <linux/usb/f_accessory.h>
#endif

#include <../../../drivers/video/samsung/s3cfb.h>

/* Android Gadget */
#include <linux/usb/android_composite.h>
#define S3C_VENDOR_ID			0x18d1
#define S3C_UMS_PRODUCT_ID		0x4E21
#define S3C_UMS_ADB_PRODUCT_ID		0x4E22
#define S3C_RNDIS_PRODUCT_ID		0x4E23
#define S3C_RNDIS_ADB_PRODUCT_ID	0x4E24
#define MAX_USB_SERIAL_NUM	17

static char *usb_functions_ums[] = {
	"usb_mass_storage",
};
static char *usb_functions_rndis[] = {
	"rndis",
#ifdef CONFIG_USB_ANDROID_DIAG
	"diag"
#endif
};

/*
static char *usb_functions_rndis_adb[] = {
	"rndis",
	"adb",
};
*/
#ifdef CONFIG_USB_ANDROID_SAMSUNG_COMPOSITE
/* Do not use below compoiste */
#else
static char *usb_functions_ums_adb[] = {
	"usb_mass_storage",
	"adb",
};
#endif

#ifdef CONFIG_USB_ANDROID_SAMSUNG_COMPOSITE
/* soonyong.cho : Variables for samsung composite such as kies, mtp, ums, etc... */
/* kies mode */
static char *usb_functions_acm_mtp[] = {
	"acm",
	"mtp",
};
/* debug mode */
static char *usb_functions_acm_ums_adb[] = {
	"acm",
#ifdef CONFIG_USB_ANDROID_DIAG
	"diag",
#endif
	"usb_mass_storage",
	"adb",
};
/* mtp only mode */
static char *usb_functions_mtp[] = {
	"mtp",
};
#endif

static char *usb_functions_all[] = {
#ifdef CONFIG_USB_ANDROID_SAMSUNG_COMPOSITE
/* soonyong.cho : Every function driver for samsung composite.
 *  		  Number of to enable function features have to be same as below.
 */
	"acm",
#ifdef CONFIG_USB_ANDROID_DIAG
	"diag",
#endif
	"usb_mass_storage",
	"adb",
	"rndis",
#ifdef CONFIG_USB_ANDROID_SAMSUNG_MTP
	"mtp",
#endif 
#ifdef CONFIG_USB_ANDROID_ACCESSORY
	"accessory",
#endif
#else /* original */
#  ifdef CONFIG_USB_ANDROID_RNDIS
	"rndis",
#  endif
	"usb_mass_storage",
	"adb",
#  ifdef CONFIG_USB_ANDROID_ACM
	"acm",
#  endif
#endif /* CONFIG_USB_ANDROID_SAMSUNG_COMPOSITE */
};

#ifdef CONFIG_USB_ANDROID_ACCESSORY
static char *usb_functions_accessory[] = {
	"accessory",
};
static char *usb_functions_accessory_adb[] = {
	"accessory",
	"adb",
};
#endif

static struct android_usb_product usb_products[] = {
#ifdef CONFIG_USB_ANDROID_SAMSUNG_COMPOSITE
/* soonyong.cho : Please modify below value correctly if you customize composite */
#  ifdef CONFIG_USB_ANDROID_SAMSUNG_ESCAPE /* USE DEVGURU HOST DRIVER */
	{
		.product_id	= SAMSUNG_DEBUG_PRODUCT_ID,
		.num_functions	= ARRAY_SIZE(usb_functions_acm_ums_adb),
		.functions	= usb_functions_acm_ums_adb,
		.bDeviceClass	= 0xEF,
		.bDeviceSubClass= 0x02,
		.bDeviceProtocol= 0x01,
		.s		= ANDROID_DEBUG_CONFIG_STRING,
		.mode		= USBSTATUS_ADB,
	},
	{
		.product_id	= SAMSUNG_KIES_PRODUCT_ID,
		.num_functions	= ARRAY_SIZE(usb_functions_acm_mtp),
		.functions	= usb_functions_acm_mtp,
		.bDeviceClass	= 0xEF,
		.bDeviceSubClass= 0x02,
		.bDeviceProtocol= 0x01,
		.s		= ANDROID_KIES_CONFIG_STRING,
		.mode		= USBSTATUS_SAMSUNG_KIES,
	},
	{
		.product_id	= SAMSUNG_UMS_PRODUCT_ID,
		.num_functions	= ARRAY_SIZE(usb_functions_ums),
		.functions	= usb_functions_ums,
		.bDeviceClass	= USB_CLASS_PER_INTERFACE,
		.bDeviceSubClass= 0,
		.bDeviceProtocol= 0,
		.s		= ANDROID_UMS_CONFIG_STRING,
		.mode		= USBSTATUS_UMS,
	},
	{
		.product_id	= SAMSUNG_RNDIS_PRODUCT_ID,
		.num_functions	= ARRAY_SIZE(usb_functions_rndis),
		.functions	= usb_functions_rndis,
#    ifdef CONFIG_USB_ANDROID_SAMSUNG_RNDIS_WITH_MS_COMPOSITE
		.bDeviceClass	= 0xEF,
		.bDeviceSubClass= 0x02,
		.bDeviceProtocol= 0x01,
#    else
#      ifdef CONFIG_USB_ANDROID_RNDIS_WCEIS
		.bDeviceClass	= USB_CLASS_WIRELESS_CONTROLLER,
#      else
		.bDeviceClass	= USB_CLASS_COMM,
#      endif
		.bDeviceSubClass= 0,
		.bDeviceProtocol= 0,
#    endif
		.s		= ANDROID_RNDIS_CONFIG_STRING,
		.mode		= USBSTATUS_VTP,
	},
	{
		.product_id	= SAMSUNG_MTP_PRODUCT_ID,
		.num_functions	= ARRAY_SIZE(usb_functions_mtp),
		.functions	= usb_functions_mtp,
		.bDeviceClass	= USB_CLASS_PER_INTERFACE,
		.bDeviceSubClass= 0,
		.bDeviceProtocol= 0x01,
		.s		= ANDROID_MTP_CONFIG_STRING,
		.mode		= USBSTATUS_MTPONLY,
	},

#  else  /* USE MCCI HOST DRIVER */
	{
		.product_id	= SAMSUNG_DEBUG_PRODUCT_ID, /* change sequence */
		.num_functions	= ARRAY_SIZE(usb_functions_acm_ums_adb),
		.functions	= usb_functions_acm_ums_adb,
		.bDeviceClass	= USB_CLASS_COMM,
		.bDeviceSubClass= 0,
		.bDeviceProtocol= 0,
		.s		= ANDROID_DEBUG_CONFIG_STRING,
		.mode		= USBSTATUS_ADB,
	},
	{
		.product_id	= SAMSUNG_KIES_PRODUCT_ID, /* change sequence */
		.num_functions	= ARRAY_SIZE(usb_functions_acm_mtp),
		.functions	= usb_functions_acm_mtp,
		.bDeviceClass	= USB_CLASS_COMM,
		.bDeviceSubClass= 0,
		.bDeviceProtocol= 0,
		.s		= ANDROID_KIES_CONFIG_STRING,
		.mode		= USBSTATUS_SAMSUNG_KIES,

	},
	{
		.product_id	= SAMSUNG_UMS_PRODUCT_ID,
		.num_functions	= ARRAY_SIZE(usb_functions_ums),
		.functions	= usb_functions_ums,
		.bDeviceClass	= USB_CLASS_PER_INTERFACE,
		.bDeviceSubClass= 0,
		.bDeviceProtocol= 0,
		.s		= ANDROID_UMS_CONFIG_STRING,
		.mode		= USBSTATUS_UMS,
	},
	{
		.product_id	= SAMSUNG_RNDIS_PRODUCT_ID,
		.num_functions	= ARRAY_SIZE(usb_functions_rndis),
		.functions	= usb_functions_rndis,
#    ifdef CONFIG_USB_ANDROID_RNDIS_WCEIS
		.bDeviceClass	= USB_CLASS_WIRELESS_CONTROLLER,
#    else
		.bDeviceClass	= USB_CLASS_COMM,
#    endif
		.bDeviceSubClass= 0,
		.bDeviceProtocol= 0,
		.s		= ANDROID_RNDIS_CONFIG_STRING,
		.mode		= USBSTATUS_VTP,
	},
	{
		.product_id	= SAMSUNG_MTP_PRODUCT_ID,
		.num_functions	= ARRAY_SIZE(usb_functions_mtp),
		.functions	= usb_functions_mtp,
		.bDeviceClass	= USB_CLASS_PER_INTERFACE,
		.bDeviceSubClass= 0,
		.bDeviceProtocol= 0x01,
		.s		= ANDROID_MTP_CONFIG_STRING,
		.mode		= USBSTATUS_MTPONLY,
	},
#ifdef CONFIG_USB_ANDROID_ACCESSORY
	{
		.vendor_id	= USB_ACCESSORY_VENDOR_ID,
		.product_id	= USB_ACCESSORY_PRODUCT_ID,
		.num_functions	= ARRAY_SIZE(usb_functions_accessory),
		.functions	= usb_functions_accessory,
                .bDeviceClass	= 0xEF,
                .bDeviceSubClass= 0x02,
                .bDeviceProtocol= 0x01,
                .s		= ANDORID_ACCESSORY_CONFIG_STRING,
                .mode		= USBSTATUS_ACCESSORY,
	},
	{
		.vendor_id	= USB_ACCESSORY_VENDOR_ID,
		.product_id	= USB_ACCESSORY_ADB_PRODUCT_ID,
		.num_functions	= ARRAY_SIZE(usb_functions_accessory_adb),
		.functions	= usb_functions_accessory_adb,
                .bDeviceClass	= 0xEF,
                .bDeviceSubClass= 0x02,
                .bDeviceProtocol= 0x01,
		.s		= ANDORID_ACCESSORY_ADB_CONFIG_STRING,
	        .mode		= USBSTATUS_ACCESSORY_ADB,
	},
#endif        
	
#  endif
#else /* original */
	{
		.product_id	= S3C_PRODUCT_ID,
		.num_functions	= ARRAY_SIZE(usb_functions_ums),
		.functions	= usb_functions_ums,
	},
	{
		.product_id	= S3C_ADB_PRODUCT_ID,
		.num_functions	= ARRAY_SIZE(usb_functions_ums_adb),
		.functions	= usb_functions_ums_adb,
	},
#endif
};
// serial number should be changed as real device for commercial release
static char device_serial[MAX_USB_SERIAL_NUM] = "0123456789ABCDEF";
/* standard android USB platform data */

/* Information should be changed as real product for commercial release */
static struct android_usb_platform_data android_usb_pdata = {
#ifdef CONFIG_USB_ANDROID_SAMSUNG_COMPOSITE
/* soonyong.cho : refered from S1 */
	.vendor_id		= SAMSUNG_VENDOR_ID,
	.product_id		= SAMSUNG_KIES_PRODUCT_ID,
	.manufacturer_name	= "SAMSUNG",
	.product_name		= "SAMSUNG_Android",
#else
	.vendor_id		= S3C_VENDOR_ID,
	.product_id		= S3C_PRODUCT_ID,
	.manufacturer_name	= "Android",//"Samsung",
	.product_name		= "Android",//"Samsung SMDKV210",
#endif
	.serial_number		= device_serial,
	.num_products		= ARRAY_SIZE(usb_products),
	.products		= usb_products,
	.num_functions		= ARRAY_SIZE(usb_functions_all),
	.functions		= usb_functions_all,
};

static struct usb_ether_platform_data rndis_pdata = {
	/* ethaddr is filled by board_serialno_setup */
	.vendorID	= 0x18d1,
	.vendorDescr	= "Samsung",
};

struct platform_device s3c_device_rndis = {
	.name	= "rndis",
	.id	= -1,
	.dev	= {
		.platform_data = &rndis_pdata,
	},
};
#ifdef CONFIG_USB_ANDROID_SAMSUNG_COMPOSITE
void __init s3c_usb_set_serial(void)
{
# ifdef CONFIG_USB_ANDROID_RNDIS
	int i;
	char *src;
# endif

	sprintf(device_serial, "%08X%08X", system_serial_high,
			system_serial_low);
# ifdef CONFIG_USB_ANDROID_RNDIS
	/* create a fake MAC address from our serial number.
	 * first byte is 0x02 to signify locally administered.
	 */
	src = device_serial;
	rndis_pdata.ethaddr[0] = 0x02;
	for (i = 0; *src; i++) {
		/* XOR the USB serial across the remaining bytes */
		rndis_pdata.ethaddr[i % (ETH_ALEN - 1) + 1] ^= *src++;
	}
# endif
}
#endif /* CONFIG_USB_ANDROID_SAMSUNG_COMPOSITE */

struct platform_device s3c_device_android_usb = {
	.name	= "android_usb",
	.id	= -1,
	.dev	= {
		.platform_data	= &android_usb_pdata,
	},
};

static struct usb_mass_storage_platform_data ums_pdata = {
	.vendor			= "Android",
	.product		= "UMS Composite",
	.release		= 1,
#ifdef CONFIG_USB_ANDROID_SAMSUNG_COMPOSITE
	.nluns			= 2,
#else
	.nluns			= 1,
#endif
	
};

struct platform_device s3c_device_usb_mass_storage = {
	.name	= "usb_mass_storage",
	.id	= -1,
	.dev	= {
		.platform_data = &ums_pdata,
	},
};

/* RTC */
static struct resource s5p_rtc_resource[] = {
	[0] = {
		.start = S5P_PA_RTC,
		.end   = S5P_PA_RTC + 0xff,
		.flags = IORESOURCE_MEM,
	},
	[1] = {
		.start = IRQ_RTC_ALARM,
		.end   = IRQ_RTC_ALARM,
		.flags = IORESOURCE_IRQ,
	},
	[2] = {
		.start = IRQ_RTC_TIC,
		.end   = IRQ_RTC_TIC,
		.flags = IORESOURCE_IRQ
	}
};

struct platform_device s5p_device_rtc = {
	.name             = "s3c2410-rtc",
	.id               = -1,
	.num_resources    = ARRAY_SIZE(s5p_rtc_resource),
	.resource         = s5p_rtc_resource,
};

/* Keypad interface */
static struct resource s3c_keypad_resource[] = {
	[0] = {
		.start = S3C_PA_KEYPAD,
		.end   = S3C_PA_KEYPAD + S3C_SZ_KEYPAD - 1,
		.flags = IORESOURCE_MEM,
	},
	[1] = {
		.start = IRQ_KEYPAD,
		.end   = IRQ_KEYPAD,
		.flags = IORESOURCE_IRQ,
	}
};

struct platform_device s3c_device_keypad = {
	.name             = "s3c-keypad",
	.id               = -1,
	.num_resources    = ARRAY_SIZE(s3c_keypad_resource),
	.resource         = s3c_keypad_resource,
};

#ifdef CONFIG_S5P_ADC
/* ADCTS */
static struct resource s3c_adc_resource[] = {
	[0] = {
		.start = S3C_PA_ADC,
		.end   = S3C_PA_ADC + SZ_4K - 1,
		.flags = IORESOURCE_MEM,
	},
	[1] = {
		.start = IRQ_PENDN,
		.end   = IRQ_PENDN,
		.flags = IORESOURCE_IRQ,
	},
	[2] = {
		.start = IRQ_ADC,
		.end   = IRQ_ADC,
		.flags = IORESOURCE_IRQ,
	}
};

struct platform_device s3c_device_adc = {
	.name		  = "s3c-adc",
	.id               = -1,
	.num_resources	  = ARRAY_SIZE(s3c_adc_resource),
	.resource	  = s3c_adc_resource,
};

void __init s3c_adc_set_platdata(struct s3c_adc_mach_info *pd)
{
	struct s3c_adc_mach_info *npd;

	npd = kmalloc(sizeof(*npd), GFP_KERNEL);
	if (npd) {
		memcpy(npd, pd, sizeof(*npd));
		s3c_device_adc.dev.platform_data = npd;
	} else {
		printk(KERN_ERR "no memory for ADC platform data\n");
	}
}
#endif /* CONFIG_S5P_ADC */

#if defined(CONFIG_VIDEO_MFC51) || defined(CONFIG_VIDEO_MFC50)
static struct resource s5p_mfc_resources[] = {
	[0] = {
		.start	= S5P_PA_MFC,
		.end	= S5P_PA_MFC + S5P_SZ_MFC - 1,
		.flags	= IORESOURCE_MEM,
	},
	[1] = {
		.start	= IRQ_MFC,
		.end	= IRQ_MFC,
		.flags	= IORESOURCE_IRQ,
	}
};

struct platform_device s5p_device_mfc = {
	.name		= "mfc",
	.id		= -1,
	.num_resources	= ARRAY_SIZE(s5p_mfc_resources),
	.resource	= s5p_mfc_resources,
};
#endif

#if defined(CONFIG_S5P_DEV_FB)
static struct resource s3cfb_resource[] = {
	[0] = {
		.start = S5P_PA_LCD,
		.end   = S5P_PA_LCD + S5P_SZ_LCD - 1,
		.flags = IORESOURCE_MEM,
	},
	[1] = {
		.start = IRQ_LCD1,
		.end   = IRQ_LCD1,
		.flags = IORESOURCE_IRQ,
	},
	[2] = {
		.start = IRQ_LCD0,
		.end   = IRQ_LCD0,
		.flags = IORESOURCE_IRQ,
	},
};

static u64 fb_dma_mask = 0xffffffffUL;

struct platform_device s3c_device_fb = {
	.name		  = "s3cfb",
	.id		  = -1,
	.num_resources	  = ARRAY_SIZE(s3cfb_resource),
	.resource	  = s3cfb_resource,
	.dev		  = {
		.dma_mask		= &fb_dma_mask,
		.coherent_dma_mask	= 0xffffffffUL
	}
};

static struct s3c_platform_fb default_fb_data __initdata = {
#if defined(CONFIG_CPU_S5PV210_EVT0)
	.hw_ver	= 0x60,
#else
	.hw_ver	= 0x62,
#endif
	.nr_wins = 5,
	.default_win = CONFIG_FB_S3C_DEFAULT_WINDOW,
	.swap = FB_SWAP_WORD | FB_SWAP_HWORD,
};

void __init s3cfb_set_platdata(struct s3c_platform_fb *pd)
{
	struct s3c_platform_fb *npd;
	struct s3cfb_lcd *lcd;
	phys_addr_t pmem_start;
	int i, default_win, num_overlay_win;
	int frame_size;

	if (!pd)
		pd = &default_fb_data;

	npd = kmemdup(pd, sizeof(struct s3c_platform_fb), GFP_KERNEL);
	if (!npd)
		printk(KERN_ERR "%s: no memory for platform data\n", __func__);
	else {
		for (i = 0; i < npd->nr_wins; i++)
			npd->nr_buffers[i] = 1;

		default_win = npd->default_win;
		num_overlay_win = CONFIG_FB_S3C_NUM_OVLY_WIN;

		if (num_overlay_win >= default_win) {
			printk(KERN_WARNING "%s: NUM_OVLY_WIN should be less than default \
					window number. set to 0.\n", __func__);
			num_overlay_win = 0;
		}

		for (i = 0; i < num_overlay_win; i++)
			npd->nr_buffers[i] = CONFIG_FB_S3C_NUM_BUF_OVLY_WIN;
		npd->nr_buffers[default_win] = CONFIG_FB_S3C_NR_BUFFERS;

		lcd = (struct s3cfb_lcd *)npd->lcd;
		frame_size = (lcd->width * lcd->height * 4);

		s3cfb_get_clk_name(npd->clk_name);
		npd->backlight_onoff = NULL;
		npd->clk_on = s3cfb_clk_on;
		npd->clk_off = s3cfb_clk_off;

		/* set starting physical address & size of memory region for overlay
		 * window */
		pmem_start = s5p_get_media_memory_bank(S5P_MDEV_FIMD, 1);
		for (i = 0; i < num_overlay_win; i++) {
			npd->pmem_start[i] = pmem_start;
			npd->pmem_size[i] = frame_size * npd->nr_buffers[i];
			pmem_start += npd->pmem_size[i];
		}

		/* set starting physical address & size of memory region for default
		 * window */
		npd->pmem_start[default_win] = pmem_start;
		npd->pmem_size[default_win] = frame_size * npd->nr_buffers[default_win];

		s3c_device_fb.dev.platform_data = npd;
	}
}
#endif

#if defined(CONFIG_VIDEO_FIMC) || defined(CONFIG_CPU_FREQ) /* TODO: use existing dev */
static struct resource s3c_fimc0_resource[] = {
	[0] = {
		.start	= S5P_PA_FIMC0,
		.end	= S5P_PA_FIMC0 + S5P_SZ_FIMC0 - 1,
		.flags	= IORESOURCE_MEM,
	},
	[1] = {
		.start	= IRQ_FIMC0,
		.end	= IRQ_FIMC0,
		.flags	= IORESOURCE_IRQ,
	},
};

struct platform_device s3c_device_fimc0 = {
	.name		= "s3c-fimc",
	.id		= 0,
	.num_resources	= ARRAY_SIZE(s3c_fimc0_resource),
	.resource	= s3c_fimc0_resource,
};

static struct s3c_platform_fimc default_fimc0_data __initdata = {
	.default_cam	= CAMERA_PAR_A,
	.hw_ver	= 0x45,
};

void __init s3c_fimc0_set_platdata(struct s3c_platform_fimc *pd)
{
	struct s3c_platform_fimc *npd;

	if (!pd)
		pd = &default_fimc0_data;

	npd = kmemdup(pd, sizeof(struct s3c_platform_fimc), GFP_KERNEL);
	if (!npd)
		printk(KERN_ERR "%s: no memory for platform data\n", __func__);
	else {
		if (!npd->cfg_gpio)
			npd->cfg_gpio = s3c_fimc0_cfg_gpio;

		if (!npd->clk_on)
			npd->clk_on = s3c_fimc_clk_on;

		if (!npd->clk_off)
			npd->clk_off = s3c_fimc_clk_off;

		npd->hw_ver = 0x45;

		/* starting physical address of memory region */
		npd->pmem_start = s5p_get_media_memory_bank(S5P_MDEV_FIMC0, 1);
		/* size of memory region */
		npd->pmem_size = s5p_get_media_memsize_bank(S5P_MDEV_FIMC0, 1);

		s3c_device_fimc0.dev.platform_data = npd;
	}
}

static struct resource s3c_fimc1_resource[] = {
	[0] = {
		.start	= S5P_PA_FIMC1,
		.end	= S5P_PA_FIMC1 + S5P_SZ_FIMC1 - 1,
		.flags	= IORESOURCE_MEM,
	},
	[1] = {
		.start	= IRQ_FIMC1,
		.end	= IRQ_FIMC1,
		.flags	= IORESOURCE_IRQ,
	},
};

struct platform_device s3c_device_fimc1 = {
	.name		= "s3c-fimc",
	.id		= 1,
	.num_resources	= ARRAY_SIZE(s3c_fimc1_resource),
	.resource	= s3c_fimc1_resource,
};

static struct s3c_platform_fimc default_fimc1_data __initdata = {
	.default_cam	= CAMERA_PAR_A,
	.hw_ver	= 0x50,
};

void __init s3c_fimc1_set_platdata(struct s3c_platform_fimc *pd)
{
	struct s3c_platform_fimc *npd;

	if (!pd)
		pd = &default_fimc1_data;

	npd = kmemdup(pd, sizeof(struct s3c_platform_fimc), GFP_KERNEL);
	if (!npd)
		printk(KERN_ERR "%s: no memory for platform data\n", __func__);
	else {
		if (!npd->cfg_gpio)
			npd->cfg_gpio = s3c_fimc1_cfg_gpio;

		if (!npd->clk_on)
			npd->clk_on = s3c_fimc_clk_on;

		if (!npd->clk_off)
			npd->clk_off = s3c_fimc_clk_off;

		npd->hw_ver = 0x50;

		/* starting physical address of memory region */
		npd->pmem_start = s5p_get_media_memory_bank(S5P_MDEV_FIMC1, 1);
		/* size of memory region */
		npd->pmem_size = s5p_get_media_memsize_bank(S5P_MDEV_FIMC1, 1);

		s3c_device_fimc1.dev.platform_data = npd;
	}
}

static struct resource s3c_fimc2_resource[] = {
	[0] = {
		.start	= S5P_PA_FIMC2,
		.end	= S5P_PA_FIMC2 + S5P_SZ_FIMC2 - 1,
		.flags	= IORESOURCE_MEM,
	},
	[1] = {
		.start	= IRQ_FIMC2,
		.end	= IRQ_FIMC2,
		.flags	= IORESOURCE_IRQ,
	},
};

struct platform_device s3c_device_fimc2 = {
	.name		= "s3c-fimc",
	.id		= 2,
	.num_resources	= ARRAY_SIZE(s3c_fimc2_resource),
	.resource	= s3c_fimc2_resource,
};

static struct s3c_platform_fimc default_fimc2_data __initdata = {
	.default_cam	= CAMERA_PAR_A,
	.hw_ver	= 0x45,
};

void __init s3c_fimc2_set_platdata(struct s3c_platform_fimc *pd)
{
	struct s3c_platform_fimc *npd;

	if (!pd)
		pd = &default_fimc2_data;

	npd = kmemdup(pd, sizeof(struct s3c_platform_fimc), GFP_KERNEL);
	if (!npd)
		printk(KERN_ERR "%s: no memory for platform data\n", __func__);
	else {
		if (!npd->cfg_gpio)
			npd->cfg_gpio = s3c_fimc2_cfg_gpio;

		if (!npd->clk_on)
			npd->clk_on = s3c_fimc_clk_on;

		if (!npd->clk_off)
			npd->clk_off = s3c_fimc_clk_off;

		npd->hw_ver = 0x45;

		/* starting physical address of memory region */
		npd->pmem_start = s5p_get_media_memory_bank(S5P_MDEV_FIMC2, 1);
		/* size of memory region */
		npd->pmem_size = s5p_get_media_memsize_bank(S5P_MDEV_FIMC2, 1);

		s3c_device_fimc2.dev.platform_data = npd;
	}
}

static struct resource s3c_ipc_resource[] = {
	[0] = {
		.start	= S5P_PA_IPC,
		.end	= S5P_PA_IPC + S5P_SZ_IPC - 1,
		.flags	= IORESOURCE_MEM,
	},
};

struct platform_device s3c_device_ipc = {
	.name		= "s3c-ipc",
	.id		= -1,
	.num_resources	= ARRAY_SIZE(s3c_ipc_resource),
	.resource	= s3c_ipc_resource,
};
static struct resource s3c_csis_resource[] = {
	[0] = {
		.start	= S5P_PA_CSIS,
		.end	= S5P_PA_CSIS + S5P_SZ_CSIS - 1,
		.flags	= IORESOURCE_MEM,
	},
	[1] = {
		.start	= IRQ_MIPICSI,
		.end	= IRQ_MIPICSI,
		.flags	= IORESOURCE_IRQ,
	},
};

struct platform_device s3c_device_csis = {
	.name		= "s3c-csis",
	.id		= 0,
	.num_resources	= ARRAY_SIZE(s3c_csis_resource),
	.resource	= s3c_csis_resource,
};

static struct s3c_platform_csis default_csis_data __initdata = {
	.srclk_name	= "mout_mpll",
	.clk_name	= "sclk_csis",
	.clk_rate	= 166000000,
};

void __init s3c_csis_set_platdata(struct s3c_platform_csis *pd)
{
	struct s3c_platform_csis *npd;

	if (!pd)
		pd = &default_csis_data;

	npd = kmemdup(pd, sizeof(struct s3c_platform_csis), GFP_KERNEL);
	if (!npd)
		printk(KERN_ERR "%s: no memory for platform data\n", __func__);

	npd->cfg_gpio = s3c_csis_cfg_gpio;
	npd->cfg_phy_global = s3c_csis_cfg_phy_global;

	s3c_device_csis.dev.platform_data = npd;
}
#endif

/* JPEG controller  */
static struct s3c_platform_jpeg default_jpeg_data __initdata = {
	.max_main_width		= 2560,
	.max_main_height	= 1920,
	.max_thumb_width	= 0,
	.max_thumb_height	= 0,
};

void __init s3c_jpeg_set_platdata(struct s3c_platform_jpeg *pd)
{
	struct s3c_platform_jpeg *npd;

	if (!pd)
		pd = &default_jpeg_data;

	npd = kmemdup(pd, sizeof(struct s3c_platform_jpeg), GFP_KERNEL);
	if (!npd)
		printk(KERN_ERR "%s: no memory for platform data\n", __func__);
	else
		s3c_device_jpeg.dev.platform_data = npd;
}

static struct resource s3c_jpeg_resource[] = {
	[0] = {
		.start = S5PV210_PA_JPEG,
		.end   = S5PV210_PA_JPEG + S5PV210_SZ_JPEG - 1,
		.flags = IORESOURCE_MEM,
	},
	[1] = {
		.start = IRQ_JPEG,
		.end   = IRQ_JPEG,
		.flags = IORESOURCE_IRQ,
	}
};

struct platform_device s3c_device_jpeg = {
	.name             = "s3c-jpg",
	.id               = -1,
	.num_resources    = ARRAY_SIZE(s3c_jpeg_resource),
	.resource         = s3c_jpeg_resource,
};

/* G3D */
struct platform_device s3c_device_g3d = {
	.name		= "pvrsrvkm",
	.id		= -1,
};

struct platform_device s3c_device_lcd = {
	.name		= "s3c_lcd",
	.id		= -1,
};

/* rotator interface */
static struct resource s5p_rotator_resource[] = {
	[0] = {
		.start = S5P_PA_ROTATOR,
		.end   = S5P_PA_ROTATOR + S5P_SZ_ROTATOR - 1,
		.flags = IORESOURCE_MEM,
	},
	[1] = {
		.start = IRQ_ROTATOR,
		.end   = IRQ_ROTATOR,
		.flags = IORESOURCE_IRQ,
	}
};

struct platform_device s5p_device_rotator = {
	.name		= "s5p-rotator",
	.id		= -1,
	.num_resources	= ARRAY_SIZE(s5p_rotator_resource),
	.resource	= s5p_rotator_resource
};

/* TVOUT interface */
static struct resource s5p_tvout_resources[] = {
	[0] = {
		.start  = S5P_PA_TVENC,
		.end    = S5P_PA_TVENC + S5P_SZ_TVENC - 1,
		.flags  = IORESOURCE_MEM,
	},
	[1] = {
		.start  = S5P_PA_VP,
		.end    = S5P_PA_VP + S5P_SZ_VP - 1,
		.flags  = IORESOURCE_MEM,
	},
	[2] = {
		.start  = S5P_PA_MIXER,
		.end    = S5P_PA_MIXER + S5P_SZ_MIXER - 1,
		.flags  = IORESOURCE_MEM,
	},
	[3] = {
		.start  = S5P_PA_HDMI,
		.end    = S5P_PA_HDMI + S5P_SZ_HDMI - 1,
		.flags  = IORESOURCE_MEM,
	},
	[4] = {
		.start  = S5P_I2C_HDMI_PHY,
		.end    = S5P_I2C_HDMI_PHY + S5P_I2C_HDMI_SZ_PHY - 1,
		.flags  = IORESOURCE_MEM,
	},
	[5] = {
		.start  = IRQ_MIXER,
		.end    = IRQ_MIXER,
		.flags  = IORESOURCE_IRQ,
	},
	[6] = {
		.start  = IRQ_HDMI,
		.end    = IRQ_HDMI,
		.flags  = IORESOURCE_IRQ,
	},
	[7] = {
		.start  = IRQ_TVENC,
		.end    = IRQ_TVENC,
		.flags  = IORESOURCE_IRQ,
	},
        [8] = {
                .start  = IRQ_EINT5,
                .end    = IRQ_EINT5,
                .flags  = IORESOURCE_IRQ,
        }
};

struct platform_device s5p_device_tvout = {
	.name           = "s5p-tvout",
	.id             = -1,
	.num_resources  = ARRAY_SIZE(s5p_tvout_resources),
	.resource       = s5p_tvout_resources,
};
EXPORT_SYMBOL(s5p_device_tvout);

/* CEC */
static struct resource s5p_cec_resources[] = {
	[0] = {
		.start  = S5P_PA_CEC,
		.end    = S5P_PA_CEC + S5P_SZ_CEC - 1,
		.flags  = IORESOURCE_MEM,
	},
	[1] = {
		.start  = IRQ_CEC,
		.end    = IRQ_CEC,
		.flags  = IORESOURCE_IRQ,
	},
};

struct platform_device s5p_device_cec = {
	.name           = "s5p-cec",
	.id             = -1,
	.num_resources  = ARRAY_SIZE(s5p_cec_resources),
	.resource       = s5p_cec_resources,
};
EXPORT_SYMBOL(s5p_device_cec);
/* HPD */
struct platform_device s5p_device_hpd = {
	.name           = "s5p-hpd",
	.id             = -1,
};
EXPORT_SYMBOL(s5p_device_hpd);

#ifdef CONFIG_USB_SUPPORT
#ifdef CONFIG_USB_ARCH_HAS_EHCI
 /* USB Host Controlle EHCI registrations */
static struct resource s3c_usb__ehci_resource[] = {
	[0] = {
		.start = S5P_PA_USB_EHCI,
		.end   = S5P_PA_USB_EHCI  + S5P_SZ_USB_EHCI - 1,
		.flags = IORESOURCE_MEM,
	},
	[1] = {
		.start = IRQ_UHOST,
		.end   = IRQ_UHOST,
		.flags = IORESOURCE_IRQ,
	}
};

static u64 s3c_device_usb_ehci_dmamask = 0xffffffffUL;

struct platform_device s3c_device_usb_ehci = {
	.name		= "s5p-ehci",
	.id		= -1,
	.num_resources	= ARRAY_SIZE(s3c_usb__ehci_resource),
	.resource	= s3c_usb__ehci_resource,
	.dev		= {
		.dma_mask = &s3c_device_usb_ehci_dmamask,
		.coherent_dma_mask = 0xffffffffUL
	}
};
#endif /* CONFIG_USB_ARCH_HAS_EHCI */

#ifdef CONFIG_USB_ARCH_HAS_OHCI
/* USB Host Controlle OHCI registrations */
static struct resource s3c_usb__ohci_resource[] = {
	[0] = {
		.start = S5P_PA_USB_OHCI,
		.end   = S5P_PA_USB_OHCI  + S5P_SZ_USB_OHCI - 1,
		.flags = IORESOURCE_MEM,
	},
	[1] = {
		.start = IRQ_UHOST,
		.end   = IRQ_UHOST,
		.flags = IORESOURCE_IRQ,
	}
};

static u64 s3c_device_usb_ohci_dmamask = 0xffffffffUL;

struct platform_device s3c_device_usb_ohci = {
	.name		= "s5p-ohci",
	.id		= -1,
	.num_resources	= ARRAY_SIZE(s3c_usb__ohci_resource),
	.resource	= s3c_usb__ohci_resource,
	.dev		= {
		.dma_mask = &s3c_device_usb_ohci_dmamask,
		.coherent_dma_mask = 0xffffffffUL
	}
};
#endif /* CONFIG_USB_ARCH_HAS_EHCI */

/* USB Device (Gadget)*/
static struct resource s3c_usbgadget_resource[] = {
	[0] = {
		.start	= S3C_PA_OTG,
		.end	= S3C_PA_OTG + S3C_SZ_OTG - 1,
		.flags	= IORESOURCE_MEM,
	},
	[1] = {
		.start	= IRQ_OTG,
		.end	= IRQ_OTG,
		.flags	= IORESOURCE_IRQ,
	}
};

struct platform_device s3c_device_usbgadget = {
	.name		= "s3c-usbgadget",
	.id		= -1,
	.num_resources	= ARRAY_SIZE(s3c_usbgadget_resource),
	.resource	= s3c_usbgadget_resource,
};
#endif

static struct resource s5p_ace_resource[] = {
	[0] = {
		.start = S5P_PA_ACE,
		.end   = S5P_PA_ACE + S5P_SZ_ACE - 1,
		.flags = IORESOURCE_MEM,
	},
	[1] = {
		.start = IRQ_SECSS,
		.end   = IRQ_SECSS,
		.flags = IORESOURCE_IRQ,
	},
};

struct platform_device s5p_device_ace = {
	.name		= "s5p-ace",
	.id		= 0,
	.num_resources	= ARRAY_SIZE(s5p_ace_resource),
	.resource	= s5p_ace_resource,
};
