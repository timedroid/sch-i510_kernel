/*
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#ifndef __M5MO_H
#define __M5MO_H

#include <linux/types.h>
#include <media/v4l2-device.h>
#include <media/v4l2-subdev.h>
#include <media/v4l2-i2c-drv.h>

#define FEATURE_BACKUP_SETTING_VALUE

enum M5MOLS_ZOOM_LEVEL
{
        M5MO_LS_ZOOM_LEVEL_0 = 0,
        M5MO_LS_ZOOM_LEVEL_1,
        M5MO_LS_ZOOM_LEVEL_2,
        M5MO_LS_ZOOM_LEVEL_3,
        M5MO_LS_ZOOM_LEVEL_4,
        M5MO_LS_ZOOM_LEVEL_5,
        M5MO_LS_ZOOM_LEVEL_6,
        M5MO_LS_ZOOM_LEVEL_7,
        M5MO_LS_ZOOM_LEVEL_8,
        M5MO_LS_ZOOM_LEVEL_9,
        M5MO_LS_ZOOM_LEVEL_10,
        M5MO_LS_ZOOM_LEVEL_11,
        M5MO_LS_ZOOM_LEVEL_12,
        M5MO_LS_ZOOM_LEVEL_13,
        M5MO_LS_ZOOM_LEVEL_14,
        M5MO_LS_ZOOM_LEVEL_15,
        M5MO_LS_ZOOM_LEVEL_16,
        M5MO_LS_ZOOM_LEVEL_17,
        M5MO_LS_ZOOM_LEVEL_18,
        M5MO_LS_ZOOM_LEVEL_19,
        M5MO_LS_ZOOM_LEVEL_20,
        M5MO_LS_ZOOM_LEVEL_21,
        M5MO_LS_ZOOM_LEVEL_22,
        M5MO_LS_ZOOM_LEVEL_23,
        M5MO_LS_ZOOM_LEVEL_24,
        M5MO_LS_ZOOM_LEVEL_25,
        M5MO_LS_ZOOM_LEVEL_26,
        M5MO_LS_ZOOM_LEVEL_27,
        M5MO_LS_ZOOM_LEVEL_28,
        M5MO_LS_ZOOM_LEVEL_29,
        M5MO_LS_ZOOM_LEVEL_30,
        M5MO_LS_ZOOM_LEVEL_MAX,
};

#ifdef FEATURE_BACKUP_SETTING_VALUE
struct m5moLS_exposure {
	/* Exposure */
	enum v4l2_photometry_mode	photometry;
	enum v4l2_exposure_auto_type	exposure_mode;
	unsigned int exposure_iso;
	unsigned int exposure_shutter;
	unsigned int exposure_iris;
	int exposure_bias;
	unsigned int wide_dr;	/* Wide dynamic range */
	unsigned int iris_stage;
	unsigned int shutter_stage;
	unsigned int ae_lock;
};

struct m5moLS_wb {
	unsigned int auto_wb;	/* auto or manual */
	unsigned int wb_temp;	/* in Kelvin */
	unsigned int wb_preset;
	unsigned int awb_lock;
};

struct m5moLS_detect {
	unsigned int detectmode;	/* face detect mode */
	unsigned int num_to_detect;	/* number to detect */
	unsigned int detect_lock;	/* face detect lock */
};

struct m5moLS_adjust {
	int	contrast;
	int	saturation;
	int	sharpness;

	unsigned int effect;	/* Color FX (AKA Color tone) */
	unsigned int scenemode;
};

struct m5moLS_orient {
	unsigned int	hflip;
	unsigned int	vflip;
	unsigned int	rot;
};

struct m5moLS_lens {
	/* focus */
	enum v4l2_focus_mode	focus_mode;
	bool			focusing;
	unsigned int af_status;
	unsigned int af_continuous;

	/* zoom */
	enum v4l2_zoom_mode	zoom_mode;
	unsigned int		zoom_level;
};

struct m5moLS_userset {
	unsigned int focus_mode;

	struct m5moLS_exposure	exposure;
	struct m5moLS_wb		wb;
	struct m5moLS_detect	detect;
	struct m5moLS_adjust	adjust;
	struct m5moLS_orient	orient;
	struct m5moLS_lens	lens;

	unsigned int stabilize;	/* IS */

	unsigned int strobe;
	unsigned int jpeg_quality;
	unsigned int preview_size;
	unsigned int capture_size;
	unsigned int thumbnail_size;
	unsigned int fps;
};
#endif

