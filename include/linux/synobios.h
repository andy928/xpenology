// Copyright (c) 2000-2003 Synology Inc. All rights reserved.
#ifndef __SYNOBIOS_OEM_H_
#define __SYNOBIOS_OEM_H_

#include <linux/syno.h>

#ifndef BCD_TO_BIN
#define BCD_TO_BIN(val) ((val)=((val)&15) + ((val)>>4)*10)
#endif
#ifndef BIN_TO_BCD
#define BIN_TO_BCD(val) ((val)=(((val)/10)<<4) + (val)%10)
#endif
#ifndef BCD2BIN
#define BCD2BIN(val) (((val)&15) + ((val)>>4)*10)
#endif
#ifndef BIN2BCD
#define BIN2BCD(val) ((((val)/10)<<4) + (val)%10)
#endif

#define SYNOBIOS_MAJOR    201
#define SYNOBIOS_NEVENTS  128

/* 
 * SynoBios Hardware status structure is as following
 * ThermalStage:	0~3: Normal, Warm, Hot, Critical, four stages
 * MemEccCount:		0~X: the count of Memory ECC
 * ButtonStatus:	each bit presents a button status pressed(1) or not(0)
 * HardDiskStatus:	each bit presents a HD status removed(1) or not(0)
 * AcPowerStatus:	a boolean value, 0:AC Power failed; 1:AC Power on-line
 */
/*#define LCD_STR_LEN		40
typedef struct _Synohw {
	unsigned long	AcPowerStatus;
	unsigned long	ThermalStage;	
	unsigned long	MemEccCount;
	unsigned long	ButtonStatus;
	unsigned long	HardDiskStatus;
	unsigned long	FanFaultStatus;
	unsigned long	PowerSupplyFault;
	unsigned long	PowerSupplyPresent;
	unsigned long	BatteryStatus;
	unsigned long	BatteryFault;
	char	 	szLcdStr[LCD_STR_LEN];
} SYNOHW;

typedef struct _Synohw_custom {
	unsigned long	obj_1;
	unsigned long	obj_2;	
	unsigned long	obj_3;	
	unsigned long	obj_4;	
	unsigned long	obj_5;	
	unsigned long	obj_6;	
	unsigned long	obj_7;	
	unsigned long	obj_8;	
	unsigned long	obj_9;	
	unsigned long	obj_10;	
	unsigned long	obj_11;	
	unsigned long	obj_12;	
	unsigned long	obj_13;	
	unsigned long	obj_14;	
	unsigned long	obj_15;	
	unsigned long	obj_16;	
	unsigned long	obj_17;	
	unsigned long	obj_18;	
	unsigned long	obj_19;	
	unsigned long	obj_20;	
	unsigned long	obj_21;	
	unsigned long	obj_22;	
	unsigned long	obj_23;	
	unsigned long	obj_24;	
	unsigned long	obj_25;	
	unsigned long	obj_26;	
	unsigned long	obj_27;	
	unsigned long	obj_28;	
	unsigned long	obj_29;	
	unsigned long	obj_30;	
	unsigned long	obj_31;	
	unsigned long	obj_32;	
} SYNOHW_CUSTOM;
*/

typedef struct _SynoMsgPkt {
	long	usNum;
	long	usLen;
	char	szMsg[128];
} SYNOMSGPKT;

#define BIOS_STR_LEN_MAX	32

/*typedef struct __bvs {
	int	addr;
	char	szBStr[BIOS_STR_LEN_MAX];
} BVS;

#define valueLongPointer(a) (*((unsigned long *) a))
#define FIELD_SIZE	4
#define LEN_SIZE	4

#define APM_SUCCESS	0
#define APM_FAILURE	1
#define APM_NOT_PRESENT -1

#define USE_BY_CONSOLE	1
#define USE_BY_UPS	0
*/

/*
 * SYNOBIOS Events (status change of SynoBIOS)
 */
#define SYNO_EVENT_CPU_THERMAL_NORMAL   0x0100
#define SYNO_EVENT_CPU_THERMAL_WARM     0x0101
#define SYNO_EVENT_CPU_THERMAL_HEAT     0x0102
#define SYNO_EVENT_CPU_THERMAL_CRITICAL 0x0103

#define SYNO_EVENT_BUTTON_0                  0x0200
#define SYNO_EVENT_BUTTON_MANUTIL            0x0200
#define SYNO_EVENT_BUTTON_NORMAL             0x0201

#define SYNO_EVENT_BUTTON_1                  0x0210
#define SYNO_EVENT_BUTTON_2                  0x0220

#define SYNO_EVENT_BUTTON_SHUTDOWN_PUSHED    0x0221
#define SYNO_EVENT_BUTTON_SHUTDOWN_RELEASED  0x0222
#define SYNO_EVENT_BUTTON_SHUTDOWN           0x0223
#define SYNO_EVENT_BUTTON_BUZZER_CLEAR       0x0224

#define SYNO_EVENT_BUTTON_3                  0x0230
#define SYNO_EVENT_BUTTON_RESET              0x0230

#define SYNO_EVENT_BUTTON_4                  0x0240
#define SYNO_EVENT_USBCOPY_START	     0x0240

#define SYNO_EVENT_BUTTON_5                  0x0250
#define SYNO_EVENT_BUTTON_6                  0x0260
#define SYNO_EVENT_BUTTON_7                  0x0270

#define SYNO_EVENT_RM_HD0               0x0300
#define SYNO_EVENT_PLUG_HD0             0x0400
#define SYNO_EVENT_MEM_ECC_1            0x0501
#define SYNO_EVENT_MEM_ECC_2_OR_MORE    0x0502

#define SYNO_EVENT_ACPOWER_FAIL         0x0600
/*#define SYNO_EVENT_IOERR_HD0            0x0700*/

#define	SYNO_EVENT_FAN_FAIL             0x0900
#define SYNO_EVENT_POWERSUPPLY_FAIL     0x0a00
#define SYNO_EVENT_RM_BATTERY           0x0b00
#define SYNO_EVENT_BATTERY_FAULT        0x0c00

#define SYNO_EVENT_FAN_RECOVER          0x0d00
#define SYNO_EVENT_BATTERY_RECOVER      0x0e00
#define SYNO_EVENT_INSERT_BATTERY       0x0f00
#define SYNO_EVENT_POWERSUPPLY_RECOVER  0x1000

#define SYNO_EVENT_NETCARD_ATTACH       0x1100
#define SYNO_EVENT_NETCARD_DETACH       0x1200

#define SYNO_EVENT_RM_SCSI0             0x1300
#define SYNO_EVENT_PLUG_SCSI0           0x1400

#define SYNO_EVENT_HDFAIL_HD0           0x1500
#define SYNO_EVENT_HDRESUME_HD0         0x1600

#define SYNO_EVENT_IOERR_HD0            0x1700

#define SYNO_EVENT_USB_ERROR            0x1800

#define SYNO_EVENT_RAID                 0x1900

#ifdef MY_ABC_HERE
#define SYNO_EVENT_HIBERNATION			0x2000
#endif
#define SYNO_EVENT_SHUTDOWN_LOG			0x2200
#define SYNO_EVENT_EBOX_REFRESH      0x2300

#ifdef MY_ABC_HERE
#define SYNO_EVENT_ECC_NOTIFICATION		0x2400
#endif

#ifdef MY_ABC_HERE
#define SYNO_EVENT_RAID_REMAP_RECORD 0x2500
#define SYNO_EVENT_LV_REMAP_RECORD 0x2600
#endif

#define SYNO_EVENT_DISK_PWR_RESET 0x2700
#define SYNO_EVENT_DISK_PORT_DISABLED 0x2800

#define SYNO_EVENT_BACK_TEMP_CRITICAL   0x4004
#define SYNO_EVENT_BACK_TEMP_HIGH       0x4003
#define SYNO_EVENT_BACK_TEMP_HEAT       0x4002
#define SYNO_EVENT_BACK_TEMP_WARM       0x4001
#define SYNO_EVENT_CASE_TEMP_CRITICAL   0x4104
#define SYNO_EVENT_CASE_TEMP_HIGH       0x4103
#define SYNO_EVENT_CASE_TEMP_HEAT       0x4102
#define SYNO_EVENT_CASE_TEMP_WARM       0x4101
#define SYNO_EVENT_VOLPIN0_ABNORMAL     0x4200
#define SYNO_EVENT_VOLPIN1_ABNORMAL     0x4201
#define SYNO_EVENT_VOLPIN2_ABNORMAL     0x4202
#define SYNO_EVENT_VOLPIN3_ABNORMAL     0x4203
#define SYNO_EVENT_VOLPIN4_ABNORMAL     0x4204
#define SYNO_EVENT_VOLPIN5_ABNORMAL     0x4205
#define SYNO_EVENT_VOLPIN6_ABNORMAL     0x4206
#define SYNO_EVENT_VOLPIN7_ABNORMAL     0x4207
#define SYNO_EVENT_VOLPIN8_ABNORMAL     0x4208
#define SYNO_EVENT_VOLPIN9_ABNORMAL     0x4209

