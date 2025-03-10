/* drivers/input/touchscreen/qt602240.c
 *
 * Quantum TSP driver.
 *
 * Copyright (C) 2009 Samsung Electronics Co. Ltd.
 *
 */

#include <linux/module.h>
#include <linux/input.h>
#include <linux/interrupt.h>
#include <linux/platform_device.h>
#include <linux/i2c.h>
#include <linux/delay.h>
#include <linux/interrupt.h>
#include <linux/irq.h>
#include <linux/earlysuspend.h>
#include <linux/slab.h>
#include <asm/io.h>
#include <mach/regs-gpio.h>
#include <plat/gpio-cfg.h>
#include <plat/irqs.h>
#include <mach/hardware.h>
#include <mach/gpio.h>
#include <linux/jiffies.h>
#include "qt602240.h"
#if TOUCH_CPU_FREQ
#include <mach/cpu-freq-v210.h>
static int touch_cpu_freq_flag;
#endif

struct i2c_driver qt602240_i2c_driver;

struct workqueue_struct *qt602240_wq = NULL;

struct qt602240_data *qt602240 = NULL;

#if ENABLE_NOISE_TEST_MODE
                                       //botton_right    botton_left            center              top_right          top_left 
unsigned char test_node[TEST_POINT_NUM] = {12,                  20,            104,                    188,                    196};        

unsigned int return_refer_0, return_refer_1, return_refer_2, return_refer_3, return_refer_4;
unsigned int return_delta_0, return_delta_1, return_delta_2, return_delta_3, return_delta_4;
uint16_t diagnostic_addr;
#endif

#ifdef _SUPPORT_MULTITOUCH_
static report_finger_info_t fingerInfo[MAX_USING_FINGER_NUM];// = {0};
static int qt_initial_ok = 0;
#endif

#ifdef QT_STYLUS_ENABLE
static int config_mode_val = 0;     //0: normal 1: stylus
#endif

#ifdef FEATUE_QT_INFOBLOCK_STATIC        //by donghoon.kwak
static info_block_t info_block_data;
static info_block_t *info_block=&info_block_data;

static report_id_map_t report_id_map_data[30];// = {0};
static report_id_map_t *report_id_map = &report_id_map_data[0];

static object_t info_object_table[25];// = {0};
static object_t *info_object_ptr = &info_object_table[0];

/*! Message buffer holding the latest message received. */
uint8_t quantum_msg[9] = {0};
#else
static info_block_t *info_block;

static report_id_map_t *report_id_map;

/*! Message buffer holding the latest message received. */
uint8_t *quantum_msg;
#endif

volatile uint8_t read_pending;

static int max_report_id = 0;

uint8_t max_message_length;

uint16_t message_processor_address;

/*! Command processor address. */
static uint16_t command_processor_address;

/*! Flag indicating if driver setup is OK. */
static enum driver_setup_t driver_setup = DRIVER_SETUP_INCOMPLETE;

/*! \brief The current address pointer. */
static U16 address_pointer;

static uint8_t tsp_version;

//20100217 julia
static uint8_t cal_check_flag = 0;
//static int CAL_THR = 10;

extern unsigned int HWREV;

static int ofl_error_count;
/*------------------------------ for tunning ATmel - start ----------------------------*/
struct class *touch_class;
EXPORT_SYMBOL(touch_class);

struct device *switch_test;
EXPORT_SYMBOL(switch_test);

#ifdef QT_STYLUS_ENABLE
struct device *qt_stylus;
EXPORT_SYMBOL(qt_stylus);
#endif

/******************************************************************************
* 
*
*       QT602240 Object table init
* 
*                                                             
* *****************************************************************************/
//General Object

gen_powerconfig_t7_config_t power_config = {0};                 //Power config settings.
gen_acquisitionconfig_t8_config_t acquisition_config = {0};     // Acquisition config. 


//Touch Object

touch_multitouchscreen_t9_config_t touchscreen_config = {0};    //Multitouch screen config.
touch_keyarray_t15_config_t keyarray_config = {0};              //Key array config.
touch_proximity_t23_config_t proximity_config = {0};        //Proximity config. 


//Signal Processing Objects

proci_gripfacesuppression_t20_config_t gripfacesuppression_config = {0};    //Grip / face suppression config.
procg_noisesuppression_t22_config_t noise_suppression_config = {0};         //Noise suppression config.
proci_onetouchgestureprocessor_t24_config_t onetouch_gesture_config = {0};  //One-touch gesture config. 
proci_twotouchgestureprocessor_t27_config_t twotouch_gesture_config = {0};  //Two-touch gesture config.

t22_freq_table_config_t noise_freq_table = {0};

//Support Objects

spt_gpiopwm_t19_config_t gpiopwm_config = {0};             //GPIO/PWM config
spt_selftest_t25_config_t selftest_config = {0};            //Selftest config.
spt_cteconfig_t28_config_t cte_config = {0};                //Capacitive touch engine config.

spt_comcconfig_t18_config_t comc_config = {0};            //Communication config settings.


//224E chip
gen_acquisitionconfig_t8_config_t_e acquisition_config_e = {0};
touch_multitouchscreen_t9_config_t_e touchscreen_config_e = {0};
proci_gripsuppression_t40_config_t grip_suppression_config = {0};
proci_touchsuppression_t42_config_t touch_suppression_config = {0};
spt_cteconfig_t46_config_t cte_config_e = {0};
proci_stylus_t47_config_t stylus_config = {0};
procg_noisesuppression_t48_config_t noise_suppression_config_e = {0};
#if MXT224E_TA_CONFIG_MODE
procg_noisesuppression_t48_config_t noise_suppression_config_e_TA = {0};
#endif

/*------------------------------ for tunning ATmel - end ----------------------------*/

#define __QT_CONFIG__
/*****************************************************************************
*
*
*
*       QT602240  Configuration Block
*
*
*
* ***************************************************************************/


/*****************************************************************************
*
*  FUNCTION
*  PURPOSE
*  INPUT
*  OUTPUT
*
* ***************************************************************************/

void qt_Power_Config_Init(void)
{
	if (info_block->info_id.family_id == 0x80) {
		/* Set Idle Acquisition Interval to 32 ms. */
		power_config.idleacqint = 64;

		/* Set Active Acquisition Interval to 16 ms. */
		dprintk("\n[TSP]%s real board \n",__func__);
		power_config.actvacqint = 255;        // 16 ->13 20100407

		/* Set Active to Idle Timeout to 4 s (one unit = 200ms). */
		power_config.actv2idleto = 15;
	} else if (info_block->info_id.family_id == 0x81) {
		power_config.idleacqint = 48;
		power_config.actvacqint = 255;
		power_config.actv2idleto = 25;
	}

	/* Write power config to chip. */
	if (write_power_config(power_config) != CFG_WRITE_OK)
	{
		dprintk("[TSP] Configuration Fail!!! , Line %d \n\r", __LINE__);
	}
}




/*****************************************************************************
*
*  FUNCTION
*  PURPOSE
*  INPUT
*  OUTPUT
*
* ***************************************************************************/

void qt_Acquisition_Config_Init(void)
{
	if (info_block->info_id.family_id == 0x80) {
		acquisition_config.chrgtime = 10;
		acquisition_config.driftst = 0;
		acquisition_config.reserved = 0;
		acquisition_config.tchdrift = 5;
		acquisition_config.tchautocal = 0;
		acquisition_config.sync = 0;
		acquisition_config.atchcalst = 9;
		acquisition_config.atchcalsthr = 30;

		if (write_acquisition_config(acquisition_config) != CFG_WRITE_OK)
			dprintk("[TSP] Configuration Fail!!! , Line %d \n\r", __LINE__);

	} else if (info_block->info_id.family_id == 0x81) {
		acquisition_config_e.chrgtime = 27;
		acquisition_config_e.reserved = 0;
		acquisition_config_e.tchdrift = 5;
		acquisition_config_e.driftst = 1;
		acquisition_config_e.tchautocal = 0;
		acquisition_config_e.sync = 0;
		acquisition_config_e.atchcalst = 8;
		acquisition_config_e.atchcalsthr = 8;
		acquisition_config_e.atchfrccalthr = 0;
		acquisition_config_e.atchfrccalratio = 0;

		if (write_acquisition_config_e(acquisition_config_e) != CFG_WRITE_OK)
			dprintk("[TSP] Configuration Fail!!! , Line %d \n\r", __LINE__);

	}
}


/*****************************************************************************
*
*  FUNCTION
*  PURPOSE
*  INPUT
*  OUTPUT
*
* ***************************************************************************/

void qt_Multitouchscreen_Init(void)
{
	if (info_block->info_id.family_id == 0x80) {
#ifdef _SUPPORT_TOUCH_AMPLITUDE_
		touchscreen_config.ctrl = 0x8B; // enable amplitude
#else
		touchscreen_config.ctrl = 143; // enable + message-enable
#endif
		touchscreen_config.xorigin = 0;
		touchscreen_config.yorigin = 0;
		touchscreen_config.xsize = 19;
		touchscreen_config.ysize = 11;
		touchscreen_config.akscfg = 0;
		touchscreen_config.blen = 32;
		touchscreen_config.tchthr = 40;//45;
		touchscreen_config.tchdi = 2;
		touchscreen_config.orient = 7;
		touchscreen_config.mrgtimeout = 0;
		touchscreen_config.movhysti = 3;	// 6;
		touchscreen_config.movhystn = 1;	// 5; ///1;
#if DRIVER_FILTER
		touchscreen_config.movfilter = 11;
#else
		touchscreen_config.movfilter = 46; //0x2e;	// 75; //0
#endif
#ifdef _SUPPORT_MULTITOUCH_	
		touchscreen_config.numtouch= MAX_USING_FINGER_NUM;	// it is 5 now and it can be set up to 10
#else
		touchscreen_config.numtouch= 1;
#endif
		touchscreen_config.mrghyst =5;
		touchscreen_config.mrgthr = 15;
		touchscreen_config.amphyst = 10;
		touchscreen_config.xrange = 799;
		touchscreen_config.yrange = 479;
		touchscreen_config.xloclip = 0;
		touchscreen_config.xhiclip = 0;
		touchscreen_config.yloclip = 0;
		touchscreen_config.yhiclip = 0;
		touchscreen_config.xedgectrl = 143;
		touchscreen_config.xedgedist = 50;
		touchscreen_config.yedgectrl = 143;
		touchscreen_config.yedgedist = 90;
		touchscreen_config.jumplimit = 18;

		if (write_multitouchscreen_config(0, touchscreen_config) != CFG_WRITE_OK)
			dprintk("[TSP] qt_Multitouchscreen_Init Configuration Fail!!! , Line %d \n\r", __LINE__);

	} else if (info_block->info_id.family_id == 0x81) {
#ifdef _SUPPORT_TOUCH_AMPLITUDE_
		touchscreen_config_e.ctrl = 0x8B; // enable amplitude
#else
		touchscreen_config_e.ctrl = 131; // enable + message-enable
#endif
		touchscreen_config_e.xorigin = 0;
		touchscreen_config_e.yorigin = 0;
		touchscreen_config_e.xsize = 19;
		touchscreen_config_e.ysize = 11;
		touchscreen_config_e.akscfg = 0;
		touchscreen_config_e.blen = 32;
		touchscreen_config_e.tchthr = 50;
		touchscreen_config_e.tchdi = 2;
		touchscreen_config_e.orient = 7;
		touchscreen_config_e.mrgtimeout = 10;
		touchscreen_config_e.movhysti = 15;
		touchscreen_config_e.movhystn = 1;
		touchscreen_config_e.movfilter = 11;
 #ifdef _SUPPORT_MULTITOUCH_	
		touchscreen_config_e.numtouch= 10;
#else
		touchscreen_config_e.numtouch= 1;
#endif
		touchscreen_config_e.mrghyst =5;
		touchscreen_config_e.mrgthr = 40;
		touchscreen_config_e.amphyst = 10;
		touchscreen_config_e.xrange = 799;
		touchscreen_config_e.yrange = 479;
		touchscreen_config_e.xloclip = 10;
		touchscreen_config_e.xhiclip = 10;
		touchscreen_config_e.yloclip = 10;
		touchscreen_config_e.yhiclip = 10;
		touchscreen_config_e.xedgectrl = 143;
		touchscreen_config_e.xedgedist = 40;
		touchscreen_config_e.yedgectrl = 143;
		touchscreen_config_e.yedgedist = 80;
		touchscreen_config_e.jumplimit = 18;
		touchscreen_config_e.tchhyst = 15;
		touchscreen_config_e.xpitch = 50;
		touchscreen_config_e.ypitch = 50;
		touchscreen_config_e.nexttchdi = 2;

		if (write_multitouchscreen_config_e(0, touchscreen_config_e) != CFG_WRITE_OK)
			dprintk("[TSP] qt_Multitouchscreen_Init Configuration Fail!!! , Line %d \n\r", __LINE__);

	}
}

#ifdef QT_STYLUS_ENABLE
void qt_Multitouchscreen_stylus_Init(void)
{
	if (info_block->info_id.family_id == 0x80) {
		touchscreen_config.tchthr = 25;//45;	
		touchscreen_config.movhysti = 1;
    
		//touchscreen_config.movhystn = 5;///1;
		touchscreen_config.movfilter = 79;//0
		touchscreen_config.numtouch= 1;

		if (write_multitouchscreen_config(0, touchscreen_config) != CFG_WRITE_OK)
			dprintk("[TSP] qt_Multitouchscreen_stylus_Init Configuration Fail!!! , Line %d \n\r", __LINE__);

	} else if (info_block->info_id.family_id == 0x81) {
		touchscreen_config_e.tchthr = 25;//45;	
		touchscreen_config_e.movhysti = 1;

		//touchscreen_config_e.movhystn = 5;///1;
		touchscreen_config_e.movfilter = 79;//0
		touchscreen_config_e.numtouch= 1;

		if (write_multitouchscreen_config_e(0, touchscreen_config_e) != CFG_WRITE_OK)
			dprintk("[TSP] qt_Multitouchscreen_stylus_Init Configuration Fail!!! , Line %d \n\r", __LINE__);
	}
}

void qt_Multitouchscreen_normal_Init(void)
{
	if (info_block->info_id.family_id == 0x80) {
		touchscreen_config.tchthr = 40;//45;	
		touchscreen_config.movhysti = 3;
		//touchscreen_config.movhystn = 6;///1;
#if DRIVER_FILTER
		touchscreen_config.movfilter = 11;
#else
		touchscreen_config.movfilter = 46; //0x2e;//0
#endif
		touchscreen_config.numtouch= MAX_USING_FINGER_NUM;

		if (write_multitouchscreen_config(0, touchscreen_config) != CFG_WRITE_OK)
			dprintk("[TSP] qt_Multitouchscreen_stylus_Init Configuration Fail!!! , Line %d \n\r", __LINE__);

	} else if (info_block->info_id.family_id == 0x81) {
		touchscreen_config_e.tchthr = 35;	
		touchscreen_config_e.movhysti = 10;
		//touchscreen_config_e.movhystn = 1;
#if DRIVER_FILTER
		touchscreen_config_e.movfilter = 11;
#else
		touchscreen_config_e.movfilter = 46;
#endif
		touchscreen_config_e.numtouch = MAX_USING_FINGER_NUM;

		if (write_multitouchscreen_config_e(0, touchscreen_config_e) != CFG_WRITE_OK)
			dprintk("[TSP] qt_Multitouchscreen_stylus_Init Configuration Fail!!! , Line %d \n\r", __LINE__);
	}
}
#endif

/*****************************************************************************
*
*  FUNCTION
*  PURPOSE
*  INPUT
*  OUTPUT
*
* ***************************************************************************/

void qt_KeyArray_Init(void)
{
	keyarray_config.ctrl = 0;
	keyarray_config.xorigin = 0;
	keyarray_config.yorigin = 0;
	keyarray_config.xsize = 0;
	keyarray_config.ysize = 0;
	keyarray_config.akscfg = 0;
	keyarray_config.blen = 0;
	keyarray_config.tchthr = 0;
	keyarray_config.tchdi = 0;
	keyarray_config.reserved[0] = 0;
	keyarray_config.reserved[1] = 0;

	if (write_keyarray_config(0, keyarray_config) != CFG_WRITE_OK)
		dprintk("[TSP] Configuration Fail!!! , Line %d \n\r", __LINE__);
}


/*****************************************************************************
*
*  FUNCTION
*  PURPOSE
*  INPUT
*  OUTPUT
*
* ***************************************************************************/

void qt_ComcConfig_Init(void)
{
	comc_config.ctrl = 0x00;
//	comc_config.cmd = COMM_MODE1;
	comc_config.cmd = NO_COMMAND;

	if (get_object_address(SPT_COMCONFIG_T18, 0) != OBJECT_NOT_FOUND) {
		if (write_comc_config(0, comc_config) != CFG_WRITE_OK)
			dprintk("[TSP] Configuration Fail!!! , Line %d \n\r", __LINE__);
	}
}


/*****************************************************************************
*
*  FUNCTION
*  PURPOSE
*  INPUT
*  OUTPUT
*
* ***************************************************************************/

void qt_Gpio_Pwm_Init(void)
{
	gpiopwm_config.ctrl = 0;
	gpiopwm_config.reportmask = 0;
	gpiopwm_config.dir = 0;
	gpiopwm_config.intpullup = 0;
	gpiopwm_config.out = 0;
	gpiopwm_config.wake = 0;
	gpiopwm_config.pwm = 0;
	gpiopwm_config.period = 0;
	gpiopwm_config.duty[0] = 0;
	gpiopwm_config.duty[1] = 0;
	gpiopwm_config.duty[2] = 0;
	gpiopwm_config.duty[3] = 0;

	if (write_gpio_config(0, gpiopwm_config) != CFG_WRITE_OK)
		dprintk("[TSP] Configuration Fail!!! , Line %d \n\r", __LINE__);
}


/*****************************************************************************
*
*  FUNCTION
*  PURPOSE
*  INPUT
*  OUTPUT
*
* ***************************************************************************/

void qt_Grip_Face_Suppression_Config_Init(void)
{
	gripfacesuppression_config.ctrl = 7;        //-> disable PALM bit
	gripfacesuppression_config.xlogrip = 0;
	gripfacesuppression_config.xhigrip = 0;
	gripfacesuppression_config.ylogrip = 0;
	gripfacesuppression_config.yhigrip = 0;
	gripfacesuppression_config.maxtchs = 0;
	gripfacesuppression_config.reserved = 0;
	gripfacesuppression_config.szthr1 = 80;
	gripfacesuppression_config.szthr2 = 40;
	gripfacesuppression_config.shpthr1 = 0;
	gripfacesuppression_config.shpthr2 = 15;
	gripfacesuppression_config.supextto = 10;//5;

    /* Write grip suppression config to chip. */
	if (get_object_address(PROCI_GRIPFACESUPPRESSION_T20, 0) != OBJECT_NOT_FOUND) {
		if (write_gripsuppression_config(0, gripfacesuppression_config) != CFG_WRITE_OK)
			dprintk("[TSP] Configuration Fail!!! , Line %d \n\r", __LINE__);
	}
}


/*****************************************************************************
*
*  FUNCTION
*  PURPOSE
*  INPUT
*  OUTPUT
*
* ***************************************************************************/

void qt_Noise_Suppression_Config_Init(void)
{
	uint16_t object_address;
	uint8_t data;
	uint8_t status;
	
	if (info_block->info_id.family_id == 0x80) {
		noise_suppression_config.ctrl = 135;//5;        
		noise_suppression_config.reserved = 0;
		noise_suppression_config.reserved1 = 0;
		noise_suppression_config.gcaful = 0;
		noise_suppression_config.gcafll = 0;
		noise_suppression_config.actvgcafvalid = 3;
		noise_suppression_config.noisethr = 30; 	//35;
		noise_suppression_config.freqhopscale = 0;///1;
		noise_suppression_config.freq[0] = 29; 	//5;	// 6;//10;
		noise_suppression_config.freq[1] = 34;	//15;	// 11;//15;
		noise_suppression_config.freq[2] = 39;	//25;	//16;//20;
		noise_suppression_config.freq[3] = 49;	//35;	// 19;//25;
		noise_suppression_config.freq[4] = 58;	//45;	// 21;//30;
		noise_suppression_config.idlegcafvalid = 3;

		/* Write Noise suppression config to chip. */
		if (get_object_address(PROCG_NOISESUPPRESSION_T22, 0) != OBJECT_NOT_FOUND) {
			if (write_noisesuppression_config(0, noise_suppression_config) != CFG_WRITE_OK)
				dprintk("[TSP] Configuration Fail!!! , Line %d \n\r", __LINE__);
		}
		noise_freq_table.fherr_setting = false;
		noise_freq_table.fherr_count = 0;
		noise_freq_table.t9_blen_for_fherr = 16;
		noise_freq_table.t9_thr_for_fherr = 60;
		noise_freq_table.t22_noisethr_for_fherr = 35;
		noise_freq_table.freq_for_fherr0[0] = 29;
		noise_freq_table.freq_for_fherr0[1] = 34;
		noise_freq_table.freq_for_fherr0[2] = 39;
		noise_freq_table.freq_for_fherr0[3] = 49;
		noise_freq_table.freq_for_fherr0[4] = 58;
		noise_freq_table.freq_for_fherr1[0] = 45;
		noise_freq_table.freq_for_fherr1[1] = 49;
		noise_freq_table.freq_for_fherr1[2] = 55;
		noise_freq_table.freq_for_fherr1[3] = 59;
		noise_freq_table.freq_for_fherr1[4] = 63;
		noise_freq_table.freq_for_fherr2[0] = 10;
		noise_freq_table.freq_for_fherr2[1] = 12;
		noise_freq_table.freq_for_fherr2[2] = 18;
		noise_freq_table.freq_for_fherr2[3] = 40;
		noise_freq_table.freq_for_fherr2[4] = 72;
		noise_freq_table.freq_for_fherr3[0] = 7;
		noise_freq_table.freq_for_fherr3[1] = 33;
		noise_freq_table.freq_for_fherr3[2] = 39;
		noise_freq_table.freq_for_fherr3[3] = 52;
		noise_freq_table.freq_for_fherr3[4] = 64;

	} if (info_block->info_id.family_id == 0x81) {
		noise_suppression_config_e.ctrl = 3;//1;
		noise_suppression_config_e.cfg = 12;
		noise_suppression_config_e.calcfg = 64;//80;
		noise_suppression_config_e.basefreq = 0;
		noise_suppression_config_e.reserved1[0] = 0;
		noise_suppression_config_e.reserved1[1] = 0;
		noise_suppression_config_e.reserved1[2] = 0;
		noise_suppression_config_e.reserved1[3] = 0;
		noise_suppression_config_e.mffreq[0] = 0;
		noise_suppression_config_e.mffreq[1] = 0;
		noise_suppression_config_e.reserved2[0] = 0;
		noise_suppression_config_e.reserved2[1] = 0;
		noise_suppression_config_e.reserved2[2] = 0;
		noise_suppression_config_e.gcactvinvldadcs = 6;
		noise_suppression_config_e.gcidlenivldadcs = 6;
		noise_suppression_config_e.reserved3[0] = 0;
		noise_suppression_config_e.reserved3[1] = 0;
		noise_suppression_config_e.gcmaxadcsperx = 100;
		noise_suppression_config_e.gclimitmin = 4;
		noise_suppression_config_e.gclimitmax = 64;
		noise_suppression_config_e.gccountmintgt = 10;
		noise_suppression_config_e.mfinvlddiffthr = 20;
		noise_suppression_config_e.mfincadcspxthr = 5;
		noise_suppression_config_e.mferrorthr = 38;
		noise_suppression_config_e.selfreqmax = 5;//20;
		noise_suppression_config_e.reserved4[0] = 0;
		noise_suppression_config_e.reserved4[1] = 0;
		noise_suppression_config_e.reserved4[2] = 0;
		noise_suppression_config_e.reserved4[3] = 0;
		noise_suppression_config_e.reserved4[4] = 0;
		noise_suppression_config_e.reserved4[5] = 0;
		noise_suppression_config_e.blen = 16;
		noise_suppression_config_e.tchthr = 35;
		noise_suppression_config_e.tchdi = 2;
		noise_suppression_config_e.movhysti = 10;
		noise_suppression_config_e.movhystn = 1;
#if DRIVER_FILTER
		noise_suppression_config_e.movfilter = 11;
#else
		noise_suppression_config_e.movfilter = 46;
#endif
		noise_suppression_config_e.numtouch = 5;
		noise_suppression_config_e.mrghyst = 5;
		noise_suppression_config_e.mrgthr = 40;
		noise_suppression_config_e.xloclip = 10;
		noise_suppression_config_e.xhiclip = 10;
		noise_suppression_config_e.yloclip = 10;
		noise_suppression_config_e.yhiclip = 10;
		noise_suppression_config_e.xedgectrl = 143;
		noise_suppression_config_e.xedgedist = 40;
		noise_suppression_config_e.yedgectrl = 143;
		noise_suppression_config_e.yedgedist = 80;
		noise_suppression_config_e.jumplimit = 18;
		noise_suppression_config_e.tchhyst = 15;
		noise_suppression_config_e.nexttchdi = 2;

		if (get_object_address(PROCG_NOISESUPPRESSION_T48, 0) != OBJECT_NOT_FOUND) {
			if (write_noisesuppression_config_e(0, noise_suppression_config_e) != CFG_WRITE_OK)
				dprintk("[TSP] Configuration Fail!!! , Line %d \n\r", __LINE__);
		}

		object_address = get_object_address(PROCG_NOISESUPPRESSION_T48, 0);
		if (object_address == 0)
			dprintk("[TSP][ERROR] TOUCH_MULTITOUCHSCREEN_T48 object_address : %d\n", __LINE__);
		else {
			data = noise_suppression_config_e.calcfg | 0x20;
			status = write_mem(object_address+2, 1, &data);
			if (status == WRITE_MEM_FAILED)
				printk(KERN_ERR"[TSP][ERROR] mxt_noise_suppression_config write_mem error : %d\n", __LINE__);
		}

#if MXT224E_TA_CONFIG_MODE
		/* for TA setting */
		noise_suppression_config_e_TA.ctrl = 1;
		noise_suppression_config_e_TA.cfg = 12;
		noise_suppression_config_e_TA.calcfg = 80;
		noise_suppression_config_e_TA.basefreq = 0;
		noise_suppression_config_e_TA.reserved1[0] = 0;
		noise_suppression_config_e_TA.reserved1[1] = 0;
		noise_suppression_config_e_TA.reserved1[2] = 0;
		noise_suppression_config_e_TA.reserved1[3] = 0;
		noise_suppression_config_e_TA.mffreq[0] = 0;
		noise_suppression_config_e_TA.mffreq[1] = 0;
		noise_suppression_config_e_TA.reserved2[0] = 0;
		noise_suppression_config_e_TA.reserved2[1] = 0;
		noise_suppression_config_e_TA.reserved2[2] = 0;
		noise_suppression_config_e_TA.gcactvinvldadcs = 6;
		noise_suppression_config_e_TA.gcidlenivldadcs = 6;
		noise_suppression_config_e_TA.reserved3[0] = 0;
		noise_suppression_config_e_TA.reserved3[1] = 0;
		noise_suppression_config_e_TA.gcmaxadcsperx = 100;
		noise_suppression_config_e_TA.gclimitmin = 4;
		noise_suppression_config_e_TA.gclimitmax = 64;
		noise_suppression_config_e_TA.gccountmintgt = 10;
		noise_suppression_config_e_TA.mfinvlddiffthr = 20;
		noise_suppression_config_e_TA.mfincadcspxthr = 5;
		noise_suppression_config_e_TA.mferrorthr = 38;
		noise_suppression_config_e_TA.selfreqmax = 20;
		noise_suppression_config_e_TA.reserved4[0] = 0;
		noise_suppression_config_e_TA.reserved4[1] = 0;
		noise_suppression_config_e_TA.reserved4[2] = 0;
		noise_suppression_config_e_TA.reserved4[3] = 0;
		noise_suppression_config_e_TA.reserved4[4] = 0;
		noise_suppression_config_e_TA.reserved4[5] = 0;
		noise_suppression_config_e_TA.blen = 16;
		noise_suppression_config_e_TA.tchthr = 35;
		noise_suppression_config_e_TA.tchdi = 2;
		noise_suppression_config_e_TA.movhysti = 10;
		noise_suppression_config_e_TA.movhystn = 1;
#if DRIVER_FILTER
		noise_suppression_config_e_TA.movfilter = 11;
#else
		noise_suppression_config_e_TA.movfilter = 46;
#endif
		noise_suppression_config_e_TA.numtouch = 5;
		noise_suppression_config_e_TA.mrghyst = 5;
		noise_suppression_config_e_TA.mrgthr = 40;
		noise_suppression_config_e_TA.xloclip = 10;
		noise_suppression_config_e_TA.xhiclip = 10;
		noise_suppression_config_e_TA.yloclip = 10;
		noise_suppression_config_e_TA.yhiclip = 10;
		noise_suppression_config_e_TA.xedgectrl = 143;
		noise_suppression_config_e_TA.xedgedist = 40;
		noise_suppression_config_e_TA.yedgectrl = 143;
		noise_suppression_config_e_TA.yedgedist = 80;
		noise_suppression_config_e_TA.jumplimit = 18;
		noise_suppression_config_e_TA.tchhyst = 15;
		noise_suppression_config_e_TA.nexttchdi = 2;
#endif
	}
}


/*****************************************************************************
*
*  FUNCTION
*  PURPOSE
*  INPUT
*  OUTPUT
*
* ***************************************************************************/

void qt_Proximity_Config_Init(void)
{
	proximity_config.ctrl = 0;
	proximity_config.xorigin = 0;
	proximity_config.xsize = 0;
	proximity_config.ysize = 0;
	proximity_config.reserved_for_future_aks_usage = 0;
	proximity_config.blen = 0;
	proximity_config.tchthr = 0;
	proximity_config.tchdi = 0;
	proximity_config.average = 0;
	proximity_config.rate = 0;

	if (get_object_address(TOUCH_PROXIMITY_T23, 0) != OBJECT_NOT_FOUND) {
		if (write_proximity_config(0, proximity_config) != CFG_WRITE_OK)
			dprintk("[TSP] Configuration Fail!!! , Line %d \n\r", __LINE__);
	}
}


/*****************************************************************************
*
*  FUNCTION
*  PURPOSE
*  INPUT
*  OUTPUT
*
* ***************************************************************************/

void qt_One_Touch_Gesture_Config_Init(void)
{
	/* Disable one touch gestures. */
	onetouch_gesture_config.ctrl = 0;
	onetouch_gesture_config.numgest = 0;
	onetouch_gesture_config.gesten = 0;
	onetouch_gesture_config.pressproc = 0;
	onetouch_gesture_config.tapto = 0;
	onetouch_gesture_config.flickto = 0;
	onetouch_gesture_config.dragto = 0;
	onetouch_gesture_config.spressto = 0;
	onetouch_gesture_config.lpressto = 0;
	onetouch_gesture_config.reppressto = 0;
	onetouch_gesture_config.flickthr = 0;
	onetouch_gesture_config.dragthr = 0;
	onetouch_gesture_config.tapthr = 0;
	onetouch_gesture_config.throwthr = 0;

	if (get_object_address(PROCI_ONETOUCHGESTUREPROCESSOR_T24, 0) != OBJECT_NOT_FOUND) {
		if (write_onetouchgesture_config(0, onetouch_gesture_config) != CFG_WRITE_OK)
			dprintk("[TSP] Configuration Fail!!! , Line %d \n\r", __LINE__);
	}
}


/*****************************************************************************
*
*  FUNCTION
*  PURPOSE
*  INPUT
*  OUTPUT
*
* ***************************************************************************/

void qt_Selftest_Init(void)
{
	selftest_config.ctrl = 0;
	selftest_config.cmd = 0;

#ifdef NUM_OF_TOUCH_OBJECTS
	siglim.upsiglim[0] = 0;
	siglim.losiglim[0] = 0;
#endif
	if (get_object_address(SPT_SELFTEST_T25, 0) != OBJECT_NOT_FOUND) {
		if (write_selftest_config(0,selftest_config) != CFG_WRITE_OK)
			dprintk("[TSP] Configuration Fail!!! , Line %d \n\r", __LINE__);
	}
}


/*****************************************************************************
*
*  FUNCTION
*  PURPOSE
*  INPUT
*  OUTPUT
*
* ***************************************************************************/

void qt_Two_touch_Gesture_Config_Init(void)
{
	/* Disable two touch gestures. */
	twotouch_gesture_config.ctrl = 0;
	twotouch_gesture_config.numgest = 0;
	twotouch_gesture_config.reserved2 = 0;
	twotouch_gesture_config.gesten = 0;
	twotouch_gesture_config.rotatethr = 0;
	twotouch_gesture_config.zoomthr = 0;

	if (get_object_address(PROCI_TWOTOUCHGESTUREPROCESSOR_T27, 0) != OBJECT_NOT_FOUND) {
		if (write_twotouchgesture_config(0, twotouch_gesture_config) != CFG_WRITE_OK)
			dprintk("[TSP] Configuration Fail!!! , Line %d \n\r", __LINE__);
	}
}


/*****************************************************************************
*
*  FUNCTION
*  PURPOSE
*  INPUT
*  OUTPUT
*
* ***************************************************************************/