#define M5MO_LSLS_NAME "OP:FUJITSU_M5MO_LS"
#define M5MO_LS_DRIVER_NAME	"M5MO_LS"
#define M5MO_LS_MOD_NAME 	"M5MO_LS:"

#define M5MO_LS_I2C_RETRY					3
#define M5MO_LS_I2C_VERIFY_RETRY			200
#define M5MO_LS_UNINIT_VALUE 				0xFF

/* JPEG width & height */
#define M5MO_LS_JPEG_WIDTH		2048
#define M5MO_LS_JPEG_HEIGHT		2048

/* FPS Capabilities */
#define M5MO_LS_5_FPS			0x1
#define M5MO_LS_7_FPS			0x2
#define M5MO_LS_10_FPS			0x3
#define M5MO_LS_12_FPS			0x4
#define M5MO_LS_15_FPS			0x5
#define M5MO_LS_24_FPS			0x6
#define M5MO_LS_30_FPS			0x7
#define M5MO_LS_60_FPS			0x8
#define M5MO_LS_120_FPS			0x9
#define M5MO_LS_AUTO_FPS		0xA

/* M5MO_LS Sensor Mode */
#define M5MO_LS_SYSINIT_MODE	0x0
#define M5MO_LS_PARMSET_MODE	0x1
#define M5MO_LS_MONITOR_MODE	0x2
#define M5MO_LS_STILLCAP_MODE	0x3

/* M5MO_LS Preview Size */
#define M5MO_LS_PREVIEW_160_90       0x00
#define M5MO_LS_PREVIEW_160_120      0x02
#define M5MO_LS_PREVIEW_320_240      0x04
#define M5MO_LS_PREVIEW_400_240      0x06
#define M5MO_LS_PREVIEW_480_360      0x09
#define M5MO_LS_PREVIEW_640_360      0x0A
#define M5MO_LS_PREVIEW_640_480      0x0B
#define M5MO_LS_PREVIEW_800_480      0x0C

#define M5MO_LS_SUB_QCIF_SIZE	0x1	
#define M5MO_LS_QQVGA_SIZE		0x3		
#define M5MO_LS_144_176_SIZE	0x4
#define M5MO_LS_QCIF_SIZE		0x5
#define M5MO_LS_176_176_SIZE	0x6
#define M5MO_LS_LQVGA_SIZE		0x8		
#define M5MO_LS_QVGA_SIZE		0x9		
#define M5MO_LS_LWQVGA_SIZE		0xC		
#define M5MO_LS_WQVGA_SIZE		0xD		
#define M5MO_LS_CIF_SIZE		0xE		
#define M5MO_LS_480_360_SIZE	0x13		
#define M5MO_LS_QHD_SIZE		0x15		
#define M5MO_LS_VGA_SIZE		0x17		
#define M5MO_LS_NTSC_SIZE		0x18		
#define M5MO_LS_WVGA_SIZE		0x1A		
#define M5MO_LS_SVGA_SIZE		0x1F		
#define M5MO_LS_HD_SIZE			0x21		
#define M5MO_LS_FULL_HD_SIZE	0x25
#define M5MO_LS_704_576_SIZE	0x2B
#define M5MO_LS_8M_SIZE			0x29
#define M5MO_LS_QVGA_SL60_SIZE	0x30		
#define M5MO_LS_QVGA_SL120_SIZE	0x31		

/* M5MO_LS Capture Size */
#define M5MO_LS_SHOT_QVGA_SIZE		0x2       //  320 x  240
#define M5MO_LS_SHOT_WQVGA_SIZE		0x4       //  400 x  240
#define M5MO_LS_SHOT_480_360_SIZE	0x7       //  480 x  360
#define M5MO_LS_SHOT_QHD_SIZE		0x8       //  640  x 360
#define M5MO_LS_SHOT_VGA_SIZE		0x9       //  640 x 480
#define M5MO_LS_SHOT_WVGA_SIZE		0xA       //  800 x 480
#define M5MO_LS_SHOT_HD_SIZE		0x10     // 1280 x 720
#define M5MO_LS_SHOT_1280_960		0x14     // 1280 x 960
#define M5MO_LS_SHOT_1600_960       0x2B     //  1600x 960
#define M5MO_LS_SHOT_1600_1200		0x17     // 1600 x 1200
#define M5MO_LS_SHOT_FULL_HD_SIZE	0x19     // 1920 x 1080
#define M5MO_LS_SHOT_2048_1152  	0x1A     // 2048 x 1152
#define M5MO_LS_SHOT_2048_1536		0x1B     // 2048 x 1536
#define M5MO_LS_SHOT_2560_1440  	0x1C     // 2560 x 1440
#define M5MO_LS_SHOT_2560_1536  	0x1D     // 2560 x 1536
#define M5MO_LS_SHOT_2560_1920		0x1F     // 2560 x 1920
#define M5MO_LS_SHOT_3264_1836		0x21     // 3264 x 1836
#define M5MO_LS_SHOT_3264_1960		0x22     // 3264 x 1960
#define M5MO_LS_SHOT_3264_2448		0x25     // 3264 x 2448
#define M5MO_LS_SHOT_2048_1232      0x2C
#define M5MO_LS_SHOT_3264_1968      0x2D