#define SYNO_EVENT_USBSTATION_RESET 	0x5000
#define SYNO_EVENT_USBSTATION_EJECT 	0x5001

#define SYNO_EVENT_WIFIWPS              0x6000

#ifdef SYNO_WIFI_WORKAROUND
#define SYNO_EVENT_WIFI_NOTIFICATION    0x6001
#endif /* SYNO_WIFI_WORKAROUND */

#define DRIVER_CLASS_FXP                0x00
#define DRIVER_CLASS_EM                 0x10
#define DRIVER_CLASS_RL                 0x20


/*************************************************************************
 * Ioctl definitions
 ************************************************************************/
/*MUST mentain the order in RTC structure*/

typedef struct _SynoRtcControlPkt {
	char    ControlR_1;
	char    ControlR_2;
} SYNORTCCONTROLPKT;

typedef struct _SynoRtcTimePkt {
	unsigned char    sec;
	unsigned char    min;
	unsigned char    hour;
	unsigned char    weekday;
	unsigned char    day;
	unsigned char    month;
	/// Value starts from 2000. Value 70 ~ 99 (in hex) represents year 2070 ~ 2099 now.
	unsigned char    year;
} SYNORTCTIMEPKT;

#define MAX_CPU 2
typedef struct _SynoCpuTemp {
	unsigned char blSurface;
	int cpu_num;	
	int cpu_temp[MAX_CPU];
} SYNOCPUTEMP;

/* for auto poweron functions */
#define I2C_RTC_ADDR            0x32
#define I2C_RTC_TIMER_OFFSET    0x00
#define I2C_RTC_TIME_TRIM_OFFSET 0x07
#define I2C_RTC_ALARMA_OFFSET   0x08
#define I2C_RTC_ALARMB_OFFSET   0x0B
#define I2C_RTC_CONTROL1_OFFSET 0x0E
#define I2C_RTC_CONTROL2_OFFSET 0x0F

#define I2C_RTC_ALARMA_ENABLE   0x80
#define I2C_RTC_ALARMB_ENABLE   0x40
#define I2C_RTC_ALARMAB_SL      0x20

#define I2C_RTC_ALARMA_24HOUR   0x20

#define I2C_TEMPERATURE_ADDR    0x48

typedef enum {
	AUTO_POWERON_SUN = 0x01,
	AUTO_POWERON_MON = 0x02,
	AUTO_POWERON_TUE = 0x04,
	AUTO_POWERON_WED = 0x08,
	AUTO_POWERON_THU = 0x10,
	AUTO_POWERON_FRI = 0x20,
	AUTO_POWERON_SAT = 0x40,
} AUTO_POWERON_WEEKDAY;
#define AUTO_POWERON_WEEKDAY_MASK ( AUTO_POWERON_SUN | \
									AUTO_POWERON_MON | \
									AUTO_POWERON_TUE | \
									AUTO_POWERON_WED | \
									AUTO_POWERON_THU | \
									AUTO_POWERON_FRI | \
									AUTO_POWERON_SAT )

typedef enum {
	SYNO_AUTO_POWERON_DISABLE = 0,
	SYNO_AUTO_POWERON_ENABLE = 1,
} SYNO_AUTO_POWERON_EN;

typedef struct _SynoRtcAlarmPkt {
	unsigned char    min;			// BCD formatted 24-hour: 17 --> 0x23, we can call DEC2BCD() to convert this field.
	unsigned char    hour;			// BCD formatted 24-hour: 17 --> 0x23, we can call DEC2BCD() to convert this field.
	unsigned char    weekdays;		// 7-bit day of week: [SUN][MON][TUE][WED][THU][FRI][SAT]
} SYNORTCALARMPKT;

#define MAX_POWER_SCHEDULE_TASKS 100

typedef struct _SynoAutoPowerOn {
	int num;
	SYNO_AUTO_POWERON_EN enabled;
	SYNORTCALARMPKT RtcAlarmPkt[MAX_POWER_SCHEDULE_TASKS];
} SYNO_AUTO_POWERON;

/*for test only*/
typedef struct _SynoRtcPkt {
	char    rg[16];          
} SYNORTCPKT;

typedef struct _SynoPWMCTL {
	int blSetPWM;
	int hz;
	int duty_cycle;
	int rpm;
} SynoPWMCTL;

typedef struct _tag_SynobiosEvent {
    unsigned int event;
    unsigned int data1;
    unsigned int data2;
    unsigned int data3;
    unsigned int data4;
} SYNOBIOSEVENT;

enum {
    MD_SECTOR_READ_ERROR = 0,
    MD_SECTOR_WRITE_ERROR = 1,
    MD_SECTOR_REWRITE_OK = 2,
};

typedef enum {
	DISK_WAKE_UP = 0,
} SYNO_DISK_HIBERNATION_EVENT;

typedef enum {
	SYNO_LED_OFF = 0,
	SYNO_LED_ON,
	SYNO_LED_BLINKING,	
} SYNO_LED;

typedef enum {
	DISK_LED_OFF = 0,
	DISK_LED_GREEN_SOLID,
	DISK_LED_ORANGE_SOLID,
	DISK_LED_ORANGE_BLINK,
	DISK_LED_GREEN_BLINK,
} SYNO_DISK_LED;

typedef struct _tag_DiskLedStatus {
    int diskno;
    SYNO_DISK_LED status;
} DISKLEDSTATUS;

typedef	enum {
	FAN_STATUS_UNKNOWN = -1,
	FAN_STATUS_STOP = 0,
	FAN_STATUS_RUNNING = 1,
} FAN_STATUS;

typedef	enum {
    BACKPLANE_STATUS_UNKNOWN = -1,
	BACKPLANE_STATUS_ERROR = 0,
	BACKPLANE_STATUS_NORMAL = 1,
} BACKPLANE_STATUS;

typedef enum {
    TEMPERATURE_OVER_NORMAL = -7,
} SYNO_SHUTDOWN_LOG;

/* When you add a new FAN_SPEED. Please modify the parsing in libsynosdk/external/?match.c */
typedef	enum {
	/* The FAN_SPEED_* is ordered from low to high speed.
 	 * Note, the FAN_SPEED_UNKNOWN = 0, must not move, we will init FAN_SPEED value to FAN_SPEED_UNKNOWN
	 * and will use it to compare with other speeds. So it must at the lowest order */
	FAN_SPEED_UNKNOWN = 0,
	FAN_SPEED_STOP,
	FAN_SPEED_ULTRA_LOW,
	FAN_SPEED_VERY_LOW,
	FAN_SPEED_LOW,
	FAN_SPEED_MIDDLE,
	FAN_SPEED_HIGH,
	FAN_SPEED_VERY_HIGH,
	FAN_SPEED_ULTRA_HIGH,
	FAN_SPEED_FULL,
	/* FAN_SPEED_TEST_X is used inside manutild for fan testing*/
	FAN_SPEED_TEST_0,
	FAN_SPEED_TEST_1,
	FAN_SPEED_TEST_2,
	FAN_SPEED_TEST_3,
	FAN_SPEED_TEST_4,
	FAN_SPEED_TEST_5,
	FAN_SPEED_TEST_6,
	FAN_SPEED_TEST_7,
	/* PWM format is only for bandon and QC test */
	FAN_SPEED_PWM_FORMAT_SHIFT = 1000,
} FAN_SPEED;

/* Mapping FAN_SPEED to duty cycle */
typedef struct _tag_PWM_FAN_SPEED_MAPPING_ {
	FAN_SPEED fanSpeed;
	int       iDutyCycle; // Range [0, 99]
} PWM_FAN_SPEED_MAPPING;

typedef struct _tag_FanStatus {
	int        fanno;   // CS406 ==>1, RS406 ==> 1,2,3
	FAN_STATUS status;  // 1 ==> stop, 0 ==> running
	FAN_SPEED  speed;   // 1 ==> high, 0 ==> low
} FANSTATUS;

/* shift 8bit and set hz value in it */
#define FAN_SPEED_SHIFT_SET(duty_cycle, hz)   ((duty_cycle) + ((hz) << 8) + FAN_SPEED_PWM_FORMAT_SHIFT)
#define FAN_SPEED_SHIFT_HZ_GET(speed)		  (((speed) - FAN_SPEED_PWM_FORMAT_SHIFT) >> 8)   /* shift 8bit and get hz value  */
#define FAN_SPEED_SHIFT_DUTY_GET(speed)		  (((speed) - FAN_SPEED_PWM_FORMAT_SHIFT) & 0xFF) /* duty cycle is set in the first 8bit, so get it in first 8bit */

// Synology Disk Station Brand
enum {
	BRAND_SYNOLOGY		= 0x00,
	BRAND_LOGITEC		= 0x01,
	BRAND_SYNOLOGY_USA	= 0x02,
};

// for hardware capabilities
#define MAX_CAPABILITY 4