void qt_CTE_Config_Init(void)
{
	if (info_block->info_id.family_id == 0x80) {
	    /* Set CTE config */
		cte_config.ctrl = 1;
		cte_config.cmd = 0;    
		cte_config.mode = 3;
		cte_config.idlegcafdepth = 16;//4;
		cte_config.actvgcafdepth = 63;//8;
		cte_config.voltage = 60;

		/* Write CTE config to chip. */
		if (get_object_address(SPT_CTECONFIG_T28, 0) != OBJECT_NOT_FOUND) {
			if (write_CTE_config(cte_config) != CFG_WRITE_OK)
				dprintk("[TSP] Configuration Fail!!! , Line %d \n\r", __LINE__);
		}
	} else if (info_block->info_id.family_id == 0x81) {
		cte_config_e.ctrl = 0;
		cte_config_e.mode = 3;
		cte_config_e.idlesyncsperx = 16;
		cte_config_e.actvsyncsperx = 48;
		cte_config_e.adcspersync = 0;
		cte_config_e.pulsesperadc = 0;
		cte_config_e.xslew = 1;
		cte_config_e.syncdelay = 0;

		if (get_object_address(SPT_CTECONFIG_T46, 0) != OBJECT_NOT_FOUND) {
			if (write_CTE_config_e(0, cte_config_e) != CFG_WRITE_OK)
				dprintk("[TSP] Configuration Fail!!! , Line %d \n\r", __LINE__);
		}
	}
}

/*****************************************************************************
*
*  FUNCTION
*  PURPOSE
*  INPUT
*  OUTPUT
*
* ***************************************************************************/


void qt_Grip_Suppression_Config_Init(void)
{
	grip_suppression_config.ctrl = 0;
	grip_suppression_config.xlogrip = 0;
	grip_suppression_config.xhigrip = 0;
	grip_suppression_config.ylogrip = 0;
	grip_suppression_config.yhigrip = 0;

	if (get_object_address(PROCI_GRIPSUPPRESSION_T40, 0) != OBJECT_NOT_FOUND) {
		if (write_grip_suppression_config(grip_suppression_config) != CFG_WRITE_OK)
			dprintk("[TSP] Configuration Fail!!! , Line %d \n\r", __LINE__);
	}
}



/*****************************************************************************
*
*  FUNCTION
*  PURPOSE
*  INPUT
*  OUTPUT
*
* ***************************************************************************/


void qt_Touch_Suppression_Config_Init(void)
{
	touch_suppression_config.ctrl = 0;
	touch_suppression_config.apprthr = 0;
	touch_suppression_config.maxapprarea = 0;
	touch_suppression_config.maxtcharea = 0;
	touch_suppression_config.supstrength = 0;
	touch_suppression_config.supextto = 0;
	touch_suppression_config.maxnumtchs = 0;
	touch_suppression_config.shapestrength = 0;

	if (get_object_address(PROCI_TOUCHSUPPRESSION_T42, 0) != OBJECT_NOT_FOUND) {
		if (write_touch_suppression_config(touch_suppression_config) != CFG_WRITE_OK)
			dprintk("[TSP] Configuration Fail!!! , Line %d \n\r", __LINE__);
	}
}


/*****************************************************************************
*
*  FUNCTION
*  PURPOSE
*  INPUT
*  OUTPUT
*
* ***************************************************************************/


void qt_Stylus_Init(void)
{
	stylus_config.ctrl = 0;
	stylus_config.contmin = 0;
	stylus_config.contmax = 0;
	stylus_config.stability = 0;
	stylus_config.maxtcharea = 0;
	stylus_config.amplthr = 0;
	stylus_config.styshape = 0;
	stylus_config.hoversup = 0;
	stylus_config.confthr = 0;
	stylus_config.syncsperx = 0;

	if (get_object_address(PROCI_STYLUS_T47, 0) != OBJECT_NOT_FOUND) {
		if (write_stylus_config(stylus_config) != CFG_WRITE_OK)
			dprintk("[TSP] Configuration Fail!!! , Line %d \n\r", __LINE__);
	}
}


/*****************************************************************************
*
*  FUNCTION
*  PURPOSE
*  INPUT
*  OUTPUT
*
* ***************************************************************************/

unsigned char Comm_Config_Process(unsigned char change_en)
{
	change_en = 0;
	return(change_en);
}


/*****************************************************************************
*
*  FUNCTION
*  PURPOSE
*  INPUT
*  OUTPUT
*
* ***************************************************************************/

uint8_t reset_chip(void)
{
	uint8_t data = 1u;

	dprintk("\n[TSP][%s] \n", __func__);
	cal_check_flag = 1; //20100309s
	return(write_mem(command_processor_address + RESET_OFFSET, 1, &data));
}


/*****************************************************************************
*
*  FUNCTION
*  PURPOSE
*  INPUT
*  OUTPUT
*
* ***************************************************************************/

uint8_t calibrate_chip(void)
{
	uint8_t data = 1u;
	int ret = WRITE_MEM_OK;
	uint8_t atchcalst, atchcalsthr;

	if (info_block->info_id.family_id == 0x80) {
		if ((tsp_version >= 0x16)&&(cal_check_flag == 0)) {
			/* change calibration suspend settings to zero until calibration confirmed good */
			/* store normal settings */
			atchcalst = acquisition_config.atchcalst;
			atchcalsthr = acquisition_config.atchcalsthr;

			/* resume calibration must be performed with zero settings */
			acquisition_config.atchcalst = 0;
			acquisition_config.atchcalsthr = 0;

			dprintk("[TSP] reset acq atchcalst=%d, atchcalsthr=%d\n",
				acquisition_config.atchcalst, acquisition_config.atchcalsthr );

			/* Write temporary acquisition config to chip. */
			if (write_acquisition_config(acquisition_config) != CFG_WRITE_OK) {
				/* "Acquisition config write failed!\n" */
				dprintk("\n[TSP][ERROR] line : %d\n", __LINE__);
				ret = WRITE_MEM_FAILED; /* calling function should retry calibration call */
			}

			/* restore settings to the local structure so that when we confirm the 
			cal is good we can correct them in the chip */

			/* this must be done before returning */
			acquisition_config.atchcalst = atchcalst;
			acquisition_config.atchcalsthr = atchcalsthr;
		}
	} else if (info_block->info_id.family_id == 0x81) {
		if (cal_check_flag == 0) {
			/* change calibration suspend settings to zero until calibration confirmed good */
			/* store normal settings */
			atchcalst = acquisition_config_e.atchcalst;
			atchcalsthr = acquisition_config_e.atchcalsthr;

			/* resume calibration must be performed with zero settings */
			acquisition_config_e.atchcalst = 0;
			acquisition_config_e.atchcalsthr = 0;

			dprintk("[TSP] reset acq atchcalst=%d, atchcalsthr=%d\n",
				acquisition_config_e.atchcalst, acquisition_config_e.atchcalsthr );

			/* Write temporary acquisition config to chip. */
			if (write_acquisition_config_e(acquisition_config_e) != CFG_WRITE_OK) {
				/* "Acquisition config write failed!\n" */
				dprintk("\n[TSP][ERROR] line : %d\n", __LINE__);
				ret = WRITE_MEM_FAILED; /* calling function should retry calibration call */
			}

			/* restore settings to the local structure so that when we confirm the 
			cal is good we can correct them in the chip */

			/* this must be done before returning */
			acquisition_config_e.atchcalst = atchcalst;
			acquisition_config_e.atchcalsthr = atchcalsthr;
		}
	}

	/* send calibration command to the chip */
	if (ret == WRITE_MEM_OK) {
	/* change calibration suspend settings to zero until calibration confirmed good */
		ret = write_mem(command_processor_address + CALIBRATE_OFFSET, 1, &data);

		/* set flag for calibration lockup recovery if cal command was successful */
		if (ret == WRITE_MEM_OK) {
			/* set flag to show we must still confirm if calibration was good or bad */
			cal_check_flag = 1;
		}
	}
	return ret;
}


/*****************************************************************************
*
*  FUNCTION
*  PURPOSE
*  INPUT
*  OUTPUT
*
* ***************************************************************************/

uint8_t diagnostic_chip(uint8_t mode)
{
	uint8_t status;
	status = write_mem(command_processor_address + DIAGNOSTIC_OFFSET, 1, &mode);
	return(status);
}


/*****************************************************************************
*
*  FUNCTION
*  PURPOSE
*  INPUT
*  OUTPUT
*
* ***************************************************************************/

uint8_t backup_config(void)
{
	/* Write 0x55 to BACKUPNV register to initiate the backup. */
	uint8_t data = 0x55u;
	return(write_mem(command_processor_address + BACKUP_OFFSET, 1, &data));
}


/*****************************************************************************
*
*  FUNCTION
*  PURPOSE
*  INPUT
*  OUTPUT
*
* ***************************************************************************/

uint8_t get_version(uint8_t *version)
{
	if (info_block)
		*version = info_block->info_id.version;
	else
		return(ID_DATA_NOT_AVAILABLE);

	return(ID_DATA_OK);
}


/*****************************************************************************
*
*  FUNCTION
*  PURPOSE
*  INPUT
*  OUTPUT
*
* ***************************************************************************/

uint8_t get_family_id(uint8_t *family_id)
{
	if (info_block)
		*family_id = info_block->info_id.family_id;
	else
		return(ID_DATA_NOT_AVAILABLE);

	return(ID_DATA_OK);
}


/*****************************************************************************
*
*  FUNCTION
*  PURPOSE
*  INPUT
*  OUTPUT
*
* ***************************************************************************/

uint8_t get_build_number(uint8_t *build)
{
	if (info_block)
		*build = info_block->info_id.build;
	else
		return(ID_DATA_NOT_AVAILABLE);

	return(ID_DATA_OK);
}


/*****************************************************************************
*
*  FUNCTION
*  PURPOSE
*  INPUT
*  OUTPUT
*
* ***************************************************************************/

uint8_t get_variant_id(uint8_t *variant)
{
	if (info_block)
		*variant = info_block->info_id.variant_id;
	else
		return(ID_DATA_NOT_AVAILABLE);

	return(ID_DATA_OK);
}


/*****************************************************************************
*
*  FUNCTION
*  PURPOSE
*  INPUT
*  OUTPUT
*
* ***************************************************************************/

uint8_t write_power_config(gen_powerconfig_t7_config_t cfg)
{
	return(write_simple_config(GEN_POWERCONFIG_T7, 0, (void *) &cfg));
}


/*****************************************************************************
*
*  FUNCTION
*  PURPOSE
*  INPUT
*  OUTPUT
*
* ***************************************************************************/

uint8_t write_acquisition_config(gen_acquisitionconfig_t8_config_t cfg)
{
	return(write_simple_config(GEN_ACQUISITIONCONFIG_T8, 0, (void *) &cfg));
}

uint8_t write_acquisition_config_e(gen_acquisitionconfig_t8_config_t_e cfg)
{
    return(write_simple_config(GEN_ACQUISITIONCONFIG_T8, 0, (void *) &cfg));
}


/*****************************************************************************
*
*  FUNCTION
*  PURPOSE
*  INPUT
*  OUTPUT
*
* ***************************************************************************/

uint8_t write_multitouchscreen_config(uint8_t instance, touch_multitouchscreen_t9_config_t cfg)
{
	uint16_t object_address = OBJECT_NOT_FOUND;
	uint8_t *tmp;
	uint8_t status;
	uint8_t object_size;

	object_size = get_object_size(TOUCH_MULTITOUCHSCREEN_T9);
	if (object_size == 0)
		return(CFG_WRITE_FAILED);

	tmp = (uint8_t *) kmalloc(object_size, GFP_KERNEL | GFP_ATOMIC);
	if (tmp == NULL)
		return(CFG_WRITE_FAILED);

	memset(tmp, 0, object_size);

	/* 18 elements at beginning are 1 byte. */
	memcpy(tmp, &cfg, 18);

	/* Next two are 2 bytes. */
	*(tmp + 18) = (uint8_t) (cfg.xrange &  0xFF);
	*(tmp + 19) = (uint8_t) (cfg.xrange >> 8);

	*(tmp + 20) = (uint8_t) (cfg.yrange &  0xFF);
	*(tmp + 21) = (uint8_t) (cfg.yrange >> 8);

	/* And the last 4(8) 1 bytes each again. */
	*(tmp + 22) = cfg.xloclip;
	*(tmp + 23) = cfg.xhiclip;
	*(tmp + 24) = cfg.yloclip;
	*(tmp + 25) = cfg.yhiclip;

#if defined(__VER_1_4__)
	*(tmp + 26) = cfg.xedgectrl;
	*(tmp + 27) = cfg.xedgedist;
	*(tmp + 28) = cfg.yedgectrl;
	*(tmp + 29) = cfg.yedgedist;
#endif
	if (tsp_version >= 0x16)
		*(tmp + 30) = cfg.jumplimit;

	object_address = get_object_address(TOUCH_MULTITOUCHSCREEN_T9, instance);
	if (object_address == 0) {
		kfree(tmp);
		return(CFG_WRITE_FAILED);
	}

	status = write_mem(object_address, object_size, tmp);

	kfree(tmp);

	return(status);
}

uint8_t write_multitouchscreen_config_e(uint8_t instance, touch_multitouchscreen_t9_config_t_e cfg)
{
	uint16_t object_address = OBJECT_NOT_FOUND;
	uint8_t *tmp;
	uint8_t status;
	uint8_t object_size;

	object_size = get_object_size(TOUCH_MULTITOUCHSCREEN_T9);
	if (object_size == 0)
		return(CFG_WRITE_FAILED);

	tmp = (uint8_t *) kmalloc(object_size, GFP_KERNEL | GFP_ATOMIC);
	if (tmp == NULL)
		return(CFG_WRITE_FAILED);

	memset(tmp, 0, object_size);

	/* 18 elements at beginning are 1 byte. */
	memcpy(tmp, &cfg, 18);

	/* Next two are 2 bytes. */
	*(tmp + 18) = (uint8_t) (cfg.xrange &  0xFF);
	*(tmp + 19) = (uint8_t) (cfg.xrange >> 8);
	*(tmp + 20) = (uint8_t) (cfg.yrange &  0xFF);
	*(tmp + 21) = (uint8_t) (cfg.yrange >> 8);

	/* And the last 13 registers are 1 bytes each again. */
#if 0
	*(tmp + 22) = cfg.xloclip;
	*(tmp + 23) = cfg.xhiclip;
	*(tmp + 24) = cfg.yloclip;
	*(tmp + 25) = cfg.yhiclip;
	*(tmp + 26) = cfg.xedgectrl;
	*(tmp + 27) = cfg.xedgedist;
	*(tmp + 28) = cfg.yedgectrl;
	*(tmp + 29) = cfg.yedgedist;
	*(tmp + 30) = cfg.jumplimit;
	*(tmp + 31) = cfg.tchhyst;
	*(tmp + 32) = cfg.xpitch;
	*(tmp + 33) = cfg.ypitch;
	*(tmp + 34) = cfg.nexttchdi;
#else
	memcpy((tmp+22), &cfg.xloclip, 13);
#endif

	object_address = get_object_address(TOUCH_MULTITOUCHSCREEN_T9, instance);
	if (object_address == 0) {
		kfree(tmp);
		return(CFG_WRITE_FAILED);
	}

	status = write_mem(object_address, object_size, tmp);

	kfree(tmp);

	return(status);
}


/*****************************************************************************
*
*  FUNCTION
*  PURPOSE
*  INPUT
*  OUTPUT
*
* ***************************************************************************/

uint8_t write_keyarray_config(uint8_t instance, touch_keyarray_t15_config_t cfg)
{
	return(write_simple_config(TOUCH_KEYARRAY_T15, instance, (void *) &cfg));
}



/*****************************************************************************
*
*  FUNCTION
*  PURPOSE
*  INPUT
*  OUTPUT
*
* ***************************************************************************/

uint8_t write_linearization_config(uint8_t instance, proci_linearizationtable_t17_config_t cfg)
{
	uint16_t object_address;
	uint8_t *tmp;
	uint8_t status;
	uint8_t object_size;

	object_size = get_object_size(PROCI_LINEARIZATIONTABLE_T17);
	if (object_size == 0)
		return(CFG_WRITE_FAILED);

	tmp = (uint8_t *) kmalloc(object_size, GFP_KERNEL | GFP_ATOMIC);
	if (tmp == NULL)
		return(CFG_WRITE_FAILED);

	memset(tmp, 0, object_size);

	*(tmp + 0) = cfg.ctrl;
	*(tmp + 1) = (uint8_t) (cfg.xoffset & 0x00FF);
	*(tmp + 2) = (uint8_t) (cfg.xoffset >> 8);

	memcpy((tmp+3), &cfg.xsegment, 16);

	*(tmp + 19) = (uint8_t) (cfg.yoffset & 0x00FF);
	*(tmp + 20) = (uint8_t) (cfg.yoffset >> 8);

	memcpy((tmp+21), &cfg.ysegment, 16);

	object_address = get_object_address(PROCI_LINEARIZATIONTABLE_T17, instance);
	if (object_address == 0) {
		kfree(tmp);
		return(CFG_WRITE_FAILED);
	}

	status = write_mem(object_address, object_size, tmp);

	kfree(tmp);

	return(status);
}


/*****************************************************************************
*
*  FUNCTION
*  PURPOSE
*  INPUT
*  OUTPUT
*
* ***************************************************************************/

uint8_t write_comc_config(uint8_t instance, spt_comcconfig_t18_config_t cfg)
{
	return(write_simple_config(SPT_COMCONFIG_T18, instance, (void *)&cfg));
}



/*****************************************************************************
*
*  FUNCTION
*  PURPOSE
*  INPUT
*  OUTPUT
*
* ***************************************************************************/

uint8_t write_gpio_config(uint8_t instance, spt_gpiopwm_t19_config_t cfg)
{
	return(write_simple_config(SPT_GPIOPWM_T19, instance, (void *)&cfg));
}



/*****************************************************************************
*
*  FUNCTION
*  PURPOSE
*  INPUT
*  OUTPUT
*
* ***************************************************************************/

uint8_t write_gripsuppression_config(uint8_t instance, proci_gripfacesuppression_t20_config_t cfg)
{
	return(write_simple_config(PROCI_GRIPFACESUPPRESSION_T20, instance, (void *)&cfg));
}


/*****************************************************************************
*
*  FUNCTION
*  PURPOSE
*  INPUT
*  OUTPUT
*
* ***************************************************************************/

uint8_t write_noisesuppression_config(uint8_t instance, procg_noisesuppression_t22_config_t cfg)
{
	return(write_simple_config(PROCG_NOISESUPPRESSION_T22, instance, (void *) &cfg));
}


/*****************************************************************************
*
*  FUNCTION
*  PURPOSE
*  INPUT
*  OUTPUT
*
* ***************************************************************************/

uint8_t write_proximity_config(uint8_t instance, touch_proximity_t23_config_t cfg)
{
	uint16_t object_address;
	uint8_t *tmp;
	uint8_t status;
	uint8_t object_size;

	object_size = get_object_size(TOUCH_PROXIMITY_T23);
	if (object_size == 0)
		return(CFG_WRITE_FAILED);

	tmp = (uint8_t *) kmalloc(object_size, GFP_KERNEL | GFP_ATOMIC);
	if (tmp == NULL)
		return(CFG_WRITE_FAILED);

	memset(tmp, 0, object_size);

	*(tmp + 0) = cfg.ctrl;
	*(tmp + 1) = cfg.xorigin;
	*(tmp + 2) = cfg.yorigin;
	*(tmp + 3) = cfg.xsize;
	*(tmp + 4) = cfg.ysize;
	*(tmp + 5) = cfg.reserved_for_future_aks_usage;
	*(tmp + 6) = cfg.blen;

	*(tmp + 7) = (uint8_t) (cfg.tchthr & 0x00FF);
	*(tmp + 8) = (uint8_t) (cfg.tchthr >> 8);

	*(tmp + 9) = cfg.tchdi;
	*(tmp + 10) = cfg.average;

	*(tmp + 11) = (uint8_t) (cfg.rate & 0x00FF);
	*(tmp + 12) = (uint8_t) (cfg.rate >> 8);

	object_address = get_object_address(TOUCH_PROXIMITY_T23, instance);
	if (object_address == 0) {
		kfree(tmp);
		return(CFG_WRITE_FAILED);
	}

	status = write_mem(object_address, object_size, tmp);

	kfree(tmp);

	return(status);
}


/*****************************************************************************
*
*  FUNCTION
*  PURPOSE
*  INPUT
*  OUTPUT
*
* ***************************************************************************/

uint8_t write_onetouchgesture_config(uint8_t instance, proci_onetouchgestureprocessor_t24_config_t cfg)
{
	uint16_t object_address;
	uint8_t *tmp;
	uint8_t status;
	uint8_t object_size;

	object_size = get_object_size(PROCI_ONETOUCHGESTUREPROCESSOR_T24);
	if (object_size == 0)
		return(CFG_WRITE_FAILED);

	tmp = (uint8_t *) kmalloc(object_size, GFP_KERNEL | GFP_ATOMIC);
	if (tmp == NULL)
		return(CFG_WRITE_FAILED);

	memset(tmp, 0, object_size);

	*(tmp + 0) = cfg.ctrl;
#if defined(__VER_1_2__)
	*(tmp + 1) = 0;
#else
	*(tmp + 1) = cfg.numgest;
#endif

	*(tmp + 2) = (uint8_t) (cfg.gesten & 0x00FF);
	*(tmp + 3) = (uint8_t) (cfg.gesten >> 8);

	memcpy((tmp+4), &cfg.pressproc, 7);

	*(tmp + 11) = (uint8_t) (cfg.flickthr & 0x00FF);
	*(tmp + 12) = (uint8_t) (cfg.flickthr >> 8);

	*(tmp + 13) = (uint8_t) (cfg.dragthr & 0x00FF);
	*(tmp + 14) = (uint8_t) (cfg.dragthr >> 8);

	*(tmp + 15) = (uint8_t) (cfg.tapthr & 0x00FF);
	*(tmp + 16) = (uint8_t) (cfg.tapthr >> 8);

	*(tmp + 17) = (uint8_t) (cfg.throwthr & 0x00FF);
	*(tmp + 18) = (uint8_t) (cfg.throwthr >> 8);

	object_address = get_object_address(PROCI_ONETOUCHGESTUREPROCESSOR_T24, instance);
	if (object_address == 0) {
		kfree(tmp);
		return(CFG_WRITE_FAILED);
	}

	status = write_mem(object_address, object_size, tmp);

	kfree(tmp);

	return(status);
}


/*****************************************************************************
*
*  FUNCTION
*  PURPOSE
*  INPUT
*  OUTPUT
*
* ***************************************************************************/

uint8_t write_selftest_config(uint8_t instance, spt_selftest_t25_config_t cfg)
{
	uint16_t object_address;
	uint8_t *tmp;
	uint8_t status;
	uint8_t object_size;

	object_size = get_object_size(SPT_SELFTEST_T25);
	if (object_size == 0)
		return(CFG_WRITE_FAILED);

	tmp = (uint8_t *) kmalloc(object_size, GFP_KERNEL | GFP_ATOMIC);
	if (tmp == NULL)
		return(CFG_WRITE_FAILED);

	memset(tmp, 0, object_size);

	*(tmp + 0) = cfg.ctrl;
	*(tmp + 1) = cfg.cmd;
#ifdef NUM_OF_TOUCH_OBJECTS
	*(tmp + 2) = (uint8_t) (cfg.upsiglim & 0x00FF);
	*(tmp + 3) = (uint8_t) (cfg.upsiglim >> 8);

	*(tmp + 2) = (uint8_t) (cfg.losiglim & 0x00FF);
	*(tmp + 3) = (uint8_t) (cfg.losiglim >> 8);
#endif

	object_address = get_object_address(SPT_SELFTEST_T25, instance);
	if (object_address == 0) {
		kfree(tmp);
		return(CFG_WRITE_FAILED);
	}

	status = write_mem(object_address, object_size, tmp);

	kfree(tmp);

	return(status);
}



/*****************************************************************************
*
*  FUNCTION
*  PURPOSE
*  INPUT
*  OUTPUT
*
* ***************************************************************************/

uint8_t write_twotouchgesture_config(uint8_t instance, proci_twotouchgestureprocessor_t27_config_t cfg)
{
	uint16_t object_address;
	uint8_t *tmp;
	uint8_t status;
	uint8_t object_size;

	object_size = get_object_size(PROCI_TWOTOUCHGESTUREPROCESSOR_T27);
	if (object_size == 0)
		return(CFG_WRITE_FAILED);

	tmp = (uint8_t *) kmalloc(object_size, GFP_KERNEL | GFP_ATOMIC);
	if (tmp == NULL)
		return(CFG_WRITE_FAILED);

	memset(tmp, 0, object_size);

	*(tmp + 0) = cfg.ctrl;

#if defined(__VER_1_2__)
	*(tmp + 1) = 0;
#else
	*(tmp + 1) = cfg.numgest;
#endif

	*(tmp + 2) = 0;

	*(tmp + 3) = cfg.gesten;

	*(tmp + 4) = cfg.rotatethr;

	*(tmp + 5) = (uint8_t) (cfg.zoomthr & 0x00FF);
	*(tmp + 6) = (uint8_t) (cfg.zoomthr >> 8);

	object_address = get_object_address(PROCI_TWOTOUCHGESTUREPROCESSOR_T27, instance);
	if (object_address == 0) {
		kfree(tmp);
		return(CFG_WRITE_FAILED);
	}

	status = write_mem(object_address, object_size, tmp);

	kfree(tmp);

	return(status);
}


/*****************************************************************************
*
*  FUNCTION
*  PURPOSE
*  INPUT
*  OUTPUT
*
* ***************************************************************************/

uint8_t write_CTE_config(spt_cteconfig_t28_config_t cfg)
{
	return(write_simple_config(SPT_CTECONFIG_T28, 0, (void *) &cfg));
}


/*****************************************************************************
*
*  FUNCTION
*  PURPOSE
*  INPUT
*  OUTPUT
*
* ***************************************************************************/

uint8_t write_grip_suppression_config(proci_gripsuppression_t40_config_t cfg)
{
	return(write_simple_config(PROCI_GRIPSUPPRESSION_T40, 0, (void *) &cfg));
}


/*****************************************************************************
*
*  FUNCTION
*  PURPOSE
*  INPUT
*  OUTPUT
*
* ***************************************************************************/

uint8_t write_touch_suppression_config(proci_touchsuppression_t42_config_t cfg)
{
	return(write_simple_config(PROCI_TOUCHSUPPRESSION_T42, 0, (void *) &cfg));
}


/*****************************************************************************
*
*  FUNCTION
*  PURPOSE
*  INPUT
*  OUTPUT
*
* ***************************************************************************/

uint8_t write_stylus_config(proci_stylus_t47_config_t cfg)
{
	return(write_simple_config(PROCI_STYLUS_T47, 0, (void *) &cfg));
}


/*****************************************************************************
*
*  FUNCTION
*  PURPOSE
*  INPUT
*  OUTPUT
*
* ***************************************************************************/

uint8_t write_CTE_config_e(uint8_t instance, spt_cteconfig_t46_config_t cfg)
{

	uint16_t object_address = OBJECT_NOT_FOUND;
	uint8_t *tmp;
	uint8_t status;
	uint8_t object_size;

	object_size = get_object_size(SPT_CTECONFIG_T46);
	if (object_size == 0)
		return(CFG_WRITE_FAILED);

	tmp = (uint8_t *) kmalloc(object_size, GFP_KERNEL | GFP_ATOMIC);
	if (tmp == NULL)
		return(CFG_WRITE_FAILED);

	memset(tmp, 0, object_size);

	/* 7 elements at beginning are 1 byte. */
	memcpy(tmp, &cfg, 7);

	/* Next two are 2 bytes. */
	*(tmp + 7) = (uint8_t) (cfg.syncdelay &  0xFF);
	*(tmp + 8) = (uint8_t) (cfg.syncdelay >> 8);

	object_address = get_object_address(SPT_CTECONFIG_T46, instance);
	if (object_address == 0) {
		kfree(tmp);
		return(CFG_WRITE_FAILED);
	}

	status = write_mem(object_address, object_size, tmp);

	kfree(tmp);

	return(status);
}


/*****************************************************************************
*
*  FUNCTION
*  PURPOSE
*  INPUT
*  OUTPUT
*
* ***************************************************************************/

uint8_t write_noisesuppression_config_e(uint8_t instance, procg_noisesuppression_t48_config_t cfg)
{
	uint16_t object_address;
	uint8_t *tmp;
	uint8_t status;
	uint8_t object_size;

	object_size = get_object_size(PROCG_NOISESUPPRESSION_T48);
	if (object_size == 0)
		return(CFG_WRITE_FAILED);

	tmp = (uint8_t *) kmalloc(object_size, GFP_KERNEL | GFP_ATOMIC);
	if (tmp == NULL)
		return(CFG_WRITE_FAILED);

	memset(tmp, 0, object_size);

	memcpy(tmp, &cfg, 20);

	*(tmp + 20) = (uint8_t) (cfg.gccountmintgt & 0x00FF);
	*(tmp + 21) = (uint8_t) (cfg.gccountmintgt >> 8);

	*(tmp + 22) = (uint8_t) cfg.mfinvlddiffthr;

	*(tmp + 23) = (uint8_t) (cfg.mfincadcspxthr& 0x00FF);
	*(tmp + 24) = (uint8_t) (cfg.mfincadcspxthr >> 8);

	*(tmp + 25) = (uint8_t) (cfg.mferrorthr& 0x00FF);
	*(tmp + 26) = (uint8_t) (cfg.mferrorthr >> 8);

	memcpy((tmp+27), &cfg.selfreqmax, 27);

	object_address = get_object_address(PROCG_NOISESUPPRESSION_T48, instance);

	if (object_address == 0) {
		kfree(tmp);
		return(CFG_WRITE_FAILED);
	}

	status = write_mem(object_address, object_size, tmp);

	kfree(tmp);

	return(status);
}


/*****************************************************************************
*
*  FUNCTION
*  PURPOSE
*  INPUT
*  OUTPUT
*
* ***************************************************************************/

uint8_t write_simple_config(uint8_t object_type, uint8_t instance, void *cfg)
{
	uint16_t object_address;
	uint8_t object_size;

	object_address = get_object_address(object_type, instance);
	object_size = get_object_size(object_type);

	if ((object_size == 0) || (object_address == 0))
		return(CFG_WRITE_FAILED);

	return(write_mem(object_address, object_size, cfg));
}



/*****************************************************************************
*
*  FUNCTION
*  PURPOSE
*  INPUT
*  OUTPUT
*
* ***************************************************************************/

uint8_t get_object_size(uint8_t object_type)
{
	uint8_t object_table_index = 0;
	uint8_t object_found = 0;
	uint16_t size = OBJECT_NOT_FOUND;

	object_t *object_table;
	object_t obj;
	object_table = info_block->objects;

	while ((object_table_index < info_block->info_id.num_declared_objects) &&
		(object_found == 0)) {
		obj = object_table[object_table_index];
		/* Does object type match? */
		if (obj.object_type == object_type) {
			object_found = 1;
			size = obj.size + 1;
		}
		object_table_index++;
	}

	return(size);
}


/*****************************************************************************
*
*  FUNCTION
*  PURPOSE
*  INPUT
*  OUTPUT
*
* ***************************************************************************/

uint8_t type_to_report_id(uint8_t object_type, uint8_t instance)
{
	uint8_t report_id = 1;
	int8_t report_id_found = 0;

	while ((report_id <= max_report_id) && (report_id_found == 0)) {
		if ((report_id_map[report_id].object_type == object_type) &&
			(report_id_map[report_id].instance == instance))
			report_id_found = 1;
		else
			report_id++;
	}

	if (report_id_found)
		return(report_id);
	else
		return(ID_MAPPING_FAILED);
}


/*****************************************************************************
*
*  FUNCTION
*  PURPOSE
*  INPUT
*  OUTPUT
*
* ***************************************************************************/

uint8_t report_id_to_type(uint8_t report_id, uint8_t *instance)
{
	if (report_id <= max_report_id) {
		*instance = report_id_map[report_id].instance;
		return(report_id_map[report_id].object_type);
	} else
		return(ID_MAPPING_FAILED);
}



/*****************************************************************************
*
*  FUNCTION
*  PURPOSE
*  INPUT
*  OUTPUT
*
* ***************************************************************************/

uint8_t read_id_block(info_id_t *id)
{
	uint8_t status;

	status = read_mem(0, 1, (void *) &id->family_id);
	if (status != READ_MEM_OK)
		return(status);

	status = read_mem(1, 1, (void *) &id->variant_id);
	if (status != READ_MEM_OK)
		return(status);

	status = read_mem(2, 1, (void *) &id->version);
	if (status != READ_MEM_OK)
		return(status);

	status = read_mem(3, 1, (void *) &id->build);
	if (status != READ_MEM_OK)
		return(status);

	status = read_mem(4, 1, (void *) &id->matrix_x_size);
	if (status != READ_MEM_OK)
		return(status);

	status = read_mem(5, 1, (void *) &id->matrix_y_size);
	if (status != READ_MEM_OK)
		return(status);

	status = read_mem(6, 1, (void *) &id->num_declared_objects);

	return(status);
}


/*****************************************************************************
*
*  FUNCTION
*  PURPOSE
*  INPUT
*  OUTPUT
*
* ***************************************************************************/