/* M5MO_LS Thumbnail Size */
#define M5MO_LS_THUMB_QVGA_SIZE		0x1
#define M5MO_LS_THUMB_400_225_SIZE	0x2
#define M5MO_LS_THUMB_WQVGA_SIZE	0x3
#define M5MO_LS_THUMB_VGA_SIZE		0x4
#define M5MO_LS_THUMB_WVGA_SIZE		0x5

/* M5MO_LS Auto Focus Mode */
#define M5MO_LS_AF_STOP  				0
#define M5MO_LS_AF_INIT_NORMAL 			1
#define M5MO_LS_AF_INIT_MACRO  			2
#define M5MO_LS_AF_START 				3
#define M5MO_LS_AF_RELEASE				4
#define M5MO_LS_AF_INIT_TOUCH_NORMAL	5
#define M5MO_LS_AF_INIT_TOUCH_MACRO 	6
#define M5MO_LS_AF_INIT_FD              7

#define M5MO_LS_AF_CONTINUOUS_OFF		0
#define M5MO_LS_AF_CONTINUOUS_ON		1

#define M5MO_LS_AF_STATUS_MOVING		5
#define M5MO_LS_AF_STATUS_SUCCESS		0x01
#define M5MO_LS_AF_STATUS_FAILED		0x00
#define M5MO_LS_AF_STATUS_CANCEL        0x02

/* Image Effect */
#define M5MO_LS_EFFECT_OFF				1
#define M5MO_LS_EFFECT_SEPIA			2
#define M5MO_LS_EFFECT_GRAY				3
#define M5MO_LS_EFFECT_RED				4
#define M5MO_LS_EFFECT_GREEN			5
#define M5MO_LS_EFFECT_BLUE				6
#define M5MO_LS_EFFECT_PINK				7
#define M5MO_LS_EFFECT_YELLOW			8
#define M5MO_LS_EFFECT_PURPLE			9
#define M5MO_LS_EFFECT_ANTIQUE			10
#define M5MO_LS_EFFECT_NEGATIVE			11
//#define M5MO_LS_EFFECT_SOLARIZATION1	12
//#define M5MO_LS_EFFECT_SOLARIZATION2	13
#define M5MO_LS_EFFECT_SOLARIZATION3	14
//#define M5MO_LS_EFFECT_SOLARIZATION4	15
#define M5MO_LS_EFFECT_EMBOSS			16
#define M5MO_LS_EFFECT_OUTLINE			17
#define M5MO_LS_EFFECT_AQUA			   	18  

/* ISO */
#define M5MO_LS_ISO_AUTO				1
#define M5MO_LS_ISO_50					2
#define M5MO_LS_ISO_100					3
#define M5MO_LS_ISO_200					4
#define M5MO_LS_ISO_400					5
#define M5MO_LS_ISO_800					6
#define M5MO_LS_ISO_1600				7
#define M5MO_LS_ISO_3200				8

/* EV */
#define M5MO_LS_EV_MINUS_4				1
#define M5MO_LS_EV_MINUS_3				2
#define M5MO_LS_EV_MINUS_2				3
#define M5MO_LS_EV_MINUS_1				4
#define M5MO_LS_EV_DEFAULT				5
#define M5MO_LS_EV_PLUS_1				6
#define M5MO_LS_EV_PLUS_2				7
#define M5MO_LS_EV_PLUS_3				8
#define M5MO_LS_EV_PLUS_4				9