typedef enum {
	CAPABILITY_THERMAL      = 1,
	CAPABILITY_DISK_LED_CTRL= 2,
	CAPABILITY_AUTO_POWERON = 3,
	CAPABILITY_CPU_TEMP     = 4,
	CAPABILITY_S_LED_BREATH = 5,
	CAPABILITY_FAN_RPM_RPT  = 6,
	CAPABILITY_MICROP_PWM   = 7,
	CAPABILITY_CARDREADER	= 8,
	CAPABILITY_LCM		= 9,
	CAPABILITY_NONE         = -1,
} SYNO_HW_CAPABILITY;

typedef enum {
	POWER_STATUS_BAD = 0,
	POWER_STATUS_GOOD,
} SYNO_POWER_STATUS;

typedef struct _tag_CAPABILITY {
	SYNO_HW_CAPABILITY      id;
	int	support;
} CAPABILITY;

// FIXME: only 4 bits, and at most 16 kinds of type fan_t, so please be careful
typedef enum {
	FAN_UNKNOWN,
	FAN_CPLD_SPEED_1LEVEL,
	FAN_CPLD_SPEED_2LEVEL,
	FAN_MULTI_EXCEPT_SLEEP,
	FAN_MICROP_ALERT_FIRM,
	FAN_MICROP_ALERT_SOFT,
	FAN_MICROP_PWM,
	FAN_MULTI_ALWAYS,
	FAN_MICROP_PWM_WITH_CPUFAN,
	FAN_MICROP_PWM_WITH_GPIO,
	FAN_MICROP_PWM_WITH_CPUFAN_AND_GPIO,
} FAN_T;

typedef enum {
	LED_UNKNOWN,
	LED_DISKS,
	LED_DISK_ESATA,
	LED_DISKS_ALARM,
} LED_T;

typedef enum {
	RAID_UNKNOWN,
	RAID_ON,
	RAID_OFF,
} RAID_T;

typedef enum {
	THERMAL_UNKNOWN,
	THERMAL_YES,
	THERMAL_NO,
}THERMAL_T;

//Support get CPU temperature or not
typedef enum {
	CPUTMP_UNKNOWN,
	CPUTMP_YES,
	CPUTMP_NO,
}CPUTMP_T;

typedef enum {
	DISK_LED_CTRL_UNKNOWN,
	DISK_LED_CTRL_SW,
	DISK_LED_CTRL_HW,
}DISK_LED_CTRL_T;

typedef enum {
	AUTO_POWERON_UNKNOWN,
	AUTO_POWERON_YES,
	AUTO_POWERON_NO,
}AUTO_POWERON_T;

typedef enum {
	POWER_UNKNOWN,
	POWER_SINGLE,
	POWER_DUAL,
}DUAL_POWER_T;

typedef enum {
	USBCOPY_UNKNOWN,
	USBCOPY_YES,
	USBCOPY_NO,
	USBCOPY_MUTE,
}USBCOPY_T;

typedef enum {
	FAN_NUMBER_X = 0,
	FAN_NUMBER_1 = 1,
	FAN_NUMBER_2 = 2,
	FAN_NUMBER_3 = 3,
	FAN_NUMBER_4 = 4,
}FAN_NUMBER_T;

typedef enum _tag_EUNIT_PWRON_TYPE {
	EUNIT_NOT_SUPPORT,
	EUNIT_PWRON_GPIO,
	EUNIT_PWRON_ATACMD,
}EUNIT_PWRON_TYPE;

typedef enum _tag_POWER_IN_SEQ {
	POWER_IN_SEQ_OFF,
	POWER_IN_SEQ_2,
	POWER_IN_SEQ_4,
}POWER_IN_SEQ;

typedef enum _tag_RTC_TYPE {
	RTC_UNKNOWN,
	RTC_RICOH,
	RTC_SEIKO,
	RTC_BANDON,
	RTC_MV,
}RTC_TYPE;

//Support 
typedef enum {
	S_LED_UNKNOWN,
	S_LED_NORMAL,
	S_LED_BREATH,
}STATUS_LED_T;

typedef enum {
	FAN_RPM_RPT_UNKNOWN,
	FAN_RPM_RPT_YES,
	FAN_RPM_RPT_NO,
}FAN_RPM_RPT_T;

typedef enum {
	LCM_UNKNOWN,
	LCM_YES,
	LCM_NO
}LCM_T;

typedef enum {
	WIFI_WPS_NO = 0x00,
	WIFI_WPS_213air = 0x28, /* GPIO PIN 40  */
	WIFI_WPS_UNKNOWN = 0xFF
}WIFI_WPS_T;

typedef enum {
	CPU_FREQ_ADJUST_UNKNOWN,
	CPU_FREQ_ADJUST_YES,
	CPU_FREQ_ADJUST_NO,
}CPUFREQ_ADJUST_T;

typedef enum {
	CARDREADER_UNKNOWN,
	CARDREADER_YES,
	CARDREADER_NO,
}CARDREADER_T;

typedef enum { 
	MICROP_PWM_UNKNOWN,
	MICROP_PWM_YES,
	MICROP_PWM_NO,
}MICROP_PWM_T;

typedef enum {
	HIBER_LED_UNKNOWN,
	HIBER_LED_NORMAL,
	HIBER_LED_ALLOUT,
	HIBER_LED_EXCEPTPWR,
}HIBERNATE_LED_T;

typedef enum {
	RTC_NOT_NEED_TO_CORRECT = 0x00,
	RTC_SEIKO_CORR_DEFAULT  = 0x03,
	RTC_SEIKO_CORR_106B1    = 0x57,
	RTC_RICOH_CORR_DEFAULT  = 0x0F,
}RTC_CORRECTION_T;

typedef enum {
	FAN_FAIL_ADJUST_NO,
	FAN_FAIL_ADJUST_FULL,
}FAN_FAIL_ADJUST_T;

typedef enum {
	MICROP_ID_710p = 0x31, /* '1' */
	MICROP_ID_411p = 0x33, /* '3' 411+II is the same*/
	MICROP_ID_1010p = 0x32, /* '2' */
	MICROP_ID_1511p = 0x36, /* '6' */
	MICROP_ID_810p = 0x35, /* '5' */
	MICROP_ID_810rp = 0x34, /* '4' */
	MICROP_ID_2211p = 0x37, /* '7' */
	MICROP_ID_2211rp = 0x38, /* '8' */
	MICROP_ID_2411p = 0x39, /* '9' */
	MICROP_ID_3411xs = 0x43, /* 'C' 3412xs is the same */
	MICROP_ID_3411rpxs = 0x41, /* 'A' 3412rpxs is the same */
	MICROP_ID_3611xs = 0x42, /* 'B' 3612xs is the same*/
	MICROP_ID_712p = 0x44, /* 'D' */
	MICROP_ID_412p = 0x45, /* 'E' */
	MICROP_ID_1512p = 0x47, /* 'G' */
	MICROP_ID_1812p = 0x46, /* 'F' */
	MICROP_ID_812p = 0x48, /* 'H' */
	MICROP_ID_812rp = 0x49, /* 'I' */
	MICROP_ID_2212p = 0x4A, /* 'J' */
	MICROP_ID_2212rp = 0x4B, /* 'K' */
	MICROP_ID_2413p = 0x4C, /* 'L' */
	MICROP_ID_10613xsp = 0x4d, /* 'M' */
	MICROP_ID_3413xsp = 0x4e, /* 'N' */
	MICROP_ID_913p = 0x4f, /* 'O' */
	MICROP_ID_713p = 0x50, /* 'P' */
	MICROP_ID_UNKNOW = 0xFF,
} SYNO_MICROP_ID;

/* This is for grpup wake setting
 * [7-4]: how many disks are in one group
 * [3-0]: the deno of 7s, it means how
 *        many seconds between each groups
 * ex. 0x47 means 4 disks in one gorup,
 *     and delay 7/7 = 1s between each waking groups
 **/
typedef enum {
	ONE_DISK_DENO_ONE = 0x11,     /* this is default settings */
	FOUR_DISK_DENO_SEVEN = 0x47,  /* if the model is >=8bay, you must use this */
	UNKNOW_DISK_DENO = 0x00,
} GROUP_WAKE_CONFIG_T;

/**
 * This structure is used to store types of each module
 * in different DS models, including module fan type,
 * module raid type, ...
 */