uint8_t get_max_report_id()
{
	return(max_report_id);
}


/*****************************************************************************
*
*  FUNCTION
*  PURPOSE
*  INPUT
*  OUTPUT
*
* ***************************************************************************/

uint16_t get_object_address(uint8_t object_type, uint8_t instance)
{
	uint8_t object_table_index = 0;
	uint8_t address_found = 0;
	uint16_t address = OBJECT_NOT_FOUND;

	object_t *object_table;
	object_t obj;
	object_table = info_block->objects;

	while ((object_table_index < info_block->info_id.num_declared_objects) &&
		(address_found == 0)) {
		obj = object_table[object_table_index];
		/* Does object type match? */
		if (obj.object_type == object_type) {
			address_found = 1;

			/* Are there enough instances defined in the FW? */
			if (obj.instances >= instance)
				address = obj.i2c_address + (obj.size + 1) * instance;
		}
		object_table_index++;
	}

	return(address);
}



/*****************************************************************************
*
*  FUNCTION
*  PURPOSE
*  INPUT
*  OUTPUT
*
* ***************************************************************************/

uint32_t get_stored_infoblock_crc()
{
	uint32_t crc;
	crc = (uint32_t) (((uint32_t) info_block->CRC_hi) << 16);
	crc = crc | info_block->CRC;
	return(crc);
}


/*****************************************************************************
*
*  FUNCTION
*  PURPOSE
*  INPUT
*  OUTPUT
*
* ***************************************************************************/

uint8_t calculate_infoblock_crc(uint32_t *crc_pointer)
{
	uint32_t crc = 0;
	uint16_t crc_area_size;
	uint8_t *mem;
	uint8_t i = 0;
	uint8_t status;

	/* 7 bytes of version data, 6 * NUM_OF_OBJECTS bytes of object table. */
	crc_area_size = 7 + info_block->info_id.num_declared_objects * 6;

	mem = (uint8_t *) kmalloc(crc_area_size, GFP_KERNEL | GFP_ATOMIC);
	if (mem == NULL)
		return(CRC_CALCULATION_FAILED);

	status = read_mem(0, crc_area_size, mem);
	if (status != READ_MEM_OK) {
		kfree(mem);
		return(CRC_CALCULATION_FAILED);
	}

	while (i < (crc_area_size - 1)) {
		crc = CRC_24(crc, *(mem + i), *(mem + i + 1));
		i += 2;
	}

	crc = CRC_24(crc, *(mem + i), 0);

	kfree(mem);

	/* Return only 24 bit CRC. */
	*crc_pointer = (crc & 0x00FFFFFF);
	return(CRC_CALCULATION_OK);
}


/*****************************************************************************
*
*  FUNCTION
*  PURPOSE
*  INPUT
*  OUTPUT
*
* ***************************************************************************/

uint32_t CRC_24(uint32_t crc, uint8_t byte1, uint8_t byte2)
{
	static const uint32_t crcpoly = 0x80001B;
	uint32_t result;
	uint16_t data_word;

	data_word = (uint16_t) ((uint16_t) (byte2 << 8u) | byte1);
	result = ((crc << 1u) ^ (uint32_t) data_word);

	if (result & 0x1000000)
		result ^= crcpoly;

	return(result);
}




/*------------------------------ main block -----------------------------------*/
void quantum_touch_probe(void)
{
	U8 report_id;
	U8 object_type, instance;
	U32 stored_crc;
	U8 family_id, variant, build;
	uint32_t crc;

#ifdef QT_FIRMUP_ENABLE
	if (init_touch_driver(QT602240_I2C_ADDR) == DRIVER_SETUP_OK) {
		/* "\nTouch device found\n" */
		dprintk("\n[TSP] Touch device found\n");
    } else {
		/* "\nTouch device NOT found\n" */
		dprintk("\n[TSP][ERROR] Touch device NOT found\n");
		return ;
	}
#else
	if (init_touch_driver(QT602240_I2C_ADDR, (void *) &message_handler) == DRIVER_SETUP_OK) {
		/* "\nTouch device found\n" */
		dprintk("\n[TSP] Touch device found\n");
	} else {
		/* "\nTouch device NOT found\n" */
		dprintk("\n[TSP][ERROR] Touch device NOT found\n");
		return ;
	}
#endif
	/* Get and show the version information. */
	get_family_id(&family_id);
	get_variant_id(&variant);
	get_version(&tsp_version);
	get_build_number(&build);

	dprintk("Version:      0x%x\n\r", tsp_version);
	dprintk("Family:       0x%x\n\r", family_id);
	dprintk("Variant:      0x%x\n\r", variant);
	dprintk("Build number: 0x%x\n\r", build);
	dprintk("Matrix X size : %d\n\r", info_block->info_id.matrix_x_size);
	dprintk("Matrix Y size : %d\n\r", info_block->info_id.matrix_y_size);

	if (calculate_infoblock_crc(&crc) != CRC_CALCULATION_OK)
		dprintk("Calculating CRC failed, skipping check!\n\r");
	else {
		dprintk("Calculated CRC:\t");
		write_message_to_usart((uint8_t *) &crc, 4);
		dprintk("\n\r");
	}

	stored_crc = get_stored_infoblock_crc();
	if (stored_crc != crc)
		dprintk("Warning: info block CRC value doesn't match the calculated!\n\r");
	else {
		dprintk("Stored CRC:\t");
		write_message_to_usart((uint8_t *) &stored_crc, 4);
		dprintk("\n\r");
		dprintk("Info block CRC value OK.\n\n\r");
	}

	/* Test the report id to object type / instance mapping: get the maximum
	 * report id and print the report id map. */

	max_report_id = get_max_report_id();
	for (report_id = 0; report_id <= max_report_id; report_id++)
		object_type = report_id_to_type(report_id, &instance);

#ifdef OPTION_WRITE_CONFIG
	if (info_block->info_id.family_id == 0x80) {
		qt_Power_Config_Init();
		qt_Acquisition_Config_Init();
		qt_Multitouchscreen_Init();
		qt_KeyArray_Init();
		qt_ComcConfig_Init();
		qt_Gpio_Pwm_Init();
		qt_Grip_Face_Suppression_Config_Init();
		qt_Noise_Suppression_Config_Init();
		qt_Proximity_Config_Init();
		qt_One_Touch_Gesture_Config_Init();
		qt_Selftest_Init();
		qt_Two_touch_Gesture_Config_Init();
		qt_CTE_Config_Init();
	} else if (info_block->info_id.family_id == 0x81) {
		qt_Power_Config_Init();
		qt_Acquisition_Config_Init();
		qt_Multitouchscreen_Init();
		qt_KeyArray_Init();
		qt_ComcConfig_Init();
		qt_Proximity_Config_Init();
		qt_Selftest_Init();
		qt_Grip_Suppression_Config_Init();
		qt_Touch_Suppression_Config_Init();
		qt_CTE_Config_Init();
		qt_Stylus_Init();
		qt_Noise_Suppression_Config_Init();
    } else
		dprintk("Not support touch chip. Support MXT224 & MXT224E only\n");

	/* Backup settings to NVM. */
	if (backup_config() != WRITE_MEM_OK) {
		dprintk("Failed to backup, exiting...\n\r");
		return;
	}
#else
	dprintk("Chip setup sequence was bypassed!\n\r");
#endif /* OPTION_WRITE_CONFIG */

	/* reset the touch IC. */
	if (reset_chip() != WRITE_MEM_OK) {
		dprintk("Failed to reset, exiting...\n\r");
		return;
	} else {
		msleep(60);
		dprintk("Chip reset OK!\n\r");
	}
}


/*------------------------------ Sub functions -----------------------------------*/
/*!
	\brief Initializes touch driver.

	This function initializes the touch driver: tries to connect to given 
	address, sets the message handler pointer, reads the info block and object
	table, sets the message processor address etc.

	@param I2C_address is the address where to connect.
	@param (*handler) is a pointer to message handler function.
	@return DRIVER_SETUP_OK if init successful, DRIVER_SETUP_FAILED otherwise.
*/
uint8_t init_touch_driver(uint8_t I2C_address)
{
	int i, current_report_id = 0;
	uint8_t tmp;
	uint16_t current_address;
	uint16_t crc_address;
	object_t *object_table;
	info_id_t *id;
	uint8_t status;

	dprintk("[QT] %s start\n",__func__);
	/* Set the message handler function pointer. */ 
	//application_message_handler = handler;

#ifdef QT_FIRMUP_ENABLE
	QT_i2c_address = I2C_address;
#endif
	/* Try to connect. */
	if (init_I2C(I2C_address) != CONNECT_OK)
		return(DRIVER_SETUP_INCOMPLETE);

#ifdef FEATUE_QT_INFOBLOCK_STATIC    // by donghoon.kwak
	/* Read the info block data. */
	id = &info_block->info_id;

	if (read_id_block(id) != 1)
		return(DRIVER_SETUP_INCOMPLETE);

	/* Read object table. */
	info_block->objects = info_object_ptr;
	object_table = info_block->objects;

	/* Reading the whole object table block to memory directly doesn't work cause sizeof object_t
	isn't necessarily the same on every compiler/platform due to alignment issues. Endianness
	can also cause trouble. */
	current_address = OBJECT_TABLE_START_ADDRESS;

	for (i = 0; i < id->num_declared_objects; i++) {
	    status = read_mem(current_address, 1, &(object_table[i]).object_type);
	    if (status != READ_MEM_OK) {
			dprintk("[TSP][ERROR] 4\n");
			return(DRIVER_SETUP_INCOMPLETE);
		}
		current_address++;

		status = read_U16(current_address, &object_table[i].i2c_address);
		if (status != READ_MEM_OK) {
			dprintk("[TSP][ERROR] 5\n");
			return(DRIVER_SETUP_INCOMPLETE);
		}
		current_address += 2;

		status = read_mem(current_address, 1, (U8*)&object_table[i].size);
		if (status != READ_MEM_OK) {
			dprintk("[TSP][ERROR] 6\n");
			return(DRIVER_SETUP_INCOMPLETE);
		}
		current_address++;

		status = read_mem(current_address, 1, &object_table[i].instances);
		if (status != READ_MEM_OK) {
			dprintk("[TSP][ERROR] 7\n");
			return(DRIVER_SETUP_INCOMPLETE);
		}
		current_address++;

		status = read_mem(current_address, 1, &object_table[i].num_report_ids);
		if (status != READ_MEM_OK) {
			dprintk("[TSP][ERROR] 8\n");
			return(DRIVER_SETUP_INCOMPLETE);
		}
		current_address++;

		max_report_id += object_table[i].num_report_ids;

		/* Find out the maximum message length. */
		if (object_table[i].object_type == GEN_MESSAGEPROCESSOR_T5) {
			max_message_length = object_table[i].size + 1;
        }
	}

	/* Check that message processor was found. */
	if (max_message_length == 0) {
		dprintk("[TSP][ERROR] 9\n");
		return(DRIVER_SETUP_INCOMPLETE);
	}

	/* Read CRC. */
	crc_address = OBJECT_TABLE_START_ADDRESS +
		id->num_declared_objects * OBJECT_TABLE_ELEMENT_SIZE;

	status = read_mem(crc_address, 1u, &tmp);
	if (status != READ_MEM_OK) {
		dprintk("[TSP][ERROR] 11\n");
		return(DRIVER_SETUP_INCOMPLETE);
	}
	info_block->CRC = tmp;

	status = read_mem(crc_address + 1u, 1u, &tmp);
	if (status != READ_MEM_OK) {
		dprintk("[TSP][ERROR] 12\n");
		return(DRIVER_SETUP_INCOMPLETE);
	}
	info_block->CRC |= (tmp << 8u);

	status = read_mem(crc_address + 2, 1, &info_block->CRC_hi);
	if (status != READ_MEM_OK) {
		dprintk("[TSP][ERROR] 13\n");
		return(DRIVER_SETUP_INCOMPLETE);
	}

	/* Store message processor address, it is needed often on message reads. */
	message_processor_address = get_object_address(GEN_MESSAGEPROCESSOR_T5, 0);

	if (message_processor_address == 0) {
		dprintk("[TSP][ERROR] 14 !!\n");
		return(DRIVER_SETUP_INCOMPLETE);
	}

	/* Store command processor address. */
	command_processor_address = get_object_address(GEN_COMMANDPROCESSOR_T6, 0);
	if (command_processor_address == 0) {
		dprintk("[TSP][ERROR] 15\n");
		return(DRIVER_SETUP_INCOMPLETE);
	}

#else
	/* Read the info block data. */
	id = (info_id_t *) kmalloc(sizeof(info_id_t), GFP_KERNEL | GFP_ATOMIC);
	if (id == NULL)
		return(DRIVER_SETUP_INCOMPLETE);

	if (read_id_block(id) != 1) {
		dprintk("[TSP][ERROR] 2\n");
		return(DRIVER_SETUP_INCOMPLETE);
	}

	/* Read object table. */
	object_table = (object_t *) kmalloc(id->num_declared_objects * sizeof(object_t), GFP_KERNEL | GFP_ATOMIC);
	if (object_table == NULL) {
		dprintk("[TSP][ERROR] 3\n");
		return(DRIVER_SETUP_INCOMPLETE);
	}


	/* Reading the whole object table block to memory directly doesn't work cause sizeof object_t
	isn't necessarily the same on every compiler/platform due to alignment issues. Endianness
	can also cause trouble. */
	current_address = OBJECT_TABLE_START_ADDRESS;

	for (i = 0; i < id->num_declared_objects; i++) {
		status = read_mem(current_address, 1, &(object_table[i]).object_type);
		if (status != READ_MEM_OK) {
			dprintk("[TSP][ERROR] 4\n");
			return(DRIVER_SETUP_INCOMPLETE);
		}
		current_address++;

		status = read_U16(current_address, &object_table[i].i2c_address);
		if (status != READ_MEM_OK) {
			dprintk("[TSP][ERROR] 5\n");
			return(DRIVER_SETUP_INCOMPLETE);
		}
		current_address += 2;

		status = read_mem(current_address, 1, (U8*)&object_table[i].size);
		if (status != READ_MEM_OK) {
			dprintk("[TSP][ERROR] 6\n");
			return(DRIVER_SETUP_INCOMPLETE);
		}
		current_address++;

		status = read_mem(current_address, 1, &object_table[i].instances);
		if (status != READ_MEM_OK) {
			dprintk("[TSP][ERROR] 7\n");
			return(DRIVER_SETUP_INCOMPLETE);
		}
		current_address++;

		status = read_mem(current_address, 1, &object_table[i].num_report_ids);
		if (status != READ_MEM_OK) {
			dprintk("[TSP][ERROR] 8\n");
			return(DRIVER_SETUP_INCOMPLETE);
		}
		current_address++;

		max_report_id += object_table[i].num_report_ids;

		/* Find out the maximum message length. */
		if (object_table[i].object_type == GEN_MESSAGEPROCESSOR_T5)
			max_message_length = object_table[i].size + 1;
	}

	/* Check that message processor was found. */
	if (max_message_length == 0) {
		dprintk("[TSP][ERROR] 9\n");
		return(DRIVER_SETUP_INCOMPLETE);
	}

	/* Read CRC. */
	CRC = (uint32_t *) kmalloc(sizeof(info_id_t), GFP_KERNEL | GFP_ATOMIC);
	if (CRC == NULL) {
		dprintk("[TSP][ERROR] 10\n");
		return(DRIVER_SETUP_INCOMPLETE);
	}

	info_block = kmalloc(sizeof(info_block_t), GFP_KERNEL | GFP_ATOMIC);
	if (info_block == NULL) {
		dprintk("err\n");
		return(DRIVER_SETUP_INCOMPLETE);
	}

	info_block->info_id = *id;

	info_block->objects = object_table;

	crc_address = OBJECT_TABLE_START_ADDRESS + 
		id->num_declared_objects * OBJECT_TABLE_ELEMENT_SIZE;

	status = read_mem(crc_address, 1u, &tmp);
	if (status != READ_MEM_OK) {
		dprintk("[TSP][ERROR] 11\n");
		return(DRIVER_SETUP_INCOMPLETE);
	}
	info_block->CRC = tmp;

	status = read_mem(crc_address + 1u, 1u, &tmp);
	if (status != READ_MEM_OK) {
		dprintk("[TSP][ERROR] 12\n");
		return(DRIVER_SETUP_INCOMPLETE);
	}
	info_block->CRC |= (tmp << 8u);

	status = read_mem(crc_address + 2, 1, &info_block->CRC_hi);
	if (status != READ_MEM_OK) {
		dprintk("[TSP][ERROR] 13\n");
		return(DRIVER_SETUP_INCOMPLETE);
	}

	/* Store message processor address, it is needed often on message reads. */
	message_processor_address = get_object_address(GEN_MESSAGEPROCESSOR_T5, 0);

	if (message_processor_address == 0) {
		dprintk("[TSP][ERROR] 14 !!\n");
		return(DRIVER_SETUP_INCOMPLETE);
	}

	/* Store command processor address. */
	command_processor_address = get_object_address(GEN_COMMANDPROCESSOR_T6, 0);
	if (command_processor_address == 0) {
		dprintk("[TSP][ERROR] 15\n");
		return(DRIVER_SETUP_INCOMPLETE);
	}

	quantum_msg = kmalloc(max_message_length, GFP_KERNEL | GFP_ATOMIC);
	if (quantum_msg == NULL) {
		dprintk("[TSP][ERROR] 16\n");
		return(DRIVER_SETUP_INCOMPLETE);
	}

	/* Allocate memory for report id map now that the number of report id's 
	is known. */

	report_id_map = kmalloc(sizeof(report_id_map_t) * max_report_id, GFP_KERNEL | GFP_ATOMIC);
	if (report_id_map == NULL) {
		dprintk("[TSP][ERROR] 17\n");
		return(DRIVER_SETUP_INCOMPLETE);
	}

#endif
	/* Report ID 0 is reserved, so start from 1. */
	report_id_map[0].instance = 0;
	report_id_map[0].object_type = 0;
	current_report_id = 1;

	for (i = 0; i < id->num_declared_objects; i++) {
		if (object_table[i].num_report_ids != 0) {
			int instance;
			for (instance = 0; instance <= object_table[i].instances; instance++) {
				int start_report_id = current_report_id;
				for (; current_report_id < (start_report_id + object_table[i].num_report_ids); current_report_id++) {
					report_id_map[current_report_id].instance = instance;
					report_id_map[current_report_id].object_type = object_table[i].object_type;
				}
			}
		}
	}
	driver_setup = DRIVER_SETUP_OK;

	/* Initialize the pin connected to touch ic pin CHANGELINE to catch the
	falling edge of signal on that line. */

	return(DRIVER_SETUP_OK);
}

void read_all_register(void)
{
	U16 addr = 0;
	U8 msg;
	U16 calc_addr = 0;

	for (addr = 0 ; addr < 1273 ; addr++) {
		calc_addr = addr;

		if (read_mem(addr, 1, &msg) == READ_MEM_OK) {
			dprintk("(0x%2x)", msg);
			if ((addr+1) % 10 == 0) {
				dprintk("\n");
				dprintk("%2d : ", addr+1);
			}
		} else
			dprintk("\n\n[TSP][ERROR] %s() read fail !! \n", __FUNCTION__);
	}
}

#if DEBUG_PRESS
void print_msg(void)
{
	dprintk("[TSP] msg = ");
	if ((quantum_msg[1] & 0x80) == 0x80 )
		dprintk("1");
	else
		dprintk("0");
	if ((quantum_msg[1] & 0x40) == 0x40 )
		dprintk("1");
	else
		dprintk("0");
	if ((quantum_msg[1] & 0x20) == 0x20 )
		dprintk("1");
	else
		dprintk("0");
	if ((quantum_msg[1] & 0x10) == 0x10 )
		dprintk("1");
	else
		dprintk("0");
	if ((quantum_msg[1] & 0x08) == 0x08 )
		dprintk("1");
	else
		dprintk("0");
	if ((quantum_msg[1] & 0x04) == 0x04 )
		dprintk("1");
	else
		dprintk("0");
	if ((quantum_msg[1] & 0x02) == 0x02 )
		dprintk("1");
	else
		dprintk("0");
	if ((quantum_msg[1] & 0x01) == 0x01 )
		dprintk("1");
	else
		dprintk("0");
	dprintk("\n");
}
#endif

static unsigned int qt_time_point=0;
static unsigned int qt_time_diff=0;
static unsigned int qt_timer_state =0;
static int check_abs_time(void)
{
	if (!qt_time_point)
		return 0;

	qt_time_diff = jiffies_to_msecs(jiffies) - qt_time_point;
	if (qt_time_diff > 0)
		return 1;
	else
		return 0;
}

//20100217 julia
//hugh 0312 void check_chip_calibration(void)
void check_chip_calibration(unsigned char one_touch_input_flag)
{
	uint8_t data_buffer[100] = { 0 };
	uint8_t try_ctr = 0;
	uint8_t data_byte = 0xF3; /* dianostic command to get touch flags */
	uint16_t diag_address;
	uint8_t tch_ch = 0, atch_ch = 0;
	uint8_t check_mask;
	uint8_t i;
	uint8_t j;
	uint8_t x_line_limit = 16;

	/* we have had the first touchscreen or face suppression message 
	 * after a calibration - check the sensor state and try to confirm if
	 * cal was good or bad */

	/* get touch flags from the chip using the diagnostic object */
	/* write command to command processor to get touch flags - 0xF3 Command required to do this */
	write_mem(command_processor_address + DIAGNOSTIC_OFFSET, 1, &data_byte);
	/* get the address of the diagnostic object so we can get the data we need */
	diag_address = get_object_address(DEBUG_DIAGNOSTIC_T37,0);

//	msleep(10);
	msleep(5);

	/* read touch flags from the diagnostic object - clear buffer so the while loop can run first time */
	memset(data_buffer, 0xFF, sizeof(data_buffer));

	/* wait for diagnostic object to update */
	while (!((data_buffer[0] == 0xF3) && (data_buffer[1] == 0x00))) {
		/* wait for data to be valid  */
		if (try_ctr > 10) { //0318 hugh 100-> 10
			/* Failed! */
			dprintk("[TSP] Diagnostic Data did not update!!\n");
			qt_timer_state = 0;//0430 hugh
			break;
		}
		msleep(2); //0318 hugh  3-> 2
		try_ctr++; /* timeout counter */
		read_mem(diag_address, 2, data_buffer);
	}

	/* data is ready - read the detection flags */
	read_mem(diag_address, 82, data_buffer);

	/* data array is 20 x 16 bits for each set of flags, 2 byte header, 40 bytes for touch flags 40 bytes for antitouch flags*/

	/* count up the channels/bits if we recived the data properly */
	if ((data_buffer[0] == 0xF3) && (data_buffer[1] == 0x00)) {

		/* mode 0 : 16 x line, mode 1 : 17 etc etc upto mode 4.*/
		if (info_block->info_id.family_id == 0x80)
			x_line_limit = 16 + cte_config.mode;
		else if (info_block->info_id.family_id == 0x81)
			x_line_limit = 16 + cte_config_e.mode;

		if (x_line_limit > 20) {
			/* hard limit at 20 so we don't over-index the array */
			x_line_limit = 20;
		}

		/* double the limit as the array is in bytes not words */
		x_line_limit = x_line_limit << 1;

		/* count the channels and print the flags to the log */
		for (i = 0; i < x_line_limit; i += 2) { /* check X lines - data is in words so increment 2 at a time */
			/* print the flags to the log - only really needed for debugging */
			/* dprintk("[TSP] Detect Flags X%d, %x%x, %x%x \n",
				i>>1,data_buffer[3+i],data_buffer[2+i],data_buffer[43+i],data_buffer[42+i]); */

			/* count how many bits set for this row */
			for (j = 0; j < 8; j++) {
				/* create a bit mask to check against */
				check_mask = 1 << j;

				/* check detect flags */
				if (data_buffer[2+i] & check_mask)
					tch_ch++;

				if (data_buffer[3+i] & check_mask)
					tch_ch++;
    
				/* check anti-detect flags */
				if (data_buffer[42+i] & check_mask)
					atch_ch++;

				if (data_buffer[43+i] & check_mask)
					atch_ch++;
			}
		}

		/* print how many channels we counted */
		printk(KERN_DEBUG"[TSP] Flags Counted channels: t:%d a:%d \n", tch_ch, atch_ch);

		/* send page up command so we can detect when data updates next time,
		* page byte will sit at 1 until we next send F3 command */
		data_byte = 0x01; //QT_PAGE_UP
		write_mem(command_processor_address + DIAGNOSTIC_OFFSET, 1, &data_byte);

		/* process counters and decide if we must re-calibrate or if cal was good */      
		if ((tch_ch > 0) && (atch_ch == 0)) {  //jwlee change.
			/* cal was good - don't need to check any more */
			//hugh 0312
			if (!check_abs_time())
				qt_time_diff=501;
			else
				dprintk("[TSP] CURRENT time diff = %d, qt_timer_state = %d\n",
					qt_time_diff, qt_timer_state);

			if (qt_timer_state == 1) {
				if (qt_time_diff > 500) { //jun change 11.5.3
					printk(KERN_DEBUG"[TSP] calibration was good\n");
					cal_check_flag = 0;
					qt_timer_state = 0;
					qt_time_point = jiffies_to_msecs(jiffies);

					if (info_block->info_id.family_id == 0x80) {
						dprintk("[TSP] reset acq atchcalst=%d, atchcalsthr=%d\n",
							acquisition_config.atchcalst, acquisition_config.atchcalsthr );
						/* Write normal acquisition config back to the chip. */
						if (write_acquisition_config(acquisition_config) != CFG_WRITE_OK) {
							/* "Acquisition config write failed!\n" */
							printk(KERN_DEBUG "\n[TSP][ERROR] line : %d\n", __LINE__);
							// MUST be fixed
						}
					} else if (info_block->info_id.family_id == 0x81) {
						dprintk("[TSP] reset acq atchcalst=%d, atchcalsthr=%d\n",
							acquisition_config_e.atchcalst, acquisition_config_e.atchcalsthr );
						/* Write normal acquisition config back to the chip. */
						if (write_acquisition_config_e(acquisition_config_e) != CFG_WRITE_OK) {
							/* "Acquisition config write failed!\n" */
							printk(KERN_DEBUG "\n[TSP][ERROR] line : %d\n", __LINE__);
							// MUST be fixed
						}
					}
				} else
					cal_check_flag = 1;
			} else {
				qt_timer_state = 1;
				qt_time_point = jiffies_to_msecs(jiffies);
				cal_check_flag = 1;
			}
		} else if (atch_ch >= 7) {		//jwlee add 0325
			/* cal was bad - must recalibrate and check afterwards */
			printk(KERN_DEBUG"[TSP] calibration was bad\n");
			calibrate_chip();
			qt_timer_state = 0;
			qt_time_point = jiffies_to_msecs(jiffies);
		} else {
			/* we cannot confirm if good or bad - we must wait for next touch  message to confirm */
			dprintk("[TSP] calibration was not decided yet\n");
			cal_check_flag = 1;
			/* Reset the 100ms timer */
			qt_timer_state = 0;//0430 hugh 1 --> 0
			qt_time_point = jiffies_to_msecs(jiffies);
		}
	}
}

unsigned int touch_state_val = 0;
EXPORT_SYMBOL(touch_state_val);

#if USE_TS_TA_DETECT_CHANGE_REG 
static bool touchscreen_power_state_on = 0;
static bool touch_TA_detect = 0;

static int set_tsp_threshhold(bool state)
{
	uint16_t object_address;
	uint8_t *tmp;
	uint8_t status;
#if MXT224E_TA_CONFIG_MODE
	uint8_t data;
#endif
	int i;

	if (info_block->info_id.family_id == 0x80) {
		object_address = get_object_address(TOUCH_MULTITOUCHSCREEN_T9, 0);
		if (object_address == 0) {
			dprintk("\n[TSP][ERROR] TOUCH_MULTITOUCHSCREEN_T9 object_address : %d\n", __LINE__);
			return -1;
		}
		tmp = &touchscreen_config.tchthr;
		status = write_mem(object_address+7, 1, tmp);

		if (status == WRITE_MEM_FAILED) {
			dprintk("\n[TSP][ERROR] TOUCH_MULTITOUCHSCREEN_T9 write_mem : %d\n", __LINE__);
		}

		object_address = get_object_address(PROCG_NOISESUPPRESSION_T22, 0);
		if (object_address == 0) {
			dprintk("\n[TSP][ERROR] PROCG_NOISESUPPRESSION_T22 object_address : %d\n", __LINE__);
			return -1;
		}
		tmp = &noise_suppression_config.noisethr ;
		status = write_mem(object_address+8, 1, tmp);

		if (status == WRITE_MEM_FAILED) {
			dprintk("\n[TSP][ERROR] PROCG_NOISESUPPRESSION_T22 write_mem : %d\n", __LINE__);
		}

		if (state == 0) {
			if (noise_freq_table.fherr_setting) {
				object_address = get_object_address(GEN_POWERCONFIG_T7, 0);
				tmp = &power_config.idleacqint;
				write_mem(object_address, 1, tmp);
				object_address = get_object_address(TOUCH_MULTITOUCHSCREEN_T9, 0);
				tmp = &touchscreen_config.blen;
				write_mem(object_address+6, 1, tmp);
				object_address = get_object_address(PROCG_NOISESUPPRESSION_T22, 0);
				for (i = 0; i < 5; i++) {
					tmp = &noise_suppression_config.freq[i];
					write_mem(object_address+11+i, 1, tmp);
				}
				noise_freq_table.fherr_count = 0;
				noise_freq_table.fherr_setting = false;
			}
		}
	} else if (info_block->info_id.family_id == 0x81) {
		object_address = get_object_address(PROCG_NOISESUPPRESSION_T48, 0);
		if (object_address == OBJECT_NOT_FOUND) {
			dprintk("\n[TSP][ERROR] PROCG_NOISESUPPRESSION_T48 object_address : %d\n", __LINE__);
			return -1;
		}
#if MXT224E_TA_CONFIG_MODE
		if (state) {
			if (write_noisesuppression_config_e(0, noise_suppression_config_e_TA) != CFG_WRITE_OK)
					dprintk("[TSP] Configuration Fail!!! , Line %d \n\r", __LINE__);
			data = noise_suppression_config_e_TA.calcfg | 0x20;
		} else  {
			if (write_noisesuppression_config_e(0, noise_suppression_config_e) != CFG_WRITE_OK)
				dprintk("[TSP] Configuration Fail!!! , Line %d \n\r", __LINE__);
			data = noise_suppression_config_e.calcfg | 0x20;
		}
		status = write_mem(object_address+2, 1, &data);
		if (status == WRITE_MEM_FAILED)
			printk(KERN_ERR"[TSP][ERROR] mxt_noise_suppression_config write_mem error : %d\n", __LINE__);
#else
		tmp = &noise_suppression_config_e.calcfg;
		status = write_mem(object_address+2, 1, tmp);
#endif
	}

	return 1;
}

int set_tsp_for_ta_detect(int state)
{
	int ret = 1;

	if (info_block->info_id.family_id == 0x80) {
		if (state) {
			touchscreen_config.tchthr = 70;
			noise_suppression_config.noisethr = 20;
		} else {
			touchscreen_config.tchthr = 40;
			noise_suppression_config.noisethr = 30;
		}
	} else if (info_block->info_id.family_id == 0x81) {
#if (MXT224E_TA_CONFIG_MODE == 0)
		if (state)
			noise_suppression_config_e.calcfg = 112;
		else
			noise_suppression_config_e.calcfg = 80;
#endif
	}

	if (state) {
		printk(KERN_DEBUG "[TSP] TA Detect!!!\n");
		touch_TA_detect = 1;
	} else {
		printk(KERN_DEBUG "[TSP] TA NON-Detect!!!\n");
		touch_TA_detect = 0;
	}

	if (touchscreen_power_state_on) {
		disable_irq(qt602240->client->irq);
		ret = set_tsp_threshhold(touch_TA_detect);
		enable_irq(qt602240->client->irq);
	}

	return ret;
}
EXPORT_SYMBOL(set_tsp_for_ta_detect);

static void freq_hop_err_setting(int state)
{
	int i = 0;
	uint16_t object_address;
	uint8_t data;

	object_address = get_object_address(GEN_POWERCONFIG_T7, 0);
	data = power_config.idleacqint = 255;
	write_mem(object_address, 1, &data);

	object_address = get_object_address(TOUCH_MULTITOUCHSCREEN_T9, 0);
	data = noise_freq_table.t9_blen_for_fherr;
	write_mem(object_address+6, 1, &data);

	data = noise_freq_table.t9_thr_for_fherr;
	write_mem(object_address+7, 1, &data);

	object_address = get_object_address(PROCG_NOISESUPPRESSION_T22, 0);
	data = noise_freq_table.t22_noisethr_for_fherr;
	write_mem(object_address+8, 1, &data);

	for (i = 0; i< 5; i++) {
		if (state == 0) {
			data = noise_freq_table.freq_for_fherr0[i];
			write_mem(object_address+11+i, 1, &data);
		} else if (state == 1) {
			data = noise_freq_table.freq_for_fherr1[i];
			write_mem(object_address+11+i, 1, &data);
		} else if (state == 2) {
			data = noise_freq_table.freq_for_fherr2[i];
			write_mem(object_address+11+i, 1, &data);
		} else if (state == 3) {
			data = noise_freq_table.freq_for_fherr3[i];
			write_mem(object_address+11+i, 1, &data);
		}
	}
	noise_freq_table.fherr_setting = true;
}
#endif