/* Saturation*/
#define M5MO_LS_SATURATION_MINUS_2		1
#define M5MO_LS_SATURATION_MINUS_1		2
#define M5MO_LS_SATURATION_DEFAULT		3
#define M5MO_LS_SATURATION_PLUS_1		4
#define M5MO_LS_SATURATION_PLUS_2		5

/* Sharpness */
#define M5MO_LS_SHARPNESS_MINUS_2		1
#define M5MO_LS_SHARPNESS_MINUS_1		2
#define M5MO_LS_SHARPNESS_DEFAULT		3
#define M5MO_LS_SHARPNESS_PLUS_1		4
#define M5MO_LS_SHARPNESS_PLUS_2		5

/* Contrast */
#define M5MO_LS_CONTRAST_MINUS_2		1
#define M5MO_LS_CONTRAST_MINUS_1		2
#define M5MO_LS_CONTRAST_DEFAULT		3
#define M5MO_LS_CONTRAST_PLUS_1			4
#define M5MO_LS_CONTRAST_PLUS_2			5

/* White Balance */
#define M5MO_LS_WB_AUTO					1
#define M5MO_LS_WB_INCANDESCENT			2
#define M5MO_LS_WB_FLUORESCENT			3
#define M5MO_LS_WB_DAYLIGHT				4
#define M5MO_LS_WB_CLOUDY				5
#define M5MO_LS_WB_SHADE				6
#define M5MO_LS_WB_HORIZON				7
#define M5MO_LS_WB_LED					8

/* Zoom */
#define M5MO_LS_ZOOM_1X_VALUE			0x01

/* Photometry */  
#define M5MO_LS_PHOTOMETRY_AVERAGE		1
#define M5MO_LS_PHOTOMETRY_CENTER		2
#define M5MO_LS_PHOTOMETRY_SPOT			3

/* Face Detection */
#define M5MO_LS_FACE_DETECTION_OFF		0
#define M5MO_LS_FACE_DETECTION_ON		1

/* Scene Mode */ 
#define M5MO_LS_SCENE_NORMAL			1
#define M5MO_LS_SCENE_PORTRAIT			2
#define M5MO_LS_SCENE_LANDSCAPE			3
#define M5MO_LS_SCENE_SPORTS			4
#define M5MO_LS_SCENE_PARTY_INDOOR		5
#define M5MO_LS_SCENE_BEACH_SNOW		6
#define M5MO_LS_SCENE_SUNSET			7
#define M5MO_LS_SCENE_DUSK_DAWN			8
#define M5MO_LS_SCENE_FALL_COLOR		9
#define M5MO_LS_SCENE_NIGHT				10
#define M5MO_LS_SCENE_FIREWORK			11	
#define M5MO_LS_SCENE_TEXT				12
#define M5MO_LS_SCENE_SHOWWINDOW		13
#define M5MO_LS_SCENE_CANDLELIGHT		14
#define M5MO_LS_SCENE_BACKLIGHT			15

/* JPEG Quality */ 
#define M5MO_LS_JPEG_SUPERFINE			0x00
#define M5MO_LS_JPEG_FINE				0x05
#define M5MO_LS_JPEG_NORMAL				0x0A
#define M5MO_LS_JPEG_ECONOMY			0x0F

/* WDR */
#define M5MO_LS_WDR_OFF					1
#define M5MO_LS_WDR_ON					2

/* Image Stabilization */
#define M5MO_LS_ISC_STILL_OFF			1
#define M5MO_LS_ISC_STILL_ON			2
#define M5MO_LS_ISC_STILL_AUTO			3 /* Not Used */
#define M5MO_LS_ISC_MOVIE_ON			4 /* Not Used */

/* Flash Fire Yes/No */
#define M5MO_LS_FLASH_FIRED				1
#define M5MO_LS_FLASH_NO_FIRED			0

/* Flash Setting */
#define M5MO_LS_FLASH_CAPTURE_OFF		1
#define M5MO_LS_FLASH_CAPTURE_ON		2
#define M5MO_LS_FLASH_CAPTURE_AUTO		3
#define M5MO_LS_FLASH_MOVIE_ON			4

/*AE Lock/Unlock */
#define M5MO_LS_AE_LOCK                 0x01
#define M5MO_LS_AE_UNLOCK               0x00

/*AWB Lock/Unlock */
#define M5MO_LS_AWB_LOCK                0x01
#define M5MO_LS_AWB_UNLOCK              0x00