typedef struct {
	FAN_T          fan_type          :4;
	RAID_T         raid_type         :2;
	LED_T          led_type          :4;
	THERMAL_T      thermal_type      :2;
	DISK_LED_CTRL_T diskled_ctrl_type:2;
	AUTO_POWERON_T auto_poweron_type :2;
	DUAL_POWER_T   dual_power_type   :2;
	USBCOPY_T      usbcopy_type      :2;
	FAN_NUMBER_T   fan_number        :4;
	EUNIT_PWRON_TYPE eunit_pwron_type:4;
	POWER_IN_SEQ   pis_type          :4;
	RTC_TYPE       rtc_type          :4;
	CPUTMP_T       cputmp_type       :2;
	STATUS_LED_T   status_led_type   :2;
	FAN_RPM_RPT_T  fan_rpm_rpt_type  :2;
	LCM_T          lcm_type		 :2;
	WIFI_WPS_T		wifi_wps_type	 :8;
	CPUFREQ_ADJUST_T cpu_freq_adjust :2;
	CARDREADER_T   has_cardreader    :2;
	HIBERNATE_LED_T  hibernate_led   :2;
	RTC_CORRECTION_T rtc_corr_value  :8;
	FAN_FAIL_ADJUST_T fan_fail_adjust:2;
	SYNO_MICROP_ID microp_id         :8;
	GROUP_WAKE_CONFIG_T group_wake_config :8;
} __attribute__((packed)) module_t;

#define HW_DS107e      "DS107e"
#define HW_DS107v10    "DS107v10"
#define HW_DS107v20    "DS107v20"
#define HW_DS107v30    "DS107v30"
#define HW_DS207       "DS207"         //"DS207v10"
#define HW_DS406       "DS406"         //"DS406v10"  "DS406v20"
#define HW_DS407ev10   "DS407ev10"     //"DS407ev10"
#define HW_DS407v10    "DS407v10"      //"DS407v10"
#define HW_DS407v20    "DS407v20"      //"DS407v20"
#define HW_RS408       "RS408"         //"RS408"
#define HW_RS408rp     "RS408rp"       //"RS408rp"
#define HW_RS409p      "RS409p"        //"RS409p"
#define HW_RS409rpp    "RS409rpp"      //"RS409rpp"
#define HW_DS408       "DS408"         //"DS408"
#define HW_DS409p      "DS409p"        //"DS409p"
#define HW_DS409pv20   "DS409pv20"     //"DS409pv20"
#define HW_DS508       "DS508"         //"DS508"
#define HW_DS509p      "DS509p"        //"DS509p"
#define HW_DS209p      "DS209p"        //"DS209p"
#define HW_DS209pII    "DS209pII"      //"DS209pII"
#define HW_DS209pIIr1  "DS209pIIr1"    //"DS209pIIr1"
#define HW_DS108jv10   "DS108jv10"     //"DS108jv10" 
#define HW_DS108jv20   "DS108jv20"     //"DS108jv20" 
#define HW_DS109j      "DS109jv10"     //"DS109jv10"
#define HW_DS209j      "DS209jv10"     //"DS209jv10"
#define HW_DS109       "DS109"         //"DS109"
#define HW_DS209       "DS209"         //"DS209"
#define HW_DS409slim   "DS409slim"     //"DS409slim"
#define HW_DS409       "DS409"         //"DS409"
#define HW_RS409v10    "RS409v10"      //"RS409v10"
#define HW_RS409v20    "RS409v20"      //"RS409v20"
#define HW_DS109p      "DS109p"        //"DS109p"
#define HW_DS410j      "DS410j"        //"DS410j"
#define HW_DS210jv10   "DS210jv10"     //"DS210jv10"
#define HW_DS210jv20   "DS210jv20"     //"DS210jv20"
#define HW_DS210jv30   "DS210jv30"     //"DS210jv30"
#define HW_DS110jv10   "DS110jv10"     //"DS110jv10"
#define HW_DS110jv20   "DS110jv20"     //"DS110jv20"
#define HW_DS110jv30   "DS110jv30"     //"DS110jv30"
#define HW_DS710p      "DS710+"        //"DS710+"
#define HW_DS712pv10   "DS712+"        //"DS712+"
#define HW_DS712pv20   "DS712+v20"     //"DS712+v20"
#define HW_DS1010p     "DS1010+"       //"DS1010+"
#define HW_DS110p      "DS110p"        //"DS110+"
#define HW_DS210p      "DS210p"        //"DS210+"
#define HW_DS410       "DS410"         //"DS410"
#define HW_DS411p      "DS411+"        //"DS411+"
#define HW_DS411pII    "DS411+II"      //"DS411+II"
#define HW_RS810p      "RS810+"        //"RS810+"
#define HW_RS810rpp    "RS810rp+"      //"RS810rp+"
#define HW_DS211j      "DS211j"        //"DS211j"
#define HW_DS411j      "DS411j"        //"DS411j"
#define HW_DS211       "DS211"         //"DS211"
#define HW_DS111       "DS111"         //"DS111"
#define HW_DS411slim   "DS411slim"     //"DS411slim"
#define HW_DS411       "DS411"         //"DS411"
#define HW_DS1511p     "DS1511+"       //"DS1511+"
#define HW_DS211pv10   "DS211pv10"     //"DS211+"
#define HW_DS211pv20   "DS211pv20"     //"DS211+"
#define HW_RS411       "RS411"         //"RS411"
#define HW_RS2211p     "RS2211+"       //"RS2211+"
#define HW_RS2211rpp   "RS2211rp+"     //"RS2211rp+"
#define HW_DS2411p     "DS2411+"       //"DS2411+"
#define HW_RS3411rpxs  "RS3411rpxs"    //"RS3411rpxs"
#define HW_RS3411xs    "RS3411xs"      //"RS3411xs"
#define HW_RS10613xsp  "RS10613xs+"    //"RS10613xs+"
#define HW_DS3611xs    "DS3611xs"      //"DS3611xs"
#define HW_RS3412rpxs  "RS3412rpxs"    //"RS3412rpxs"
#define HW_RS3412xs    "RS3412xs"      //"RS3412xs"
#define HW_DS3612xs    "DS3612xs"      //"DS3612xs"
#define HW_RS3413xsp    "RS3413xs+"      //"RS3413xs+"
#define HW_DS111j      "DS111j"        //"DS111j"
#define HW_DS212       "DS212"         //"DS212v10"
#define HW_DS413       "DS413"         //"DS413"
#define HW_DS412p      "DS412+"        //"DS412+"
#define HW_DS913p      "DS913+"        //"DS913+"
#define HW_DS713p      "DS713+"        //"DS713+"
#define HW_RS812p      "RS812+"        //"RS812+"
#define HW_RS812rpp	   "RS812rp+"      //"RS812rp+"
#define HW_DS1812p     "DS1812+"       //"DS1812+"
#define HW_RS2212p     "RS2212+"       //"RS2212+"
#define HW_RS2212rpp   "RS2212rp+"     //"RS2212rp+"
#define HW_DS2413p     "DS2413+"       //"DS2413+"
#define HW_RS212       "RS212"         //"RS212"
#define HW_DS212jv10   "DS212j"        //"DS212j"
#define HW_DS212jv20   "DS212jv20"     //"DS212j"
#define HW_RS812       "RS812"         //"RS812"
#define HW_DS1512p     "DS1512+"       //"DS1512+"
#define HW_DS212pv10   "DS212pv10"     //"DS212+"
#define HW_DS212pv20   "DS212pv20"     //"DS212+"
#define HW_DS112j      "DS112jv10"     //"DS112j"
#define HW_DS112       "DS112v10"      //"DS112"
#define HW_DS112pv10   "DS112pv10"     //"DS112+"
#define HW_DS112slim   "DS112slim"     //"DS112slim"
#define HW_DS413jv10   "DS413jv10"     //"DS413jv10"
#define HW_DS213pv10   "DS213pv10"     //"DS213pv10"
#define HW_DS213airv10 "DS213airv10"   //"DS213airv10"
#define HW_DS213v10    "DS213v10"      //"DS213v10"
#define HW_NVR413v10   "NVR413v10"     //"NVR413v10"
#define HW_VS240hdv10  "VS240hdv10"    //"VS240hdv10"
#define HW_RS813       "RS813v10"      //"RS813v10"
#define HW_RS213p      "RS213pv10"     //"RS213pv10"
#define HW_RS213       "RS213v10"      //"RS213v10"
#define HW_UNKNOWN     "DSUnknown"
									    
typedef struct _tag_HwCapability {
	char*           szHwVersion;
	CAPABILITY      capability[MAX_CAPABILITY];
} HwCapability;