void TSP_forced_release(void)
{
	int i;
	int temp_value = 0;

	if (qt_initial_ok == 0)
		return;

	for (i = 0; i < MAX_USING_FINGER_NUM; i++) {
		if (fingerInfo[i].pressure == -1)
			continue;

		fingerInfo[i].pressure = 0;
		input_report_abs(qt602240->input_dev, ABS_MT_POSITION_X, fingerInfo[i].x);
		input_report_abs(qt602240->input_dev, ABS_MT_POSITION_Y, fingerInfo[i].y);
		input_report_abs(qt602240->input_dev, ABS_MT_TOUCH_MAJOR, fingerInfo[i].pressure);
		input_report_abs(qt602240->input_dev, ABS_MT_WIDTH_MAJOR, fingerInfo[i].size);   
		input_report_abs(qt602240->input_dev, ABS_MT_TRACKING_ID, i);
		input_report_abs(qt602240->input_dev, ABS_PRESSURE, fingerInfo[i].pressure);
		input_report_key(qt602240->input_dev, BTN_TOUCH, 0);
		input_mt_sync(qt602240->input_dev);

		fingerInfo[i].pressure = -1;
		temp_value++;

#if DEBUG_LOG_ENABLE
		printk(KERN_DEBUG"[TSP] forced released. [%d] : %d, %d\n",
			i, fingerInfo[i].x, fingerInfo[i].y);
#else
		printk(KERN_DEBUG"[TSP] forced released\n");
#endif
    }

	if (temp_value > 0)
		input_sync(qt602240->input_dev);

	touch_state_val = 0;
#if TOUCH_CPU_FREQ
	if (touch_cpu_freq_flag == 1) {
		touch_cpu_freq_flag = 0;
		s5pv210_unlock_dvfs_high_level(DVFS_LOCK_TOKEN_7);
	}
#endif
}
EXPORT_SYMBOL(TSP_forced_release);

#if DRIVER_FILTER
static void equalize_coordinate(bool detect, u8 id, u16 *px, u16 *py)
{
	static int tcount[MAX_USING_FINGER_NUM] = {0, };
	static u16 pre_x[MAX_USING_FINGER_NUM][4] = {{0}, };
	static u16 pre_y[MAX_USING_FINGER_NUM][4] = {{0}, };
	int coff[4] = {0,};
	int distance = 0;

	if (detect)
		tcount[id] = 0;

	pre_x[id][tcount[id]%4] = *px;
	pre_y[id][tcount[id]%4] = *py;

	if (tcount[id] > 3) {
		distance = abs(pre_x[id][(tcount[id]-1)%4] - *px) + abs(pre_y[id][(tcount[id]-1)%4] - *py);

		coff[0] = (u8)(4 + distance/5);
		if (coff[0] < 8) {
			coff[0] = max(4, coff[0]);
			coff[1] = min((10 - coff[0]), (coff[0]>>1)+1);
			coff[2] = min((10 - coff[0] - coff[1]), (coff[1]>>1)+1);
			coff[3] = 10 - coff[0] - coff[1] - coff[2];
			/* dprintk("[TSP][equalize] %d, %d, %d, %d", coff[0], coff[1], coff[2], coff[3]); */

			*px = (u16)((*px*(coff[0]) + pre_x[id][(tcount[id]-1)%4]*(coff[1])
				+ pre_x[id][(tcount[id]-2)%4]*(coff[2]) + pre_x[id][(tcount[id]-3)%4]*(coff[3]))/10);
			*py = (u16)((*py*(coff[0]) + pre_y[id][(tcount[id]-1)%4]*(coff[1])
				+ pre_y[id][(tcount[id]-2)%4]*(coff[2]) + pre_y[id][(tcount[id]-3)%4]*(coff[3]))/10);
		} else {
			*px = (u16)((*px*4 + pre_x[id][(tcount[id]-1)%4])/5);
			*py = (u16)((*py*4 + pre_y[id][(tcount[id]-1)%4])/5);
		}
	}
	tcount[id]++;
}
#endif

void  get_message(void)
{
	unsigned long x, y;
	unsigned int press = 3;
	uint8_t ret_val = MESSAGE_READ_FAILED;
	int i, size = 2;    
	uint8_t touch_message_flag = 0;
	uint8_t one_touch_input_flag = 0;//hugh 0312
	uint16_t object_address;
	uint8_t data;
	int error = 0;

#ifdef _SUPPORT_MULTITOUCH_
	static int nPrevID = -1;
	uint8_t id = 0;
	int bChangeUpDn = 0;
#endif

	if (driver_setup == DRIVER_SETUP_INCOMPLETE) {
		printk(KERN_ERR"[TSP] driver_setup is not completed.\n");
		return;
	}

#ifdef _SUPPORT_TOUCH_AMPLITUDE_
	if (read_mem(message_processor_address, 7, quantum_msg) == READ_MEM_FAILED) {
#else
	if (read_mem(message_processor_address, 6, quantum_msg) == READ_MEM_FAILED) {
#endif
		printk(KERN_ERR"[TSP] read_mem is failed -- check your TSP chip!!\n");
		return;
	}

	/* Call the main application to handle the message. */
	//20102017 julia
	if ((quantum_msg[0] < 2) || (quantum_msg[0] >= 12)) {
		printk(KERN_DEBUG "[TSP] msg id = %x %x %x %x %x %x %x %x %x\n",\
			quantum_msg[0], quantum_msg[1], quantum_msg[2], quantum_msg[3], quantum_msg[4],\
			quantum_msg[5], quantum_msg[6], quantum_msg[7], quantum_msg[8]);

		if (quantum_msg[0] == 1) {
			if ((quantum_msg[1]) == 0) {
				if (cal_check_flag == 2)
					cal_check_flag = 1;
			}
			if ((quantum_msg[1]&0x04) == 0x04)
				printk(KERN_ERR"[TSP] COMSERR error\n");
			if ((quantum_msg[1]&0x08) == 0x08) {
				printk(KERN_ERR"[TSP] CFGERR error\n");
				error = 1;
			}
			if ((quantum_msg[1]&0x10) == 0x10) {
				dprintk("[TSP] CAL - chip calibrating\n");
				if (cal_check_flag == 0)
					cal_check_flag = 2;
			}
			if ((quantum_msg[1]&0x20) == 0x20) {
				printk(KERN_ERR"[TSP] SIGERR error\n");
				error = 1;
			}
			if ((quantum_msg[1]&0x40) == 0x40) {
				printk(KERN_ERR"[TSP] OFL error\n");
				error = 1;
				ofl_error_count++;
			}
			if ((quantum_msg[1]&0x80) == 0x80)
				dprintk("[TSP] RESET - chip reset excuted\n");

			if (error == 1) {
				/* force release */
				TSP_forced_release();
				/* excute hardware reset */
				if (reset_chip() == WRITE_MEM_FAILED) {
					printk(KERN_ERR"[TSP] reset_chip fail. hardware reset.\n");
					TSP_Restart();
					ofl_error_count = 0;
				}
				if (ofl_error_count == 5) {
					printk(KERN_ERR"[TSP] ofl_error 5times. hardware reset.\n");
					TSP_Restart();
					ofl_error_count = 0;
				}
				msleep(100);
				set_tsp_threshhold(touch_TA_detect);
				return;
			}
		} else if (quantum_msg[0] == 14) {  /* Palm Touch */
			if ((quantum_msg[1]&0x01) == 0x01) {
				dprintk("[TSP] Palm Touch press\n");
				touch_message_flag = 1;
				one_touch_input_flag = 1; //hugh 0312
			} else {
				dprintk("[TSP] Palm Touch relese\n");
				TSP_forced_release();
			}
		} else if (quantum_msg[0] == 15) { /* noise suppression */
#if 0
			if ((quantum_msg[1]&0x01) == 0x01) {
				printk(KERN_DEBUG"[TSP] Freq changed\n");
			}
			if ((quantum_msg[1]&0x04) == 0x04) {
				printk(KERN_DEBUG"[TSP] high noise level\n");
			}
#endif
			if ((quantum_msg[1]&0x08) == 0x08) {
				if (touch_TA_detect) {
					noise_freq_table.fherr_count++;
					if (noise_freq_table.fherr_count == 12)
						noise_freq_table.fherr_count = 0;
					if ((noise_freq_table.fherr_count%3) == 0)
						printk(KERN_DEBUG"[TSP] freq changed. noise level too high. %d\n",
							noise_freq_table.fherr_count/3);
					freq_hop_err_setting(noise_freq_table.fherr_count/3);
				}
			}
		}

		if (info_block->info_id.family_id == 0x81) {	/* MXT-224E */
			if ( quantum_msg[0] == 18) {
				if (quantum_msg[4] == 5) {
					printk(KERN_ERR"[TSP] Median filter Error\n");
					if (write_noisesuppression_config_e(0, noise_suppression_config_e) != CFG_WRITE_OK)
						dprintk("[TSP] Configuration Fail!!! , Line %d \n\r", __LINE__);

					object_address = get_object_address(PROCG_NOISESUPPRESSION_T48, 0);
					if (object_address == OBJECT_NOT_FOUND)
						dprintk("\n[TSP][ERROR] PROCG_NOISESUPPRESSION_T48 object_address : %d\n", __LINE__);

					data = noise_suppression_config_e.calcfg;
					if (write_mem(object_address+2, 1, &data) == WRITE_MEM_FAILED)
						printk(KERN_ERR"[TSP][ERROR] mxt_noise_suppression_config write_mem error : %d\n", __LINE__);
					msleep(5);
					data = noise_suppression_config_e.calcfg | 0x20;
					if (write_mem(object_address+2, 1, &data) == WRITE_MEM_FAILED)
						printk(KERN_ERR"[TSP][ERROR] mxt_noise_suppression_config write_mem error : %d\n", __LINE__);
				}
			}
		}

		if ((quantum_msg[0] ==  14) || (quantum_msg[0] == 0) || (quantum_msg[0] == 0xFF)) {
			if (touch_message_flag && (cal_check_flag == 1)) {
				if (((info_block->info_id.family_id == 0x80) && (tsp_version >= 0x16)) ||
					(info_block->info_id.family_id == 0x81))
					check_chip_calibration(one_touch_input_flag);
			}
		}
		return ;
	} 
	ofl_error_count = 0;
#if TOUCH_CPU_FREQ
	if (touch_cpu_freq_flag == 0) {
		touch_cpu_freq_flag = 1;
		s5pv210_lock_dvfs_high_level(DVFS_LOCK_TOKEN_7, L1);
	}
#endif
#ifdef _SUPPORT_MULTITOUCH_
	id= quantum_msg[0] - 2;
#endif

	x = quantum_msg[2];
	x = x << 2;
	x = x | quantum_msg[4] >> 6;

	y = quantum_msg[3];
	y = y << 2;
	y = y | ((quantum_msg[4] & 0x0C)  >> 2);

	size = quantum_msg[5];
	/* 
	 * quantum_msg[1] & 0x80 : 10000000 -> DETECT 
	 * quantum_msg[1] & 0x40 : 01000000 -> PRESS
	 * quantum_msg[1] & 0x20 : 00100000 -> RELEASE
	 * quantum_msg[1] & 0x10 : 00010000 -> MOVE
	 * quantum_msg[1] & 0x08 : 00001000 -> VECTOR
	 * quantum_msg[1] & 0x04 : 00000100 -> AMP
	 * quantum_msg[1] & 0x02 : 00000010 -> SUPPRESS
	 */

#ifdef _SUPPORT_MULTITOUCH_
	if ((quantum_msg[1] & 0x20)) {
	/* Release */
		fingerInfo[id].size = size;
		fingerInfo[id].pressure = 0;
		bChangeUpDn = 1;
#if DEBUG_LOG_ENABLE /* #ifdef CONFIG_KERNEL_DEBUG_SEC */
		printk(KERN_DEBUG"[TSP] released. %d : %ld, %ld\n", id, x, y);
#else
		printk(KERN_DEBUG"[TSP] released\n");
#endif

	} else if ((quantum_msg[1] & 0x80) && (quantum_msg[1] & 0x40)) {
	/* Detect & Press */
		touch_message_flag = 1; //20100217 julia
		touch_state_val = 1;
		fingerInfo[id].size = size;
#ifdef _SUPPORT_TOUCH_AMPLITUDE_
		fingerInfo[id].pressure = quantum_msg[6];
#else
		fingerInfo[id].pressure = 40;
#endif
		fingerInfo[id].x = (int16_t)x;
		fingerInfo[id].y = (int16_t)y;
		bChangeUpDn = 1;
#if DRIVER_FILTER
		equalize_coordinate(1, id, &fingerInfo[id].x, &fingerInfo[id].y);
#endif
#if DEBUG_LOG_ENABLE /* #ifdef CONFIG_KERNEL_DEBUG_SEC */
			printk(KERN_DEBUG"[TSP] pressed. %d : %ld, %ld\n", id, x, y);
#else
			printk(KERN_DEBUG"[TSP] pressed\n");
#endif

	} else if ((quantum_msg[1] & 0x80) && (quantum_msg[1] & 0x10)) {
	/*Detect & Move */
		touch_message_flag = 1;
		fingerInfo[id].size = size;
#ifdef _SUPPORT_TOUCH_AMPLITUDE_
		fingerInfo[id].pressure = quantum_msg[6];
#endif
		fingerInfo[id].x = (int16_t)x;
		fingerInfo[id].y = (int16_t)y;
#if DRIVER_FILTER
		equalize_coordinate(0, id, &fingerInfo[id].x, &fingerInfo[id].y);
#endif

	} else {
		press = 3;
		printk(KERN_ERR"[TSP]Unknown state 0x%x 0x%x!\n ", quantum_msg[0], quantum_msg[1]);
		//print_msg();
		//dprintk("\n");
	}

	ret_val = MESSAGE_READ_OK;

	if ((nPrevID >= id) || bChangeUpDn) {
		for (i = 0; i < MAX_USING_FINGER_NUM; i++ ) {
			if (fingerInfo[i].pressure == -1 )
				continue;

			input_report_abs(qt602240->input_dev, ABS_MT_POSITION_X, fingerInfo[i].x);
			input_report_abs(qt602240->input_dev, ABS_MT_POSITION_Y, fingerInfo[i].y);
			input_report_abs(qt602240->input_dev, ABS_MT_TOUCH_MAJOR, fingerInfo[i].pressure);
			input_report_abs(qt602240->input_dev, ABS_MT_WIDTH_MAJOR, fingerInfo[i].size);
			input_report_abs(qt602240->input_dev, ABS_MT_TRACKING_ID, i); // i = Finger ID 
			input_report_abs(qt602240->input_dev, ABS_MT_TOUCH_MAJOR, fingerInfo[i].pressure);

			if (fingerInfo[i].pressure == 0) {
				input_report_key(qt602240->input_dev, BTN_TOUCH, 0);
				fingerInfo[i].pressure = -1;
			}
			else
				input_report_key(qt602240->input_dev, BTN_TOUCH, 1);

			input_mt_sync(qt602240->input_dev);

			//else if (fingerInfo[i].pressure > 0)
			//	one_touch_input_flag++;//hugh 0312
#ifdef CONFIG_MACH_AEGIS
			printk("x=%d,y=%d,z=%d,id=%d\n",fingerInfo[i].x,fingerInfo[i].y,fingerInfo[i].pressure,i);
#endif
		}
		input_sync(qt602240->input_dev);
		if (one_touch_input_flag == 0) {
			touch_state_val = 0;
#if TOUCH_CPU_FREQ
			if (touch_cpu_freq_flag == 1) {
				touch_cpu_freq_flag = 0;
				s5pv210_unlock_dvfs_high_level(DVFS_LOCK_TOKEN_7);
			}
#endif
		}
	}
	nPrevID = id;

#else // _SUPPORT_MULTITOUCH_
	if (((quantum_msg[1] & 0x80) == 0x80) && ((quantum_msg[1] & 0x40) == 0x40)) {    // case 11000000 -> DETECT & PRESS
		press = 1;
		btn_report = 1;
		print_msg();
	} else if (((quantum_msg[1] & 0x80) == 0x80) && ((quantum_msg[1] & 0x10) == 0x10)) {    // case 10010000 -> DETECT & MOVE
		press = 1;
		print_msg();
	} else if (((quantum_msg[1] & 0x20) == 0x20)) {  // case 00100000 -> RELEASE
		press = 0;
		print_msg();
		input_report_key(qt602240->input_dev, BTN_TOUCH, 0);
	} else {
		press = 3;
		dprintk("[TSP]Unknown state 0x%x 0x%x!\n ", quantum_msg[0], quantum_msg[1]);
		//print_msg();
		//dprintk("\n");
	}

	ret_val = MESSAGE_READ_OK;

	if (press == 1) {
		input_report_abs(qt602240->input_dev, ABS_X, x);
		input_report_abs(qt602240->input_dev, ABS_Y, y);
		if (btn_report == 1)
			input_report_key(qt602240->input_dev, BTN_TOUCH, 1);

		input_sync(qt602240->input_dev);
		amplitude = quantum_msg[6];
		printk("%s x=%3d, y=%3d, BTN=%d, size=%d, amp=%d\n",__FUNCTION__, x, y,press, size , amplitude);
	} else if (press == 0) {
	    input_sync(qt602240->input_dev);
	    amplitude = quantum_msg[6];
	    printk("%s x=%3d, y=%3d, BTN=%d, size=%d, amp=%d\n",__FUNCTION__, x, y,press, size , amplitude);
	}
#endif // _SUPPORT_MULTITOUCH_

	// 20100217 julia
	if (touch_message_flag && (cal_check_flag == 1)) {
		if (((info_block->info_id.family_id == 0x80) && (tsp_version >=0x16)) ||
			(info_block->info_id.family_id == 0x81))
			check_chip_calibration(one_touch_input_flag);
	}
	return ;
}



/*------------------------------ I2C Driver block -----------------------------------*/

#define I2C_M_WR 0 /* for i2c */
#define I2C_MAX_SEND_LENGTH     300
int qt602240_i2c_write(u16 reg, u8 *read_val, unsigned int len)
{
	struct i2c_msg wmsg;
	//unsigned char wbuf[3];
	unsigned char data[I2C_MAX_SEND_LENGTH];
	int ret, i;

	address_pointer = reg;

	if (len+2 > I2C_MAX_SEND_LENGTH) {
		dprintk("[TSP][ERROR] %s() data length error\n", __FUNCTION__);
		return -ENODEV;
	}

	wmsg.addr = qt602240->client->addr;
	wmsg.flags = I2C_M_WR;
	wmsg.len = len + 2;
	wmsg.buf = data;

	data[0] = reg & 0x00ff;
	data[1] = reg >> 8;

	for (i = 0; i < len; i++)
		data[i+2] = *(read_val+i);

	ret = i2c_transfer(qt602240->client->adapter, &wmsg, 1);
	return ret;
}

int boot_qt602240_i2c_write(u16 reg, u8 *read_val, unsigned int len)
{
	struct i2c_msg wmsg;
	unsigned char data[I2C_MAX_SEND_LENGTH];
	int ret, i;

	if (len+2 > I2C_MAX_SEND_LENGTH) {
		dprintk("[TSP][ERROR] %s() data length error\n", __FUNCTION__);
		return -ENODEV;
	}

	wmsg.addr = 0x24;
	wmsg.flags = I2C_M_WR;
	wmsg.len = len;
	wmsg.buf = data;

	for (i = 0; i < len; i++)
		data[i] = *(read_val+i);

	ret = i2c_transfer(qt602240->client->adapter, &wmsg, 1);
	return ret;
}


int qt602240_i2c_read(u16 reg,unsigned char *rbuf, int buf_size)
{
	static unsigned char first_read=1;
	struct i2c_msg rmsg;
	int ret;
	unsigned char data[2];

	rmsg.addr = qt602240->client->addr;

	if (first_read == 1) {
		first_read = 0;
		address_pointer = reg+1;
	}

	if ((address_pointer != reg) || (reg != message_processor_address)) {
		address_pointer = reg;

		rmsg.flags = I2C_M_WR;
		rmsg.len = 2;
		rmsg.buf = data;
		data[0] = reg & 0x00ff;
		data[1] = reg >> 8;
		ret = i2c_transfer(qt602240->client->adapter, &rmsg, 1);
	}

	rmsg.flags = I2C_M_RD;
	rmsg.len = buf_size;
	rmsg.buf = rbuf;
	ret = i2c_transfer(qt602240->client->adapter, &rmsg, 1);

	return ret;
}


/* ------------------------- ????????????? -----------------*/
/*!
 * \brief Initializes the I2C interface.
 *
 * @param I2C_address_arg the touch chip I2C address.
 *
 */
U8 init_I2C(U8 I2C_address_arg)
{
	dprintk("[QT] %s start\n",__func__);
	return(I2C_INIT_OK);
}


/*! \brief Enables pin change interrupts on falling edge. */
void enable_changeline_int(void)
{
}

/*! \brief Returns the changeline state. */
U8 read_changeline(void)
{
    return 0;
}

/*! \brief Maxtouch Memory read by I2C bus */
U8 read_mem(U16 start, U8 size, U8 *mem)
{
	int ret;

	memset(mem, 0xFF, size);
	ret = qt602240_i2c_read(start,mem,size);
	if (ret < 0) {
		dprintk("%s : i2c read failed\n",__func__);
		return(READ_MEM_FAILED);
	}

	return(READ_MEM_OK);
}

U8 boot_read_mem(U16 start, U8 size, U8 *mem)
{
	struct i2c_msg rmsg;
	int ret;

	rmsg.addr = 0x24;
	rmsg.flags = I2C_M_RD;
	rmsg.len = size;
	rmsg.buf = mem;
	ret = i2c_transfer(qt602240->client->adapter, &rmsg, 1);

	return ret;
}

U8 read_U16(U16 start, U16 *mem)
{
	U8 status;

	status = read_mem(start, 2, (U8 *)mem);

	return(status);
}

U8 write_mem(U16 start, U8 size, U8 *mem)
{
	int ret;

	ret = qt602240_i2c_write(start,mem,size);
	if (ret < 0) 
		return(WRITE_MEM_FAILED);
	else
		return(WRITE_MEM_OK);
}

U8 boot_write_mem(U16 start, U16 size, U8 *mem)
{
	int ret;

	ret = boot_qt602240_i2c_write(start,mem,size);
	if (ret < 0) {
		dprintk("boot write mem fail: %d \n",ret);
		return(WRITE_MEM_FAILED);
	} else
		return(WRITE_MEM_OK);
}



/*****************************************************************************
*
*  FUNCTION
*  PURPOSE
*  INPUT
*  OUTPUT
*
* ***************************************************************************/

void write_message_to_usart(uint8_t msg[], uint8_t length)
{
	int i;
	for (i = 0; i < length; i++)
		dprintk("0x%02x ", msg[i]);
	dprintk("\n\r");
}

/*
 * Message handler that is called by the touch chip driver when messages
 * are received.
 * 
 * This example message handler simply prints the messages as they are
 * received to USART1 port of EVK1011 board.
 */
void message_handler(U8 *msg, U8 length)
{  
}


irqreturn_t qt602240_irq_handler(int irq, void *dev_id)
{
	get_message();

	return IRQ_HANDLED;
}

int qt602240_probe(struct i2c_client *client,
               const struct i2c_device_id *id)
{
	int ret;

	qt602240->client = client;
	qt602240->client->irq = IRQ_TOUCH_INT;

	dprintk("qt602240_i2c is attached..\n");

	//DEBUG;
	printk(KERN_DEBUG"+-----------------------------------------+\n");
	printk(KERN_DEBUG "|  Quantum Touch Driver Probe!            |\n");
	printk(KERN_DEBUG"+-----------------------------------------+\n");

	gpio_set_value(GPIO_TOUCH_EN, 1);
	msleep(70);

	//INIT_WORK(&qt602240->ts_event_work, get_message );

	qt602240->input_dev = input_allocate_device();
	if (qt602240->input_dev == NULL) {
		ret = -ENOMEM;
		printk(KERN_DEBUG "qt602240_probe: Failed to allocate input device\n");
		goto err_input_dev_alloc_failed;
	}
	qt602240->input_dev->name = "qt602240_ts_input";
	set_bit(EV_SYN, qt602240->input_dev->evbit);
	set_bit(EV_KEY, qt602240->input_dev->evbit);
	set_bit(BTN_TOUCH, qt602240->input_dev->keybit);
	set_bit(EV_ABS, qt602240->input_dev->evbit);

	input_set_abs_params(qt602240->input_dev, ABS_X, 0, 479, 0, 0);
	input_set_abs_params(qt602240->input_dev, ABS_Y, 0, 799, 0, 0);
	input_set_abs_params(qt602240->input_dev, ABS_PRESSURE, 0, 255, 0, 0);
	input_set_abs_params(qt602240->input_dev, ABS_TOOL_WIDTH, 0, 15, 0, 0);

#ifdef _SUPPORT_MULTITOUCH_
	input_set_abs_params(qt602240->input_dev, ABS_MT_POSITION_X, 0, 479, 0, 0);
	input_set_abs_params(qt602240->input_dev, ABS_MT_POSITION_Y, 0, 799, 0, 0);
	input_set_abs_params(qt602240->input_dev, ABS_MT_TOUCH_MAJOR, 0, 255, 0, 0);
	input_set_abs_params(qt602240->input_dev, ABS_MT_WIDTH_MAJOR, 0, 30, 0, 0);
	input_set_abs_params(qt602240->input_dev, ABS_MT_TRACKING_ID, 0, MAX_USING_FINGER_NUM-1, 0, 0);
#endif    

	ret = input_register_device(qt602240->input_dev);
	if (ret) {
		printk(KERN_DEBUG "qt602240_probe: Unable to register %s input device\n", qt602240->input_dev->name);
		goto err_input_register_device_failed;
	}
#ifdef QT_FIRMUP_ENABLE
	QT_reprogram();
#else
	quantum_touch_probe();
#endif

	set_irq_type(IRQ_TOUCH_INT, IRQ_TYPE_LEVEL_LOW); // IRQ_TYPE_EDGE_FALLING);
	s3c_gpio_cfgpin(GPIO_TOUCH_INT, S3C_GPIO_SFN(0xf));
	s3c_gpio_setpull(GPIO_TOUCH_INT, S3C_GPIO_PULL_NONE);

	touchscreen_power_state_on = 1;
	set_tsp_threshhold(touch_TA_detect);

	ret = request_threaded_irq(qt602240->client->irq, NULL, qt602240_irq_handler,
		IRQF_TRIGGER_LOW | IRQF_ONESHOT, "qt602240 irq", qt602240);
	if (ret == 0)
		dprintk("[TSP] qt602240_probe: Start touchscreen %s\n", qt602240->input_dev->name);
	else
		dprintk("[TSP] request_irq failed\n");

	dprintk("%s ,  %d\n",__FUNCTION__, __LINE__ );
#ifdef USE_TSP_EARLY_SUSPEND
	qt602240->early_suspend.level = EARLY_SUSPEND_LEVEL_BLANK_SCREEN + 1;
	qt602240->early_suspend.suspend = qt602240_early_suspend;
	qt602240->early_suspend.resume = qt602240_late_resume;
	register_early_suspend(&qt602240->early_suspend);
#endif    /* CONFIG_HAS_EARLYSUSPEND */

	qt_initial_ok = 1;

	return 0;

	err_input_register_device_failed:
	input_free_device(qt602240->input_dev);

	err_input_dev_alloc_failed:
	kfree(qt602240);
	return ret;
}

static int qt602240_remove(struct i2c_client *client)
{
	struct qt602240_data *data = i2c_get_clientdata(client);
#ifdef USE_TSP_EARLY_SUSPEND
	unregister_early_suspend(&data->early_suspend);
#endif    /* CONFIG_HAS_EARLYSUSPEND */

	free_irq(data->irq, data);
	free_irq(qt602240->client->irq, 0);
	input_unregister_device(qt602240->input_dev);
	kfree(qt602240);
	i2c_set_clientdata(client, NULL);

	return 0;
}

#ifndef USE_TSP_EARLY_SUSPEND
static int qt602240_suspend(struct i2c_client *client, pm_message_t mesg)
{
	gen_powerconfig_t7_config_t power_config_sleep = {0};
	int i = 0;

	ENTER_FUNC;

	/* Set power config. */
	/* Set Idle Acquisition Interval to 32 ms. */
	power_config_sleep.idleacqint = 0;
	/* Set Active Acquisition Interval to 16 ms. */
	power_config_sleep.actvacqint = 0;

	/* Write power config to chip. */
	if (write_power_config(power_config_sleep) != CFG_WRITE_OK) {
		/* "Power config write failed!\n" */
		dprintk("\n[TSP][ERROR] line : %d\n", __LINE__);
	}

#ifdef _SUPPORT_MULTITOUCH_
	for (i = 0; i < MAX_USING_FINGER_NUM; i++)
		fingerInfo[i].pressure = -1;
#endif
	qt_timer_state = 0;

	LEAVE_FUNC;
	return 0;
}

static int qt602240_resume(struct i2c_client *client)
{
	int ret,i;

	ENTER_FUNC;

	if ((ret = write_power_config(power_config)) != CFG_WRITE_OK) {
		/* "Power config write failed!\n" */
		for (i = 0; i < 10; i++) {
			dprintk("\n[TSP][ERROR] config error %d,  line : %d i %d\n",ret, __LINE__,i);
			msleep(20);
			if ((ret = write_power_config(power_config)) == CFG_WRITE_OK)
				break;
		}
		if (i == 10)
			return -1;
	}

	calibrate_chip();
	LEAVE_FUNC;
	return 0;
}
#endif

#ifdef USE_TSP_EARLY_SUSPEND
static void qt602240_early_suspend(struct early_suspend *h)
{
	gen_powerconfig_t7_config_t power_config_sleep = {0};
//	int i = 0;

	printk("%s\n",__FUNCTION__);

	disable_irq(qt602240->client->irq);

	touchscreen_power_state_on = 0;

	/* Set power config. */
	/* Set Idle Acquisition Interval to 32 ms. */
	power_config_sleep.idleacqint = 0;
	/* Set Active Acquisition Interval to 16 ms. */
	power_config_sleep.actvacqint = 0;

	/* Write power config to chip. */
	if (write_power_config(power_config_sleep) != CFG_WRITE_OK) {
		/* "Power config write failed!\n" */
		dprintk("\n[TSP][ERROR] line : %d\n", __LINE__);
	}

#ifdef _SUPPORT_MULTITOUCH_
#if 0
	for (i = 0; i < MAX_USING_FINGER_NUM; i++){
		fingerInfo[i].pressure = -1;
	}
	touch_state_val = 0;
#else
	TSP_forced_release();
#endif
#endif
	qt_timer_state = 0;

	/*reset the gpio's for the sleep configuration*/
	s3c_gpio_cfgpin(GPIO_TOUCH_INT, S3C_GPIO_INPUT);
	s3c_gpio_setpull(GPIO_TOUCH_INT, S3C_GPIO_PULL_DOWN);

	gpio_set_value(GPIO_TOUCH_EN, 0);
#ifdef CONFIG_MACH_AEGIS
	gpio_set_value(TOUCHKEY_VDD_EN, 0);
#endif
	LEAVE_FUNC;
}

static void qt602240_late_resume(struct early_suspend *h)
{
#ifdef CONFIG_MACH_AEGIS
	int ret,i;
#endif

	printk("%s\n",__FUNCTION__);

#ifdef CONFIG_MACH_AEGIS
	gpio_set_value(TOUCHKEY_VDD_EN, 1);
	msleep(5);
#endif
	gpio_set_value(GPIO_TOUCH_EN, 1);
	msleep(70);
	s3c_gpio_cfgpin(GPIO_TOUCH_INT, S3C_GPIO_SFN(0xf));
	s3c_gpio_setpull(GPIO_TOUCH_INT, S3C_GPIO_PULL_NONE);

#ifdef CONFIG_MACH_AEGIS
	if(HWREV==0x09){
		if ((ret = write_power_config(power_config)) != CFG_WRITE_OK) {
			/* "Power config write failed!\n" */
			for (i = 0; i < 10; i++) {
				dprintk("\n[TSP][ERROR] config error %d,  line : %d i %d\n",ret, __LINE__,i);
				msleep(20);
				if ((ret = write_power_config(power_config)) == CFG_WRITE_OK)
					break;
			}
		}
	}
#endif

	msleep(20);
	calibrate_chip();

	noise_freq_table.fherr_count = 0;

	touchscreen_power_state_on = 1;
	set_tsp_threshhold(touch_TA_detect);

	enable_irq(qt602240->client->irq);

	LEAVE_FUNC;
}
#endif // End of USE_TSP_EARLY_SUSPEND

static struct i2c_device_id qt602240_idtable[] = {
	{ "qt602240_ts", 0 },
	{ }
};

MODULE_DEVICE_TABLE(i2c, qt602240_idtable);

struct i2c_driver qt602240_i2c_driver = {
	.driver = {
		.owner = THIS_MODULE,
		.name = "qt602240_ts",
	},
	.id_table = qt602240_idtable,
	.probe = qt602240_probe,
	.remove = __devexit_p(qt602240_remove),
#ifndef USE_TSP_EARLY_SUSPEND
	.suspend = qt602240_suspend,
	.resume = qt602240_resume,
#endif //USE_TSP_EARLY_SUSPEND
};

extern struct class *sec_class;
struct device *ts_dev;

static ssize_t i2c_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	int ret;
	unsigned char read_buf[5];

	ret = qt602240_i2c_read(0,read_buf, 5);
	if (ret < 0)
		dprintk("qt602240 i2c read failed.\n");

	dprintk("qt602240 read: %x, %x, %x, %x, %x\n", read_buf[0], read_buf[1], read_buf[2], read_buf[3], read_buf[4]);

	return sprintf(buf, "%s\n", buf);
}