/* Category */
#define M5MO_CATEGORY_SYS		0x00
#define M5MO_CATEGORY_PARM		0x01
#define M5MO_CATEGORY_MON		0x02
#define M5MO_CATEGORY_AE		0x03
#define M5MO_CATEGORY_WB		0x06
#define M5MO_CATEGORY_EXIF		0x07
#define M5MO_CATEGORY_FD		0x09
#define M5MO_CATEGORY_LENS		0x0A
#define M5MO_CATEGORY_CAPPARM	0x0B
#define M5MO_CATEGORY_CAPCTRL	0x0C
#define M5MO_CATEGORY_TEST		0x0D
#define M5MO_CATEGORY_ADJST		0x0E
#define M5MO_CATEGORY_FLASH		0x0F    /* F/W update */

/* M5MO_CATEGORY_SYS */
#define M5MO_SYS_PJT_CODE		0x01
#define M5MO_SYS_VER_FW			0x02
#define M5MO_SYS_VER_HW			0x04
#define M5MO_SYS_VER_PARAM		0x06
#define M5MO_SYS_VER_AWB		0x08
#define M5MO_SYS_USER_VER		0x0A
#define M5MO_SYS_MODE			0x0B
#define M5MO_SYS_STATUS		    0x0C
#define M5MO_SYS_INT_FACTOR		0x10
#define M5MO_SYS_INT_EN			0x11

/* M5MO_CATEGORY_PARM */
#define M5MO_PARM_OUT_SEL		0x00
#define M5MO_PARM_MON_SIZE		0x01
#define M5MO_PARM_MON_FPS		0x02
#define M5MO_PARM_EFFECT		0x0B
#define M5MO_PARM_FLEX_FPS	    0x31
#define M5MO_PARM_HDMOVIE		0x32

/* M5MO_CATEGORY_MON */
#define M5MO_MON_ZOOM			0x01
#define M5MO_MON_MON_REVERSE	0x05
#define M5MO_MON_MON_MIRROR		0x06
#define M5MO_MON_SHOT_REVERSE	0x07
#define M5MO_MON_SHOT_MIRROR	0x08
#define M5MO_MON_CFIXB			0x09
#define M5MO_MON_CFIXR			0x0A
#define M5MO_MON_COLOR_EFFECT	0x0B
#define M5MO_MON_CHROMA_LVL		0x0F
#define M5MO_MON_CHROMA_EN		0x10
#define M5MO_MON_EDGE_LVL		0x11
#define M5MO_MON_EDGE_EN		0x12
#define M5MO_MON_TONE_CTRL		0x25

/* M5MO_CATEGORY_AE */
#define M5MO_AE_LOCK			0x00
#define M5MO_AE_MODE			0x01
#define M5MO_AE_ISOSEL			0x05
#define M5MO_AE_FLICKER		    0x06
#define M5MO_AE_EVP_MON		    0x0A
#define M5MO_AE_EVP_CAP		    0x0B
#define M5MO_AE_EV_BIAS			0x09
#define M5MO_AE_ONESHOT_MAX_EXP	0x36
#define M5MO_EV_CONTROL	        0x38

/* M5MO_CATEGORY_WB */
#define M5MO_WB_AWB_LOCK		0x00
#define M5MO_WB_AWB_MODE		0x02
#define M5MO_WB_AWB_MANUAL		0x03

/* M5MO_CATEGORY_EXIF */
#define M5MO_EXIF_EXPTIME_NUM	0x00
#define M5MO_EXIF_EXPTIME_DEN	0x04
#define M5MO_EXIF_TV_NUM		0x08
#define M5MO_EXIF_TV_DEN		0x0C
#define M5MO_EXIF_AV_NUM		0x10
#define M5MO_EXIF_AV_DEN		0x14
#define M5MO_EXIF_BV_NUM		0x18
#define M5MO_EXIF_BV_DEN		0x1C
#define M5MO_EXIF_EBV_NUM		0x20
#define M5MO_EXIF_EBV_DEN		0x24
#define M5MO_EXIF_ISO			0x28
#define M5MO_EXIF_FLASH			0x2A

/* M5MO_CATEGORY_FD */
#define M5MO_FD_CTL				0x00
#define M5MO_FD_SIZE			0x01
#define M5MO_FD_MAX				0x02