// Synology Disk Station model
typedef enum {
	MODEL_CS406e    = 0x00,
	MODEL_CS406     = 0x01,
	MODEL_RS406     = 0x02,
	MODEL_DS107e,
	MODEL_DS107,
	MODEL_DS107mv,
	MODEL_DS207,
	MODEL_RS407,
	MODEL_CS407,
	MODEL_CS407e,
	MODEL_DS207mv,
	MODEL_DS107r2,
	MODEL_DS508,
	MODEL_RS408,
	MODEL_DS408,
	MODEL_RS408rp,
	MODEL_DS409p,
	MODEL_DS509p,
	MODEL_RS409p,
	MODEL_RS409rpp,
	MODEL_DS209p,
	MODEL_DS209pII,
	MODEL_DS108j,
	MODEL_DS109j,
	MODEL_DS209j,
	MODEL_DS109,
	MODEL_DS209,
	MODEL_DS409slim,
	MODEL_DS409,
	MODEL_RS409,
	MODEL_DS109p,
	MODEL_DS410j,
	MODEL_DS210j,
	MODEL_DS110j,
	MODEL_DS710p,
	MODEL_DS712p,
	MODEL_DS1010p,
	MODEL_DS210p,
	MODEL_DS410,
	MODEL_DS411p,
	MODEL_DS411pII,
	MODEL_DS110p,
	MODEL_RS810p,
	MODEL_RS810rpp,
	MODEL_DS211j,
	MODEL_DS411j,
	MODEL_DS211,
	MODEL_DS011j,
	MODEL_DS111,
	MODEL_DS411slim,
	MODEL_DS411,
	MODEL_DS1511p,
	MODEL_DS211p,
	MODEL_RS3411rpxs,
	MODEL_RS3411xs,
	MODEL_RS10613xsp,
	MODEL_DS3611xs,
	MODEL_RS3412rpxs,
	MODEL_RS3412xs,
	MODEL_DS3612xs,
	MODEL_RS3413xsp,
	MODEL_RS411,
	MODEL_DS111j,
	MODEL_RS2211p,
	MODEL_RS2211rpp,
	MODEL_DS2411p,
	MODEL_DS212,
	MODEL_DS413,
	MODEL_DS412p,
	MODEL_DS913p,
	MODEL_DS713p,
	MODEL_RS812p,
	MODEL_RS812rpp,
	MODEL_DS1812p,
	MODEL_RS2212p,
	MODEL_RS2212rpp,
	MODEL_DS2413p,
	MODEL_RS212,
	MODEL_DS212j,
	MODEL_RS812,
	MODEL_DS1512p,
	MODEL_DS212p,
	MODEL_DS112j,
	MODEL_DS112,
	MODEL_DS112p,
	MODEL_DS112slim,
	MODEL_DS413j,
	MODEL_DS213p,
	MODEL_DS213air,
	MODEL_DS213,
	MODEL_NVR413,
	MODEL_VS240hd,
	MODEL_RS813,
	MODEL_RS213p,
	MODEL_RS213,
	MODEL_INVALID
} PRODUCT_MODEL;

typedef struct _tag_SYNO_MODEL_NAME {
	PRODUCT_MODEL	model;
	char*			szHwVersion;
} SYNO_MODEL_NAME;


typedef struct _tag_CPLDReg {
    unsigned char diskledctrl;
    unsigned char diskpowerstate;
    unsigned char modelnumber;
    unsigned char fanstatus;
} CPLDREG;

typedef struct _tag_MEMORY_BYTE {
    unsigned char offset;
    unsigned char value;
} MEMORY_BYTE;

typedef struct _tag_GPIO_PIN {
    int pin;
    int value;
} GPIO_PIN;

typedef struct _tag_POWER_INFO {
	SYNO_POWER_STATUS power_1;
	SYNO_POWER_STATUS power_2;
} POWER_INFO;

typedef enum {
    NET_LINK = 0,
	NET_NOLINK,
} SYNO_NET_LINK_EVENT;


/** 
 * from first 0 bit to 6th bit is signature 
 * 7th bit indicate component status, true is compoent fail 
 * the others bits are id
 */ 
typedef unsigned int sys_comp_stat_t;
#define COMP_STAT_BITS						sizeof(sys_comp_stat_t)*8

#define SYS_STAT_SIGNATURE_MASK 0x7F
#define SYS_STAT_COMPONENT_MASK 0x80
#define SYS_STAT_ID_MASK ~(0xFF)

#define SYS_STAT_COMPONENT_SHIFT 7
#define SYS_STAT_IDX_SHIFT 8

#define DEFINE_COMP_STAT(x)					sys_comp_stat_t x = 0

#define ID_LIST_GET(sys_comp_stat_t)		(sys_comp_stat_t & SYS_STAT_ID_MASK)
#define ID_LIST_ENCODE(idx)					((1 << idx) << SYS_STAT_IDX_SHIFT)
#define ID_LIST_DECODE(sys_comp_stat_t)		(sys_comp_stat_t >> SYS_STAT_IDX_SHIFT)
#define ID_LIST_EMPTY(sys_comp_stat_t)		(0 == ID_LIST_DECODE(sys_comp_stat_t))
#define SIGNATURE_GET(sig)					(sig & SYS_STAT_SIGNATURE_MASK)
#define SIGNATURE_INCLUDE(sys_comp_stat_t)	(sys_comp_stat_t & SYS_STAT_SIGNATURE_MASK)
#define COMP_STAT_ENCODE(boolean)			(boolean << SYS_STAT_COMPONENT_SHIFT)
#define COMP_FAIL(sys_comp_stat_t)			(sys_comp_stat_t & SYS_STAT_COMPONENT_MASK)

/**
 * This is the primaty key for look up SYNO_SYS_STATUS.
 * When we want add a new component status. You must add a new
 * signature too
 */ 
typedef enum {
	SIGNATURE_FAN_FAIL = 0x1,
	SIGNATURE_VOLUME_DEGRADE = 0x2,
	SIGNATURE_VOLUME_CRASHED = 0x3,
	SIGNATURE_POWER_FAIL = 0x4,
	SIGNATURE_EBOX_FAN_FAIL = 0x5,
	SIGNATURE_CPU_FAN_FAIL = 0x6,
} SYNO_SYS_STAT_SIGNATURE;

/**
 * These are system components stat, we don't use array here. 
 * Because it still need hard code signature in synobios.c 
 */ 
typedef struct _tag_SYNO_SYS_STATUS {
	sys_comp_stat_t fan_fail;
	sys_comp_stat_t volume_degrade;
	sys_comp_stat_t volume_crashed;
	sys_comp_stat_t power_fail;
	sys_comp_stat_t ebox_fan_fail;
	sys_comp_stat_t cpu_fan_fail;
} SYNO_SYS_STATUS;

/* Use 'K' as magic number */
#define SYNOBIOS_IOC_MAGIC  'K'

#define SYNOIO_EXDISPLAY          _IOWR(SYNOBIOS_IOC_MAGIC, 1, struct _SynoMsgPkt)
#define SYNOIO_NEXTEVENT          _IOWR(SYNOBIOS_IOC_MAGIC, 2, u_int)

#define SYNOIO_RTC_CONTROL_READ   _IOWR(SYNOBIOS_IOC_MAGIC, 3, struct _SynoRtcControlPkt)
#define SYNOIO_RTC_CONTROL_WRITE  _IOWR(SYNOBIOS_IOC_MAGIC, 4, struct _SynoRtcControlPkt)
#define SYNOIO_RTC_TIME_READ      _IOWR(SYNOBIOS_IOC_MAGIC, 5, struct _SynoRtcTimePkt)
#define SYNOIO_RTC_TIME_WRITE     _IOWR(SYNOBIOS_IOC_MAGIC, 6, struct _SynoRtcTimePkt)

#define SYNOIO_BUTTON_POWER		_IOWR(SYNOBIOS_IOC_MAGIC, 7, int)
#define SYNOIO_BUTTON_RESET		_IOWR(SYNOBIOS_IOC_MAGIC, 8, int)
#define SYNOIO_BUTTON_USB		_IOWR(SYNOBIOS_IOC_MAGIC, 9, int)