static ssize_t i2c_store(struct device *dev, struct device_attribute *attr,
	const char *buf, size_t size)
{
	return size;
}

static ssize_t gpio_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	dprintk("qt602240 GPIO Status\n");
	dprintk("TOUCH_EN  : %s\n", gpio_get_value(GPIO_TOUCH_EN)? "High":"Low");
	//dprintk("TOUCH_RST : %s\n", gpio_get_value(GPIO_TOUCH_RST)? "High":"Low");
	dprintk("TOUCH_RST is NC\n");
	dprintk("TOUCH_INT : %s\n", gpio_get_value(GPIO_TOUCH_INT)? "High":"Low");

	return sprintf(buf, "%s\n", buf);
}

static ssize_t gpio_store(struct device *dev, struct device_attribute *attr,
	const char *buf, size_t size)
{
	if (strncmp(buf, "ENHIGH", 6) == 0 || strncmp(buf, "enhigh", 6) == 0) {
		gpio_set_value(GPIO_TOUCH_EN, 1);
		dprintk("set TOUCH_EN High.\n");
		msleep(100);
	}
	if (strncmp(buf, "ENLOW", 5) == 0 || strncmp(buf, "enlow", 5) == 0) {
		gpio_set_value(GPIO_TOUCH_EN, 0);
		dprintk("set TOUCH_EN Low.\n");
		msleep(100);
	}

	if (strncmp(buf, "RSTHIGH", 7) == 0 || strncmp(buf, "rsthigh", 7) == 0) {
//		gpio_set_value(GPIO_TOUCH_RST, 1);
		dprintk("TOUCH_RST is NC.\n");
		msleep(100);
	}
	if (strncmp(buf, "RSTLOW", 6) == 0 || strncmp(buf, "rstlow", 6) == 0) {
//		gpio_set_value(GPIO_TOUCH_RST, 0);
		dprintk("TOUCH_RST is NC\n");
		msleep(100);
	}

	return size;
}

static ssize_t firmware1_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	char a, b;
	a = tsp_version & 0xf0;
	a = a >> 4;
	b = tsp_version & 0x0f;
	return sprintf(buf, "%d.%d\n", a, b);
}

static ssize_t firmware1_store(struct device *dev, struct device_attribute *attr,
	const char *buf, size_t size)
{
	return size;
}

static ssize_t key_threshold_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	if (info_block->info_id.family_id == 0x80)
		return sprintf(buf, "%d\n", touchscreen_config.tchthr);
	else if (info_block->info_id.family_id == 0x81)
		return sprintf(buf, "%d\n", touchscreen_config_e.tchthr);
	else
		return sprintf(buf, "0\n");
}

static ssize_t key_threshold_store(struct device *dev, struct device_attribute *attr,
	const char *buf, size_t size)
{
	int i;
	if (sscanf(buf,"%d",&i)==1) {
		if (info_block->info_id.family_id == 0x80)
			touchscreen_config.tchthr = i; //basically,40;
		else if (info_block->info_id.family_id == 0x81)
			touchscreen_config_e.tchthr = i;
		dprintk("threshold is changed to %d\n", i);
	} else
		dprintk("threshold write error\n");

	return 0;
}

#ifdef QT_FIRMUP_ENABLE
uint8_t boot_unlock(void)
{
	int ret;
	unsigned char data[2];

	data[0] = 0xDC;
	data[1] = 0xAA;

	ret = boot_qt602240_i2c_write(0,data,2);
	if (ret < 0) {
		dprintk("%s : i2c write failed\n",__func__);
		return(WRITE_MEM_FAILED);
	}

	return(WRITE_MEM_OK);
}

void TSP_Restart(void)
{
	gpio_set_value(GPIO_TOUCH_EN, 0);
	msleep(300);
	gpio_set_value(GPIO_TOUCH_EN, 1);
}

uint8_t QT_Boot(void)
{
	unsigned char boot_status;
	unsigned char boot_ver;
	unsigned char retry_cnt;
	unsigned long int character_position;
	unsigned int frame_size=0;
	unsigned int next_frame;
	unsigned int crc_error_count;
	unsigned int size1,size2;
	unsigned int j, read_status_flag;
	uint8_t data = 0xA5;

	uint8_t reset_result = 0;

	unsigned char *firmware_data ;

	if (info_block->info_id.family_id == 0x81)
		firmware_data = mxt224e_firmware;
	else
		firmware_data = QT602240_firmware;

	crc_error_count = 0;
	character_position = 0;
	next_frame = 0;

	reset_result = write_mem(command_processor_address + RESET_OFFSET, 1, &data);

	if (reset_result != WRITE_MEM_OK) {
		for (retry_cnt = 0; retry_cnt < 3; retry_cnt++) {
			msleep(100);
			reset_result = write_mem(command_processor_address + RESET_OFFSET, 1, &data);
			if (reset_result == WRITE_MEM_OK)
				break;
		}
	}
	if (reset_result == WRITE_MEM_OK)
		dprintk("Boot reset OK\r\n");

	msleep(100);

	for (retry_cnt = 0; retry_cnt < 30; retry_cnt++) {
		if ((boot_read_mem(0, 1, &boot_status) == READ_MEM_OK) && (boot_status & 0xC0) == 0xC0) {
			boot_ver = boot_status & 0x3F;
			crc_error_count = 0;
			character_position = 0;
			next_frame = 0;

			while (1) { 
				for (j = 0; j < 5; j++) {
					msleep(60);
					if (boot_read_mem(0, 1, &boot_status) == READ_MEM_OK) {
						read_status_flag = 1;
						break;
					} else {
						read_status_flag = 0;
					}
				}

				if (read_status_flag == 1) {
					retry_cnt = 0;
					dprintk("TSP boot status is %x        stage 2 \n", boot_status);
					if ((boot_status & QT_WAITING_BOOTLOAD_COMMAND) == QT_WAITING_BOOTLOAD_COMMAND) {
						if (boot_unlock() == WRITE_MEM_OK) {
							msleep(10);

							dprintk("Unlock OK\n");

						} else {
							dprintk("Unlock fail\n");
						}
					} else if ((boot_status & 0xC0) == QT_WAITING_FRAME_DATA) {
						/* Add 2 to frame size, as the CRC bytes are not included */
						size1 = *(firmware_data+character_position);
						size2 = *(firmware_data+character_position+1) + 2;
						frame_size = (size1<<8) + size2;

						dprintk("Frame size:%d\n", frame_size);
						dprintk("Firmware pos:%lu\n", character_position);
						/* Exit if frame data size is zero */
						if (0 == frame_size) {
							if (QT_i2c_address == I2C_BOOT_ADDR_0)
								QT_i2c_address = I2C_APPL_ADDR_0;
							dprintk("0 == frame_size\n");
							return 1;
						}
						next_frame = 1;
						boot_write_mem(0, frame_size, (firmware_data +character_position));
						msleep(10);
						dprintk(".");
					} else if (boot_status == QT_FRAME_CRC_CHECK) {
							dprintk("CRC Check\n");
					} else if (boot_status == QT_FRAME_CRC_PASS) {
						if (next_frame == 1) {
							dprintk("CRC Ok\n");
							character_position += frame_size;
							next_frame = 0;
						} else {
							dprintk("next_frame != 1\n");
						}
					} else if (boot_status == QT_FRAME_CRC_FAIL) {
						dprintk("CRC Fail\n");
						crc_error_count++;
					}

					if (crc_error_count > 10)
						return QT_FRAME_CRC_FAIL;
				} else {
					return 0;
				}
			}
		} else {
			dprintk("[TSP] read_boot_state() or (boot_status & 0xC0) == 0xC0) is fail!!!\n");
		}
	}

	dprintk("QT_Boot end \n");
	return 0;
}

uint8_t QT_Boot_no_reset(void)
{
	unsigned char boot_status;
	unsigned char boot_ver;
	unsigned char retry_cnt;
	unsigned short character_position;
	unsigned short frame_size = 0;
	unsigned short next_frame;
	unsigned short crc_error_count;
	unsigned char size1,size2;
	unsigned short j, read_status_flag;

	unsigned char *firmware_data ;

	if (info_block->info_id.family_id == 0x81)
		firmware_data = mxt224e_firmware;
	else
		firmware_data = QT602240_firmware;

	crc_error_count = 0;
	character_position = 0;
	next_frame = 0;

	for (retry_cnt = 0; retry_cnt < 30; retry_cnt++) {
		if ((boot_read_mem(0,1,&boot_status) == READ_MEM_OK) && (boot_status & 0xC0) == 0xC0) {
			boot_ver = boot_status & 0x3F;
			crc_error_count = 0;
			character_position = 0;
			next_frame = 0;

			while (1) { 
				for (j = 0; j < 5; j++) {
					msleep(60);
					if (boot_read_mem(0,1,&boot_status) == READ_MEM_OK) {
						read_status_flag = 1;
						break;
					} else {
						read_status_flag = 0;
					}
				}

				if (read_status_flag == 1) {
					retry_cnt = 0;
					dprintk("TSP boot status is %x        stage 2 \n", boot_status);
					if ((boot_status & QT_WAITING_BOOTLOAD_COMMAND) == QT_WAITING_BOOTLOAD_COMMAND) {
						if (boot_unlock() == WRITE_MEM_OK) {
							msleep(10);

							dprintk("Unlock OK\n");
						} else {
							dprintk("Unlock fail\n");
						}
					} else if ((boot_status & 0xC0) == QT_WAITING_FRAME_DATA) {
						/* Add 2 to frame size, as the CRC bytes are not included */
						size1 =  *(firmware_data+character_position);
						size2 =  *(firmware_data+character_position+1) + 2;
						frame_size = (size1<<8) + size2;

						dprintk("Frame size:%d\n", frame_size);
						dprintk("Firmware pos:%d\n", character_position);
						/* Exit if frame data size is zero */
						if (0 == frame_size) {
							if (QT_i2c_address == I2C_BOOT_ADDR_0) {
								QT_i2c_address = I2C_APPL_ADDR_0;
							}
							dprintk("0 == frame_size\n");
							return 1;
						}
						next_frame = 1;
						boot_write_mem(0,frame_size, (firmware_data +character_position));
						msleep(10);
						dprintk(".");
					} else if (boot_status == QT_FRAME_CRC_CHECK) {
						dprintk("CRC Check\n");
					} else if (boot_status == QT_FRAME_CRC_PASS) {
						if (next_frame == 1) {
							dprintk("CRC Ok\n");
							character_position += frame_size;
							next_frame = 0;
						} else {
							dprintk("next_frame != 1\n");
						}
					} else if (boot_status == QT_FRAME_CRC_FAIL) {
						dprintk("CRC Fail\n");
						crc_error_count++;
					}

					if (crc_error_count > 10) {
						return QT_FRAME_CRC_FAIL;
					}
				} else {
					return 0;
				}
			}
		} else {
			dprintk("[TSP] read_boot_state() or (boot_status & 0xC0) == 0xC0) is fail!!!\n");
		}
	}

	dprintk("QT_Boot_no_reset end \n");
	return 0;
}



//*****************************************************************************
//
// 
//
//*****************************************************************************

void QT_reprogram(void)
{
	uint8_t family_id, version, build;
	unsigned char rxdata;
	int firm_update_flag = 0;

	dprintk("QT_reprogram check\n");

	if (boot_read_mem(0,1,&rxdata) == READ_MEM_OK) {
		dprintk("Enter to new firmware : boot mode\n");
		if (QT_Boot_no_reset())
			TSP_Restart();
		dprintk("Reprogram done : boot mode\n");
	}

	quantum_touch_probe();  /* find and initialise QT device */

	get_family_id(&family_id);
	get_version(&version);
	get_build_number(&build);

	if (family_id == 0x80) {
		if (((version != 0x14)&&(version < 0x16)) || ((version == 0x16)&&(build == 0xAA))) 
			firm_update_flag = 1;
	} else if (family_id == 0x81) {
		if ((version < 0x10) || ((version == 0x10) && (build < 0xaa)))
			firm_update_flag = 1;
	}

	if (firm_update_flag == 1) {
		if (family_id == 0x80)
			printk(KERN_DEBUG"MXT224 : version = 0x%x(%d), build = 0x%x(%d)\n", version, version, build, build);
		else if (family_id == 0x81)
			printk(KERN_DEBUG"MXT224E : version = 0x%x(%d), build = 0x%x(%d)\n", version, version, build, build);
		printk(KERN_DEBUG"Enter to new firmware : ADDR = Other Version\n");
		if (!QT_Boot()) {
			TSP_Restart();
			quantum_touch_probe();
			printk(KERN_DEBUG"QT_reprogram done. firmware update fail.\n");
		}
		printk(KERN_DEBUG"QT_reprogram done. firmware update success.\n");
	}
}
#endif //QT_FIRMUP_ENABLE

static ssize_t setup_show(struct device *dev, struct device_attribute *attr, char *buf)
{    
	dprintk("qt602240 Setup Status\n");

	return 0;
}

static ssize_t setup_store( struct device *dev, struct device_attribute *attr,
	const char *buf, size_t size)
{
	return size;
}

static DEVICE_ATTR(gpio, S_IRUGO | S_IWUSR, gpio_show, gpio_store);
static DEVICE_ATTR(i2c, S_IRUGO | S_IWUSR, i2c_show, i2c_store);
static DEVICE_ATTR(setup, S_IRUGO | S_IWUSR, setup_show, setup_store);
static DEVICE_ATTR(firmware1, S_IRUGO | S_IWUSR, firmware1_show, firmware1_store);
static DEVICE_ATTR(key_threshold, S_IRUGO | S_IWUSR, key_threshold_show, key_threshold_store);

/*------------------------------ for tunning ATmel - start ----------------------------*/

static ssize_t set_power_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	printk(KERN_INFO "[TSP] %s : operate nothing\n", __FUNCTION__);

	return 0;
}
static ssize_t set_power_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t size)
{
	int cmd_no,config_value = 0;
	char *after;

	unsigned long value = simple_strtoul(buf, &after, 10);    
	printk(KERN_DEBUG "[TSP] %s\n", __FUNCTION__);
	cmd_no = (int)(value / 1000);
	config_value = (int)(value % 1000 );

	if (cmd_no == 0) {
		dprintk("[%s] CMD 0 , power_config.idleacqint = %d\n", __func__,config_value );
		power_config.idleacqint = config_value;
	} else if (cmd_no == 1) {
		dprintk("[%s] CMD 1 , power_config.actvacqint = %d\n", __func__, config_value);
		power_config.actvacqint = config_value;
	} else if (cmd_no == 2) {
		dprintk("[%s] CMD 2 , power_config.actv2idleto= %d\n", __func__, config_value);
		power_config.actv2idleto = config_value;
	} else
		dprintk("[%s] unknown CMD\n", __func__);

	if (write_power_config(power_config) != CFG_WRITE_OK)
		dprintk("[TSP] Configuration Fail!!! , Line %d \n\r", __LINE__);

    return size;
}
static DEVICE_ATTR(set_power, 0664, set_power_show, set_power_store);


static ssize_t set_acquisition_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	printk(KERN_DEBUG "[TSP] %s : operate nothing\n", __FUNCTION__);

	return 0;
}
static ssize_t set_acquisition_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t size)
{
	int cmd_no,config_value = 0;
	char *after;

	unsigned long value = simple_strtoul(buf, &after, 10);    
	printk(KERN_DEBUG "[TSP] %s\n", __FUNCTION__);
	cmd_no = (int) (value / 1000);
	config_value = (int) (value % 1000 );

	if (info_block->info_id.family_id == 0x80) {
		if (cmd_no == 0) {
			dprintk("[%s] CMD 0 , acquisition_config.chrgtime = %d\n", __func__, config_value );
			acquisition_config.chrgtime = config_value;
		} else if (cmd_no == 1) {
			dprintk("[%s] CMD 1 , acquisition_config.tchdrift = %d\n", __func__, config_value );
			acquisition_config.tchdrift = config_value;
		} else if (cmd_no == 2) {
			dprintk("[%s] CMD 2 , acquisition_config.driftst = %d\n", __func__, config_value );
			acquisition_config.driftst = config_value;
		} else if (cmd_no == 3) {
			dprintk("[%s] CMD 3 , acquisition_config.tchautocal = %d\n", __func__, config_value );
			acquisition_config.tchautocal= config_value;
		} else if (cmd_no == 4) {
			dprintk("[%s] CMD 4 , acquisition_config.sync = %d\n", __func__, config_value );
			acquisition_config.sync = config_value;
		} else if (cmd_no == 5) {
			dprintk("[%s] CMD 5 , acquisition_config.atchcalst = %d\n", __func__, config_value );
			acquisition_config.atchcalst = config_value;
		} else if (cmd_no == 6) {
			dprintk("[%s] CMD 6 , acquisition_config.atchcalsthr = %d\n", __func__, config_value );
			acquisition_config.atchcalsthr = config_value;
		} else
			dprintk("[%s] unknown CMD\n", __func__);

		if (write_acquisition_config(acquisition_config) != CFG_WRITE_OK)
			dprintk("[TSP] Configuration Fail!!! , Line %d \n\r", __LINE__);
	}else if (info_block->info_id.family_id == 0x81) {
		if (cmd_no == 0) {
			dprintk("[%s] CMD 0 , acquisition_config_e.chrgtime = %d\n", __func__, config_value );
			acquisition_config_e.chrgtime = config_value;
		} else if (cmd_no == 1) {
			dprintk("[%s] CMD 1 , acquisition_config_e.tchdrift = %d\n", __func__, config_value );
			acquisition_config_e.tchdrift = config_value;
		} else if (cmd_no == 2) {
			dprintk("[%s] CMD 2 , acquisition_config_e.driftst = %d\n", __func__, config_value );
			acquisition_config_e.driftst = config_value;
		} else if (cmd_no == 3) {
			dprintk("[%s] CMD 3 , acquisition_config_e.tchautocal = %d\n", __func__, config_value );
			acquisition_config_e.tchautocal = config_value;
		} else if (cmd_no == 4) {
			dprintk("[%s] CMD 4 , acquisition_config_e.sync = %d\n", __func__, config_value );
			acquisition_config_e.sync = config_value;
		} else if (cmd_no == 5) {
			dprintk("[%s] CMD 5 , acquisition_config_e.atchcalst = %d\n", __func__, config_value );
			acquisition_config_e.atchcalst = config_value;
		} else if (cmd_no == 6) {
			dprintk("[%s] CMD 6 , acquisition_config_e.atchcalsthr = %d\n", __func__, config_value );
			acquisition_config_e.atchcalsthr = config_value;
		} else if (cmd_no == 7) {
			dprintk("[%s] CMD 7 , acquisition_config_e.atchfrccalthr = %d\n", __func__, config_value );
			acquisition_config_e.atchfrccalthr = config_value;
		} else if (cmd_no == 8) {
			dprintk("[%s] CMD 8 , acquisition_config_e.atchfrccalratio = %d\n", __func__, config_value );
			acquisition_config_e.atchfrccalratio = config_value;
		} else
			dprintk("[%s] unknown CMD\n", __func__);

		if (write_acquisition_config_e(acquisition_config_e) != CFG_WRITE_OK)
			dprintk("[TSP] Configuration Fail!!! , Line %d \n\r", __LINE__);
	}

    return size;
}
static DEVICE_ATTR(set_acquisition, 0664, set_acquisition_show, set_acquisition_store);


static ssize_t set_touchscreen_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	printk(KERN_DEBUG "[TSP] %s : operate nothing\n", __FUNCTION__);

	return 0;
}
static ssize_t set_touchscreen_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t size)
{
	int  cmd_no,config_value = 0;
	char *after;

	unsigned long value = simple_strtoul(buf, &after, 10);    
	printk(KERN_DEBUG "[TSP] %s\n", __FUNCTION__);
	cmd_no = (int) (value / 1000);
	config_value = (int) (value % 1000 );

	if (info_block->info_id.family_id == 0x80) {
		if (cmd_no == 0) {
			dprintk("[%s] CMD 0 , touchscreen_config.ctrl = %d\n", __func__, config_value );
			touchscreen_config.ctrl = config_value;
		} else if (cmd_no == 1) {
			dprintk("[%s] CMD 1 , touchscreen_config.xorigin = %d\n", __func__, config_value );
			touchscreen_config.xorigin = config_value;
		} else if (cmd_no == 2) {
			dprintk("[%s] CMD 2 , touchscreen_config.yorigin = %d\n", __func__, config_value );
			touchscreen_config.yorigin = config_value;
		} else if (cmd_no == 3) {
			dprintk("[%s] CMD 3 , touchscreen_config.xsize = %d\n", __func__, config_value );
			touchscreen_config.xsize = config_value;
		} else if (cmd_no == 4) {
			dprintk("[%s] CMD 4 , touchscreen_config.ysize = %d\n", __func__, config_value );
			touchscreen_config.ysize = config_value;
		} else if (cmd_no == 5) {
			dprintk("[%s] CMD 5 , touchscreen_config.akscfg = %d\n", __func__, config_value );
			touchscreen_config.akscfg = config_value;
		} else if (cmd_no == 6) {
			dprintk("[%s] CMD 6 , touchscreen_config.blen = %d\n", __func__, config_value );
			touchscreen_config.blen = config_value;
		} else if (cmd_no == 7) {
			dprintk("[%s] CMD 7 , touchscreen_config.tchthr = %d\n", __func__, config_value );
			touchscreen_config.tchthr = config_value;
		} else if (cmd_no == 8) {
			dprintk("[%s] CMD 8 , touchscreen_config.tchdi = %d\n", __func__, config_value );
			touchscreen_config.tchdi = config_value;
		} else if (cmd_no == 9) {
			dprintk("[%s] CMD 9 , touchscreen_config.orient = %d\n", __func__, config_value );
			touchscreen_config.orient = config_value;
		} else if (cmd_no == 10) {
			dprintk("[%s] CMD 10 , touchscreen_config.mrgtimeout = %d\n", __func__, config_value );
			touchscreen_config.mrgtimeout = config_value;
		} else if (cmd_no == 11) {
			dprintk("[%s] CMD 11 , touchscreen_config.movhysti = %d\n", __func__, config_value );
			touchscreen_config.movhysti = config_value;
		} else if (cmd_no == 12) {
			dprintk("[%s] CMD 12 , touchscreen_config.movhystn = %d\n", __func__, config_value );
			touchscreen_config.movhystn = config_value;
		} else if (cmd_no == 13) {
			dprintk("[%s] CMD 13 , touchscreen_config.movfilter = %d\n", __func__, config_value );
			touchscreen_config.movfilter = config_value;
		} else if (cmd_no == 14) {
			dprintk("[%s] CMD 14 , touchscreen_config.numtouch = %d\n", __func__, config_value );
			touchscreen_config.numtouch = config_value;
		} else if (cmd_no == 15) {
			dprintk("[%s] CMD 15 , touchscreen_config.mrghyst = %d\n", __func__, config_value );
			touchscreen_config.mrghyst = config_value;
		} else if (cmd_no == 16) {
			dprintk("[%s] CMD 16 , touchscreen_config.mrgthr = %d\n", __func__, config_value );
			touchscreen_config.mrgthr = config_value;
		} else if (cmd_no == 17) {
			dprintk("[%s] CMD 17 , touchscreen_config.tchamphyst = %d\n", __func__, config_value );
			touchscreen_config.amphyst = config_value;
		} else if (cmd_no == 18) {
			dprintk("[%s] CMD 18 , touchscreen_config.xrange = %d\n", __func__, config_value );
			touchscreen_config.xrange = config_value;
		} else if (cmd_no == 19) {
			dprintk("[%s] CMD 19 , touchscreen_config.yrange = %d\n", __func__, config_value );
			touchscreen_config.yrange = config_value;
		} else if (cmd_no == 20) {
			dprintk("[%s] CMD 20 , touchscreen_config.xloclip = %d\n", __func__, config_value );
			touchscreen_config.xloclip = config_value;
		} else if (cmd_no == 21) {
			dprintk("[%s] CMD 21 , touchscreen_config.xhiclip = %d\n", __func__, config_value );
			touchscreen_config.xhiclip = config_value;
		} else if (cmd_no == 22) {
			dprintk("[%s] CMD 22 , touchscreen_config.yloclip = %d\n", __func__, config_value );
			touchscreen_config.yloclip = config_value;
		} else if (cmd_no == 23) {
			dprintk("[%s] CMD 23 , touchscreen_config.yhiclip = %d\n", __func__, config_value );
			touchscreen_config.yhiclip = config_value;
		} else if (cmd_no == 24) {
			dprintk("[%s] CMD 24 , touchscreen_config.xedgectrl = %d\n", __func__, config_value );
			touchscreen_config.xedgectrl = config_value;
		} else if (cmd_no == 25) {
			dprintk("[%s] CMD 25 , touchscreen_config.xedgedist = %d\n", __func__, config_value );
			touchscreen_config.xedgedist = config_value;
		} else if (cmd_no == 26) {
			dprintk("[%s] CMD 26 , touchscreen_config.yedgectrl = %d\n", __func__, config_value );
			touchscreen_config.yedgectrl = config_value;
		} else if (cmd_no == 27) {
			dprintk("[%s] CMD 27 , touchscreen_config.yedgedist = %d\n", __func__, config_value );
			touchscreen_config.yedgedist = config_value;
		} else if (cmd_no == 28) {
			if (tsp_version >= 0x16) {
				dprintk("[%s] CMD 28 , touchscreen_config.jumplimit = %d\n", __func__, config_value );
				touchscreen_config.jumplimit = config_value;
			} else
				dprintk("[%s] CMD 28 , touchscreen_config.jumplimit is not supported in this version.\n", __func__ );
		} else
			dprintk("[%s] unknown CMD\n", __func__);

		if (write_multitouchscreen_config(0, touchscreen_config) != CFG_WRITE_OK)
			dprintk("[TSP] Configuration Fail!!! , Line %d \n\r", __LINE__);

	} else if (info_block->info_id.family_id == 0x81) {
		if (cmd_no == 0) {
			dprintk("[%s] CMD 0 , touchscreen_config_e.ctrl = %d\n", __func__, config_value );
			touchscreen_config_e.ctrl = config_value;
		} else if (cmd_no == 1) {
			dprintk("[%s] CMD 1 , touchscreen_config_e.xorigin = %d\n", __func__, config_value );
			touchscreen_config_e.xorigin = config_value;
		} else if (cmd_no == 2) {
			dprintk("[%s] CMD 2 , touchscreen_config_e.yorigin = %d\n", __func__, config_value );
			touchscreen_config_e.yorigin = config_value;
		} else if (cmd_no == 3) {
			dprintk("[%s] CMD 3 , touchscreen_config_e.xsize = %d\n", __func__, config_value );
			touchscreen_config_e.xsize = config_value;
		} else if (cmd_no == 4) {
			dprintk("[%s] CMD 4 , touchscreen_config_e.ysize = %d\n", __func__, config_value );
			touchscreen_config_e.ysize = config_value;
		} else if (cmd_no == 5) {
			dprintk("[%s] CMD 5 , touchscreen_config_e.akscfg = %d\n", __func__, config_value );
			touchscreen_config_e.akscfg = config_value;
		} else if (cmd_no == 6) {
			dprintk("[%s] CMD 6 , touchscreen_config_e.blen = %d\n", __func__, config_value );
			touchscreen_config_e.blen = config_value;
		} else if (cmd_no == 7) {
			dprintk("[%s] CMD 7 , touchscreen_config_e.tchthr = %d\n", __func__, config_value );
			touchscreen_config_e.tchthr = config_value;
		} else if (cmd_no == 8) {
			dprintk("[%s] CMD 8 , touchscreen_config_e.tchdi = %d\n", __func__, config_value );
			touchscreen_config_e.tchdi = config_value;
		} else if (cmd_no == 9) {
			dprintk("[%s] CMD 9 , touchscreen_config_e.orient = %d\n", __func__, config_value );
			touchscreen_config_e.orient = config_value;
		} else if (cmd_no == 10) {
			dprintk("[%s] CMD 10 , touchscreen_config_e.mrgtimeout = %d\n", __func__, config_value );
			touchscreen_config_e.mrgtimeout = config_value;
		} else if (cmd_no == 11) {
			dprintk("[%s] CMD 11 , touchscreen_config_e.movhysti = %d\n", __func__, config_value );
			touchscreen_config_e.movhysti = config_value;
		} else if (cmd_no == 12) {
			dprintk("[%s] CMD 12 , touchscreen_config_e.movhystn = %d\n", __func__, config_value );
			touchscreen_config_e.movhystn = config_value;
		} else if (cmd_no == 13) {
			dprintk("[%s] CMD 13 , touchscreen_config_e.movfilter = %d\n", __func__, config_value );
			touchscreen_config_e.movfilter = config_value;
		} else if (cmd_no == 14) {
			dprintk("[%s] CMD 14 , touchscreen_config_e.numtouch = %d\n", __func__, config_value );
			touchscreen_config_e.numtouch = config_value;
		} else if (cmd_no == 15) {
			dprintk("[%s] CMD 15 , touchscreen_config_e.mrghyst = %d\n", __func__, config_value );
			touchscreen_config_e.mrghyst = config_value;
		} else if (cmd_no == 16) {
			dprintk("[%s] CMD 16 , touchscreen_config_e.mrgthr = %d\n", __func__, config_value );
			touchscreen_config_e.mrgthr = config_value;
		} else if (cmd_no == 17) {
			dprintk("[%s] CMD 17 , touchscreen_config_e.tchamphyst = %d\n", __func__, config_value );
			touchscreen_config_e.amphyst = config_value;
		} else if (cmd_no == 18) {
			dprintk("[%s] CMD 18 , touchscreen_config_e.xrange = %d\n", __func__, config_value );
			touchscreen_config_e.xrange = config_value;
		} else if (cmd_no == 19) {
			dprintk("[%s] CMD 19 , touchscreen_config_e.yrange = %d\n", __func__, config_value );
			touchscreen_config_e.yrange = config_value;
		} else if (cmd_no == 20) {
			dprintk("[%s] CMD 20 , touchscreen_config_e.xloclip = %d\n", __func__, config_value );
			touchscreen_config_e.xloclip = config_value;
		} else if (cmd_no == 21) {
			dprintk("[%s] CMD 21 , touchscreen_config_e.xhiclip = %d\n", __func__, config_value );
			touchscreen_config_e.xhiclip = config_value;
		} else if (cmd_no == 22) {
			dprintk("[%s] CMD 22 , touchscreen_config_e.yloclip = %d\n", __func__, config_value );
			touchscreen_config_e.yloclip = config_value;
		} else if (cmd_no == 23) {
			dprintk("[%s] CMD 23 , touchscreen_config_e.yhiclip = %d\n", __func__, config_value );
			touchscreen_config_e.yhiclip = config_value;
		} else if (cmd_no == 24) {
			dprintk("[%s] CMD 24 , touchscreen_config_e.xedgectrl = %d\n", __func__, config_value );
			touchscreen_config_e.xedgectrl = config_value;
		} else if (cmd_no == 25) {
			dprintk("[%s] CMD 25 , touchscreen_config_e.xedgedist = %d\n", __func__, config_value );
			touchscreen_config_e.xedgedist = config_value;
		} else if (cmd_no == 26) {
			dprintk("[%s] CMD 26 , touchscreen_config_e.yedgectrl = %d\n", __func__, config_value );
			touchscreen_config_e.yedgectrl = config_value;
		} else if (cmd_no == 27) {
			dprintk("[%s] CMD 27 , touchscreen_config_e.yedgedist = %d\n", __func__, config_value );
			touchscreen_config_e.yedgedist = config_value;
		} else if (cmd_no == 28) {
			dprintk("[%s] CMD 28 , touchscreen_config_e.jumplimit = %d\n", __func__, config_value );
			touchscreen_config_e.jumplimit = config_value;
		} else if (cmd_no == 29) {
			dprintk("[%s] CMD 29 , touchscreen_config_e.tchhyst = %d\n", __func__, config_value );
			touchscreen_config_e.tchhyst = config_value;
		} else if (cmd_no == 30) {
			dprintk("[%s] CMD 30 , touchscreen_config_e.xpitch = %d\n", __func__, config_value );
			touchscreen_config_e.xpitch = config_value;
		} else if (cmd_no == 31) {
			dprintk("[%s] CMD 31 , touchscreen_config_e.ypitch = %d\n", __func__, config_value );
			touchscreen_config_e.ypitch = config_value;
		} else if (cmd_no == 32) {
			dprintk("[%s] CMD 32 , touchscreen_config_e.nexttchdi = %d\n", __func__, config_value );
			touchscreen_config_e.nexttchdi = config_value;
		} else
			dprintk("[%s] unknown CMD\n", __func__);

		if (write_multitouchscreen_config_e(0, touchscreen_config_e) != CFG_WRITE_OK)
			dprintk("[TSP] Configuration Fail!!! , Line %d \n\r", __LINE__);
	}
	return size;
}
static DEVICE_ATTR(set_touchscreen, 0664, set_touchscreen_show, set_touchscreen_store);