/* M5MO_CATEGORY_LENS */
#define M5MO_LENS_AF_INIT_POS   0x00
#define M5MO_LENS_AF_MODE		0x01
#define M5MO_LENS_AF_START		0x02
#define M5MO_LENS_AF_STATUS		0x03
//  lens move to zero step
#define M5MO_LENS_AF_STEP_UPPER	    0x06
#define M5MO_LENS_AF_STEP_LOWER	    0x07
#define M5MO_LENS_AF_MACRO_VER      0x22
#define M5MO_LENS_AF_INFINITE_VER   0x24
#define M5MO_LENS_AF_TOUCH_POSX_H   0x30
#define M5MO_LENS_AF_TOUCH_POSX_L   0x31
#define M5MO_LENS_AF_TOUCH_POSY_H   0x32
#define M5MO_LENS_AF_TOUCH_POSY_L   0x33
#define M5MO_ADJST_AF_CAL           0x1D

/* M5MO_CATEGORY_CAPPARM */
#define M5MO_CAPPARM_YUVOUT_MAIN	    0x00
#define M5MO_CAPPARM_MAIN_IMG_SIZE	    0x01
#define M5MO_CAPPARM_YUVOUT_PREVIEW	    0x05
#define M5MO_CAPPARM_PREVIEW_IMG_SIZE   0x06
#define M5MO_CAPPARM_YUVOUT_THUMB	    0x0A
#define M5MO_CAPPARM_THUMB_IMG_SIZE	    0x0B
#define M5MO_CAPPARM_JPEG_SIZE_MAX	    0x0F
#define M5MO_CAPPARM_JPEG_RATIO		    0x17
// MCC for Scene Mode
#define M5MO_CAPPARM_MCC_MODE		    0x1D 
#define M5MO_CAPPARM_WDR_EN			    0x2C 
// image quality
#define M5MO_CAPPARM_JPEG_RATIO_OFS	    0x34
#define M5MO_CAPPARM_LIGHT_CTRL		    0x40
#define M5MO_CAPPARM_FLASH_CTRL		    0x41
#define M5MO_CAPPARM_THUMB_JPEG_MAX     0x3C
#define M5MO_CAPPRAM_VINTAGE_EN      	0x50
#define M5MO_CAPPRAM_VINTAGE_TYPE     	0x51
#define M5MO_CAPPRAM_BEAUTY_EN      	0x53

/* M5MO_CATEGORY_CAPCTRL */
#define M5MO_CAPCTRL_FRM_SEL	0x06
#define M5MO_CAPCTRL_TRANSFER	0x09
#define M5MO_CAPCTRL_IMG_SIZE	0x0D
#define M5MO_CAPCTRL_THUMB_SIZE	0x11

/* M5MO_CATEGORY_FLASH */
#define M5MO_FLASH_ADDR			0x00
#define M5MO_FLASH_BYTE			0x04
#define M5MO_FLASH_ERASE		0x06
#define M5MO_FLASH_WR			0x07
#define M5MO_FLASH_RAM_CLEAR	0x08
#define M5MO_FLASH_CAM_START	0x12
#define M5MO_FLASH_SEL			0x13

/* M5MO_CATEGORY_TEST:	0x0D */
#define M5MO_TEST_OUTPUT_YCO_TEST_DATA		0x1B
#define M5MO_TEST_OUTPUT_FIXED_PATTERN		0x1C
#define M5MO_TEST_OUTPUT_TEST_PATTERN		0x1D

/* M5MO Sensor Mode */
#define M5MO_SYSINIT_MODE		0x0
#define M5MO_PARMSET_MODE		0x1
#define M5MO_MONITOR_MODE		0x2
#define M5MO_STILLCAP_MODE		0x3

/* M5MO_CATEGORY_ADJST */
#define M5MO_ADJST_RG_HIGH		0x3c
#define M5MO_ADJST_RG_LOW 		0x3d
#define M5MO_ADJST_BG_HIGH		0x3e
#define M5MO_ADJST_BG_LOW       0x3f

/* Interrupt Factor */
#define M5MO_LS_INT_SOUND       (1 << 7)
#define M5MO_LS_INT_LENS_INIT   (1 << 6)
#define M5MO_LS_INT_FD			(1 << 5)
#define M5MO_LS_INT_FRAME_SYNC	(1 << 4)
#define M5MO_LS_INT_CAPTURE		(1 << 3)
#define M5MO_LS_INT_ZOOM        (1 << 2)
#define M5MO_LS_INT_AF			(1 << 1)
#define M5MO_LS_INT_MODE        (1 << 0)

#endif /* __M5MO_H */