#define SYNOIO_SET_DISK_LED     _IOWR(SYNOBIOS_IOC_MAGIC, 10, DISKLEDSTATUS)
#define SYNOIO_GET_FAN_STATUS   _IOWR(SYNOBIOS_IOC_MAGIC, 11, FANSTATUS)
#define SYNOIO_SET_FAN_STATUS   _IOWR(SYNOBIOS_IOC_MAGIC, 12, FANSTATUS)
#define SYNOIO_GET_DS_MODEL  _IOWR(SYNOBIOS_IOC_MAGIC, 13, int)
#define SYNOIO_GET_DS_BRAND  _IOWR(SYNOBIOS_IOC_MAGIC, 14, int)
#define SYNOIO_GET_CPLD_VERSION _IOWR(SYNOBIOS_IOC_MAGIC, 15, int)
#define SYNOIO_GET_TEMPERATURE  _IOWR(SYNOBIOS_IOC_MAGIC, 16, int)
#define SYNOIO_GET_CPLD_REG  _IOWR(SYNOBIOS_IOC_MAGIC, 17, CPLDREG)
#define SYNOIO_GET_AUTO_POWERON     _IOWR(SYNOBIOS_IOC_MAGIC, 18, SYNO_AUTO_POWERON)
#define SYNOIO_SET_AUTO_POWERON     _IOWR(SYNOBIOS_IOC_MAGIC, 19, SYNO_AUTO_POWERON)
#define SYNOIO_GET_MEM_BYTE  _IOWR(SYNOBIOS_IOC_MAGIC, 20, MEMORY_BYTE)
#define SYNOIO_SET_MEM_BYTE  _IOWR(SYNOBIOS_IOC_MAGIC, 21, MEMORY_BYTE)
#define SYNOIO_SET_ALARM_LED  _IOWR(SYNOBIOS_IOC_MAGIC, 22, unsigned char)
#define SYNOIO_GET_HW_CAPABILITY	_IOWR(SYNOBIOS_IOC_MAGIC, 23, CAPABILITY)
#define SYNOIO_GET_FAN_NUM			_IOWR(SYNOBIOS_IOC_MAGIC, 24, int)
#define SYNOIO_GET_POWER_STATUS			_IOWR(SYNOBIOS_IOC_MAGIC, 25, POWER_INFO)
#define SYNOIO_SHUTDOWN_LOG			_IOWR(SYNOBIOS_IOC_MAGIC, 26, SYNO_SHUTDOWN_LOG)
#define SYNOIO_UNINITIALIZE			_IOWR(SYNOBIOS_IOC_MAGIC, 27, int)
#define SYNOIO_GET_SYS_STATUS		_IOWR(SYNOBIOS_IOC_MAGIC, 28, SYNO_SYS_STATUS)
#define SYNOIO_SET_SYS_STATUS		_IOWR(SYNOBIOS_IOC_MAGIC, 29, sys_comp_stat_t)
#define SYNOIO_GET_MODULE_TYPE		_IOWR(SYNOBIOS_IOC_MAGIC, 30, module_t)
#define SYNOIO_GET_BUZZER_CLEARED	_IOWR(SYNOBIOS_IOC_MAGIC, 31, unsigned char)
#define SYNOIO_GET_BACKPLANE_STATUS  _IOWR(SYNOBIOS_IOC_MAGIC, 32, BACKPLANE_STATUS)
#define SYNOIO_SET_UART2			_IOWR(SYNOBIOS_IOC_MAGIC, 33, unsigned char)
#define SYNOIO_GET_CPU_TEMPERATURE	_IOWR(SYNOBIOS_IOC_MAGIC, 34, SYNOCPUTEMP)
#define SYNOIO_SET_CPU_FAN_STATUS   _IOWR(SYNOBIOS_IOC_MAGIC, 35, FANSTATUS)
#define SYNOIO_SET_PHY_LED     _IOWR(SYNOBIOS_IOC_MAGIC, 36, SYNO_LED)
#define SYNOIO_SET_HDD_LED     _IOWR(SYNOBIOS_IOC_MAGIC, 37, SYNO_LED)
#define SYNOIO_SET_PWR_LED     _IOWR(SYNOBIOS_IOC_MAGIC, 38, SYNO_LED)
#define SYNOIO_PWM_CTL     _IOWR(SYNOBIOS_IOC_MAGIC, 39, SynoPWMCTL)
#define SYNOIO_CHECK_MICROP_ID     _IO(SYNOBIOS_IOC_MAGIC, 40)
#define SYNOIO_GET_EUNIT_TYPE     _IOR(SYNOBIOS_IOC_MAGIC, 41, EUNIT_PWRON_TYPE)

#define SYNOIO_MANUTIL_MODE       _IOWR(SYNOBIOS_IOC_MAGIC, 128, int)
#define SYNOIO_RECORD_EVENT       _IOWR(SYNOBIOS_IOC_MAGIC, 129, int)

#define SYNOIO_GPIO_PIN_READ      _IOWR(SYNOBIOS_IOC_MAGIC, 205, int)
#define SYNOIO_GPIO_PIN_WRITE     _IOWR(SYNOBIOS_IOC_MAGIC, 206, int)


#define SYNOIO_RTC_READ           _IOWR(SYNOBIOS_IOC_MAGIC, 208, struct _SynoRtcPkt)
#define SYNOIO_RTC_READ_3         _IOWR(SYNOBIOS_IOC_MAGIC, 209, struct _SynoRtcPkt)
#define SYNOIO_SDA_SDL_READ       _IOWR(SYNOBIOS_IOC_MAGIC, 210, int)


#ifdef SYNO_BAD_SECTOR_DISK_DEBUG
#define DISK_BADSECTOR_ON      _IOWR(SYNOBIOS_IOC_MAGIC, 211, int)
#define DISK_BADSECTOR_OFF     _IOWR(SYNOBIOS_IOC_MAGIC, 212, int)
#define DISK_BADSECTOR_SET     _IOWR(SYNOBIOS_IOC_MAGIC, 213, struct disk_bs_map_ctlcmd)
#define DISK_BADSECTOR_GET     _IOWR(SYNOBIOS_IOC_MAGIC, 214, struct disk_bs_map_ctlcmd)
#define DISK_BADSECTOR_RESET   _IOWR(SYNOBIOS_IOC_MAGIC, 215, int)
#endif
/*
#define SYNOIO_HWSTATUS		_IOR('A', 102, struct _Synohw)
#define SYNOIO_TESTING		_IOW('P', 104, int)
#define SYNOIO_SERIAL		_IOR('A', 105, int)
#define SYNOIO_HDBACK		_IOW('P', 106, int)
#define SYNOIO_SYNOVER		_IOR('A', 107, unsigned long)
#define SYNOIO_GETSERIALNUM	_IOR('A', 108, off_t)
#define SYNOIO_SETHWSTATUS	_IOW('P', 109, HWBLOCK)
#define SYNOIO_HWHDSUPPORT	_IOR('A', 110, int)
#define SYNOIO_HWTHERMALSUPPORT	_IOR('A', 111, int)
#define SYNOIO_POWEROFF		_IO('A', 114)
#define SYNOIO_SETHWSTATUS_INIT _IOW('P', 111, HWBLOCK)
#define SYNOIO_HWSTATUS_CUSTOM  _IOR('A', 112, struct _Synohw_custom)
#define SYNOIO_SET_EVENT        _IOW('P', 113, u_int)
#define SYNOIO_BIOSVER		_IOWR('P', 115, BVS)
*/
#define SCEM_IOC_TESTING_DISABLE_TEST		0x00
#define SCEM_IOC_TESTING_ENABLE_TEST		0x01

#define SCEM_IOC_TESTING_ACPOWER_ON		0x10
#define SCEM_IOC_TESTING_ACPOWER_OFF		0x11

#define SCEM_IOC_TESTING_SET_OVERHEATING_0	0x20
#define SCEM_IOC_TESTING_SET_OVERHEATING_1	0x21
#define SCEM_IOC_TESTING_SET_OVERHEATING_2	0x22
#define SCEM_IOC_TESTING_SET_OVERHEATING_3	0x23

#define SCEM_IOC_TESTING_SET_MEMORY_ECC_0	0x30
#define SCEM_IOC_TESTING_SET_MEMORY_ECC_1	0x31
#define SCEM_IOC_TESTING_SET_MEMORY_ECC_2	0x32

#define SCEM_IOC_TESTING_SET_BUTTON_0		0x40
#define SCEM_IOC_TESTING_SET_BUTTON_1		0x41
#define SCEM_IOC_TESTING_SET_BUTTON_2		0x42
#define SCEM_IOC_TESTING_SET_BUTTON_3		0x43
#define SCEM_IOC_TESTING_SET_BUTTON_4		0x44
#define SCEM_IOC_TESTING_SET_BUTTON_5		0x45
#define SCEM_IOC_TESTING_SET_BUTTON_6		0x46
#define SCEM_IOC_TESTING_SET_BUTTON_7		0x47
#define SCEM_IOC_TESTING_SET_BUTTON_8		0x48

#define SCEM_IOC_TESTING_RM_HDA			0x51
#define SCEM_IOC_TESTING_RM_HDB			0x52
#define SCEM_IOC_TESTING_RM_HDC			0x53
#define SCEM_IOC_TESTING_RM_HDD			0x54
#define SCEM_IOC_TESTING_RM_HDE			0x55
#define SCEM_IOC_TESTING_RM_HDF			0x56
#define SCEM_IOC_TESTING_RM_HDG			0x57
#define SCEM_IOC_TESTING_RM_HDH			0x58

#define SCEM_IOC_TESTING_PLUG_HDA		0x61
#define SCEM_IOC_TESTING_PLUG_HDB		0x62
#define SCEM_IOC_TESTING_PLUG_HDC		0x63
#define SCEM_IOC_TESTING_PLUG_HDD		0x64
#define SCEM_IOC_TESTING_PLUG_HDE		0x65
#define SCEM_IOC_TESTING_PLUG_HDF		0x66
#define SCEM_IOC_TESTING_PLUG_HDG		0x67
#define SCEM_IOC_TESTING_PLUG_HDH		0x68

#define ACPOWER_FAILURE		1

#define THERMAL_NORMAL		0
#define THERMAL_WARM		1
#define THERMAL_HEAT		2
#define THERMAL_CRITICAL	3

#define NO_MEM_ECC		0
#define MEM_ECC_ONCE		1
#define MEM_ECC_TWICE_OR_MORE	2

#define THERMAL_STAGE(a)				\
        ((a >= iThermal3) ? THERMAL_CRITICAL :		\
        ((a >= iThermal2) ? THERMAL_HEAT :		\
        ((a >= iThermal1) ? THERMAL_WARM : THERMAL_NORMAL)))