static ssize_t set_keyarray_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	printk(KERN_DEBUG "[TSP] %s : operate nothing\n", __FUNCTION__);

	return 0;
}
static ssize_t set_keyarray_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t size)
{
	int cmd_no,config_value = 0;
	char *after;

	unsigned long value = simple_strtoul(buf, &after, 10);    
	printk(KERN_DEBUG "[TSP] %s\n", __FUNCTION__);
	cmd_no = (int) (value / 1000);
	config_value = (int) (value % 1000 );

	if (cmd_no == 0) {
		dprintk("[%s] CMD 0 , keyarray_config.ctrl = %d\n", __func__, config_value );
		keyarray_config.ctrl = config_value;
	} else if (cmd_no == 1) {
		dprintk("[%s] CMD 1 , keyarray_config.xorigin = %d\n", __func__, config_value );
		keyarray_config.xorigin = config_value;
	} else if (cmd_no == 2) {
		dprintk("[%s] CMD 2 , keyarray_config.xsize = %d\n", __func__, config_value );
		keyarray_config.xsize = config_value;
	} else if (cmd_no == 3) {
		dprintk("[%s] CMD 3 , keyarray_config.yorigin = %d\n", __func__, config_value );
		keyarray_config.yorigin = config_value;
	} else if (cmd_no == 4) {
		dprintk("[%s] CMD 4 , keyarray_config.ysize = %d\n", __func__, config_value );
		keyarray_config.ysize = config_value;
	} else if (cmd_no == 5) {
		dprintk("[%s] CMD 5 , keyarray_config.akscfg = %d\n", __func__, config_value );
		keyarray_config.akscfg = config_value;
	} else if (cmd_no == 6) {
		dprintk("[%s] CMD 6 , keyarray_config.blen = %d\n", __func__, config_value );
		keyarray_config.blen = config_value;
	} else if (cmd_no == 7) {
		dprintk("[%s] CMD 7 , keyarray_config.tchthr = %d\n", __func__, config_value );
		keyarray_config.tchthr = config_value;
	} else if (cmd_no == 8) {
		dprintk("[%s] CMD 8 , keyarray_config.tchdi = %d\n", __func__, config_value );
		keyarray_config.tchdi = config_value;
	} else
		dprintk("[%s] unknown CMD\n", __func__);

	if (write_keyarray_config(0, keyarray_config) != CFG_WRITE_OK)
		dprintk("[TSP] Configuration Fail!!! , Line %d \n\r", __LINE__);

	return size;
}
static DEVICE_ATTR(set_keyarray, 0664, set_keyarray_show, set_keyarray_store);

static ssize_t set_noise_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	printk(KERN_DEBUG "[TSP] %s : operate nothing\n", __FUNCTION__);

	return 0;
}
static ssize_t set_noise_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t size)
{
	int cmd_no,config_value = 0;
	char *after;

	unsigned long value = simple_strtoul(buf, &after, 10);    
	printk(KERN_DEBUG "[TSP] %s\n", __FUNCTION__);
	cmd_no = (int) (value / 1000);
	config_value = (int) (value % 1000 );

	if (info_block->info_id.family_id == 0x80) {
		if (cmd_no == 0) {
			dprintk("[%s] CMD 0 , noise_suppression_config.ctrl = %d\n", __func__, config_value );
			noise_suppression_config.ctrl = config_value;
		} else if (cmd_no == 1) {
			dprintk("[%s] CMD 1 , noise_suppression_config.reserved = %d\n", __func__, config_value );
#if defined(__VER_1_2__)
			noise_suppression_config.outflen = config_value;
#elif defined(__VER_1_4__)
			noise_suppression_config.reserved = config_value;
#endif
		} else if (cmd_no == 2) {
			dprintk("[%s] CMD 2 , noise_suppression_config.reserved1 = %d\n", __func__, config_value );
			noise_suppression_config.reserved1  = config_value;
		} else if (cmd_no == 3) {
			dprintk("[%s] CMD 3 , noise_suppression_config.gcaful = %d\n", __func__, config_value );
			noise_suppression_config.gcaful = config_value;
		} else if (cmd_no == 4) {
			dprintk("[%s] CMD 4 , noise_suppression_config.gcafll = %d\n", __func__, config_value );
			noise_suppression_config.gcafll  = config_value;
		} else if (cmd_no == 5) {
			dprintk("[%s] CMD 5 , noise_suppression_config.actvgcafvalid = %d\n", __func__, config_value );
#if defined(__VER_1_2__)
			noise_suppression_config.gcaflcount = config_value;
#elif defined(__VER_1_4__)
			noise_suppression_config.actvgcafvalid = config_value;
#endif
		} else if (cmd_no == 6) {
			dprintk("[%s] CMD 6 , noise_suppression_config.noisethr = %d\n", __func__, config_value );
			noise_suppression_config.noisethr  = config_value;
		} else if (cmd_no == 7) {
			dprintk("[%s] CMD 7 , noise_suppression_config.freqhopscale = %d\n", __func__, config_value );
			noise_suppression_config.freqhopscale  = config_value;
		} else if (cmd_no == 8) {
			dprintk("[%s] CMD 8 , noise_suppression_config.freq[0] = %d\n", __func__, config_value );
			noise_suppression_config.freq[0]  = config_value;
		} else if (cmd_no == 9) {
			dprintk("[%s] CMD 9 , noise_suppression_config.freq[1] = %d\n", __func__, config_value );
			noise_suppression_config.freq[1]  = config_value;
		} else if (cmd_no == 10) {
			dprintk("[%s] CMD 10 , noise_suppression_config.freq[2] = %d\n", __func__, config_value );
			noise_suppression_config.freq[2]  = config_value;
		} else if (cmd_no == 11) {
			dprintk("[%s] CMD 11 , noise_suppression_config.freq[3] = %d\n", __func__, config_value );
			noise_suppression_config.freq[3]  = config_value;
		} else if (cmd_no == 12) {
			dprintk("[%s] CMD 12 , noise_suppression_config.freq[4] = %d\n", __func__, config_value );
			noise_suppression_config.freq[4]  = config_value;
		} else if (cmd_no == 13) {
			dprintk("[%s] CMD 13 , noise_suppression_config.idlegcafvalid = %d\n", __func__, config_value );
			noise_suppression_config.idlegcafvalid  = config_value;
		} else
			dprintk("[%s] unknown CMD\n", __func__);

		/* Write Noise suppression config to chip. */
		if (get_object_address(PROCG_NOISESUPPRESSION_T22, 0) != OBJECT_NOT_FOUND) {
			if (write_noisesuppression_config(0, noise_suppression_config) != CFG_WRITE_OK)
				dprintk("[TSP] Configuration Fail!!! , Line %d \n\r", __LINE__);
		}

		/* Write grip suppression config to chip. */
		if (get_object_address(PROCI_GRIPFACESUPPRESSION_T20, 0) != OBJECT_NOT_FOUND) {
			if (write_gripsuppression_config(0, gripfacesuppression_config) != CFG_WRITE_OK)
				dprintk("[TSP] Configuration Fail!!! , Line %d \n\r", __LINE__);
		}
	} else if (info_block->info_id.family_id == 0x81) {
		if (cmd_no == 0) {
			dprintk("[%s] CMD 0 , noise_suppression_config_e.ctrl = %d\n", __func__, config_value );
			noise_suppression_config_e.ctrl = config_value;
		} else if (cmd_no == 1) {
			dprintk("[%s] CMD 1 , noise_suppression_config_e.cfg = %d\n", __func__, config_value );
			noise_suppression_config_e.cfg = config_value;
		} else if (cmd_no == 2) {
			dprintk("[%s] CMD 2 , noise_suppression_config_e.calcfg = %d\n", __func__, config_value );
			noise_suppression_config_e.calcfg = config_value;
		} else if (cmd_no == 3) {
			dprintk("[%s] CMD 3 , noise_suppression_config_e.basefreq = %d\n", __func__, config_value );
			noise_suppression_config_e.basefreq = config_value;
		} else if (cmd_no == 4) {
			dprintk("[%s] CMD 4 , noise_suppression_config_e.mffreq[0] = %d\n", __func__, config_value );
			noise_suppression_config_e.mffreq[0] = config_value;
		} else if (cmd_no == 5) {
			dprintk("[%s] CMD 5 , noise_suppression_config_e.mffreq[1] = %d\n", __func__, config_value );
			noise_suppression_config_e.mffreq[1] = config_value;
		} else if (cmd_no == 6) {
			dprintk("[%s] CMD 6 , noise_suppression_config_e.gcactvinvldadcs = %d\n", __func__, config_value );
			noise_suppression_config_e.gcactvinvldadcs = config_value;
		} else if (cmd_no == 7) {
			dprintk("[%s] CMD 7 , noise_suppression_config_e.gcidlenivldadcs = %d\n", __func__, config_value );
			noise_suppression_config_e.gcidlenivldadcs = config_value;
		} else if (cmd_no == 8) {
			dprintk("[%s] CMD 8 , noise_suppression_config_e.gcmaxadcsperx = %d\n", __func__, config_value );
			noise_suppression_config_e.gcmaxadcsperx = config_value;
		} else if (cmd_no == 9) {
			dprintk("[%s] CMD 9 , noise_suppression_config_e.gclimitmin = %d\n", __func__, config_value );
			noise_suppression_config_e.gclimitmin = config_value;
		} else if (cmd_no == 10) {
			dprintk("[%s] CMD 10 , noise_suppression_config_e.gclimitmax = %d\n", __func__, config_value );
			noise_suppression_config_e.gclimitmax = config_value;
		} else if (cmd_no == 11) {
			dprintk("[%s] CMD 11 , noise_suppression_config_e.gccountmintgt = %d\n", __func__, config_value );
			noise_suppression_config_e.gccountmintgt = config_value;
		} else if (cmd_no == 12) {
			dprintk("[%s] CMD 12 , noise_suppression_config_e.mfinvlddiffthr = %d\n", __func__, config_value );
			noise_suppression_config_e.mfinvlddiffthr = config_value;
		} else if (cmd_no == 13) {
			dprintk("[%s] CMD 13 , noise_suppression_config_e.mfincadcspxthr = %d\n", __func__, config_value );
			noise_suppression_config_e.mfincadcspxthr = config_value;
		} else if (cmd_no == 14) {
			dprintk("[%s] CMD 14 , noise_suppression_config_e.mferrorthr = %d\n", __func__, config_value );
			noise_suppression_config_e.mferrorthr = config_value;
		} else if (cmd_no == 15) {
			dprintk("[%s] CMD 15 , noise_suppression_config_e.selfreqmax = %d\n", __func__, config_value );
			noise_suppression_config_e.selfreqmax = config_value;
		} else if (cmd_no == 16) {
			dprintk("[%s] CMD 16 , noise_suppression_config_e.blen = %d\n", __func__, config_value );
			noise_suppression_config_e.blen = config_value;
		} else if (cmd_no == 17) {
			dprintk("[%s] CMD 17 , noise_suppression_config_e.tchthr = %d\n", __func__, config_value );
			noise_suppression_config_e.tchthr = config_value;
		} else if (cmd_no == 18) {
			dprintk("[%s] CMD 18 , noise_suppression_config_e.tchdi = %d\n", __func__, config_value );
			noise_suppression_config_e.tchdi = config_value;
		} else if (cmd_no == 19) {
			dprintk("[%s] CMD 19 , noise_suppression_config_e.movhysti = %d\n", __func__, config_value );
			noise_suppression_config_e.movhysti = config_value;
		} else if (cmd_no == 20) {
			dprintk("[%s] CMD 20 , noise_suppression_config_e.movhystn = %d\n", __func__, config_value );
			noise_suppression_config_e.movhystn = config_value;
		} else if (cmd_no == 21) {
			dprintk("[%s] CMD 21 , noise_suppression_config_e.movfilter = %d\n", __func__, config_value );
			noise_suppression_config_e.movfilter = config_value;
		} else if (cmd_no == 22) {
			dprintk("[%s] CMD 22 , noise_suppression_config_e.numtouch = %d\n", __func__, config_value );
			noise_suppression_config_e.numtouch = config_value;
		} else if (cmd_no == 23) {
			dprintk("[%s] CMD 23 , noise_suppression_config_e.mrghyst = %d\n", __func__, config_value );
			noise_suppression_config_e.mrghyst = config_value;
		} else if (cmd_no == 24) {
			dprintk("[%s] CMD 24 , noise_suppression_config_e.mrgthr = %d\n", __func__, config_value );
			noise_suppression_config_e.mrgthr = config_value;
		} else if (cmd_no == 25) {
			dprintk("[%s] CMD 25 , noise_suppression_config_e.xloclip = %d\n", __func__, config_value );
			noise_suppression_config_e.xloclip = config_value;
		} else if (cmd_no == 26) {
			dprintk("[%s] CMD 26 , noise_suppression_config_e.xhiclip = %d\n", __func__, config_value );
			noise_suppression_config_e.xhiclip = config_value;
		} else if (cmd_no == 27) {
			dprintk("[%s] CMD 27 , noise_suppression_config_e.yloclip = %d\n", __func__, config_value );
			noise_suppression_config_e.yloclip = config_value;
		} else if (cmd_no == 28) {
			dprintk("[%s] CMD 28 , noise_suppression_config_e.yhiclip = %d\n", __func__, config_value );
			noise_suppression_config_e.yhiclip = config_value;
		} else if (cmd_no == 29) {
			dprintk("[%s] CMD 29 , noise_suppression_config_e.xedgectrl = %d\n", __func__, config_value );
			noise_suppression_config_e.xedgectrl = config_value;
		} else if (cmd_no == 30) {
			dprintk("[%s] CMD 30 , noise_suppression_config_e.xedgedist = %d\n", __func__, config_value );
			noise_suppression_config_e.xedgedist = config_value;
		} else if (cmd_no == 31) {
			dprintk("[%s] CMD 31 , noise_suppression_config_e.yedgectrl = %d\n", __func__, config_value );
			noise_suppression_config_e.yedgectrl = config_value;
		} else if (cmd_no == 32) {
			dprintk("[%s] CMD 32 , noise_suppression_config_e.yedgedist = %d\n", __func__, config_value );
			noise_suppression_config_e.yedgedist = config_value;
		} else if (cmd_no == 33) {
			dprintk("[%s] CMD 33 , noise_suppression_config_e.jumplimit = %d\n", __func__, config_value );
			noise_suppression_config_e.jumplimit = config_value;
		} else if (cmd_no == 34) {
			dprintk("[%s] CMD 34 , noise_suppression_config_e.tchhyst = %d\n", __func__, config_value );
			noise_suppression_config_e.tchhyst = config_value;
		} else if (cmd_no == 35) {
			dprintk("[%s] CMD 35 , noise_suppression_config_e.nexttchdi = %d\n", __func__, config_value );
			noise_suppression_config_e.nexttchdi = config_value;
		} else
			dprintk("[%s] unknown CMD\n", __func__);

		/* Write Noise suppression config to chip. */
		if (get_object_address(PROCG_NOISESUPPRESSION_T48, 0) != OBJECT_NOT_FOUND) {
			if (write_noisesuppression_config_e(0, noise_suppression_config_e) != CFG_WRITE_OK)
				dprintk("[TSP] Configuration Fail!!! , Line %d \n\r", __LINE__);
		}
	}
	return size;
}
static DEVICE_ATTR(set_noise, 0664, set_noise_show, set_noise_store);


static ssize_t set_grip_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	printk(KERN_DEBUG "[TSP] %s : operate nothing\n", __FUNCTION__);

	return 0;
}
static ssize_t set_grip_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t size)
{
	int cmd_no,config_value = 0;
	char *after;

	unsigned long value = simple_strtoul(buf, &after, 10);    
	cmd_no = (int) (value / 1000);
	config_value = (int) (value % 1000 );

	if (info_block->info_id.family_id == 0x80) {
		if (cmd_no == 0) {
			dprintk("[%s] CMD 0 , gripfacesuppression_config.ctrl = %d\n", __func__, config_value );
			gripfacesuppression_config.ctrl = config_value;
		} else if (cmd_no == 1) {
			dprintk("[%s] CMD 1 , gripfacesuppression_config.xlogrip = %d\n", __func__, config_value );
			gripfacesuppression_config.xlogrip = config_value;
		} else if (cmd_no == 2) {
			dprintk("[%s] CMD 2 , gripfacesuppression_config.xhigrip = %d\n", __func__, config_value );
			gripfacesuppression_config.xhigrip = config_value;
		} else if (cmd_no == 3) {
			dprintk("[%s] CMD 3 , gripfacesuppression_config.ylogrip = %d\n", __func__, config_value );
			gripfacesuppression_config.ylogrip = config_value;
		} else if (cmd_no == 4) {
			dprintk("[%s] CMD 4 , gripfacesuppression_config.yhigrip = %d\n", __func__, config_value );
			gripfacesuppression_config.yhigrip = config_value;
		} else if (cmd_no == 5) {
			dprintk("[%s] CMD 5 , gripfacesuppression_config.maxtchs = %d\n", __func__, config_value );
			gripfacesuppression_config.maxtchs = config_value;
		} else if (cmd_no == 6) {
			dprintk("[%s] CMD 6 , gripfacesuppression_config.reserved = %d\n", __func__, config_value );
			gripfacesuppression_config.reserved = config_value;
		} else if (cmd_no == 7) {
			dprintk("[%s] CMD 7 , gripfacesuppression_config.szthr1 = %d\n", __func__, config_value );
			gripfacesuppression_config.szthr1 = config_value;
		} else if (cmd_no == 8) {
			dprintk("[%s] CMD 8 , gripfacesuppression_config.szthr2 = %d\n", __func__, config_value );
			gripfacesuppression_config.szthr2 = config_value;
		} else if (cmd_no == 9) {
			dprintk("[%s] CMD 9 , gripfacesuppression_config.shpthr1 = %d\n", __func__, config_value );
			gripfacesuppression_config.shpthr1 = config_value;
		} else if (cmd_no == 10) {
			dprintk("[%s] CMD 10 , gripfacesuppression_config.shpthr2 = %d\n", __func__, config_value );
			gripfacesuppression_config.shpthr2 = config_value;
		} else
			dprintk("[%s] unknown CMD\n", __func__);

		/* Write grip suppression config to chip. */
		if (get_object_address(PROCI_GRIPFACESUPPRESSION_T20, 0) != OBJECT_NOT_FOUND) {
			if (write_gripsuppression_config(0, gripfacesuppression_config) != CFG_WRITE_OK)
				dprintk("[TSP] Configuration Fail!!! , Line %d \n\r", __LINE__);
		}
	} else if (info_block->info_id.family_id == 0x81) {
		if (cmd_no == 0) {
			dprintk("[%s] CMD 0 , grip_suppression_config.ctrl = %d\n", __func__, config_value );
			grip_suppression_config.ctrl = config_value;
		} else if (cmd_no == 1) {
			dprintk("[%s] CMD 1 , grip_suppression_config.xlogrip = %d\n", __func__, config_value );
			grip_suppression_config.xlogrip = config_value;
		} else if (cmd_no == 2) {
			dprintk("[%s] CMD 2 , grip_suppression_config.xhigrip = %d\n", __func__, config_value );
			grip_suppression_config.xhigrip = config_value;
		} else if (cmd_no == 3) {
			dprintk("[%s] CMD 3 , grip_suppression_config.ylogrip = %d\n", __func__, config_value );
			grip_suppression_config.ylogrip = config_value;
		} else if (cmd_no == 4) {
			dprintk("[%s] CMD 4 , grip_suppression_config.yhigrip = %d\n", __func__, config_value );
			grip_suppression_config.yhigrip = config_value;
		} else
			dprintk("[%s] unknown CMD\n", __func__);

		/* Write grip suppression config to chip. */
		if (get_object_address(PROCI_GRIPSUPPRESSION_T40, 0) != OBJECT_NOT_FOUND) {
			if (write_grip_suppression_config(grip_suppression_config) != CFG_WRITE_OK)
				dprintk("[TSP] Configuration Fail!!! , Line %d \n\r", __LINE__);
		}
	}
	return size;
}
static DEVICE_ATTR(set_grip, 0664, set_grip_show, set_grip_store);


static ssize_t set_touchsuppression_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	printk(KERN_DEBUG "[TSP] %s : operate nothing\n", __FUNCTION__);

	return 0;
}
static ssize_t set_touchsuppression_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t size)
{
	int cmd_no,config_value = 0;
	char *after;

	unsigned long value = simple_strtoul(buf, &after, 10);    
	cmd_no = (int) (value / 1000);
	config_value = (int) (value % 1000 );

	if (info_block->info_id.family_id == 0x80) {
		dprintk("[%s] unknown CMD\n", __func__);
		dprintk("not support register of mxt224\n");
		return size;
	} else if (info_block->info_id.family_id == 0x81) {
		if (cmd_no == 0) {
			dprintk("[%s] CMD 0 , touch_suppression_config.ctrl = %d\n", __func__, config_value );
			touch_suppression_config.ctrl = config_value;
		} else if (cmd_no == 1) {
			dprintk("[%s] CMD 1 , touch_suppression_config.apprthr = %d\n", __func__, config_value );
			touch_suppression_config.apprthr = config_value;
		} else if (cmd_no == 2) {
			dprintk("[%s] CMD 2 , touch_suppression_config.maxapprarea = %d\n", __func__, config_value );
			touch_suppression_config.maxapprarea = config_value;
		} else if (cmd_no == 3) {
			dprintk("[%s] CMD 3 , touch_suppression_config.maxtcharea = %d\n", __func__, config_value );
			touch_suppression_config.maxtcharea = config_value;
		} else if (cmd_no == 4) {
			dprintk("[%s] CMD 4 , touch_suppression_config.supstrength = %d\n", __func__, config_value );
			touch_suppression_config.supstrength =  config_value;
		} else if (cmd_no == 5) {
			dprintk("[%s] CMD 5 , touch_suppression_config.supextto = %d\n", __func__, config_value );
			touch_suppression_config.supextto = config_value;
		} else if (cmd_no == 6) {
			dprintk("[%s] CMD 6 , touch_suppression_config.maxtcharea = %d\n", __func__, config_value );
			touch_suppression_config.maxtcharea = config_value;
		} else if (cmd_no == 7) {
			dprintk("[%s] CMD 7 , touch_suppression_config.supstrength = %d\n", __func__, config_value );
			touch_suppression_config.supstrength = config_value;
		} else
			dprintk("[%s] unknown CMD\n", __func__);

		/* Write touch suppression config to chip. */
		if (get_object_address(PROCI_TOUCHSUPPRESSION_T42, 0) != OBJECT_NOT_FOUND) {
			if (write_touch_suppression_config(touch_suppression_config) != CFG_WRITE_OK)
				dprintk("[TSP] Configuration Fail!!! , Line %d \n\r", __LINE__);
		}
	}
	return size;
}
static DEVICE_ATTR(set_touchsuppression, 0664, set_touchsuppression_show, set_touchsuppression_store);


static ssize_t set_stylus_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	printk(KERN_DEBUG "[TSP] %s : operate nothing\n", __FUNCTION__);

	return 0;
}
static ssize_t set_stylus_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t size)
{
	int cmd_no,config_value = 0;
	char *after;

	unsigned long value = simple_strtoul(buf, &after, 10);    
	cmd_no = (int) (value / 1000);
	config_value = (int) (value % 1000 );

	if (info_block->info_id.family_id == 0x80) {
		dprintk("[%s] unknown CMD\n", __func__);
		dprintk("not support register of mxt224\n");
		return size;
	} else if (info_block->info_id.family_id == 0x81) {
		if (cmd_no == 0) {
			dprintk("[%s] CMD 0 , stylus_config.ctrl = %d\n", __func__, config_value );
			stylus_config.ctrl = config_value;
		} else if (cmd_no == 1) {
			dprintk("[%s] CMD 1 , stylus_config.contmin = %d\n", __func__, config_value );
			stylus_config.contmin = config_value;
		} else if (cmd_no == 2) {
			dprintk("[%s] CMD 2 , stylus_config.contmax = %d\n", __func__, config_value );
			stylus_config.contmax = config_value;
		} else if (cmd_no == 3) {
			dprintk("[%s] CMD 3 , stylus_config.stability = %d\n", __func__, config_value );
			stylus_config.stability = config_value;
		} else if (cmd_no == 4) {
			dprintk("[%s] CMD 4 , stylus_config.maxtcharea = %d\n", __func__, config_value );
			stylus_config.maxtcharea =  config_value;
		} else if (cmd_no == 5) {
			dprintk("[%s] CMD 5 , stylus_config.amplthr = %d\n", __func__, config_value );
			stylus_config.amplthr = config_value;
		} else if (cmd_no == 6) {
			dprintk("[%s] CMD 6 , stylus_config.styshape = %d\n", __func__, config_value );
			stylus_config.styshape = config_value;
		} else if (cmd_no == 7) {
			dprintk("[%s] CMD 7 , stylus_config.hoversup = %d\n", __func__, config_value );
			stylus_config.hoversup = config_value;
		} else if (cmd_no == 8) {
			dprintk("[%s] CMD 8 , stylus_config.confthr = %d\n", __func__, config_value );
			stylus_config.confthr = config_value;
		} else if (cmd_no == 9) {
			dprintk("[%s] CMD 9 , stylus_config.syncsperx = %d\n", __func__, config_value );
			stylus_config.syncsperx = config_value;
		} else
			dprintk("[%s] unknown CMD\n", __func__);

		/* Write stylus config to chip. */
		if (get_object_address(PROCI_STYLUS_T47, 0) !=  OBJECT_NOT_FOUND) {
			if (write_stylus_config(stylus_config) != CFG_WRITE_OK)
				dprintk("[TSP] Configuration Fail!!! , Line %d \n\r", __LINE__);
		}
	}
	return size;
}
static DEVICE_ATTR(set_stylus, 0664, set_stylus_show, set_stylus_store);


static ssize_t set_cte_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	printk(KERN_DEBUG "[TSP] %s : operate nothing\n", __FUNCTION__);

	return 0;
}
static ssize_t set_cte_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t size)
{
	int cmd_no,config_value = 0;
	char *after;

	unsigned long value = simple_strtoul(buf, &after, 10);    
	printk(KERN_DEBUG "[TSP] %s\n", __FUNCTION__);
	cmd_no = (int) (value / 1000);
	config_value = (int) (value % 1000 );

	if (info_block->info_id.family_id == 0x80) {
		if (cmd_no == 0) {
			dprintk("[%s] CMD 0 , cte_config.ctrl = %d\n", __func__, config_value );
			cte_config.ctrl = config_value;
		} else if (cmd_no == 1) {
			dprintk("[%s] CMD 1 , cte_config.cmd = %d\n", __func__, config_value );
			cte_config.cmd = config_value;
		} else if (cmd_no == 2) {
			dprintk("[%s] CMD 2 , cte_config.mode = %d\n", __func__, config_value );
			cte_config.mode = config_value;
		} else if (cmd_no == 3) {
			dprintk("[%s] CMD 3 , cte_config.idlegcafdepth = %d\n", __func__, config_value );
			cte_config.idlegcafdepth= config_value;
		} else if (cmd_no == 4) {
			dprintk("[%s] CMD 4 , cte_config.actvgcafdepth = %d\n", __func__, config_value );
			cte_config.actvgcafdepth = config_value;
		} else if (cmd_no == 5) {
			dprintk("[%s] CMD 5 , cte_config.voltage = %d\n", __func__, config_value );
			cte_config.voltage = config_value;
		} else
			dprintk("[%s] unknown CMD\n", __func__);

		if (write_CTE_config(cte_config) != CFG_WRITE_OK)
			dprintk("[TSP] Configuration Fail!!! , Line %d \n\r", __LINE__);
	}else if (info_block->info_id.family_id == 0x81) {
		if (cmd_no == 0) {
			dprintk("[%s] CMD 0 , cte_config_e.ctrl = %d\n", __func__, config_value );
			cte_config_e.ctrl = config_value;
		} else if (cmd_no == 1) {
			dprintk("[%s] CMD 1 , cte_config_e.mode = %d\n", __func__, config_value );
			cte_config_e.mode = config_value;
		} else if (cmd_no == 2) {
			dprintk("[%s] CMD 2 , cte_config_e.idlesyncsperx = %d\n", __func__, config_value );
			cte_config_e.idlesyncsperx = config_value;
		} else if (cmd_no == 3) {
			dprintk("[%s] CMD 3 , cte_config_e.actvsyncsperx = %d\n", __func__, config_value );
			cte_config_e.actvsyncsperx = config_value;
		} else if (cmd_no == 4) {
			dprintk("[%s] CMD 4 , cte_config_e.adcspersync = %d\n", __func__, config_value );
			cte_config_e.adcspersync = config_value;
		} else if (cmd_no == 5) {
			dprintk("[%s] CMD 5 , cte_config_e.pulsesperadc = %d\n", __func__, config_value );
			cte_config_e.pulsesperadc = config_value;
		} else if (cmd_no == 6) {
			dprintk("[%s] CMD 6 , cte_config_e.xslew = %d\n", __func__, config_value );
			cte_config_e.xslew = config_value;
		} else if (cmd_no == 7) {
			dprintk("[%s] CMD 7 , cte_config_e.syncdelay = %d\n", __func__, config_value );
			cte_config_e.syncdelay = config_value;
		} else
			dprintk("[%s] unknown CMD\n", __func__);

		if (write_CTE_config_e(0, cte_config_e) != CFG_WRITE_OK)
			dprintk("[TSP] Configuration Fail!!! , Line %d \n\r", __LINE__);
	}

    return size;
}
static DEVICE_ATTR(set_cte, 0664, set_cte_show, set_cte_store);


static ssize_t set_total_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	printk(KERN_DEBUG "[TSP] %s : operate nothing\n", __FUNCTION__);

	return 0;
}
static ssize_t set_total_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t size)
{
	int cmd_no,config_value = 0;
	char *after;

	unsigned long value = simple_strtoul(buf, &after, 10);    
	printk(KERN_DEBUG "[TSP] %s\n", __FUNCTION__);
	cmd_no = (int) (value / 1000);
	config_value = (int) (value % 1000 );

	if (info_block->info_id.family_id == 0x80) {
		if (cmd_no == 0) {
			dprintk("[%s] CMD 0 , twotouch_gesture_config.ctrl = %d\n", __func__, config_value );
			twotouch_gesture_config.ctrl = config_value;
		} else if (cmd_no == 1) {
			dprintk("[%s] CMD 1 , onetouch_gesture_config.ctrl = %d\n", __func__, config_value );
			onetouch_gesture_config.ctrl = config_value;
		} else if (cmd_no == 2) {
			dprintk("[%s] CMD 2 , noise_suppression_config.ctrl = %d\n", __func__, config_value );
			noise_suppression_config.ctrl = config_value;
		} else if (cmd_no == 3) {
			dprintk("[%s] CMD 3 , selftest_config.ctrl = %d\n", __func__, config_value );
			selftest_config.ctrl = config_value;
		} else if (cmd_no == 4) {
			dprintk("[%s] CMD 4 , gripfacesuppression_config.ctrl = %d\n", __func__, config_value );
			gripfacesuppression_config.ctrl = config_value;
		} else if (cmd_no == 5) {
			dprintk("[%s] CMD 5 , cte_config.ctrl = %d\n", __func__, config_value );
			cte_config.ctrl = config_value;
		} else if (cmd_no == 6) {
			dprintk("[%s] CMD 6 , cte_config.idlegcafdepth= %d\n", __func__, config_value );
			cte_config.idlegcafdepth = config_value;
		} else if (cmd_no == 7) {
			dprintk("[%s] CMD 7 , cte_config.actvgcafdepth = %d\n", __func__, config_value );
			cte_config.actvgcafdepth = config_value;
		} else
			dprintk("[%s] unknown CMD\n", __func__);
	} else if (info_block->info_id.family_id == 0x81) {
		if (cmd_no == 0) {
			dprintk("[%s] CMD 0 , grip_suppression_config.ctrl = %d\n", __func__, config_value );
			grip_suppression_config.ctrl = config_value;
		} else if (cmd_no == 1) {
			dprintk("[%s] CMD 1 , touch_suppression_config.ctrl = %d\n", __func__, config_value );
			touch_suppression_config.ctrl = config_value;
		} else if (cmd_no == 2) {
			dprintk("[%s] CMD 2 , noise_suppression_config_e.ctrl = %d\n", __func__, config_value );
			noise_suppression_config_e.ctrl = config_value;
		} else if (cmd_no == 3) {
			dprintk("[%s] CMD 3 , selftest_config.ctrl = %d\n", __func__, config_value );
			selftest_config.ctrl = config_value;
		} else if (cmd_no == 4) {
			dprintk("[%s] CMD 4 , stylus_config.ctrl = %d\n", __func__, config_value );
			stylus_config.ctrl = config_value;
		} else if (cmd_no == 5) {
			dprintk("[%s] CMD 5 , cte_config_e.ctrl = %d\n", __func__, config_value );
			cte_config_e.ctrl = config_value;
		} else
			dprintk("[%s] unknown CMD\n", __func__);
	}

	return size;
}
static DEVICE_ATTR(set_total, 0664, set_total_show, set_total_store);

static ssize_t set_write_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	if (info_block->info_id.family_id == 0x80) {
		dprintk("power_config.idleacqint = %d\n", power_config.idleacqint);
		dprintk("power_config.actvacqint = %d\n", power_config.actvacqint);
		dprintk("power_config.actv2idleto= %d\n", power_config.actv2idleto);

		if (write_power_config(power_config) != CFG_WRITE_OK) {
			/* "Power config write failed!\n" */
			dprintk("\n[TSP][ERROR] config erroe line : %d\n", __LINE__);
			return CFG_WRITE_FAILED;
		}

		dprintk("acquisition_config.chrgtime = %d\n", acquisition_config.chrgtime);
		dprintk("acquisition_config.tchdrift = %d\n", acquisition_config.tchdrift);
		dprintk("acquisition_config.driftst = %d\n", acquisition_config.driftst);
		dprintk("acquisition_config.tchautocal = %d\n", acquisition_config.tchautocal);
		dprintk("acquisition_config.sync = %d\n", acquisition_config.sync);
		dprintk("acquisition_config.atchcalst = %d\n", acquisition_config.atchcalst);
		dprintk("acquisition_config.atchcalsthr = %d\n", acquisition_config.atchcalsthr);

		/* Write acquisition config to chip. */
		if (write_acquisition_config(acquisition_config) != CFG_WRITE_OK) {
			/* "Acquisition config write failed!\n" */
			dprintk("\n[TSP][ERROR] line : %d\n", __LINE__);
			return CFG_WRITE_FAILED;
		}

		dprintk("0 , touchscreen_config.ctrl = %d\n",  touchscreen_config.ctrl);
		dprintk("1 , touchscreen_config.xorigin = %d\n", touchscreen_config.xorigin);
		dprintk("2 , touchscreen_config.yorigin = %d\n",  touchscreen_config.yorigin);
		dprintk("3 , touchscreen_config.xsize = %d\n",touchscreen_config.xsize);
		dprintk("4 , touchscreen_config.ysize = %d\n", touchscreen_config.ysize);
		dprintk("5 , touchscreen_config.akscfg = %d\n", touchscreen_config.akscfg);
		dprintk("6 , touchscreen_config.blen = %d\n", touchscreen_config.blen);
		dprintk("7 , touchscreen_config.tchthr = %d\n",  touchscreen_config.tchthr);
		dprintk("8 , touchscreen_config.tchdi = %d\n",touchscreen_config.tchdi);
		dprintk("9 , touchscreen_config.orient = %d\n", touchscreen_config.orient);
		dprintk("10 , touchscreen_config.mrgtimeout = %d\n",touchscreen_config.mrgtimeout);
		dprintk("11 , touchscreen_config.movhysti = %d\n",touchscreen_config.movhysti);
		dprintk("12 , touchscreen_config.movhystn = %d\n",touchscreen_config.movhystn);
		dprintk("13 , touchscreen_config.movfilter = %d\n",touchscreen_config.movfilter);
		dprintk("14 , touchscreen_config.numtouch = %d\n",touchscreen_config.numtouch);
		dprintk("15 , touchscreen_config.mrghyst = %d\n",touchscreen_config.mrghyst);
		dprintk("16 , touchscreen_config.mrgthr = %d\n",touchscreen_config.mrgthr);
		dprintk("17 , touchscreen_config.amphyst = %d\n",touchscreen_config.amphyst);
		dprintk("18 , touchscreen_config.xrange = %d\n",touchscreen_config.xrange);
		dprintk("19 , touchscreen_config.yrange = %d\n",touchscreen_config.yrange);
		dprintk("20 , touchscreen_config.xloclip = %d\n",touchscreen_config.xloclip);
		dprintk("21 , touchscreen_config.xhiclip = %d\n",touchscreen_config.xhiclip);
		dprintk("22 , touchscreen_config.yloclip = %d\n",touchscreen_config.yloclip);
		dprintk("23 , touchscreen_config.yhiclip = %d\n",touchscreen_config.yhiclip);
		dprintk("24 , touchscreen_config.xedgectrl = %d\n",touchscreen_config.xedgectrl);
		dprintk("25 , touchscreen_config.xedgedist = %d\n",touchscreen_config.xedgedist);
		dprintk("26 , touchscreen_config.yedgectrl = %d\n",touchscreen_config.yedgectrl);
		dprintk("27 , touchscreen_config.yedgedist = %d\n",touchscreen_config.yedgedist);
		if (tsp_version >= 0x16)
			dprintk("28 , touchscreen_config.jumplimit = %d\n",touchscreen_config.jumplimit);
		else
			dprintk("28 , touchscreen_config.jumplimit is not supported in this version.\n" );

		/* Write touchscreen (1st instance) config to chip. */
		if (write_multitouchscreen_config(0, touchscreen_config) != CFG_WRITE_OK) {
			/* "Multitouch screen config write failed!\n" */
			dprintk("\n[TSP][ERROR] line : %d\n", __LINE__);
			return CFG_WRITE_FAILED;
		}

		dprintk("0 , keyarray_config.ctrl = %d\n", keyarray_config.ctrl);
		dprintk("1 , keyarray_config.xorigin = %d\n", keyarray_config.xorigin);
		dprintk("2 , keyarray_config.xsize = %d\n",keyarray_config.xsize);
		dprintk("3 , keyarray_config.yorigin = %d\n", keyarray_config.yorigin); 
		dprintk("4 , keyarray_config.ysize = %d\n", keyarray_config.ysize);
		dprintk("5 , keyarray_config.akscfg = %d\n", keyarray_config.akscfg);
		dprintk("6 , keyarray_config.blen = %d\n", keyarray_config.blen);
		dprintk("7 , keyarray_config.tchthr = %d\n", keyarray_config.tchthr);
		dprintk("8 , keyarray_config.tchdi = %d\n", keyarray_config.tchdi);

		/* Write key array (1st instance) config to chip. */
		if (write_keyarray_config(0, keyarray_config) != CFG_WRITE_OK) {
			/* "Key array config write failed!\n" */
			dprintk("\n[TSP][ERROR] line : %d\n", __LINE__);
			return CFG_WRITE_FAILED;
		}

		//dprintk("0 , linearization_config.ctrl = %d\n", linearization_config.ctrl );
		dprintk("0 , twotouch_gesture_config.ctrl = %d\n", twotouch_gesture_config.ctrl);
		dprintk("1 , onetouch_gesture_config.ctrl = %d\n",onetouch_gesture_config.ctrl);
		dprintk("2 , noise_suppression_config.ctrl = %d\n", noise_suppression_config.ctrl); 
		dprintk("3 , selftest_config.ctrl = %d\n", selftest_config.ctrl);
		dprintk("4 , gripfacesuppression_config.ctrl = %d\n", gripfacesuppression_config.ctrl);
		dprintk("5 , cte_config.ctrl = %d\n", cte_config.ctrl);
		dprintk("6 , cte_config.idlegcafdepth = %d\n", cte_config.idlegcafdepth);
		dprintk("7 , cte_config.actvgcafdepth = %d\n", cte_config.actvgcafdepth);

		/* Write twotouch gesture config to chip. */
		if (get_object_address(PROCI_TWOTOUCHGESTUREPROCESSOR_T27, 0) != OBJECT_NOT_FOUND) {
			if (write_twotouchgesture_config(0, twotouch_gesture_config) != CFG_WRITE_OK) {
				/* "Two touch gesture config write failed!\n" */
				dprintk("\n[TSP][ERROR] line : %d\n", __LINE__);
				return CFG_WRITE_FAILED;
			}
		}

		/* Write one touch gesture config to chip. */
		if (get_object_address(PROCI_ONETOUCHGESTUREPROCESSOR_T24, 0) != OBJECT_NOT_FOUND) {
			if (write_onetouchgesture_config(0, onetouch_gesture_config) != CFG_WRITE_OK) {
				/* "One touch gesture config write failed!\n" */
				dprintk("\n[TSP][ERROR] line : %d\n", __LINE__);
				return CFG_WRITE_FAILED;
			}
		}

		dprintk("gripfacesuppression_config.ctrl = %d\n", gripfacesuppression_config.ctrl);
		dprintk("gripfacesuppression_config.xlogrip = %d\n", gripfacesuppression_config.xlogrip);
		dprintk("gripfacesuppression_config.xhigrip = %d\n", gripfacesuppression_config.xhigrip);
		dprintk("gripfacesuppression_config.ylogrip = %d\n", gripfacesuppression_config.ylogrip);
		dprintk("gripfacesuppression_config.yhigrip = %d\n", gripfacesuppression_config.yhigrip);
		dprintk("gripfacesuppression_config.maxtchs = %d\n", gripfacesuppression_config.maxtchs);
		dprintk("gripfacesuppression_config.reserved = %d\n", gripfacesuppression_config.reserved);
		dprintk("gripfacesuppression_config.szthr1 = %d\n", gripfacesuppression_config.szthr1);
		dprintk("gripfacesuppression_config.szthr2 = %d\n", gripfacesuppression_config.szthr2);
		dprintk("gripfacesuppression_config.shpthr1 = %d\n", gripfacesuppression_config.shpthr1);
		dprintk("gripfacesuppression_config.shpthr2 = %d\n", gripfacesuppression_config.shpthr2);

		/* Write grip suppression config to chip. */
		if (get_object_address(PROCI_GRIPFACESUPPRESSION_T20, 0) != OBJECT_NOT_FOUND) {
			if (write_gripsuppression_config(0, gripfacesuppression_config) != CFG_WRITE_OK) {
				/* "Grip suppression config write failed!\n" */
				dprintk("\n[TSP][ERROR] line : %d\n", __LINE__);
				return CFG_WRITE_FAILED;
			}
		}

		dprintk("noise_suppression_config.ctrl = %d\n", noise_suppression_config.ctrl);
		dprintk("noise_suppression_config.reserved = %d\n", noise_suppression_config.reserved);
		dprintk("noise_suppression_config.reserved1 = %d\n", noise_suppression_config.reserved1);
		dprintk("noise_suppression_config.gcaful = %d\n", noise_suppression_config.gcaful);
		dprintk("noise_suppression_config.gcafll = %d\n", noise_suppression_config.gcafll);
		dprintk("noise_suppression_config.actvgcafvalid = %d\n", noise_suppression_config.actvgcafvalid);
		dprintk("noise_suppression_config.noisethr = %d\n", noise_suppression_config.noisethr);
		dprintk("noise_suppression_config.freqhopscale = %d\n", noise_suppression_config.freqhopscale);
		dprintk("noise_suppression_config.freq[0] = %d\n", noise_suppression_config.freq[0]);
		dprintk("noise_suppression_config.freq[1] = %d\n", noise_suppression_config.freq[1]);
		dprintk("noise_suppression_config.freq[2] = %d\n", noise_suppression_config.freq[2]);
		dprintk("noise_suppression_config.freq[3] = %d\n", noise_suppression_config.freq[3]);
		dprintk("noise_suppression_config.freq[4] = %d\n", noise_suppression_config.freq[4]);
		dprintk("noise_suppression_config.idlegcafvalid = %d\n", noise_suppression_config.idlegcafvalid);

		/* Write Noise suppression config to chip. */
		if (get_object_address(PROCG_NOISESUPPRESSION_T22, 0) != OBJECT_NOT_FOUND) {
			if (write_noisesuppression_config(0, noise_suppression_config) != CFG_WRITE_OK) {
				dprintk("\n[TSP][ERROR] line : %d\n", __LINE__);
				return CFG_WRITE_FAILED;
			}
		}

		dprintk("0 , cte_config.ctrl = %d\n", cte_config.ctrl);
		dprintk("1 , cte_config.cmd = %d\n", cte_config.cmd);
		dprintk("2 , cte_config.mode = %d\n", cte_config.mode);
		dprintk("3 , cte_config.idlegcafdepth = %d\n", cte_config.idlegcafdepth);
		dprintk("4 , cte_config.actvgcafdepth = %d\n", cte_config.actvgcafdepth);
		dprintk("5 , cte_config.voltage = %d\n", cte_config.voltage);

		/* Write CTE config to chip. */
		if (get_object_address(SPT_CTECONFIG_T28, 0) != OBJECT_NOT_FOUND) {
			if (write_CTE_config(cte_config) != CFG_WRITE_OK) {
				/* "CTE config write failed!\n" */
				dprintk("\n[TSP][ERROR] line : %d\n", __LINE__);
				return CFG_WRITE_FAILED;
			}
		}
	} else if (info_block->info_id.family_id == 0x81) {
		dprintk("power_config.idleacqint = %d\n", power_config.idleacqint);
		dprintk("power_config.actvacqint = %d\n", power_config.actvacqint);
		dprintk("power_config.actv2idleto= %d\n", power_config.actv2idleto);

		if (write_power_config(power_config) != CFG_WRITE_OK) {
			/* "Power config write failed!\n" */
			dprintk("\n[TSP][ERROR] config erroe line : %d\n", __LINE__);
			return CFG_WRITE_FAILED;
		}

		dprintk("acquisition_config_e.chrgtime = %d\n", acquisition_config_e.chrgtime);
		dprintk("acquisition_config_e.tchdrift = %d\n", acquisition_config_e.tchdrift);
		dprintk("acquisition_config_e.driftst = %d\n", acquisition_config_e.driftst);
		dprintk("acquisition_config_e.tchautocal = %d\n", acquisition_config_e.tchautocal);
		dprintk("acquisition_config_e.sync = %d\n", acquisition_config_e.sync);
		dprintk("acquisition_config_e.atchcalst = %d\n", acquisition_config_e.atchcalst);
		dprintk("acquisition_config_e.atchcalsthr = %d\n", acquisition_config_e.atchcalsthr);
		dprintk("acquisition_config_e.atchfrccalthr = %d\n", acquisition_config_e.atchfrccalthr);
		dprintk("acquisition_config_e.atchfrccalratio = %d\n", acquisition_config_e.atchfrccalratio);

		/* Write acquisition config to chip. */
		if (write_acquisition_config_e(acquisition_config_e) != CFG_WRITE_OK) {
			/* "Acquisition config write failed!\n" */
			dprintk("\n[TSP][ERROR] line : %d\n", __LINE__);
			return CFG_WRITE_FAILED;
		}

		dprintk("0 , touchscreen_config_e.ctrl = %d\n", touchscreen_config_e.ctrl);
		dprintk("1 , touchscreen_config_e.xorigin = %d\n", touchscreen_config_e.xorigin);
		dprintk("2 , touchscreen_config_e.yorigin = %d\n", touchscreen_config_e.yorigin);
		dprintk("3 , touchscreen_config_e.xsize = %d\n", touchscreen_config_e.xsize);
		dprintk("4 , touchscreen_config_e.ysize = %d\n", touchscreen_config_e.ysize);
		dprintk("5 , touchscreen_config_e.akscfg = %d\n", touchscreen_config_e.akscfg);
		dprintk("6 , touchscreen_config_e.blen = %d\n", touchscreen_config_e.blen);
		dprintk("7 , touchscreen_config_e.tchthr = %d\n", touchscreen_config_e.tchthr);
		dprintk("8 , touchscreen_config_e.tchdi = %d\n", touchscreen_config_e.tchdi);
		dprintk("9 , touchscreen_config_e.orient = %d\n", touchscreen_config_e.orient);
		dprintk("10 , touchscreen_config_e.mrgtimeout = %d\n", touchscreen_config_e.mrgtimeout);
		dprintk("11 , touchscreen_config_e.movhysti = %d\n", touchscreen_config_e.movhysti);
		dprintk("12 , touchscreen_config_e.movhystn = %d\n", touchscreen_config_e.movhystn);
		dprintk("13 , touchscreen_config_e.movfilter = %d\n", touchscreen_config_e.movfilter);
		dprintk("14 , touchscreen_config_e.numtouch = %d\n", touchscreen_config_e.numtouch);
		dprintk("15 , touchscreen_config_e.mrghyst = %d\n", touchscreen_config_e.mrghyst);
		dprintk("16 , touchscreen_config_e.mrgthr = %d\n", touchscreen_config_e.mrgthr);
		dprintk("17 , touchscreen_config_e.amphyst = %d\n", touchscreen_config_e.amphyst);
		dprintk("18 , touchscreen_config_e.xrange = %d\n", touchscreen_config_e.xrange);
		dprintk("19 , touchscreen_config_e.yrange = %d\n", touchscreen_config_e.yrange);
		dprintk("20 , touchscreen_config_e.xloclip = %d\n", touchscreen_config_e.xloclip);
		dprintk("21 , touchscreen_config_e.xhiclip = %d\n", touchscreen_config_e.xhiclip);
		dprintk("22 , touchscreen_config_e.yloclip = %d\n", touchscreen_config_e.yloclip);
		dprintk("23 , touchscreen_config_e.yhiclip = %d\n", touchscreen_config_e.yhiclip);
		dprintk("24 , touchscreen_config_e.xedgectrl = %d\n", touchscreen_config_e.xedgectrl);
		dprintk("25 , touchscreen_config_e.xedgedist = %d\n", touchscreen_config_e.xedgedist);
		dprintk("26 , touchscreen_config_e.yedgectrl = %d\n", touchscreen_config_e.yedgectrl);
		dprintk("27 , touchscreen_config_e.yedgedist = %d\n", touchscreen_config_e.yedgedist);
		dprintk("28 , touchscreen_config_e.jumplimit = %d\n", touchscreen_config_e.jumplimit);
		dprintk("29 , touchscreen_config_e.tchhyst = %d\n", touchscreen_config_e.tchhyst);
		dprintk("30 , touchscreen_config_e.xpitch = %d\n", touchscreen_config_e.xpitch);
		dprintk("31 , touchscreen_config_e.ypitch = %d\n", touchscreen_config_e.ypitch);
		dprintk("32 , touchscreen_config_e.nexttchdi = %d\n", touchscreen_config_e.nexttchdi);

		/* Write touchscreen (1st instance) config to chip. */
		if (write_multitouchscreen_config_e(0, touchscreen_config_e) != CFG_WRITE_OK) {
			/* "Multitouch screen config write failed!\n" */
			dprintk("\n[TSP][ERROR] line : %d\n", __LINE__);
			return CFG_WRITE_FAILED;
		}

		dprintk("0 , keyarray_config.ctrl = %d\n", keyarray_config.ctrl);
		dprintk("1 , keyarray_config.xorigin = %d\n", keyarray_config.xorigin);
		dprintk("2 , keyarray_config.xsize = %d\n",keyarray_config.xsize);
		dprintk("3 , keyarray_config.yorigin = %d\n", keyarray_config.yorigin); 
		dprintk("4 , keyarray_config.ysize = %d\n", keyarray_config.ysize);
		dprintk("5 , keyarray_config.akscfg = %d\n", keyarray_config.akscfg);
		dprintk("6 , keyarray_config.blen = %d\n", keyarray_config.blen);
		dprintk("7 , keyarray_config.tchthr = %d\n", keyarray_config.tchthr);
		dprintk("8 , keyarray_config.tchdi = %d\n", keyarray_config.tchdi);

		/* Write key array (1st instance) config to chip. */
		if (write_keyarray_config(0, keyarray_config) != CFG_WRITE_OK) {
			/* "Key array config write failed!\n" */
			dprintk("\n[TSP][ERROR] line : %d\n", __LINE__);
			return CFG_WRITE_FAILED;
		}

		dprintk("0 , noise_suppression_config_e.ctrl = %d\n", noise_suppression_config_e.ctrl);
		dprintk("1 , noise_suppression_config_e.cfg = %d\n", noise_suppression_config_e.cfg);
		dprintk("2 , noise_suppression_config_e.calcfg = %d\n", noise_suppression_config_e.calcfg);
		dprintk("3 , noise_suppression_config_e.basefreq = %d\n", noise_suppression_config_e.basefreq);
		dprintk("4 , noise_suppression_config_e.mffreq[0] = %d\n", noise_suppression_config_e.mffreq[0]);
		dprintk("5 , noise_suppression_config_e.mffreq[1] = %d\n", noise_suppression_config_e.mffreq[1]);
		dprintk("6 , noise_suppression_config_e.gcactvinvldadcs = %d\n", noise_suppression_config_e.gcactvinvldadcs);
		dprintk("7 , noise_suppression_config_e.gcidlenivldadcs = %d\n", noise_suppression_config_e.gcidlenivldadcs);
		dprintk("8 , noise_suppression_config_e.gcmaxadcsperx = %d\n", noise_suppression_config_e.gcmaxadcsperx);
		dprintk("9 , noise_suppression_config_e.gclimitmin = %d\n", noise_suppression_config_e.gclimitmin);
		dprintk("10 , noise_suppression_config_e.gclimitmax = %d\n", noise_suppression_config_e.gclimitmax);
		dprintk("11 , noise_suppression_config_e.gccountmintgt = %d\n", noise_suppression_config_e.gccountmintgt);
		dprintk("12 , noise_suppression_config_e.mfinvlddiffthr = %d\n", noise_suppression_config_e.mfinvlddiffthr);
		dprintk("13 , noise_suppression_config_e.mfincadcspxthr = %d\n", noise_suppression_config_e.mfincadcspxthr);
		dprintk("14 , noise_suppression_config_e.mferrorthr = %d\n", noise_suppression_config_e.mferrorthr);
		dprintk("15 , noise_suppression_config_e.selfreqmax = %d\n", noise_suppression_config_e.selfreqmax);
		dprintk("16 , noise_suppression_config_e.blen = %d\n", noise_suppression_config_e.blen);
		dprintk("17 , noise_suppression_config_e.tchthr = %d\n", noise_suppression_config_e.tchthr);
		dprintk("18 , noise_suppression_config_e.tchdi = %d\n", noise_suppression_config_e.tchdi);
		dprintk("19 , noise_suppression_config_e.movhysti = %d\n", noise_suppression_config_e.movhysti);
		dprintk("20 , noise_suppression_config_e.movhystn = %d\n", noise_suppression_config_e.movhystn);
		dprintk("21 , noise_suppression_config_e.movfilter = %d\n", noise_suppression_config_e.movfilter);
		dprintk("22 , noise_suppression_config_e.numtouch = %d\n", noise_suppression_config_e.numtouch);
		dprintk("23 , noise_suppression_config_e.mrghyst = %d\n", noise_suppression_config_e.mrghyst);
		dprintk("24 , noise_suppression_config_e.mrgthr = %d\n", noise_suppression_config_e.mrgthr);
		dprintk("25 , noise_suppression_config_e.xloclip = %d\n", noise_suppression_config_e.xloclip);
		dprintk("26 , noise_suppression_config_e.xhiclip = %d\n", noise_suppression_config_e.xhiclip);
		dprintk("27 , noise_suppression_config_e.yloclip = %d\n", noise_suppression_config_e.yloclip);
		dprintk("28 , noise_suppression_config_e.yhiclip = %d\n", noise_suppression_config_e.yhiclip);
		dprintk("29 , noise_suppression_config_e.xedgectrl = %d\n", noise_suppression_config_e.xedgectrl);
		dprintk("30 , noise_suppression_config_e.xedgedist = %d\n", noise_suppression_config_e.xedgedist);
		dprintk("31 , noise_suppression_config_e.yedgectrl = %d\n", noise_suppression_config_e.yedgectrl);
		dprintk("32 , noise_suppression_config_e.yedgedist = %d\n", noise_suppression_config_e.yedgedist);
		dprintk("33 , noise_suppression_config_e.jumplimit = %d\n", noise_suppression_config_e.jumplimit);
		dprintk("34 , noise_suppression_config_e.tchhyst = %d\n", noise_suppression_config_e.tchhyst);
		dprintk("35 , noise_suppression_config_e.nexttchdi = %d\n", noise_suppression_config_e.nexttchdi);
	
		/* Write Noise suppression config to chip. */
		if (get_object_address(PROCG_NOISESUPPRESSION_T48, 0) != OBJECT_NOT_FOUND) {
			if (write_noisesuppression_config_e(0, noise_suppression_config_e) != CFG_WRITE_OK) {
				dprintk("\n[TSP][ERROR] line : %d\n", __LINE__);
				return CFG_WRITE_FAILED;
			}
		}

		dprintk("0 , grip_suppression_config.ctrl = %d\n", grip_suppression_config.ctrl);
		dprintk("1 , grip_suppression_config.xlogrip = %d\n", grip_suppression_config.xlogrip);
		dprintk("2 , grip_suppression_config.xhigrip = %d\n", grip_suppression_config.xhigrip);
		dprintk("3 , grip_suppression_config.ylogrip = %d\n", grip_suppression_config.ylogrip);
		dprintk("4 , grip_suppression_config.yhigrip = %d\n", grip_suppression_config.yhigrip);

		/* Write grip suppression config to chip. */
		if (get_object_address(PROCI_GRIPSUPPRESSION_T40, 0) != OBJECT_NOT_FOUND) {
			if (write_grip_suppression_config(grip_suppression_config) != CFG_WRITE_OK) {
				dprintk("\n[TSP][ERROR] line : %d\n", __LINE__);
				return CFG_WRITE_FAILED;
			}
		}

		dprintk("0 , touch_suppression_config.ctrl = %d\n", touch_suppression_config.ctrl);
		dprintk("1 , touch_suppression_config.apprthr = %d\n", touch_suppression_config.apprthr);
		dprintk("2 , touch_suppression_config.maxapprarea = %d\n", touch_suppression_config.maxapprarea);
		dprintk("3 , touch_suppression_config.maxtcharea = %d\n", touch_suppression_config.maxtcharea);
		dprintk("4 , touch_suppression_config.supstrength = %d\n", touch_suppression_config.supstrength);
		dprintk("5 , touch_suppression_config.supextto = %d\n", touch_suppression_config.supextto);
		dprintk("6 , touch_suppression_config.maxtcharea = %d\n", touch_suppression_config.maxtcharea);
		dprintk("7 , touch_suppression_config.supstrength = %d\n", touch_suppression_config.supstrength);

		/* Write touch suppression config to chip. */
		if (get_object_address(PROCI_TOUCHSUPPRESSION_T42, 0) != OBJECT_NOT_FOUND) {
			if (write_touch_suppression_config(touch_suppression_config) != CFG_WRITE_OK) {
				dprintk("\n[TSP][ERROR] line : %d\n", __LINE__);
				return CFG_WRITE_FAILED;
			}
		}

		dprintk("0 , stylus_config.ctrl = %d\n", stylus_config.ctrl);
		dprintk("1 , stylus_config.contmin = %d\n", stylus_config.contmin);
		dprintk("2 , stylus_config.contmax = %d\n", stylus_config.contmax);
		dprintk("3 , stylus_config.stability = %d\n", stylus_config.stability);
		dprintk("4 , stylus_config.maxtcharea = %d\n", stylus_config.maxtcharea);
		dprintk("5 , stylus_config.amplthr = %d\n", stylus_config.amplthr);
		dprintk("6 , stylus_config.styshape = %d\n", stylus_config.styshape);
		dprintk("7 , stylus_config.hoversup = %d\n", stylus_config.hoversup);
		dprintk("8 , stylus_config.confthr = %d\n", stylus_config.confthr);
		dprintk("9 , stylus_config.syncsperx = %d\n", stylus_config.syncsperx);

		/* Write stylus config to chip. */
		if (get_object_address(PROCI_STYLUS_T47, 0) !=	OBJECT_NOT_FOUND) {
			if (write_stylus_config(stylus_config) != CFG_WRITE_OK) {
				dprintk("\n[TSP][ERROR] line : %d\n", __LINE__);
				return CFG_WRITE_FAILED;
			}
		}

		dprintk("0 , cte_config_e.ctrl = %d\n", cte_config_e.ctrl);
		dprintk("1 , cte_config_e.mode = %d\n", cte_config_e.mode);
		dprintk("2 , cte_config_e.idlesyncsperx = %d\n", cte_config_e.idlesyncsperx);
		dprintk("3 , cte_config_e.actvsyncsperx = %d\n", cte_config_e.actvsyncsperx);
		dprintk("4 , cte_config_e.adcspersync = %d\n", cte_config_e.adcspersync);
		dprintk("5 , cte_config_e.pulsesperadc = %d\n", cte_config_e.pulsesperadc);
		dprintk("6 , cte_config_e.xslew = %d\n", cte_config_e.xslew);
		dprintk("7 , cte_config_e.syncdelay = %d\n", cte_config_e.syncdelay);

		if (write_CTE_config_e(0, cte_config_e) != CFG_WRITE_OK) {
			dprintk("\n[TSP][ERROR] line : %d\n", __LINE__);
			return CFG_WRITE_FAILED;
		}
	}

	/* Backup settings to NVM. */
	if (backup_config() != WRITE_MEM_OK) {
		/* "Failed to backup, exiting...\n" */
		return WRITE_MEM_FAILED;
	}
	/* Calibrate the touch IC. */
	if (calibrate_chip() != WRITE_MEM_OK) {
		dprintk("Failed to calibrate, exiting...\n");
		return WRITE_MEM_FAILED;
	}

	dprintk("\n[TSP] configs saved : %d\n", __LINE__);

	return 0;
}
static ssize_t set_write_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t size)
{
	printk(KERN_DEBUG "[TSP] %s : operate nothing\n", __FUNCTION__);

	return size;
}
static DEVICE_ATTR(set_write, 0664, set_write_show, set_write_store);