#define F_GET_ACPOWER(a)	(a->AcPowerStatus)
#define F_GET_THERMAL(a)	(a->ThermalStage)
#define F_GET_BUTTON(a)		(a->ButtonStatus)
#define F_GET_MEMORY(a)		(a->MemEccCount)

#define F_GET_1_BUTTON(a)	((a->ButtonStatus)&0x0001)
#define F_GET_IPBUTTON(a)	((a->ButtonStatus)&0x0001)
#define F_GET_2_BUTTON(a)	((a->ButtonStatus)&0x0002)
#define F_GET_BACKUP_BUTTON(a)	((a->ButtonStatus)&0x0002)
#define F_GET_3_BUTTON(a)	((a->ButtonStatus)&0x0004)
#define F_GET_4_BUTTON(a)	((a->ButtonStatus)&0x0008)
#define F_GET_5_BUTTON(a)	((a->ButtonStatus)&0x0010)
#define F_GET_6_BUTTON(a)	((a->ButtonStatus)&0x0020)
#define F_GET_7_BUTTON(a)	((a->ButtonStatus)&0x0040)
#define F_GET_8_BUTTON(a)	((a->ButtonStatus)&0x0080)

#define EQ_ACPOWER_STATUS(a,b)	((a->AcPowerStatus)==(b->AcPowerStatus))
#define EQ_THERMAL_STATUS(a,b)	(THERMAL_STAGE(a->ThermalStage)==THERMAL_STAGE(b->ThermalStage))
#define EQ_MEMORY_STATUS(a,b)	((a->MemEccCount)==(b->MemEccCount))
#define EQ_BUTTON_STATUS(a,b)	((a->ButtonStatus)==(b->ButtonStatus))
#define EQ_HD_STATUS(a, b)	((a->HardDiskStatus) == (b->HardDiskStatus))
#define EQ_FAN_STATUS(a, b)	((a->FanFaultStatus) == (b->FanFaultStatus))

#define EQ_BUTTON_1_4(a, b)	((a->ButtonStatus&0x000f)==(b->ButtonStatus&0x000f))
#define EQ_BUTTON_1_2(a, b)	((a->ButtonStatus&0x0003)==(b->ButtonStatus&0x0003))
#define EQ_BUTTON_3_4(a, b)	((a->ButtonStatus&0x000c)==(b->ButtonStatus&0x000c))
#define EQ_BUTTON_5_8(a, b)	((a->ButtonStatus&0x00f0)==(b->ButtonStatus&0x00f0))
#define EQ_BUTTON_5_6(a, b)	((a->ButtonStatus&0x0030)==(b->ButtonStatus&0x0030))
#define EQ_BUTTON_7_8(a, b)	((a->ButtonStatus&0x00c0)==(b->ButtonStatus&0x00c0))
#define EQ_BUTTON_1(a, b)	((a->ButtonStatus&0x0001)==(b->ButtonStatus&0x0001))
#define EQ_BUTTON_2(a, b)	((a->ButtonStatus&0x0002)==(b->ButtonStatus&0x0002))
#define EQ_BUTTON_3(a, b)	((a->ButtonStatus&0x0004)==(b->ButtonStatus&0x0004))
#define EQ_BUTTON_4(a, b)	((a->ButtonStatus&0x0008)==(b->ButtonStatus&0x0008))
#define EQ_BUTTON_5(a, b)	((a->ButtonStatus&0x0010)==(b->ButtonStatus&0x0010))
#define EQ_BUTTON_6(a, b)	((a->ButtonStatus&0x0020)==(b->ButtonStatus&0x0020))
#define EQ_BUTTON_7(a, b)	((a->ButtonStatus&0x0040)==(b->ButtonStatus&0x0040))
#define EQ_BUTTON_8(a, b)	((a->ButtonStatus&0x0080)==(b->ButtonStatus&0x0080))

#define F_GET_HD_STATUS_A(a)	((a->HardDiskStatus&0x0001))
#define F_GET_HD_STATUS_B(a)	((a->HardDiskStatus&0x0002))
#define F_GET_HD_STATUS_C(a)	((a->HardDiskStatus&0x0004))
#define F_GET_HD_STATUS_D(a)	((a->HardDiskStatus&0x0008))
#define F_GET_HD_STATUS_E(a)	((a->HardDiskStatus&0x0010))
#define F_GET_HD_STATUS_F(a)	((a->HardDiskStatus&0x0020))
#define F_GET_HD_STATUS_G(a)	((a->HardDiskStatus&0x0040))
#define F_GET_HD_STATUS_H(a)	((a->HardDiskStatus&0x0080))

#define EQ_HD_STATUS_A_D(a,b)   ((a->HardDiskStatus&0x000f)==(b->HardDiskStatus&0x000f))
#define EQ_HD_STATUS_E_H(a,b)   ((a->HardDiskStatus&0x00f0)==(b->HardDiskStatus&0x00f0))
#define EQ_HD_STATUS_A_B(a,b)   ((a->HardDiskStatus&0x0003)==(b->HardDiskStatus&0x0003))
#define EQ_HD_STATUS_C_D(a,b)   ((a->HardDiskStatus&0x000c)==(b->HardDiskStatus&0x000c))
#define EQ_HD_STATUS_E_F(a,b)   ((a->HardDiskStatus&0x0030)==(b->HardDiskStatus&0x0030))
#define EQ_HD_STATUS_G_H(a,b)   ((a->HardDiskStatus&0x00c0)==(b->HardDiskStatus&0x00c0))
#define EQ_HD_STATUS_A(a,b)	((a->HardDiskStatus&0x0001)==(b->HardDiskStatus&0x0001))
#define EQ_HD_STATUS_B(a,b)	((a->HardDiskStatus&0x0002)==(b->HardDiskStatus&0x0002))
#define EQ_HD_STATUS_C(a,b)	((a->HardDiskStatus&0x0004)==(b->HardDiskStatus&0x0004))
#define EQ_HD_STATUS_D(a,b)	((a->HardDiskStatus&0x0008)==(b->HardDiskStatus&0x0008))
#define EQ_HD_STATUS_E(a,b)	((a->HardDiskStatus&0x0010)==(b->HardDiskStatus&0x0010))
#define EQ_HD_STATUS_F(a,b)	((a->HardDiskStatus&0x0020)==(b->HardDiskStatus&0x0020))
#define EQ_HD_STATUS_G(a,b)	((a->HardDiskStatus&0x0040)==(b->HardDiskStatus&0x0040))
#define EQ_HD_STATUS_H(a,b)	((a->HardDiskStatus&0x0080)==(b->HardDiskStatus&0x0080))

#define PMEV_SYNO_EVENT0	0x0200
#define PMEV_SYNO_EVENT1	0x0201
#define PMEV_SYNO_EVENT2	0x0202
#define PMEV_SYNO_EVENT3	0x0203
#define PMEV_SYNO_EVENT4	0x0204
#define PMEV_SYNO_EVENT5	0x0205
#define PMEV_SYNO_EVENT6	0x0206
#define PMEV_SYNO_EVENT7	0x0207
#define PMEV_SYNO_EVENT8	0x0208
#define PMEV_SYNO_EVENT9	0x0209

#define VMAX              32
#define MAX_HD_NUM        16
#define HD_NUM_MAX        16
#define FAN_NUM_MAX       16
#define POWER_NUM_MAX     16
#define BATTERY_NUM_MAX   16


/*************************************************************************
 * SYNOHWWxternalControl message Id
 ************************************************************************/