static ssize_t set_object_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	int count = 0;
	int i;
	uint8_t val;
	uint16_t object_address;
	uint8_t object_size;

	if (info_block->info_id.family_id == 0x80) {
		object_address = get_object_address(GEN_POWERCONFIG_T7, 0);
		object_size = get_object_size(GEN_POWERCONFIG_T7);
		count += sprintf(buf + count, "GEN_POWERCONFIG_T7\n");
		for (i = 0; i < object_size; i++) {
			read_mem(object_address + i, 1, &val);
			count += sprintf(buf + count," Byte %d: 0x%x (%d)\n", i, val, val);
		}
		count += sprintf(buf + count, "\n");

		object_address = get_object_address(GEN_ACQUISITIONCONFIG_T8, 0);
		object_size = get_object_size(GEN_ACQUISITIONCONFIG_T8);
		count += sprintf(buf + count, "GEN_ACQUISITIONCONFIG_T8\n");
		for (i = 0; i < object_size; i++) {
			read_mem(object_address + i, 1, &val);
			count += sprintf(buf + count," Byte %d: 0x%x (%d)\n", i, val, val);
		}
		count += sprintf(buf + count, "\n");

		object_address = get_object_address(TOUCH_MULTITOUCHSCREEN_T9, 0);
		object_size = get_object_size(TOUCH_MULTITOUCHSCREEN_T9);
		count += sprintf(buf + count, "TOUCH_MULTITOUCHSCREEN_T9\n");
		for (i = 0; i < object_size; i++) {
			read_mem(object_address + i, 1, &val);
			count += sprintf(buf + count," Byte %d: 0x%x (%d)\n", i, val, val);
		}
		count += sprintf(buf + count, "\n");

		object_address = get_object_address(TOUCH_KEYARRAY_T15, 0);
		object_size = get_object_size(TOUCH_KEYARRAY_T15);
		count += sprintf(buf + count, "TOUCH_KEYARRAY_T15\n");
		for (i = 0; i < object_size; i++) {
			read_mem(object_address + i, 1, &val);
			count += sprintf(buf + count," Byte %d: 0x%x (%d)\n", i, val, val);
		}
		count += sprintf(buf + count, "\n");

		object_address = get_object_address(SPT_COMCONFIG_T18, 0);
		object_size = get_object_size(SPT_COMCONFIG_T18);
		count += sprintf(buf + count, "SPT_COMCONFIG_T18\n");
		for (i = 0; i < object_size; i++) {
			read_mem(object_address + i, 1, &val);
			count += sprintf(buf + count," Byte %d: 0x%x (%d)\n", i, val, val);
		}
		count += sprintf(buf + count, "\n");

		object_address = get_object_address(SPT_GPIOPWM_T19, 0);
		object_size = get_object_size(SPT_GPIOPWM_T19);
		count += sprintf(buf + count, "SPT_GPIOPWM_T19\n");
		for (i = 0; i < object_size; i++) {
			read_mem(object_address + i, 1, &val);
			count += sprintf(buf + count," Byte %d: 0x%x (%d)\n", i, val, val);
		}
		count += sprintf(buf + count, "\n");

		object_address = get_object_address(PROCI_GRIPFACESUPPRESSION_T20, 0);
		object_size = get_object_size(PROCI_GRIPFACESUPPRESSION_T20);
		count += sprintf(buf + count, "PROCI_GRIPFACESUPPRESSION_T20\n");
		for (i = 0; i < object_size; i++) {
			read_mem(object_address + i, 1, &val);
			count += sprintf(buf + count," Byte %d: 0x%x (%d)\n", i, val, val);
		}
		count += sprintf(buf + count, "\n");

		object_address = get_object_address(PROCG_NOISESUPPRESSION_T22, 0);
		object_size = get_object_size(PROCG_NOISESUPPRESSION_T22);
		count += sprintf(buf + count, "PROCG_NOISESUPPRESSION_T22\n");
		for (i = 0; i < object_size; i++) {
			read_mem(object_address + i, 1, &val);
			count += sprintf(buf + count," Byte %d: 0x%x (%d)\n", i, val, val);
		}
		count += sprintf(buf + count, "\n");

		object_address = get_object_address(TOUCH_PROXIMITY_T23, 0);
		object_size = get_object_size(TOUCH_PROXIMITY_T23);
		count += sprintf(buf + count, "TOUCH_PROXIMITY_T23\n");
		for (i = 0; i < object_size; i++) {
			read_mem(object_address + i, 1, &val);
			count += sprintf(buf + count," Byte %d: 0x%x (%d)\n", i, val, val);
		}
		count += sprintf(buf + count, "\n");

		object_address = get_object_address(PROCI_ONETOUCHGESTUREPROCESSOR_T24, 0);
		object_size = get_object_size(PROCI_ONETOUCHGESTUREPROCESSOR_T24);
		count += sprintf(buf + count, "PROCI_ONETOUCHGESTUREPROCESSOR_T24\n");
		for (i = 0; i < object_size; i++) {
			read_mem(object_address + i, 1, &val);
			count += sprintf(buf + count," Byte %d: 0x%x (%d)\n", i, val, val);
		}
		count += sprintf(buf + count, "\n");

		object_address = get_object_address(SPT_SELFTEST_T25, 0);
		object_size = get_object_size(SPT_SELFTEST_T25);
		count += sprintf(buf + count, "SPT_SELFTEST_T25\n");
		for (i = 0; i < object_size; i++) {
			read_mem(object_address + i, 1, &val);
			count += sprintf(buf + count," Byte %d: 0x%x (%d)\n", i, val, val);
		}
		count += sprintf(buf + count, "\n");

		object_address = get_object_address(PROCI_TWOTOUCHGESTUREPROCESSOR_T27, 0);
		object_size = get_object_size(PROCI_TWOTOUCHGESTUREPROCESSOR_T27);
		count += sprintf(buf + count, "PROCI_TWOTOUCHGESTUREPROCESSOR_T27\n");
		for (i = 0; i < object_size; i++) {
			read_mem(object_address + i, 1, &val);
			count += sprintf(buf + count," Byte %d: 0x%x (%d)\n", i, val, val);
		}
		count += sprintf(buf + count, "\n");

		object_address = get_object_address(SPT_CTECONFIG_T28, 0);
		object_size = get_object_size(SPT_CTECONFIG_T28);
		count += sprintf(buf + count, "SPT_CTECONFIG_T28\n");
		for (i = 0; i < object_size; i++) {
			read_mem(object_address + i, 1, &val);
			count += sprintf(buf + count," Byte %d: 0x%x (%d)\n", i, val, val);
		}
		count += sprintf(buf + count, "\n");
	} else if (info_block->info_id.family_id == 0x81) {
		object_address = get_object_address(GEN_POWERCONFIG_T7, 0);
		object_size = get_object_size(GEN_POWERCONFIG_T7);
		count += sprintf(buf + count, "GEN_POWERCONFIG_T7\n");
		for (i = 0; i < object_size; i++) {
			read_mem(object_address + i, 1, &val);
			count += sprintf(buf + count," Byte %d: 0x%x (%d)\n", i, val, val);
		}
		count += sprintf(buf + count, "\n");

		object_address = get_object_address(GEN_ACQUISITIONCONFIG_T8, 0);
		object_size = get_object_size(GEN_ACQUISITIONCONFIG_T8);
		count += sprintf(buf + count, "GEN_ACQUISITIONCONFIG_T8\n");
		for (i = 0; i < object_size; i++) {
			read_mem(object_address + i, 1, &val);
			count += sprintf(buf + count," Byte %d: 0x%x (%d)\n", i, val, val);
		}
		count += sprintf(buf + count, "\n");

		object_address = get_object_address(TOUCH_MULTITOUCHSCREEN_T9, 0);
		object_size = get_object_size(TOUCH_MULTITOUCHSCREEN_T9);
		count += sprintf(buf + count, "TOUCH_MULTITOUCHSCREEN_T9\n");
		for (i = 0; i < object_size; i++) {
			read_mem(object_address + i, 1, &val);
			count += sprintf(buf + count," Byte %d: 0x%x (%d)\n", i, val, val);
		}
		count += sprintf(buf + count, "\n");

		object_address = get_object_address(TOUCH_KEYARRAY_T15, 0);
		object_size = get_object_size(TOUCH_KEYARRAY_T15);
		count += sprintf(buf + count, "TOUCH_KEYARRAY_T15\n");
		for (i = 0; i < object_size; i++) {
			read_mem(object_address + i, 1, &val);
			count += sprintf(buf + count," Byte %d: 0x%x (%d)\n", i, val, val);
		}
		count += sprintf(buf + count, "\n");

		object_address = get_object_address(SPT_COMCONFIG_T18, 0);
		object_size = get_object_size(SPT_COMCONFIG_T18);
		count += sprintf(buf + count, "SPT_COMCONFIG_T18\n");
		for (i = 0; i < object_size; i++) {
			read_mem(object_address + i, 1, &val);
			count += sprintf(buf + count," Byte %d: 0x%x (%d)\n", i, val, val);
		}
		count += sprintf(buf + count, "\n");

		object_address = get_object_address(SPT_GPIOPWM_T19, 0);
		object_size = get_object_size(SPT_GPIOPWM_T19);
		count += sprintf(buf + count, "SPT_GPIOPWM_T19\n");
		for (i = 0; i < object_size; i++) {
			read_mem(object_address + i, 1, &val);
			count += sprintf(buf + count," Byte %d: 0x%x (%d)\n", i, val, val);
		}
		count += sprintf(buf + count, "\n");

		object_address = get_object_address(TOUCH_PROXIMITY_T23, 0);
		object_size = get_object_size(TOUCH_PROXIMITY_T23);
		count += sprintf(buf + count, "TOUCH_PROXIMITY_T23\n");
		for (i = 0; i < object_size; i++) {
			read_mem(object_address + i, 1, &val);
			count += sprintf(buf + count," Byte %d: 0x%x (%d)\n", i, val, val);
		}
		count += sprintf(buf + count, "\n");

		object_address = get_object_address(SPT_SELFTEST_T25, 0);
		object_size = get_object_size(SPT_SELFTEST_T25);
		count += sprintf(buf + count, "SPT_SELFTEST_T25\n");
		for (i = 0; i < object_size; i++) {
			read_mem(object_address + i, 1, &val);
			count += sprintf(buf + count," Byte %d: 0x%x (%d)\n", i, val, val);
		}
		count += sprintf(buf + count, "\n");

		object_address = get_object_address(PROCI_GRIPSUPPRESSION_T40, 0);
		object_size = get_object_size(PROCI_GRIPSUPPRESSION_T40);
		count += sprintf(buf + count, "PROCI_GRIPSUPPRESSION_T40\n");
		for (i = 0; i < object_size; i++) {
			read_mem(object_address + i, 1, &val);
			count += sprintf(buf + count," Byte %d: 0x%x (%d)\n", i, val, val);
		}
		count += sprintf(buf + count, "\n");

		object_address = get_object_address(PROCI_TOUCHSUPPRESSION_T42, 0);
		object_size = get_object_size(PROCI_TOUCHSUPPRESSION_T42);
		count += sprintf(buf + count, "PROCI_TOUCHSUPPRESSION_T42\n");
		for (i = 0; i < object_size; i++) {
			read_mem(object_address + i, 1, &val);
			count += sprintf(buf + count," Byte %d: 0x%x (%d)\n", i, val, val);
		}
		count += sprintf(buf + count, "\n");
		
		object_address = get_object_address(SPT_CTECONFIG_T46, 0);
		object_size = get_object_size(SPT_CTECONFIG_T46);
		count += sprintf(buf + count, "SPT_CTECONFIG_T46\n");
		for (i = 0; i < object_size; i++) {
			read_mem(object_address + i, 1, &val);
			count += sprintf(buf + count," Byte %d: 0x%x (%d)\n", i, val, val);
		}
		count += sprintf(buf + count, "\n");
		
		object_address = get_object_address(PROCI_STYLUS_T47, 0);
		object_size = get_object_size(PROCI_STYLUS_T47);
		count += sprintf(buf + count, "PROCI_STYLUS_T47\n");
		for (i = 0; i < object_size; i++) {
			read_mem(object_address + i, 1, &val);
			count += sprintf(buf + count," Byte %d: 0x%x (%d)\n", i, val, val);
		}
		count += sprintf(buf + count, "\n");
		
		object_address = get_object_address(PROCG_NOISESUPPRESSION_T48, 0);
		object_size = get_object_size(PROCG_NOISESUPPRESSION_T48);
		count += sprintf(buf + count, "PROCG_NOISESUPPRESSION_T48\n");
		for (i = 0; i < object_size; i++) {
			read_mem(object_address + i, 1, &val);
			count += sprintf(buf + count," Byte %d: 0x%x (%d)\n", i, val, val);
		}
		count += sprintf(buf + count, "\n");
	}

	return count;
}
static ssize_t set_object_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t size)
{
	uint16_t object_address;
	unsigned int type;
	unsigned int offset;
	unsigned int val;
	uint8_t value;

	if (sscanf(buf, "%u %u %u", &type, &offset, &val) != 3) {
		dprintk("Invalid values(type offset val)\n");
		return size;
	}

	printk("[TSP] object write type = 0x%x(%d), offset = 0x%x(%d), val = 0x%x(%d)\n",
			type, type, offset, offset, val, val);

	value = (uint8_t)val;
	object_address = get_object_address((uint8_t)type, 0);
	write_mem(object_address + offset, 1, &value);

	return size;
}
static DEVICE_ATTR(set_object, 0664, set_object_show, set_object_store);


/*****************************************************************************
*
*  FUNCTION
*  PURPOSE
*  INPUT
*  OUTPUT
*                                   
* ***************************************************************************/
#if ENABLE_NOISE_TEST_MODE
struct device *qt602240_noise_test;

uint8_t read_uint16_t( uint16_t Address, uint16_t *Data )
{
	uint8_t status;
	uint8_t temp[2];

	status = read_mem(Address, 2, temp);
	//status = read_mem(0, 2, temp);
	*Data = ((uint16_t)temp[1]<<8) + (uint16_t)temp[0];

	return(status);
}

uint8_t read_dbg_data(uint8_t dbg_mode , uint8_t node, uint16_t * dbg_data)
{
	uint8_t status;
	uint8_t mode,page, i;
	uint8_t read_page, read_point;

	diagnostic_addr = get_object_address(DEBUG_DIAGNOSTIC_T37, 0);

	read_page = node / 64;
	node %= 64;
	read_point = (node * 2) + 2;

	//Page Num Clear
	status = diagnostic_chip(QT_CTE_MODE);
	msleep(20);
	 
	status = diagnostic_chip(dbg_mode);
	msleep(20); 

	for (i = 0; i < 5; i++) {
		msleep(20);    
		status = read_mem(diagnostic_addr, 1, &mode);
		if (status == READ_MEM_OK) {
            if (mode == dbg_mode)
				break;
		} else {
			printk(KERN_DEBUG "[TSP] READ_MEM_FAILED \n");            
			return READ_MEM_FAILED;
		}
	}

	for (page = 0; page < read_page; page ++) {
		status = diagnostic_chip(QT_PAGE_UP); 
		msleep(10); 
	}

	status = read_uint16_t(diagnostic_addr + read_point,dbg_data);  

	msleep(10); 

	return(status);
}


static ssize_t set_refer0_mode_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	unsigned char status;
	uint16_t qt_refrence = 0;

	status = read_dbg_data(QT_REFERENCE_MODE, test_node[0], &qt_refrence);
	if (info_block->info_id.family_id == 0x80)
		return sprintf(buf, "%u\n", qt_refrence);    
	else
		return sprintf(buf, "%u\n", qt_refrence-16384);    
}

static ssize_t set_refer1_mode_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	unsigned char status;
	uint16_t qt_refrence = 0;

	status = read_dbg_data(QT_REFERENCE_MODE, test_node[1], &qt_refrence);
	if (info_block->info_id.family_id == 0x80)
		return sprintf(buf, "%u\n", qt_refrence);    
	else
		return sprintf(buf, "%u\n", qt_refrence-16384);    

}

static ssize_t set_refer2_mode_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	unsigned char status;
	uint16_t qt_refrence = 0;

	status = read_dbg_data(QT_REFERENCE_MODE, test_node[2], &qt_refrence);
	if (info_block->info_id.family_id == 0x80)
		return sprintf(buf, "%u\n", qt_refrence);    
	else
		return sprintf(buf, "%u\n", qt_refrence-16384);    

}

static ssize_t set_refer3_mode_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	unsigned char status;
	uint16_t qt_refrence = 0;

	status = read_dbg_data(QT_REFERENCE_MODE, test_node[3], &qt_refrence);
	if (info_block->info_id.family_id == 0x80)
		return sprintf(buf, "%u\n", qt_refrence);    
	else
		return sprintf(buf, "%u\n", qt_refrence-16384);    

}

static ssize_t set_refer4_mode_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	unsigned char status;
	uint16_t qt_refrence = 0;

	status = read_dbg_data(QT_REFERENCE_MODE, test_node[4], &qt_refrence);
	if (info_block->info_id.family_id == 0x80)
		return sprintf(buf, "%u\n", qt_refrence);    
	else
		return sprintf(buf, "%u\n", qt_refrence-16384);    

}

static ssize_t set_refer_minmax_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	unsigned char status;
	uint16_t qt_refrence_min = 0, qt_refrence_max = 0, qt_refrence = 0;
	int node_num_min = 0, node_num_max = 0;
	int i;

	printk("set_refer_min_show!!!!!!!!!!!!!!!!!!\n");

	for (i = 0; i < 209; i++) {
		status = read_dbg_data(QT_REFERENCE_MODE, i, &qt_refrence);
		if (i == 0) {
			qt_refrence_min = qt_refrence;
			qt_refrence_max = qt_refrence;
		}
		if (qt_refrence <= qt_refrence_min) {
			node_num_min = i;
			qt_refrence_min = qt_refrence;
		}
		if (qt_refrence >= qt_refrence_max) {
			node_num_max = i;
			qt_refrence_max = qt_refrence;
		}
	}

	printk("set_refer_min_show end!!!!!!!!!!!!!!!!!!\n");

	return sprintf(buf, "%u , %d , %u, %d\n", qt_refrence_min, node_num_min,
		qt_refrence_max, node_num_max);    
}
static ssize_t set_delta0_mode_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	unsigned char status;
	uint16_t qt_delta = 0;

	status = read_dbg_data(QT_DELTA_MODE, test_node[0], &qt_delta);
	if (qt_delta < 32767)
		return sprintf(buf, "%u\n", qt_delta);
	else {
		qt_delta = 65535 - qt_delta;
		return sprintf(buf, "-%u\n", qt_delta);
	}
}

static ssize_t set_delta1_mode_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	unsigned char status;
	uint16_t qt_delta = 0;

	status = read_dbg_data(QT_DELTA_MODE, test_node[1], &qt_delta);
	if (qt_delta < 32767)
		return sprintf(buf, "%u\n", qt_delta);
	else {
		qt_delta = 65535 - qt_delta;
		return sprintf(buf, "-%u\n", qt_delta);
	}
}

static ssize_t set_delta2_mode_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	unsigned char status;
	uint16_t qt_delta = 0;

	status = read_dbg_data(QT_DELTA_MODE, test_node[2], &qt_delta);
	if (qt_delta < 32767)
		return sprintf(buf, "%u\n", qt_delta);
	else {
		qt_delta = 65535 - qt_delta;
		return sprintf(buf, "-%u\n", qt_delta);
	}
}

static ssize_t set_delta3_mode_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	unsigned char status;
	uint16_t qt_delta = 0;

	status = read_dbg_data(QT_DELTA_MODE, test_node[3], &qt_delta);
	if (qt_delta < 32767)
		return sprintf(buf, "%u\n", qt_delta);
	else {
		qt_delta = 65535 - qt_delta;
		return sprintf(buf, "-%u\n", qt_delta);
	}
}

static ssize_t set_delta4_mode_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	unsigned char status;
	uint16_t qt_delta = 0;

	status = read_dbg_data(QT_DELTA_MODE, test_node[4], &qt_delta);
	if (qt_delta < 32767)
		return sprintf(buf, "%u\n", qt_delta);
	else {
		qt_delta = 65535 - qt_delta;
		return sprintf(buf, "-%u\n", qt_delta);
	}
}

static ssize_t set_delta_minmax_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	unsigned char status;
	uint16_t qt_delta_min = 0, qt_delta_max = 0, qt_delta = 0;
	int i, node_num_min = 0, node_num_max = 0;

	printk("set_delta_min_show!!!!!!!!!!!!!!!!!!\n");

	for (i = 0; i < 209; i++) {
		status = read_dbg_data(QT_DELTA_MODE, i, &qt_delta);
		if (qt_delta < 32767) {
			if (qt_delta >= qt_delta_max) {
				node_num_max = i;
				qt_delta_max = qt_delta;
			}
		} else {
			qt_delta = 65535 - qt_delta;
			if (qt_delta >= qt_delta_min) {
				node_num_min = i;
				qt_delta_min = qt_delta;
			}
		}
	}
	
	printk("set_delta_min_show end!!!!!!!!!!!!!!!!!!\n");

	return sprintf(buf, "-%u, %d, %u, %d\n", qt_delta_min, node_num_min, 
		qt_delta_max, node_num_max);
	
}
static ssize_t set_threshold_mode_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	if (info_block->info_id.family_id == 0x80)
		return sprintf(buf, "%u\n", touchscreen_config.tchthr);
	else
		return sprintf(buf, "%u\n", touchscreen_config_e.tchthr);
}

static DEVICE_ATTR(set_refer0, 0664, set_refer0_mode_show, NULL);
static DEVICE_ATTR(set_delta0, 0664, set_delta0_mode_show, NULL);
static DEVICE_ATTR(set_refer1, 0664, set_refer1_mode_show, NULL);
static DEVICE_ATTR(set_delta1, 0664, set_delta1_mode_show, NULL);
static DEVICE_ATTR(set_refer2, 0664, set_refer2_mode_show, NULL);
static DEVICE_ATTR(set_delta2, 0664, set_delta2_mode_show, NULL);
static DEVICE_ATTR(set_refer3, 0664, set_refer3_mode_show, NULL);
static DEVICE_ATTR(set_delta3, 0664, set_delta3_mode_show, NULL);
static DEVICE_ATTR(set_refer4, 0664, set_refer4_mode_show, NULL);
static DEVICE_ATTR(set_delta4, 0664, set_delta4_mode_show, NULL);
static DEVICE_ATTR(set_referminmax, 0664, set_refer_minmax_show, NULL);
static DEVICE_ATTR(set_deltaminmax, 0664, set_delta_minmax_show, NULL);
static DEVICE_ATTR(set_threshould, 0664, set_threshold_mode_show, NULL);
#endif /* ENABLE_NOISE_TEST_MODE */

#ifdef QT_ATCOM_TEST
struct device *qt602240_atcom_test;
struct work_struct qt_touch_update_work;

unsigned int qt_firm_status_data = 0;
void set_qt_update_exe(struct work_struct * p)
{
	if (!QT_Boot()) {
		qt_firm_status_data = 2;        // firmware update success
		dprintk("Reprogram done : Firmware update Success~~~~~~~~~~\n");
	} else {
		qt_firm_status_data = 3;        // firmware update Fail
		dprintk("[QT]Reprogram done : Firmware update Fail~~~~~~~~~~\n");
	}
	TSP_Restart();
	quantum_touch_probe();
}

static ssize_t set_qt_update_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	int count;

	qt_firm_status_data=1;    //start firmware updating
	INIT_WORK(&qt_touch_update_work, set_qt_update_exe);
	queue_work(qt602240_wq, &qt_touch_update_work);

	if (qt_firm_status_data == 3)
		count = sprintf(buf,"FAIL\n");
	else
		count = sprintf(buf,"OK\n");

	return count;
}
static ssize_t set_qt_firm_version_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	return sprintf(buf, "%d\n", tsp_version);
}

static ssize_t set_qt_firm_version_read_show(struct device *dev, struct device_attribute *attr, char *buf)
{   
	return sprintf(buf, "%d\n", info_block->info_id.version);
}

static ssize_t set_qt_firm_status_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	int count;

	if (qt_firm_status_data == 1)
	    count = sprintf(buf,"Downloading\n");
	else if (qt_firm_status_data == 2)
	    count = sprintf(buf,"PASS\n");
	else if (qt_firm_status_data == 3)
	    count = sprintf(buf,"FAIL\n");
	else
	    count = sprintf(buf,"PASS\n");

	return count;
}

static DEVICE_ATTR(set_qt_update, 0664, set_qt_update_show, NULL);
static DEVICE_ATTR(set_qt_firm_version, 0664, set_qt_firm_version_show, NULL);
static DEVICE_ATTR(set_qt_firm_status, 0664, set_qt_firm_status_show, NULL);
static DEVICE_ATTR(set_qt_firm_version_read, 0664, set_qt_firm_version_read_show, NULL);
#endif

#ifdef QT_STYLUS_ENABLE
static ssize_t qt602240_config_mode_show(struct device *dev,    struct device_attribute *attr, char *buf)
{
	if (config_mode_val)
		dprintk("config: stylus mode\n");
	else
		dprintk("config: normal mode\n");

	return snprintf(buf, PAGE_SIZE, "%d\n", config_mode_val);
}

static ssize_t qt602240_config_mode_store(struct device *dev,    struct device_attribute *attr, const char *buf, size_t count)
{
	unsigned int mode;

	if (*buf == 0x00)
		mode = 0;
	else if (*buf == 0x01)
		mode = 1;
	else {
		if (sscanf(buf, "%u", &mode) != 1) {
		    dev_err(dev, "Invalid values(0x%x)\n", *buf);
		    return -EINVAL;
		}
	}

	switch(mode)
	{
		case 1: {
			if (mode != config_mode_val) {
				dprintk("set stylus mode\n");
				qt_Multitouchscreen_stylus_Init();
				calibrate_chip();
			} else
				dprintk("already stylus mode\n");

			config_mode_val = mode;
			break;
		}
		case 0: {
			if (mode != config_mode_val) {
				dprintk("set normal mode\n");
				qt_Multitouchscreen_normal_Init();
				calibrate_chip();
			} else
				dprintk("already normal mode\n");

			config_mode_val = mode;
			break;
		}
		default:
			dprintk("invalid mode\n");
			break;
	}

	return count;
}

static DEVICE_ATTR(config_mode, 0664, qt602240_config_mode_show, qt602240_config_mode_store);
#endif
/*------------------------------ for tunning ATmel - end ----------------------------*/

int __init qt602240_init(void)
{
	int ret;
	int i=0;

	DEBUG;

	qt602240_wq = create_singlethread_workqueue("qt602240_wq");
	if (!qt602240_wq)
		return -ENOMEM;

	gpio_set_value(GPIO_TOUCH_EN, 1);
	msleep(70);

	qt602240 = kzalloc(sizeof(struct qt602240_data), GFP_KERNEL);
	if (qt602240 == NULL) {
		return -ENOMEM;
	}

	qt_time_point = jiffies_to_msecs(jiffies);

	ret = i2c_add_driver(&qt602240_i2c_driver);
	if (ret)
		dprintk("[%s], i2c_add_driver failed...(%d)\n", __func__, ret);

	dprintk("[QT] ret : %d, qt602240->client name : %s\n",ret,qt602240->client->name);

	if (!(qt602240->client)) {
		dprintk("[%s],slave address changed try to firmware reprogram \n",__func__);
		i2c_del_driver(&qt602240_i2c_driver);

		ret = i2c_add_driver(&qt602240_i2c_driver);
		if (ret)
			dprintk("[%s], i2c_add_driver failed...(%d)\n", __func__, ret);
		dprintk("[QT] ret : %d, qt602240->client name : %s\n",ret,qt602240->client->name);

		if (qt602240->client) {
			QT_reprogram();
			i2c_del_driver(&qt602240_i2c_driver);

			ret = i2c_add_driver(&qt602240_i2c_driver);
			if (ret)
				dprintk("[%s], i2c_add_driver failed...(%d)\n", __func__, ret);
			dprintk("[QT] ret : %d, qt602240->client name : %s\n",ret,qt602240->client->name);
		}
	}

	if (!(qt602240->client)){
		dprintk("###################################################\n");
		dprintk("##                                               ##\n");
		dprintk("##    WARNING! TOUCHSCREEN DRIVER CAN'T WORK.    ##\n");
		dprintk("##    PLEASE CHECK YOUR TOUCHSCREEN CONNECTOR!   ##\n");
		dprintk("##                                               ##\n");
		dprintk("###################################################\n");
		i2c_del_driver(&qt602240_i2c_driver);
		return 0;
	}
	dprintk("[QT] %s/%d\n",__func__,__LINE__);

	if (sec_class == NULL)
		sec_class = class_create(THIS_MODULE, "sec");
	if (IS_ERR(sec_class))
		pr_err("Failed to create class(sec)!\n");

	ts_dev = device_create(sec_class, NULL, 0, NULL, "ts");
	if (IS_ERR(ts_dev))
		pr_err("Failed to create device(ts)!\n");
	if (device_create_file(ts_dev, &dev_attr_gpio) < 0)
		pr_err("Failed to create device file(%s)!\n", dev_attr_gpio.attr.name);
	if (device_create_file(ts_dev, &dev_attr_i2c) < 0)
		pr_err("Failed to create device file(%s)!\n", dev_attr_i2c.attr.name);
	if (device_create_file(ts_dev, &dev_attr_setup) < 0)
		pr_err("Failed to create device file(%s)!\n", dev_attr_setup.attr.name);
	if (device_create_file(ts_dev, &dev_attr_firmware1) < 0)
		pr_err("Failed to create device file(%s)!\n", dev_attr_firmware1.attr.name);
	if (device_create_file(ts_dev, &dev_attr_key_threshold) < 0)
		pr_err("Failed to create device file(%s)!\n", dev_attr_key_threshold.attr.name);

	/*------------------------------ for tunning ATmel - start ----------------------------*/
	touch_class = class_create(THIS_MODULE, "touch");
	if (IS_ERR(touch_class))
		pr_err("Failed to create class(touch)!\n");
	switch_test = device_create(touch_class, NULL, 0, NULL, "switch");
	if (IS_ERR(switch_test))
		pr_err("Failed to create device(switch)!\n");

	if (device_create_file(switch_test, &dev_attr_set_power) < 0)
		pr_err("Failed to create device file(%s)!\n", dev_attr_set_power.attr.name);
	if (device_create_file(switch_test, &dev_attr_set_acquisition) < 0)
		pr_err("Failed to create device file(%s)!\n", dev_attr_set_acquisition.attr.name);
	if (device_create_file(switch_test, &dev_attr_set_touchscreen) < 0)
		pr_err("Failed to create device file(%s)!\n", dev_attr_set_touchscreen.attr.name);
	if (device_create_file(switch_test, &dev_attr_set_keyarray) < 0)
		pr_err("Failed to create device file(%s)!\n", dev_attr_set_keyarray.attr.name);
	if (device_create_file(switch_test, &dev_attr_set_total ) < 0)
		pr_err("Failed to create device file(%s)!\n", dev_attr_set_total.attr.name);
	if (device_create_file(switch_test, &dev_attr_set_write ) < 0)
		pr_err("Failed to create device file(%s)!\n", dev_attr_set_write.attr.name);
	if (device_create_file(switch_test, &dev_attr_set_noise ) < 0)
		pr_err("Failed to create device file(%s)!\n", dev_attr_set_noise.attr.name);
	if (device_create_file(switch_test, &dev_attr_set_grip ) < 0)
		pr_err("Failed to create device file(%s)!\n", dev_attr_set_grip.attr.name);
	if (device_create_file(switch_test, &dev_attr_set_touchsuppression ) < 0)
		pr_err("Failed to create device file(%s)!\n", dev_attr_set_touchsuppression.attr.name);
	if (device_create_file(switch_test, &dev_attr_set_stylus ) < 0)
		pr_err("Failed to create device file(%s)!\n", dev_attr_set_stylus.attr.name);
	if (device_create_file(switch_test, &dev_attr_set_cte ) < 0)
		pr_err("Failed to create device file(%s)!\n", dev_attr_set_cte.attr.name);
	if (device_create_file(switch_test, &dev_attr_set_object ) < 0)
		pr_err("Failed to create device file(%s)!\n", dev_attr_set_object.attr.name);
	/*------------------------------ for tunning ATmel - end ----------------------------*/

	/*------------------------------ for Noise APP - start ----------------------------*/
#if ENABLE_NOISE_TEST_MODE
	qt602240_noise_test = device_create(sec_class, NULL, 0, NULL, "qt602240_noise_test");
	if (IS_ERR(qt602240_noise_test))
		dprintk("Failed to create device(qt602240_noise_test)!\n");

	if (device_create_file(qt602240_noise_test, &dev_attr_set_refer0)< 0)
		dprintk("Failed to create device file(%s)!\n", dev_attr_set_refer0.attr.name);
	if (device_create_file(qt602240_noise_test, &dev_attr_set_delta0) < 0)
		dprintk("Failed to create device file(%s)!\n", dev_attr_set_delta0.attr.name);
	if (device_create_file(qt602240_noise_test, &dev_attr_set_refer1)< 0)
		dprintk("Failed to create device file(%s)!\n", dev_attr_set_refer1.attr.name);
	if (device_create_file(qt602240_noise_test, &dev_attr_set_delta1) < 0)
		dprintk("Failed to create device file(%s)!\n", dev_attr_set_delta1.attr.name);
	if (device_create_file(qt602240_noise_test, &dev_attr_set_refer2)< 0)
		dprintk("Failed to create device file(%s)!\n", dev_attr_set_refer2.attr.name);
	if (device_create_file(qt602240_noise_test, &dev_attr_set_delta2) < 0)
		dprintk("Failed to create device file(%s)!\n", dev_attr_set_delta2.attr.name);
	if (device_create_file(qt602240_noise_test, &dev_attr_set_refer3)< 0)
		dprintk("Failed to create device file(%s)!\n", dev_attr_set_refer3.attr.name);
	if (device_create_file(qt602240_noise_test, &dev_attr_set_delta3) < 0)
		dprintk("Failed to create device file(%s)!\n", dev_attr_set_delta3.attr.name);
	if (device_create_file(qt602240_noise_test, &dev_attr_set_refer4)< 0)
		dprintk("Failed to create device file(%s)!\n", dev_attr_set_refer4.attr.name);
	if (device_create_file(qt602240_noise_test, &dev_attr_set_delta4) < 0)
		dprintk("Failed to create device file(%s)!\n", dev_attr_set_delta4.attr.name);
	if (device_create_file(qt602240_noise_test, &dev_attr_set_referminmax)< 0)
		printk("Failed to create device file(%s)!\n", dev_attr_set_referminmax.attr.name);
	if (device_create_file(qt602240_noise_test, &dev_attr_set_deltaminmax) < 0)
		printk("Failed to create device file(%s)!\n", dev_attr_set_deltaminmax.attr.name);
	if (device_create_file(qt602240_noise_test, &dev_attr_set_threshould) < 0)
		dprintk("Failed to create device file(%s)!\n", dev_attr_set_threshould.attr.name);
#endif
	/*------------------------------ for Noise APP - end ----------------------------*/

	/*------------------------------     AT COMMAND TEST         ---------------------*/
#ifdef QT_ATCOM_TEST
	qt602240_atcom_test = device_create(sec_class, NULL, 0, NULL, "qt602240_atcom_test");
	if (IS_ERR(qt602240_atcom_test))
		dprintk("Failed to create device(qt602240_atcom_test)!\n");

	if (device_create_file(qt602240_atcom_test, &dev_attr_set_qt_update)< 0)
		dprintk("Failed to create device file(%s)!\n", dev_attr_set_qt_update.attr.name);
	if (device_create_file(qt602240_atcom_test, &dev_attr_set_qt_firm_version)< 0)
		dprintk("Failed to create device file(%s)!\n", dev_attr_set_qt_firm_version.attr.name);
	if (device_create_file(qt602240_atcom_test, &dev_attr_set_qt_firm_status)< 0)
		dprintk("Failed to create device file(%s)!\n", dev_attr_set_qt_firm_status.attr.name);
	if (device_create_file(qt602240_atcom_test, &dev_attr_set_qt_firm_version_read)< 0)
		dprintk("Failed to create device file(%s)!\n", dev_attr_set_qt_firm_version_read.attr.name);
#endif
	/*------------------------------     AT COMMAND TEST         ---------------------*/

#ifdef QT_STYLUS_ENABLE
	qt_stylus = device_create(touch_class, NULL, 0, NULL, "qt_stylus");
	if (IS_ERR(qt_stylus))
		pr_err("Failed to create device(qt_stylus)!\n");

	if (device_create_file(qt_stylus, &dev_attr_config_mode) < 0)
		pr_err("Failed to create device file(%s)!\n", dev_attr_config_mode.attr.name);
#endif

#ifdef _SUPPORT_MULTITOUCH_
	for (i = 0; i < MAX_USING_FINGER_NUM ; i++)        // touchscreen_config.numtouch is 5
		fingerInfo[i].pressure = -1;
#endif

	dprintk("[QT] %s/%d, platform_driver_register!!\n",__func__,__LINE__);

	return 0;
}

void __exit qt602240_exit(void)
{
	i2c_del_driver(&qt602240_i2c_driver);
	if (qt602240_wq)
		destroy_workqueue(qt602240_wq);
}
late_initcall(qt602240_init);
module_exit(qt602240_exit);

MODULE_DESCRIPTION("Quantum Touchscreen Driver");
MODULE_LICENSE("GPL");