#define SVALUE(u, v, d)                 ((v<d)?d:((v>u)?u:v))
#define SYNO_PURE_MESSAGE               0x0000
#define SYNO_DISABLE_CRITICAL           0x0100
#define SYNO_ENABLE_CRITICAL            0x0101
#define SYNO_E_MAIL_SUCCESS             0x0102
#define SYNO_E_MAIL_FAILED              0x0103
#define SYNO_DISABLE_IDENTIFY           0x0200
#define SYNO_ENABLE_IDENTIFY            0x0300
#define SYNO_BEEPER_MUTE                0x0400
#define SYNO_BEEPER_BEEPBEEP            0x0500
#define SYNO_BEEPER_BEEP(num)           (0x0500+SVALUE(50,num,1))
#define SYNO_DISK_FULL(p)               (0x0900+SVALUE(100,p,0))
#define SYNO_CANCEL_DISK_FULL(p)        (0x0800+SVALUE(100,p,0))
#define SYNO_CANCEL_VOLUME_FULL(v,p)    (0x8000+0x100*SVALUE(VMAX,v,0)+SVALUE(100,p,0))
#define SYNO_VOLUME_FULL(v,p)           (0x9000+0x100*SVALUE(VMAX,v,0)+SVALUE(100,p,0))
#define SYNO_DISK_NORMAL(num)           (0x1000+SVALUE(HD_NUM_MAX,num,0))
#define SYNO_DISK_FAILURE(num)          (0x1100+SVALUE(HD_NUM_MAX,num,0))
#define SYNO_DISK_REBUILD(num)          (0x1200+SVALUE(HD_NUM_MAX,num,0))
#define SYNO_DISK_ABSENT(num)           (0x1300+SVALUE(HD_NUM_MAX,num,0))
#define SYNO_DISK_INITIAL(num)          (0x1400+SVALUE(HD_NUM_MAX,num,0))
#define SYNO_DISK_EMPTY(num)            (0x1500+SVALUE(HD_NUM_MAX,num,0))
#define SYNO_DISK_BAD_SECTOR(num)       (0x1600+SVALUE(HD_NUM_MAX,num,0))
#define SYNO_DISK_BAD_SYS(num)          (0x1700+SVALUE(HD_NUM_MAX,num,0))
#define SYNO_DISK_BAD_SYSNSEC(num)      (0x1800+SVALUE(HD_NUM_MAX,num,0))
#define SYNO_CPU_OVERHEAT               0x2101
#define SYNO_CPU_NORMAL                 0x2001
#define SYNO_MEM_ECC                    0x2104
#define SYNO_FAN_FAULT(num)             (0x3000+SVALUE(FAN_NUM_MAX,num,0))
#define SYNO_POWER_FAULT(num)           (0x3100+SVALUE(POWER_NUM_MAX,num,0))
#define SYNO_BATTERY_FAILURE(num)       (0x3200+SVALUE(BATTERY_NUM_MAX,num,0))
#define SYNO_BATTERY_REMOVED(num)       (0x3300+SVALUE(BATTERY_NUM_MAX,num,0))
#define SYNO_FAN_RECOVERY(num)          (0x3400+SVALUE(FAN_NUM_MAX,num,0))
#define SYNO_POWER_RECOVERY(num)        (0x3500+SVALUE(POWER_NUM_MAX,num,0))
#define SYNO_NOP_SUCCESS                0x3600
#define SYNO_NOP_FAILURE                0x3700
#define SYNO_VOL_NORMAL(num)            (0x3800+SVALUE(256,num,0))
#define SYNO_VOL_BUILT(num)             (0x3900+SVALUE(256,num,0))
#define SYNO_VOL_FAILED(num)            (0x3a00+SVALUE(256,num,0))
#define SYNO_VOL_CREATE(num)            (0x3b00+SVALUE(256,num,0))
#define SYNO_VOL_REMOVE(num)            (0x3c00+SVALUE(256,num,0))
#define SYNO_SYS_MSG                    0x4000
#define SYNO_SYS_BOOT                   0x4001
#define SYNO_SYS_RUN                    0x4002
#define SYNO_SYS_SHUTDOWN               0x4003
#define SYNO_SYS_NODISK                 0x4004
#define SYNO_LED_USB_COPY_NONE          0x5100
#define SYNO_LED_USB_COPY_STEADY        0x5101
#define SYNO_LED_USB_COPY_BLINK         0x5102
#define SYNO_LED_HDD_GS                 0x5200
#define SYNO_LED_HDD_AS                 0x5201
#define SYNO_LED_HDD_AB                 0x5202
#define SYNO_LED_HDD_OFF                0x5203
#define SYNO_LED_HDD_GB                 0x5204
#define SYNO_BEEP_OFF                   0x5300
#define SYNO_BEEP_ON                    0x5301
#define SYNO_BEEP_200MS                 0x5302
#define SYNO_POWER_OFF                  0x5400
#define SYNO_RCPOWER_ON                 0x5500
#define SYNO_RCPOWER_OFF                0x5600
#define SYNO_AUTO_POWERON_ON            0x5700
#define SYNO_AUTO_POWERON_OFF           0x5701
#define SYNO_UART2_FANCHEK_ON           0x5800
#define SYNO_UART2_FANCHEK_OFF          0x5801
#define SYNO_LED_USBSTATION_DISK_GREEN  0x5900
#define SYNO_LED_USBSTATION_DISK_ORANGE 0x5901
#define SYNO_LED_USBSTATION_POWER       0x5902

/*add by chchou : moved from synobios.c*/
/*int SYNOBiosSetEvent(u_int event_type);
int	ErrSYNOHardwareConfigure __P((int , int *));
*/
/**/

struct synobios_ops {
	struct module	*owner;
	int		(*get_brand)(void);
	int		(*get_model)(void);
	int		(*get_cpld_version)(void);
	int		(*get_rtc_time)(struct _SynoRtcTimePkt *);
	int		(*set_rtc_time)(struct _SynoRtcTimePkt *);
	int		(*get_fan_status)(int, FAN_STATUS *);
	int		(*set_fan_status)(FAN_STATUS, FAN_SPEED);
	int		(*get_sys_temperature)(int *);
	int		(*get_cpu_temperature)(struct _SynoCpuTemp *);
	int		(*set_disk_led)(int, SYNO_DISK_LED);
	int		(*set_power_led)(SYNO_LED);
	int		(*get_cpld_reg)(CPLDREG *);
	int		(*set_mem_byte)(MEMORY_BYTE *);
	int		(*get_mem_byte)(MEMORY_BYTE *);
	int		(*set_gpio_pin)(GPIO_PIN *);
	int		(*get_gpio_pin)(GPIO_PIN *);
	int		(*set_gpio_blink)(GPIO_PIN *);
	int		(*set_auto_poweron)(SYNO_AUTO_POWERON *);
	int		(*get_auto_poweron)(SYNO_AUTO_POWERON *);
	int		(*init_auto_poweron)(void);
	int		(*uninit_auto_poweron)(void);
	int		(*set_alarm_led)(unsigned char);
	int		(*get_buzzer_cleared)(unsigned char *buzzer_cleared);
	int		(*get_power_status)(POWER_INFO *);
	int		(*get_backplane_status)(BACKPLANE_STATUS *);
	int		(*module_type_init)(struct synobios_ops *);
	int		(*uninitialize)(void);
	int		(*set_cpu_fan_status)(FAN_STATUS, FAN_SPEED);
	int             (*set_phy_led)(SYNO_LED);
	int             (*set_hdd_led)(SYNO_LED);
	int		(*pwm_ctl)(SynoPWMCTL *);
	int		(*check_microp_id)(const struct synobios_ops *);
	int		(*set_microp_id)(void);
};

/* TODO: Because user space also need this define, so we define them here. 
 * But userspace didn't have a common define like SYNO_SATA_PM_DEVICE_GPIO include
 * kernel space. So we can't define it inside some define */
#define EBOX_GPIO_KEY			"gpio"
#define EBOX_INFO_DEV_LIST_KEY	"syno_device_list"
#define EBOX_INFO_VENDOR_KEY	"vendorid"
#define EBOX_INFO_DEVICE_KEY	"deviceid"
#define EBOX_INFO_ERROR_HANDLE	"error_handle"
#define EBOX_INFO_UNIQUE_KEY	"Unique"
#define EBOX_INFO_EMID_KEY		"EMID"
#define EBOX_INFO_SATAHOST_KEY	"sata_host"
#define EBOX_INFO_PORTNO_KEY	"port_no"
#define EBOX_INFO_CPLDVER_KEY	"cpld_version"
#define EBOX_INFO_DEEP_SLEEP	"deepsleep_support"
#define EBOX_INFO_IRQ_OFF		"irq_off"
#define EBOX_INFO_UNIQUE_RX410	"RX410"
#define EBOX_INFO_UNIQUE_DX510	"DX510"
#define EBOX_INFO_UNIQUE_DX513	"DX513"
#define EBOX_INFO_UNIQUE_RX4	"RX4"
#define EBOX_INFO_UNIQUE_DX5	"DX5"
#define EBOX_INFO_UNIQUE_RXC	"RX1211"
#define EBOX_INFO_UNIQUE_DXC	"DX1211"
#define EBOX_INFO_UNIQUE_RXCRP	"RX1211rp"
#define EBOX_INFO_UNIQUE_DX213	"DX213"

#define SYNO_UNIQUE(x)		(x>>2)
#define IS_SYNOLOGY_RX4(x) (SYNO_UNIQUE(x) == 0x15 || SYNO_UNIQUE(x) == 0xd)
#define IS_SYNOLOGY_RX410(x) (SYNO_UNIQUE(x) == 0xd)
#define IS_SYNOLOGY_DX5(x) (SYNO_UNIQUE(x) == 0xa || SYNO_UNIQUE(x) == 0x1a)
#define IS_SYNOLOGY_DX510(x) (SYNO_UNIQUE(x) == 0x1a)
#define IS_SYNOLOGY_DX513(x) (SYNO_UNIQUE(x) == 0x6)
#define IS_SYNOLOGY_DXC(x) (SYNO_UNIQUE(x) == 0x13)
#define IS_SYNOLOGY_RXC(x) (SYNO_UNIQUE(x) == 0xb)
#define IS_SYNOLOGY_DX213(x) (SYNO_UNIQUE(x) == 0x16)

/**************************/
#define IXP425


PRODUCT_MODEL synobios_getmodel(void);

#endif

