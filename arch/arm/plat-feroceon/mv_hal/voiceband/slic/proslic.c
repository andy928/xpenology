
#include "voiceband/slic/proslic.h"
#include "voiceband/tdm/mvTdm.h"

#undef DEBUG
#ifdef DEBUG
#define DBG(fmt, arg...)	mvOsPrintf(KERN_INFO fmt, ##arg) 
#else 
#define DBG(fmt, arg...)
#endif

/* Static APIs */
static unsigned char mvSlicFamily(unsigned int lineId);
static int mvSlicStart(unsigned int lineId);
static void mvSlicStart2(unsigned int lineId);
static void mvSlicGoActive(unsigned int lineId);
static int mvSlicCalibrate(unsigned int lineId);
static unsigned char mvSlicPowerUp(unsigned int lineId);
static void mvSlicInitializeDirectRegisters(unsigned int lineId);
static void mvSlicClearInterrupts(unsigned int lineId);
static void mvSlicEnablePCMhighway(unsigned int lineId);
static void mvSlicDisableOscillators(unsigned int lineId);
static void mvSlicInitializeIndirectRegisters(unsigned int lineId);
#ifdef DEBUG
static int mvSlicVerifyIndirectRegisters(unsigned int lineId);
#endif
static unsigned char mvSlicDirectRegRead(unsigned int lineId, unsigned char address);
static void mvSlicDirectRegWrite(unsigned int lineId, unsigned char address, unsigned char data);
static void mvSlicIndirectRegWrite(unsigned int lineId, unsigned char address, unsigned short data);
static unsigned short mvSlicIndirectRegRead(unsigned int lineId, unsigned char address);
static void mvSlicManualCalibrate(unsigned int lineId);
static void mvSlicClearAlarmBits(unsigned int lineId);
static unsigned char mvSlicChipType (unsigned int lineId);
static unsigned char mvSlicLoopStatus(unsigned int lineId);
static unsigned char mvSlicVersion(unsigned int lineId);
#ifdef SLIC_TEST_LOOPSTATUS_AND_DIGIT_DETECTION
static unsigned char mvSlicDigit(unsigned int lineId);
#endif
static void mvSlicPrintFreqRevision(unsigned int lineId);
static MV_STATUS mvSlicPrintInfo(unsigned int lineId);
static void mvSlicIntEnable(unsigned int lineId);
static void mvSlicIntDisable(unsigned int lineId);
static void mvSlicPcmFormatSet(unsigned int lineId, mv_pcm_format_t pcmFormat);
static int mvSlicDaisyChainGet(unsigned int lineId);
static void mvSlicPcmStartCountRegsSet(unsigned int lineId);
static int mvSlicIntCheck(unsigned int lineId);

char * exceptionStrings[] = 
{	"ProSLIC not communicating", 
	"Time out durring Power Up",
	"Time out durring Power Down",
	"Power is Leaking; might be a short",
	"Tip or Ring Ground Short",
	"Too Many Q1 Power Alarms",
	"Too Many Q2 Power Alarms",
	"Too Many Q3 Power Alarms", 
	"Too Many Q4 Power Alarms",
	"Too Many Q5 Power Alarms",
	"Too Many Q6 Power Alarms"
};

typedef struct {
	unsigned int lineId;
	unsigned short txSample;
	unsigned short rxSample;
}MV_SLIC_LINE_INFO;

static MV_SLIC_LINE_INFO *slicLineInfo = NULL;
static int totalSLICs = 0;
static unsigned char spiMode;
static mv_band_mode_t mvBandMode;
static mv_pcm_format_t mvPcmFormat;
static unsigned short firstSlic = 0;
static unsigned short lastSlic = 0;
static int calibrated = 0;
 
void exception (enum exceptions e)
{
	mvOsPrintf( "\n\tE X C E P T I O N: %s\n",exceptionStrings[e] );
	mvOsPrintf( " SLIC Interface Error\n");
}


int Si3215_Flag = 0;
int TestForSi3215(unsigned int lineId)
{
	return ((mvSlicDirectRegRead(lineId, 1) & 0x80)==0x80);
}

static unsigned char mvSlicDirectRegRead(unsigned int lineId, unsigned char address)
{
	unsigned char data;

	mvVbSpiRead(lineId, address, MV_LINE_FXS, &data);
	return data;
}

static void mvSlicDirectRegWrite(unsigned int lineId, unsigned char address, unsigned char data)
{
	mvVbSpiWrite(lineId, address, MV_LINE_FXS, data);
}

static void waitForIndirectReg(unsigned int lineId)
{
	while(mvSlicDirectRegRead(lineId, I_STATUS));
	if(mvBoardIdGet() == RD_88F6281A_ID)
		mvOsDelay(10);
}

static unsigned short mvSlicIndirectRegRead(unsigned int lineId, unsigned char address)
{ 
	unsigned short res;

	if (Si3215_Flag) 
	{
		/* 
		** re-map the indirect registers for Si3215.
		*/
		if (address < 13 ) return 0;
		if ((address >= 13) && (address <= 40))
			address -= 13;
		else if ((address == 41) || (address == 43))
			address += 23;
		else if ((address >= 99) && (address <= 104))
			address -= 30;
	}

	mvSlicDirectRegWrite(lineId, IAA,address); 
	waitForIndirectReg(lineId);
	res = ( mvSlicDirectRegRead(lineId, IDA_LO) | (mvSlicDirectRegRead (lineId, IDA_HI))<<8);
	waitForIndirectReg(lineId);
	return res;
}

static void mvSlicIndirectRegWrite(unsigned int lineId, unsigned char address, unsigned short data)
{

	if (Si3215_Flag)
	{
		/* 
		** re-map the indirect registers for Si3215.
		*/
		if (address < 13 ) return;
		if ((address >= 13) && (address <= 40))
			address -= 13;
		else if ((address == 41) || (address == 43))
			address += 23;
		else if ((address >= 99) && (address <= 104))
			address -= 30;
	}

	mvSlicDirectRegWrite(lineId, IDA_LO,(unsigned char)(data & 0xFF));
	mvSlicDirectRegWrite(lineId, IDA_HI,(unsigned char)((data & 0xFF00)>>8));
	mvSlicDirectRegWrite(lineId, IAA,address);
	waitForIndirectReg(lineId);
}


/*
The following Array contains:
*/
indirectRegister  indirectRegisters[] =
{
/* Reg#			Label		Initial Value  */

{	0,	"DTMF_ROW_0_PEAK",	0x55C2	},
{	1,	"DTMF_ROW_1_PEAK",	0x51E6	},
{	2,	"DTMF_ROW2_PEAK",	0x4B85	},
{	3,	"DTMF_ROW3_PEAK",	0x4937	},
{	4,	"DTMF_COL1_PEAK",	0x3333	},
{	5,	"DTMF_FWD_TWIST",	0x0202	},
{	6,	"DTMF_RVS_TWIST",	0x0202	},
{	7,	"DTMF_ROW_RATIO",	0x0198	},
{	8,	"DTMF_COL_RATIO",	0x0198	},
{	9,	"DTMF_ROW_2ND_ARM",	0x0611	},
{	10,	"DTMF_COL_2ND_ARM",	0x0202	},
{	11,	"DTMF_PWR_MIN_",	0x00E5	},
{	12,	"DTMF_OT_LIM_TRES",	0x0A1C	},
{	13,	"OSC1_COEF",		0x7b30	},
{	14,	"OSC1X",			0x0063	},
{	15,	"OSC1Y",			0x0000	},
{	16,	"OSC2_COEF",		0x7870	},
{	17,	"OSC2X",			0x007d	},
{	18,	"OSC2Y",			0x0000	},
{	19,	"RING_V_OFF",		0x0000	},
{	20,	"RING_OSC",			0x7EF0	},
{	21,	"RING_X",			0x0160	},
{	22,	"RING_Y",			0x0000	},
{	23,	"PULSE_ENVEL",		0x2000	},
{	24,	"PULSE_X",			0x2000	},
{	25,	"PULSE_Y",			0x0000	},
{	26,	"RECV_DIGITAL_GAIN",0x4000	},
{	27,	"XMIT_DIGITAL_GAIN",0x4000	},
{	28,	"LOOP_CLOSE_TRES",	0x1000	},
{	29,	"RING_TRIP_TRES",	0x3600	},
{	30,	"COMMON_MIN_TRES",	0x1000	},
{	31,	"COMMON_MAX_TRES",	0x0200	},
{	32,	"PWR_ALARM_Q1Q2",	0x7c0   },
{	33,	"PWR_ALARM_Q3Q4",	0x2600	},
{	34,	"PWR_ALARM_Q5Q6",	0x1B80	},
{	35,	"LOOP_CLSRE_FlTER",	0x8000	},
{	36,	"RING_TRIP_FILTER",	0x0320	},
{	37,	"TERM_LP_POLE_Q1Q2",0x08c	},
{	38,	"TERM_LP_POLE_Q3Q4",0x0100	},
{	39,	"TERM_LP_POLE_Q5Q6",0x0010	},
{	40,	"CM_BIAS_RINGING",	0x0C00	},
{	41,	"DCDC_MIN_V",		0x0C00	},
{	43,	"LOOP_CLOSE_TRES Low",	0x1000	},
{	99,	"FSK 0 FREQ PARAM",	0x00DA	},
{	100,"FSK 0 AMPL PARAM",	0x6B60	},
{	101,"FSK 1 FREQ PARAM",	0x0074	},
{	102,"FSK 1 AMPl PARAM",	0x79C0	},
{	103,"FSK 0to1 SCALER",	0x1120	},
{	104,"FSK 1to0 SCALER",	0x3BE0	},
{	97,	"RCV_FLTR",			0},
{	0,"",0},
};


void mvSlicStopTone(unsigned int lineId) 
{
  mvSlicDirectRegWrite(lineId, 32, INIT_DR32	);//0x00	Oper. Oscillator 1 Controltone generation
  mvSlicDirectRegWrite(lineId, 33, INIT_DR33	);//0x00	Oper. Oscillator 2 Controltone generation

  mvSlicIndirectRegWrite(lineId, 13,INIT_IR13);
  mvSlicIndirectRegWrite(lineId, 14,INIT_IR14);
  mvSlicIndirectRegWrite(lineId, 16,INIT_IR16);
  mvSlicIndirectRegWrite(lineId, 17,INIT_IR17);
  mvSlicDirectRegWrite(lineId, 36,  INIT_DR36);
  mvSlicDirectRegWrite(lineId, 37,  INIT_DR37);
  mvSlicDirectRegWrite(lineId, 38,  INIT_DR38);
  mvSlicDirectRegWrite(lineId, 39,  INIT_DR39);
  mvSlicDirectRegWrite(lineId, 40,  INIT_DR40);
  mvSlicDirectRegWrite(lineId, 41,  INIT_DR41);
  mvSlicDirectRegWrite(lineId, 42,  INIT_DR42);
  mvSlicDirectRegWrite(lineId, 43,  INIT_DR43);

//  mvSlicDirectRegWrite(lineId, 32, INIT_DR32	);//0x00	Oper. Oscillator 1 Controltone generation
//  mvSlicDirectRegWrite(lineId, 33, INIT_DR33	);//0x00	Oper. Oscillator 2 Controltone generation
}

void genTone(tone_struct tone, unsigned int lineId) 
{ 
	// Uber function to extract values for oscillators from tone_struct
	// place them in the registers, and enable the oscillators.
	unsigned char osc1_ontimer_enable=0, osc1_offtimer_enable=0,osc2_ontimer_enable=0,osc2_offtimer_enable=0;
	int enable_osc2=0;

	//loopBackOff();
	mvSlicDisableOscillators(lineId); // Make sure the oscillators are not already on.

	if (tone.osc1.coeff == 0 || tone.osc1.x == 0) {
		// Error!
		mvOsPrintf("You passed me a invalid struct!\n");
		return;
	}
	// Setup osc 1
	mvSlicIndirectRegWrite(lineId,  OSC1_COEF, tone.osc1.coeff);
	mvSlicIndirectRegWrite(lineId,  OSC1X, tone.osc1.x);
	mvSlicIndirectRegWrite(lineId,  OSC1Y, tone.osc1.y);
	//mvOsPrintf("OUt-> 0x%04x\n",tone.osc1.coeff);
	// Active Timer


	if (tone.osc1.on_hi_byte != 0) {
		mvSlicDirectRegWrite(lineId,  OSC1_ON__LO, tone.osc1.on_low_byte);
		mvSlicDirectRegWrite(lineId,  OSC1_ON_HI, tone.osc1.on_hi_byte);
		osc1_ontimer_enable = 0x10;
	}
	// Inactive Timer
	if (tone.osc1.off_hi_byte != 0) {
		mvSlicDirectRegWrite(lineId,  OSC1_OFF_LO, tone.osc1.off_low_byte);
		mvSlicDirectRegWrite(lineId,  OSC1_OFF_HI, tone.osc1.off_hi_byte);
		osc1_offtimer_enable = 0x08;
	}
	
	if (tone.osc2.coeff != 0) {
		// Setup OSC 2
		mvSlicIndirectRegWrite(lineId,  OSC2_COEF, tone.osc2.coeff);
		mvSlicIndirectRegWrite(lineId,  OSC2X, tone.osc2.x);
		mvSlicIndirectRegWrite(lineId,  OSC2Y, tone.osc2.y);
		
		// Active Timer
		if (tone.osc1.on_hi_byte != 0) {
			mvSlicDirectRegWrite(lineId,  OSC2_ON__LO, tone.osc2.on_low_byte);
			mvSlicDirectRegWrite(lineId,  OSC2_ON_HI, tone.osc2.on_hi_byte);
			osc2_ontimer_enable = 0x10;
		}
		// Inactive Timer
		if (tone.osc1.off_hi_byte != 0) {
			mvSlicDirectRegWrite(lineId,  OSC2_OFF_LO, tone.osc2.off_low_byte);
			mvSlicDirectRegWrite(lineId,  OSC2_OFF_HI, tone.osc2.off_hi_byte);
			osc2_offtimer_enable = 0x08;
		}
		enable_osc2 = 1;
	}

	mvSlicDirectRegWrite(lineId,  OSC1, (unsigned char)(0x06 | osc1_ontimer_enable | osc1_offtimer_enable));
	if (enable_osc2)
		mvSlicDirectRegWrite(lineId,  OSC2, (unsigned char)(0x06 | osc2_ontimer_enable | osc2_offtimer_enable));
	return;
}


void mvSlicDialTone(unsigned int lineId)
{
  if (Si3215_Flag)
  {
	  mvSlicIndirectRegWrite(lineId, 13,SI3215_DIALTONE_IR13);
	  mvSlicIndirectRegWrite(lineId, 14,SI3215_DIALTONE_IR14);
	  mvSlicIndirectRegWrite(lineId, 16,SI3215_DIALTONE_IR16);
	  mvSlicIndirectRegWrite(lineId, 17,SI3215_DIALTONE_IR17);
  }
  else
  {
	  mvSlicIndirectRegWrite(lineId, 13,SI3210_DIALTONE_IR13);
	  mvSlicIndirectRegWrite(lineId, 14,SI3210_DIALTONE_IR14);
	  mvSlicIndirectRegWrite(lineId, 16,SI3210_DIALTONE_IR16);
	  mvSlicIndirectRegWrite(lineId, 17,SI3210_DIALTONE_IR17);
  }

  mvSlicDirectRegWrite(lineId, 36,  DIALTONE_DR36);
  mvSlicDirectRegWrite(lineId, 37,  DIALTONE_DR37);
  mvSlicDirectRegWrite(lineId, 38,  DIALTONE_DR38);
  mvSlicDirectRegWrite(lineId, 39,  DIALTONE_DR39);
  mvSlicDirectRegWrite(lineId, 40,  DIALTONE_DR40);
  mvSlicDirectRegWrite(lineId, 41,  DIALTONE_DR41);
  mvSlicDirectRegWrite(lineId, 42,  DIALTONE_DR42);
  mvSlicDirectRegWrite(lineId, 43,  DIALTONE_DR43);

  mvSlicDirectRegWrite(lineId, 32,  DIALTONE_DR32);
  mvSlicDirectRegWrite(lineId, 33,  DIALTONE_DR33);
}


void mvSlicBusyTone(unsigned int lineId)
{
  if (Si3215_Flag)
  {
	  mvSlicIndirectRegWrite(lineId, 13,SI3215_BUSYTONE_IR13);
	  mvSlicIndirectRegWrite(lineId, 14,SI3215_BUSYTONE_IR14);
	  mvSlicIndirectRegWrite(lineId, 16,SI3215_BUSYTONE_IR16);
	  mvSlicIndirectRegWrite(lineId, 17,SI3215_BUSYTONE_IR17);
  }
  else
  {
	  mvSlicIndirectRegWrite(lineId, 13,SI3210_BUSYTONE_IR13);
	  mvSlicIndirectRegWrite(lineId, 14,SI3210_BUSYTONE_IR14);
	  mvSlicIndirectRegWrite(lineId, 16,SI3210_BUSYTONE_IR16);
	  mvSlicIndirectRegWrite(lineId, 17,SI3210_BUSYTONE_IR17);
  }
  mvSlicDirectRegWrite(lineId, 36,  BUSYTONE_DR36);
  mvSlicDirectRegWrite(lineId, 37,  BUSYTONE_DR37);
  mvSlicDirectRegWrite(lineId, 38,  BUSYTONE_DR38);
  mvSlicDirectRegWrite(lineId, 39,  BUSYTONE_DR39);
  mvSlicDirectRegWrite(lineId, 40,  BUSYTONE_DR40);
  mvSlicDirectRegWrite(lineId, 41,  BUSYTONE_DR41);
  mvSlicDirectRegWrite(lineId, 42,  BUSYTONE_DR42);
  mvSlicDirectRegWrite(lineId, 43,  BUSYTONE_DR43);
  
  mvSlicDirectRegWrite(lineId, 32,  BUSYTONE_DR32);
  mvSlicDirectRegWrite(lineId, 33,  BUSYTONE_DR33);
}

void mvSlicReorderTone(unsigned int lineId)
{
  if (Si3215_Flag)
  {
	  mvSlicIndirectRegWrite(lineId, 13,SI3215_REORDERTONE_IR13);
	  mvSlicIndirectRegWrite(lineId, 14,SI3215_REORDERTONE_IR14);
	  mvSlicIndirectRegWrite(lineId, 16,SI3215_REORDERTONE_IR16);
	  mvSlicIndirectRegWrite(lineId, 17,SI3215_REORDERTONE_IR17);
  }
  else {
	  mvSlicIndirectRegWrite(lineId, 13,SI3210_REORDERTONE_IR13);
	  mvSlicIndirectRegWrite(lineId, 14,SI3210_REORDERTONE_IR14);
	  mvSlicIndirectRegWrite(lineId, 16,SI3210_REORDERTONE_IR16);
	  mvSlicIndirectRegWrite(lineId, 17,SI3210_REORDERTONE_IR17);
  }

  mvSlicDirectRegWrite(lineId, 36,  REORDERTONE_DR36);
  mvSlicDirectRegWrite(lineId, 37,  REORDERTONE_DR37);
  mvSlicDirectRegWrite(lineId, 38,  REORDERTONE_DR38);
  mvSlicDirectRegWrite(lineId, 39,  REORDERTONE_DR39);
  mvSlicDirectRegWrite(lineId, 40,  REORDERTONE_DR40);
  mvSlicDirectRegWrite(lineId, 41,  REORDERTONE_DR41);
  mvSlicDirectRegWrite(lineId, 42,  REORDERTONE_DR42);
  mvSlicDirectRegWrite(lineId, 43,  REORDERTONE_DR43);
  
  mvSlicDirectRegWrite(lineId, 32,  REORDERTONE_DR32);
  mvSlicDirectRegWrite(lineId, 33,  REORDERTONE_DR33);
 
}

void mvSlicCongestionTone(unsigned int lineId)
{
  if (Si3215_Flag)
  {
	  mvSlicIndirectRegWrite(lineId, 13,SI3215_CONGESTIONTONE_IR13);
	  mvSlicIndirectRegWrite(lineId, 14,SI3215_CONGESTIONTONE_IR14);
	  mvSlicIndirectRegWrite(lineId, 16,SI3215_CONGESTIONTONE_IR16);
	  mvSlicIndirectRegWrite(lineId, 17,SI3215_CONGESTIONTONE_IR17);
  }
  else {
	  mvSlicIndirectRegWrite(lineId, 13,SI3210_CONGESTIONTONE_IR13);
	  mvSlicIndirectRegWrite(lineId, 14,SI3210_CONGESTIONTONE_IR14);
	  mvSlicIndirectRegWrite(lineId, 16,SI3210_CONGESTIONTONE_IR16);
	  mvSlicIndirectRegWrite(lineId, 17,SI3210_CONGESTIONTONE_IR17);
  }

  mvSlicDirectRegWrite(lineId, 36,  CONGESTIONTONE_DR36);
  mvSlicDirectRegWrite(lineId, 37,  CONGESTIONTONE_DR37);
  mvSlicDirectRegWrite(lineId, 38,  CONGESTIONTONE_DR38);
  mvSlicDirectRegWrite(lineId, 39,  CONGESTIONTONE_DR39);
  mvSlicDirectRegWrite(lineId, 40,  CONGESTIONTONE_DR40);
  mvSlicDirectRegWrite(lineId, 41,  CONGESTIONTONE_DR41);
  mvSlicDirectRegWrite(lineId, 42,  CONGESTIONTONE_DR42);
  mvSlicDirectRegWrite(lineId, 43,  CONGESTIONTONE_DR43);

  mvSlicDirectRegWrite(lineId, 32,  CONGESTIONTONE_DR32);
  mvSlicDirectRegWrite(lineId, 33,  CONGESTIONTONE_DR33);

}

void mvSlicRingbackPbxTone(unsigned int lineId)
{
  if (Si3215_Flag)
  {
	  mvSlicIndirectRegWrite(lineId, 13,SI3215_RINGBACKPBXTONE_IR13);
	  mvSlicIndirectRegWrite(lineId, 14,SI3215_RINGBACKPBXTONE_IR14);
	  mvSlicIndirectRegWrite(lineId, 16,SI3215_RINGBACKPBXTONE_IR16);
	  mvSlicIndirectRegWrite(lineId, 17,SI3215_RINGBACKPBXTONE_IR17);
  }
  else {
	  mvSlicIndirectRegWrite(lineId, 13,SI3210_RINGBACKPBXTONE_IR13);
	  mvSlicIndirectRegWrite(lineId, 14,SI3210_RINGBACKPBXTONE_IR14);
	  mvSlicIndirectRegWrite(lineId, 16,SI3210_RINGBACKPBXTONE_IR16);
	  mvSlicIndirectRegWrite(lineId, 17,SI3210_RINGBACKPBXTONE_IR17);
  }

  mvSlicDirectRegWrite(lineId, 36,  RINGBACKPBXTONE_DR36);
  mvSlicDirectRegWrite(lineId, 37,  RINGBACKPBXTONE_DR37);
  mvSlicDirectRegWrite(lineId, 38,  RINGBACKPBXTONE_DR38);
  mvSlicDirectRegWrite(lineId, 39,  RINGBACKPBXTONE_DR39);
  mvSlicDirectRegWrite(lineId, 40,  RINGBACKPBXTONE_DR40);
  mvSlicDirectRegWrite(lineId, 41,  RINGBACKPBXTONE_DR41);
  mvSlicDirectRegWrite(lineId, 42,  RINGBACKPBXTONE_DR42);
  mvSlicDirectRegWrite(lineId, 43,  RINGBACKPBXTONE_DR43);

  mvSlicDirectRegWrite(lineId, 32,  RINGBACKPBXTONE_DR32);
  mvSlicDirectRegWrite(lineId, 33,  RINGBACKPBXTONE_DR33);

}


void mvSlicRingBackTone(unsigned int lineId)
{
  if (Si3215_Flag)
  {
	  mvSlicIndirectRegWrite(lineId, 13,SI3215_RINGBACKTONE_IR13);
	  mvSlicIndirectRegWrite(lineId, 14,SI3215_RINGBACKTONE_IR14);
	  mvSlicIndirectRegWrite(lineId, 16,SI3215_RINGBACKTONE_IR16);
	  mvSlicIndirectRegWrite(lineId, 17,SI3215_RINGBACKTONE_IR17);
  }
  else {
	  mvSlicIndirectRegWrite(lineId, 13,SI3210_RINGBACKTONE_IR13);
	  mvSlicIndirectRegWrite(lineId, 14,SI3210_RINGBACKTONE_IR14);
	  mvSlicIndirectRegWrite(lineId, 16,SI3210_RINGBACKTONE_IR16);
	  mvSlicIndirectRegWrite(lineId, 17,SI3210_RINGBACKTONE_IR17);
  }

  mvSlicDirectRegWrite(lineId, 36,  RINGBACKTONE_DR36);
  mvSlicDirectRegWrite(lineId, 37,  RINGBACKTONE_DR37);
  mvSlicDirectRegWrite(lineId, 38,  RINGBACKTONE_DR38);
  mvSlicDirectRegWrite(lineId, 39,  RINGBACKTONE_DR39);
  mvSlicDirectRegWrite(lineId, 40,  RINGBACKTONE_DR40);
  mvSlicDirectRegWrite(lineId, 41,  RINGBACKTONE_DR41);
  mvSlicDirectRegWrite(lineId, 42,  RINGBACKTONE_DR42);
  mvSlicDirectRegWrite(lineId, 43,  RINGBACKTONE_DR43);
  
  mvSlicDirectRegWrite(lineId, 32,  RINGBACKTONE_DR32);
  mvSlicDirectRegWrite(lineId, 33,  RINGBACKTONE_DR33);
 
}

void ringBackToneSi3216(unsigned int lineId)
{
  mvSlicIndirectRegWrite(lineId, 13,RINGBACKTONE_SI3216_IR13);
  mvSlicIndirectRegWrite(lineId, 14,RINGBACKTONE_SI3216_IR14);
  mvSlicIndirectRegWrite(lineId, 16,RINGBACKTONE_SI3216_IR16);
  mvSlicIndirectRegWrite(lineId, 17,RINGBACKTONE_SI3216_IR17);
  mvSlicDirectRegWrite(lineId, 36,  RINGBACKTONE_SI3216_DR36);
  mvSlicDirectRegWrite(lineId, 37,  RINGBACKTONE_SI3216_DR37);
  mvSlicDirectRegWrite(lineId, 38,  RINGBACKTONE_SI3216_DR38);
  mvSlicDirectRegWrite(lineId, 39,  RINGBACKTONE_SI3216_DR39);
  mvSlicDirectRegWrite(lineId, 40,  RINGBACKTONE_SI3216_DR40);
  mvSlicDirectRegWrite(lineId, 41,  RINGBACKTONE_SI3216_DR41);
  mvSlicDirectRegWrite(lineId, 42,  RINGBACKTONE_SI3216_DR42);
  mvSlicDirectRegWrite(lineId, 43,  RINGBACKTONE_SI3216_DR43);
  
  mvSlicDirectRegWrite(lineId, 32,  RINGBACKTONE_SI3216_DR32);
  mvSlicDirectRegWrite(lineId, 33,  RINGBACKTONE_SI3216_DR33);
 
}


void mvSlicRingBackJapan(unsigned int lineId)
{
  if (Si3215_Flag)
  {
	  mvSlicIndirectRegWrite(lineId, 13,SI3215_RINGBACKJAPANTONE_IR13);
	  mvSlicIndirectRegWrite(lineId, 14,SI3215_RINGBACKJAPANTONE_IR14);
	  mvSlicIndirectRegWrite(lineId, 16,SI3215_RINGBACKJAPANTONE_IR16);
	  mvSlicIndirectRegWrite(lineId, 17,SI3215_RINGBACKJAPANTONE_IR17);
  }
  else {
	  mvSlicIndirectRegWrite(lineId, 13,SI3210_RINGBACKJAPANTONE_IR13);
	  mvSlicIndirectRegWrite(lineId, 14,SI3210_RINGBACKJAPANTONE_IR14);
	  mvSlicIndirectRegWrite(lineId, 16,SI3210_RINGBACKJAPANTONE_IR16);
	  mvSlicIndirectRegWrite(lineId, 17,SI3210_RINGBACKJAPANTONE_IR17);
  }

  mvSlicDirectRegWrite(lineId, 36,  RINGBACKJAPANTONE_DR36);
  mvSlicDirectRegWrite(lineId, 37,  RINGBACKJAPANTONE_DR37);
  mvSlicDirectRegWrite(lineId, 38,  RINGBACKJAPANTONE_DR38);
  mvSlicDirectRegWrite(lineId, 39,  RINGBACKJAPANTONE_DR39);
  mvSlicDirectRegWrite(lineId, 40,  RINGBACKJAPANTONE_DR40);
  mvSlicDirectRegWrite(lineId, 41,  RINGBACKJAPANTONE_DR41);
  mvSlicDirectRegWrite(lineId, 42,  RINGBACKJAPANTONE_DR42);
  mvSlicDirectRegWrite(lineId, 43,  RINGBACKJAPANTONE_DR43);
  
  mvSlicDirectRegWrite(lineId, 32,  RINGBACKJAPANTONE_DR32);
  mvSlicDirectRegWrite(lineId, 33,  RINGBACKJAPANTONE_DR33);
 
}


void mvSlicBusyJapanTone(unsigned int lineId)
{
  if (Si3215_Flag)
  {
	  mvSlicIndirectRegWrite(lineId, 13,SI3215_BUSYJAPANTONE_IR13);
	  mvSlicIndirectRegWrite(lineId, 14,SI3215_BUSYJAPANTONE_IR14);
	  mvSlicIndirectRegWrite(lineId, 16,SI3215_BUSYJAPANTONE_IR16);
	  mvSlicIndirectRegWrite(lineId, 17,SI3215_BUSYJAPANTONE_IR17);
  }
  else {
	  mvSlicIndirectRegWrite(lineId, 13,SI3210_BUSYJAPANTONE_IR13);
	  mvSlicIndirectRegWrite(lineId, 14,SI3210_BUSYJAPANTONE_IR14);
	  mvSlicIndirectRegWrite(lineId, 16,SI3210_BUSYJAPANTONE_IR16);
	  mvSlicIndirectRegWrite(lineId, 17,SI3210_BUSYJAPANTONE_IR17);
  }

  mvSlicDirectRegWrite(lineId, 36,  BUSYJAPANTONE_DR36);
  mvSlicDirectRegWrite(lineId, 37,  BUSYJAPANTONE_DR37);
  mvSlicDirectRegWrite(lineId, 38,  BUSYJAPANTONE_DR38);
  mvSlicDirectRegWrite(lineId, 39,  BUSYJAPANTONE_DR39);
  mvSlicDirectRegWrite(lineId, 40,  BUSYJAPANTONE_DR40);
  mvSlicDirectRegWrite(lineId, 41,  BUSYJAPANTONE_DR41);
  mvSlicDirectRegWrite(lineId, 42,  BUSYJAPANTONE_DR42);
  mvSlicDirectRegWrite(lineId, 43,  BUSYJAPANTONE_DR43);
  
  mvSlicDirectRegWrite(lineId, 32,  BUSYJAPANTONE_DR32);
  mvSlicDirectRegWrite(lineId, 33,  BUSYJAPANTONE_DR33);
 
}


static unsigned char mvSlicPowerUp(unsigned int lineId)
{ 
	unsigned char vBat ; 
	int i=0;


	if (mvSlicChipType(lineId) == 3)  // M version correction
	{
		mvSlicDirectRegWrite(lineId, 92,INIT_SI3210M_DR92);// M version
		mvSlicDirectRegWrite(lineId, 93,INIT_SI3210M_DR93);// M version
	}
	else	
	{
		/* MA */
		mvSlicDirectRegWrite(lineId, 93, 12); 
		mvSlicDirectRegWrite(lineId, 92, 0xff); /* set the period of the DC-DC converter to 1/64 kHz  START OUT SLOW*/

	}

	mvSlicDirectRegWrite(lineId, 14, 0); /* Engage the DC-DC converter */
  
	while ((vBat=mvSlicDirectRegRead(lineId, 82)) < 0xc0)
	{ 
		mvOsUDelay (100);
		++i;
		if (i > 5000) return 0;
	}
  	
	if (mvSlicChipType(lineId) == 3)  // M version correction
	{
		mvSlicDirectRegWrite(lineId, 92,0x80 +INIT_SI3210M_DR92);// M version
	}
	else
		mvSlicDirectRegWrite(lineId, 93, 0x8c);  /* DC-DC Calibration  */ /* MA */

	while(0x80 & mvSlicDirectRegRead(lineId, 93));  // Wait for DC-DC Calibration to complete

	return vBat;
}
 
void xcalibrate(unsigned int lineId)
{ unsigned char i ;

	mvSlicDirectRegWrite(lineId, 97, 0x1e); /* Do all of the Calibarations */
	mvSlicDirectRegWrite(lineId, 96, 0x47); /* Start the calibration No Speed Up */
	
	mvSlicManualCalibrate(lineId);

	mvSlicDirectRegWrite(lineId, 23,4);  // enable interrupt for the balance Cal
	mvSlicDirectRegWrite(lineId, 97,0x1);
	mvSlicDirectRegWrite(lineId, 96,0x40);


	while (mvSlicDirectRegRead(lineId, 96) != 0 );
	mvOsPrintf("\nCalibration Vector Registers 98 - 107: ");
	
	for (i=98; i < 107; i++)  mvOsPrintf( "%X.", mvSlicDirectRegRead(lineId, i));
	mvOsPrintf("%X \n\n",mvSlicDirectRegRead(lineId, 107));

}

static void mvSlicGoActive(unsigned int lineId)

{
	mvSlicDirectRegWrite(lineId, 64,1);	/* LOOP STATE REGISTER SET TO ACTIVE */
				/* Active works for on-hook and off-hook see spec. */
				/* The phone hook-switch sets the off-hook and on-hook substate*/
}


static unsigned char mvSlicVersion(unsigned int lineId)
{
	return 0xf & mvSlicDirectRegRead(lineId, 0); 
}

static unsigned char mvSlicChipType (unsigned int lineId)
{
	return (0x30 & mvSlicDirectRegRead(lineId, 0)) >> 4; 
}

static unsigned char mvSlicFamily (unsigned int lineId)
{
        return (mvSlicDirectRegRead (lineId, 1) & 0x80);
}

static int selfTest(unsigned int lineId)
{
	/*  Begin Sanity check  Optional */
	if (mvSlicDirectRegRead(lineId, 8) !=2) {exception(PROSLICiNSANE); return 0;}
	if (mvSlicDirectRegRead(lineId, 64) !=0) {exception(PROSLICiNSANE); return 0;}
	if (mvSlicDirectRegRead(lineId, 11) !=0x33) {exception(PROSLICiNSANE); return 0;}
	/* End Sanity check */
	return 1;
}

int mvSlicStart(unsigned int lineId)
{
	volatile unsigned char t,v;
#ifdef DEBUG
	volatile unsigned short i;
#endif
	
	// Test if Si3210 or Si3215 is used.
	Si3215_Flag = TestForSi3215(lineId);

	if (Si3215_Flag)
		DBG("Proslic Si3215 Detected.\n");
	else
		DBG("Proslic Si3210 detected.\n");

	/*  Another Quarter of a Second */
	if (!selfTest(lineId))
	{
		mvOsPrintf("self test failed !!!\n");
		return 0;
	}

	v = mvSlicVersion(lineId);
	DBG("Si321x Version = %x\n", v);

	t = mvSlicChipType(lineId);
	DBG("chip Type t=%d\n", t);

	mvSlicInitializeIndirectRegisters(lineId);
#ifdef DEBUG
	i=mvSlicVerifyIndirectRegisters (lineId);
	if (i != 0) {
		mvOsPrintf ("verifyIndirect failed\n");
		return 0;
	}
#endif
	mvSlicDirectRegWrite (lineId, 8, 0);
	if (v == 5)
		mvSlicDirectRegWrite (lineId, 108, 0xeb); /* turn on Rev E features. */

	
	if (   t == 0 ) // Si3210 not the Si3211 or Si3212	
	{
		mvSlicDirectRegWrite(lineId, 67,0x17); // Make VBat switch not automatic 
		// The above is a saftey measure to prevent Q7 from accidentaly turning on and burning out.
		//  It works in combination with the statement below.  Pin 34 DCDRV which is used for the battery switch on the
		//  Si3211 & Si3212 
	
		mvSlicDirectRegWrite(lineId, 66,1);  //    Q7 should be set to OFF for si3210
	}

	if (v <=2 ) {  //  REVISION B   
		mvSlicDirectRegWrite(lineId, 73,4);  // set common mode voltage to 6 volts
	}

	// PCm Start Count. 
	mvSlicDirectRegWrite (lineId, 2, 0);
	mvSlicDirectRegWrite (lineId, 4, 0);


	/* Do Flush durring powerUp and calibrate */
	if (t == 0 || t==3) //  Si3210
	{
		DBG("Powerup the Slic\n");

		// Turn on the DC-DC converter and verify voltage.
		if (!mvSlicPowerUp(lineId))
			return 0;
	}

	mvSlicInitializeDirectRegisters(lineId);

	if (!mvSlicCalibrate(lineId)) {
		mvOsPrintf("calibrate failed\n");
		return 0;
	}

	
	return 1;
}

static void mvSlicStart2(unsigned int lineId)
{
	if(!calibrated)
	{	
		/* Perform manual calibration anyway */
		mvSlicManualCalibrate (lineId);
		calibrated = 1;
		DBG("Caliberation done\n");
	}

	DBG("Register 98 = %x\n", mvSlicDirectRegRead(lineId, 98));
	DBG("Register 99 = %x\n", mvSlicDirectRegRead(lineId, 99));

	DBG("Slic is Active\n");
	mvSlicGoActive(lineId);
}

void mvSlicStopRinging(unsigned int lineId)
{
	
	if ((0xf & mvSlicDirectRegRead(lineId, 0))<=2 )  // if REVISION B  
        mvSlicDirectRegWrite(lineId, 69,10);   // Loop Debounce Register  = initial value
    
	mvSlicGoActive(lineId);
	
}

#if 0
static void mvSlicManualCalibrate(unsigned int lineId)
{ 
unsigned char x,y,i,progress=0; // progress contains individual bits for the Tip and Ring Calibrations

	//Initialized DR 98 and 99 to get consistant results.
	// 98 and 99 are the results registers and the search should have same intial conditions.
	mvSlicDirectRegWrite(lineId, 98,0x10); // This is necessary if the calibration occurs other than at reset time
	mvSlicDirectRegWrite(lineId, 99,0x10);

	for ( i=0x16; i>0; i--)
	{
		mvSlicDirectRegWrite(lineId, 98,i);
		mvOsDelay(50); /* need to wait 40 ms according to An35 */
		if((mvSlicDirectRegRead(lineId, 88)) == 0)
		{	progress|=1;
		x=i;
		break;
		}
	} // for

	for ( i=0x16; i>0; i--)
	{
		mvSlicDirectRegWrite(lineId, 99,i);
		mvOsDelay(50); /* need to wait 40 ms according to An35 */
		if((mvSlicDirectRegRead(lineId, 89)) == 0){
			progress|=2;
			y=i;
		break;
		}
	
	}//for

	//return progress;
}
#endif
static void mvSlicManualCalibrate(unsigned int lineId)
{
	unsigned char i,sum = 0;
	unsigned short line;
	unsigned long progress = 0;

	for(line = firstSlic; line < (lastSlic+1); line++)
	{
		//Initialized DR 98 and 99 to get consistant results.
		// 98 and 99 are the results registers and the search should have same intial conditions.
		mvSlicDirectRegWrite(line, 98,0x10); // necessary if the calibration occurs other than at reset time
		mvSlicDirectRegWrite(line, 99,0x10);
	}


	for ( i=0x16; i>0; i--)
	{
		if(sum == totalSLICs)
			break;

		for(line = firstSlic; line < (lastSlic+1); line++)
		{
			if(!(progress & (1 << line)))
				mvSlicDirectRegWrite(line, 98,i);
		}

			mvOsDelay(40); /* need to wait 40 ms according to An35 */

		for(line = firstSlic; line < (lastSlic+1); line++)
		{
			if(!(progress & (1 << line)))
			{
				if((mvSlicDirectRegRead(lineId, 88)) == 0)
				{	
					progress |= (1 << line);
					sum++;
			}	}
		}
	} // for


	sum = 0;


	for ( i=0x16; i>0; i--)
	{
		if(sum == totalSLICs)
			break;

		for(line = firstSlic; line < (lastSlic+1); line++)
		{
			if(!(progress & (1 << line)))
				mvSlicDirectRegWrite(line, 99,i);
		}

			mvOsDelay(40); /* need to wait 40 ms according to An35 */

		for(line = firstSlic; line < (lastSlic+1); line++)
		{
			if(!(progress & (1 << line)))
			{
				if((mvSlicDirectRegRead(lineId, 89)) == 0)
				{	
					progress |= (1 << line);
					sum++;
			}	}
		}
	} // for

	return;
}

static void mvSlicClearAlarmBits(unsigned int lineId)
{
	mvSlicDirectRegWrite(lineId, 19,0xFC); //Clear Alarm bits
}

void mvSlicActivateRinging(unsigned int lineId)
{
	mvSlicDirectRegWrite(lineId,  LINE_STATE, RING_LINE); // REG 64,4
}


static void mvSlicDisableOscillators(unsigned int lineId) 
{ 
	// Turns of OSC1 and OSC2
	unsigned char i;

	//mvOsPrintf("Disabling Oscillators!!!\n");
	for ( i=32; i<=45; i++) 
		if (i !=34)  // Don't write to the ringing oscillator control
		mvSlicDirectRegWrite(lineId, i,0);
}


static unsigned char mvSlicLoopStatus(unsigned int lineId)
{
static int ReadBack;

	// check for power alarms
	if ((mvSlicDirectRegRead(lineId, 19) & 0xfc) != 0) {
		mvOsPrintf ("Power alarm = %x\n",(mvSlicDirectRegRead(lineId, 19) & 0xfc));
		mvSlicClearAlarmBits (lineId);
	} 
	ReadBack = mvSlicDirectRegRead (lineId, 68);
	return (ReadBack & 0x3);

}

#ifdef SLIC_TEST_LOOPSTATUS_AND_DIGIT_DETECTION
static unsigned char mvSlicDigit(unsigned int lineId)
{
	return mvSlicDirectRegRead(lineId, 24) & 0x1f;
}
#endif

static void mvSlicPrintFreqRevision(unsigned int lineId)
{

	int freq;
#ifdef DEBUG
	char* freqs[ ] = {"8192","4096","2048","1024","512","256","1536","768","32768"};
#endif
	// Turn on all interrupts
	freq=mvSlicDirectRegRead(lineId, 13)>>4;  /* Read the frequency */
	DBG("PCM clock =  %s KHz   Rev %c \n",  freqs[freq], 'A'-1 + mvSlicVersion(lineId)); 
}

int mvSlicCalibrate(unsigned int lineId)
{ 
	unsigned short i=0;
	volatile unsigned char   DRvalue;
	int timeOut,nCalComplete;

	/* Do Flush durring powerUp and calibrate */
	/*mvSlicDirectRegWrite(lineId, 21,DISABLE_ALL_DR21);//(0)  Disable all interupts in DR21
	mvSlicDirectRegWrite(lineId, 22,DISABLE_ALL_DR22);//(0)	Disable all interupts in DR21
	mvSlicDirectRegWrite(lineId, 23,DISABLE_ALL_DR23);//(0)	Disabel all interupts in DR21 */
	mvSlicDirectRegWrite(lineId, 64,OPEN_DR64);//(0)
  
	mvSlicDirectRegWrite(lineId, 97,0x1e); //(0x18)Calibrations without the ADC and DAC offset and without common mode calibration.
	mvSlicDirectRegWrite(lineId, 96,0x47); //(0x47)	Calibrate common mode and differential DAC mode DAC + ILIM
	
	DBG("Start Calibration(vBat = %x)\n", mvSlicDirectRegRead(lineId, 82));
 
	i=0;
	do 
	{
		DRvalue = mvSlicDirectRegRead(lineId, 96);
		nCalComplete = DRvalue==CAL_COMPLETE_DR96;// (0)  When Calibration completes DR 96 will be zero
		timeOut= i++>MAX_CAL_PERIOD;
		mvOsDelay(1);
	}
	while (!nCalComplete&&!timeOut);

	if (timeOut) {
	    mvOsPrintf ("Error in Caliberatation: timeOut\n");
	    return 0;
	}        

    
/*Initialized DR 98 and 99 to get consistant results.*/
/* 98 and 99 are the results registers and the search should have same intial conditions.*/
/*******The following is the manual gain mismatch calibration******/
/*******This is also available as a function **********************/
	// Wait for any power alarms to settle. 
	//mvOsDelay(100); /* is it enough ? */

	mvSlicIndirectRegWrite(lineId, 88,0);
	mvSlicIndirectRegWrite(lineId, 89,0);
	mvSlicIndirectRegWrite(lineId, 90,0);
	mvSlicIndirectRegWrite(lineId, 91,0);
	mvSlicIndirectRegWrite(lineId, 92,0);
	mvSlicIndirectRegWrite(lineId, 93,0);


	mvSlicGoActive(lineId);

	if  (mvSlicLoopStatus(lineId) & 4) {
		mvOsPrintf ("Error in Caliberate:  ERRORCODE_LONGBALCAL\n");
		return 0 ;
	}

	mvSlicDirectRegWrite(lineId, 64,OPEN_DR64);

	mvSlicDirectRegWrite(lineId, 23,ENB2_DR23);  // enable interrupt for the balance Cal
	mvSlicDirectRegWrite(lineId, 97,BIT_CALCM_DR97); // this is a singular calibration bit for longitudinal calibration
	mvSlicDirectRegWrite(lineId, 96,0x40);

	DRvalue = mvSlicDirectRegRead(lineId, 96);
	i=0;
	do 
	{
       	DRvalue = mvSlicDirectRegRead(lineId, 96);
        nCalComplete = DRvalue==CAL_COMPLETE_DR96;// (0)  When Calibration completes DR 96 will be zero
		timeOut= i++>MAX_CAL_PERIOD;// (800) MS
		mvOsDelay(1);
	}
	while (!nCalComplete&&!timeOut);
	  
	if (timeOut) {
	    mvOsPrintf ("Error in Caliberate:  timeOut\n");
	    return 0;
	}

	//mvOsDelay(100); /* is it enough ? */

	for (i=88; i<=95; i++) {
		mvSlicIndirectRegWrite (lineId, i, 0);
	}
	mvSlicIndirectRegWrite (lineId, 97, 0);

	for (i=193; i<=211; i++) {
		mvSlicIndirectRegWrite (lineId, i, 0);
	}
    	
	return(1);

}// End of first calibration

static void mvSlicInitializeIndirectRegisters(unsigned int lineId)
{										
if (!Si3215_Flag)
{
	mvSlicIndirectRegWrite(lineId, 	0	,	INIT_IR0		);	//	0x55C2	DTMF_ROW_0_PEAK
	mvSlicIndirectRegWrite(lineId, 	1	,	INIT_IR1		);	//	0x51E6	DTMF_ROW_1_PEAK
	mvSlicIndirectRegWrite(lineId, 	2	,	INIT_IR2		);	//	0x4B85	DTMF_ROW2_PEAK
	mvSlicIndirectRegWrite(lineId, 	3	,	INIT_IR3		);	//	0x4937	DTMF_ROW3_PEAK
	mvSlicIndirectRegWrite(lineId, 	4	,	INIT_IR4		);	//	0x3333	DTMF_COL1_PEAK
	mvSlicIndirectRegWrite(lineId, 	5	,	INIT_IR5		);	//	0x0202	DTMF_FWD_TWIST
	mvSlicIndirectRegWrite(lineId, 	6	,	INIT_IR6		);	//	0x0202	DTMF_RVS_TWIST
	mvSlicIndirectRegWrite(lineId, 	7	,	INIT_IR7		);	//	0x0198	DTMF_ROW_RATIO
	mvSlicIndirectRegWrite(lineId, 	8	,	INIT_IR8		);	//	0x0198	DTMF_COL_RATIO
	mvSlicIndirectRegWrite(lineId, 	9	,	INIT_IR9		);	//	0x0611	DTMF_ROW_2ND_ARM
	mvSlicIndirectRegWrite(lineId, 	10	,	INIT_IR10		);	//	0x0202	DTMF_COL_2ND_ARM
	mvSlicIndirectRegWrite(lineId, 	11	,	INIT_IR11		);	//	0x00E5	DTMF_PWR_MIN_
	mvSlicIndirectRegWrite(lineId, 	12	,	INIT_IR12		);	//	0x0A1C	DTMF_OT_LIM_TRES
}

	mvSlicIndirectRegWrite(lineId, 	13	,	INIT_IR13		);	//	0x7b30	OSC1_COEF
	mvSlicIndirectRegWrite(lineId, 	14	,	INIT_IR14		);	//	0x0063	OSC1X
	mvSlicIndirectRegWrite(lineId, 	15	,	INIT_IR15		);	//	0x0000	OSC1Y
	mvSlicIndirectRegWrite(lineId, 	16	,	INIT_IR16		);	//	0x7870	OSC2_COEF
	mvSlicIndirectRegWrite(lineId, 	17	,	INIT_IR17		);	//	0x007d	OSC2X
	mvSlicIndirectRegWrite(lineId, 	18	,	INIT_IR18		);	//	0x0000	OSC2Y
	mvSlicIndirectRegWrite(lineId, 	19	,	INIT_IR19		);	//	0x0000	RING_V_OFF
	mvSlicIndirectRegWrite(lineId, 	20	,	INIT_IR20		);	//	0x7EF0	RING_OSC
	mvSlicIndirectRegWrite(lineId, 	21	,	INIT_IR21		);	//	0x0160	RING_X
	mvSlicIndirectRegWrite(lineId, 	22	,	INIT_IR22		);	//	0x0000	RING_Y
	mvSlicIndirectRegWrite(lineId, 	23	,	INIT_IR23		);	//	0x2000	PULSE_ENVEL
	mvSlicIndirectRegWrite(lineId, 	24	,	INIT_IR24		);	//	0x2000	PULSE_X
	mvSlicIndirectRegWrite(lineId, 	25	,	INIT_IR25		);	//	0x0000	PULSE_Y
	mvSlicIndirectRegWrite(lineId, 	26	,	INIT_IR26		);	//	0x4000	RECV_DIGITAL_GAIN
	mvSlicIndirectRegWrite(lineId, 	27	,	INIT_IR27		);	//	0x4000	XMIT_DIGITAL_GAIN
	mvSlicIndirectRegWrite(lineId, 	28	,	INIT_IR28		);	//	0x1000	LOOP_CLOSE_TRES
	mvSlicIndirectRegWrite(lineId, 	29	,	INIT_IR29		);	//	0x3600	RING_TRIP_TRES
	mvSlicIndirectRegWrite(lineId, 	30	,	INIT_IR30		);	//	0x1000	COMMON_MIN_TRES
	mvSlicIndirectRegWrite(lineId, 	31	,	INIT_IR31		);	//	0x0200	COMMON_MAX_TRES
	mvSlicIndirectRegWrite(lineId, 	32	,	INIT_IR32		);	//	0x7c0  	PWR_ALARM_Q1Q2
	mvSlicIndirectRegWrite(lineId, 	33	,	INIT_IR33		);	//	0x2600	PWR_ALARM_Q3Q4
	mvSlicIndirectRegWrite(lineId, 	34	,	INIT_IR34		);	//	0x1B80	PWR_ALARM_Q5Q6
	mvSlicIndirectRegWrite(lineId, 	35	,	INIT_IR35		);	//	0x8000	LOOP_CLSRE_FlTER
	mvSlicIndirectRegWrite(lineId, 	36	,	INIT_IR36		);	//	0x0320	RING_TRIP_FILTER
	mvSlicIndirectRegWrite(lineId, 	37	,	INIT_IR37		);	//	0x08c	TERM_LP_POLE_Q1Q2
	mvSlicIndirectRegWrite(lineId, 	38	,	INIT_IR38		);	//	0x0100	TERM_LP_POLE_Q3Q4
	mvSlicIndirectRegWrite(lineId, 	39	,	INIT_IR39		);	//	0x0010	TERM_LP_POLE_Q5Q6
	mvSlicIndirectRegWrite(lineId, 	40	,	INIT_IR40		);	//	0x0C00	CM_BIAS_RINGING
	mvSlicIndirectRegWrite(lineId, 	41	,	INIT_IR41		);	//	0x0C00	DCDC_MIN_V
	mvSlicIndirectRegWrite(lineId, 	43	,	INIT_IR43		);	//	0x1000	LOOP_CLOSE_TRES Low
	mvSlicIndirectRegWrite(lineId, 	99	,	INIT_IR99		);	//	0x00DA	FSK 0 FREQ PARAM
	mvSlicIndirectRegWrite(lineId, 	100	,	INIT_IR100		);	//	0x6B60	FSK 0 AMPL PARAM
	mvSlicIndirectRegWrite(lineId, 	101	,	INIT_IR101		);	//	0x0074	FSK 1 FREQ PARAM
	mvSlicIndirectRegWrite(lineId, 	102	,	INIT_IR102		);	//	0x79C0	FSK 1 AMPl PARAM
	mvSlicIndirectRegWrite(lineId, 	103	,	INIT_IR103		);	//	0x1120	FSK 0to1 SCALER
	mvSlicIndirectRegWrite(lineId, 	104	,	INIT_IR104		);	//	0x3BE0	FSK 1to0 SCALER
	mvSlicIndirectRegWrite(lineId, 	97	,	INIT_IR97		);	//	0x0000	TRASMIT_FILTER
}										

	

static void mvSlicInitializeDirectRegisters(unsigned int lineId)
{

if(spiMode)
	mvSlicDirectRegWrite(lineId, 0,	INIT_DR0_DC	);//0X00	Serial Interface
else
	mvSlicDirectRegWrite(lineId, 0,	INIT_DR0	);//0X00	Serial Interface

mvSlicDirectRegWrite(lineId, 1,	INIT_DR1	);//0X08	PCM Mode - INIT TO disable
mvSlicDirectRegWrite(lineId, 2,	INIT_DR2	);//0X00	PCM TX Clock Slot Low Byte (1 PCLK cycle/LSB)
mvSlicDirectRegWrite(lineId, 3,	INIT_DR3	);//0x00	PCM TX Clock Slot High Byte
mvSlicDirectRegWrite(lineId, 4,	INIT_DR4	);//0x00	PCM RX Clock Slot Low Byte (1 PCLK cycle/LSB)
mvSlicDirectRegWrite(lineId, 5,	INIT_DR5	);//0x00	PCM RX Clock Slot High Byte
mvSlicDirectRegWrite(lineId, 8,	INIT_DR8	);//0X00	Loopbacks (digital loopback default)
mvSlicDirectRegWrite(lineId, 9,	INIT_DR9	);//0x00	Transmit and receive path gain and control
mvSlicDirectRegWrite(lineId, 10,	INIT_DR10	);//0X28	Initialization Two-wire impedance (600  and enabled)
mvSlicDirectRegWrite(lineId, 11,	INIT_DR11	);//0x33	Transhybrid Balance/Four-wire Return Loss
mvSlicDirectRegWrite(lineId, 18,	INIT_DR18	);//0xff	Normal Oper. Interrupt Register 1 (clear with 0xFF)
mvSlicDirectRegWrite(lineId, 19,	INIT_DR19	);//0xff	Normal Oper. Interrupt Register 2 (clear with 0xFF)
mvSlicDirectRegWrite(lineId, 20,	INIT_DR20	);//0xff	Normal Oper. Interrupt Register 3 (clear with 0xFF)
/*mvSlicDirectRegWrite(lineId, 21,	INIT_DR21	);//0xff	Interrupt Mask 1
mvSlicDirectRegWrite(lineId, 22,	INIT_DR22	);//0xff	Initialization Interrupt Mask 2
mvSlicDirectRegWrite(lineId, 23,	INIT_DR23	);//0xff	 Initialization Interrupt Mask 3 */
mvSlicDirectRegWrite(lineId, 32,	INIT_DR32	);//0x00	Oper. Oscillator 1 Controltone generation
mvSlicDirectRegWrite(lineId, 33,	INIT_DR33	);//0x00	Oper. Oscillator 2 Controltone generation
mvSlicDirectRegWrite(lineId, 34,	INIT_DR34	);//0X18	34 0x22 0x00 Initialization Ringing Oscillator Control
mvSlicDirectRegWrite(lineId, 35,	INIT_DR35	);//0x00	Oper. Pulse Metering Oscillator Control
mvSlicDirectRegWrite(lineId, 36,	INIT_DR36	);//0x00	36 0x24 0x00 Initialization OSC1 Active Low Byte (125 탎/LSB)
mvSlicDirectRegWrite(lineId, 37,	INIT_DR37	);//0x00	37 0x25 0x00 Initialization OSC1 Active High Byte (125 탎/LSB)
mvSlicDirectRegWrite(lineId, 38,	INIT_DR38	);//0x00	38 0x26 0x00 Initialization OSC1 Inactive Low Byte (125 탎/LSB)
mvSlicDirectRegWrite(lineId, 39,	INIT_DR39	);//0x00	39 0x27 0x00 Initialization OSC1 Inactive High Byte (125 탎/LSB)
mvSlicDirectRegWrite(lineId, 40,	INIT_DR40	);//0x00	40 0x28 0x00 Initialization OSC2 Active Low Byte (125 탎/LSB)
mvSlicDirectRegWrite(lineId, 41,	INIT_DR41	);//0x00	41 0x29 0x00 Initialization OSC2 Active High Byte (125 탎/LSB)
mvSlicDirectRegWrite(lineId, 42,	INIT_DR42	);//0x00	42 0x2A 0x00 Initialization OSC2 Inactive Low Byte (125 탎/LSB)
mvSlicDirectRegWrite(lineId, 43,	INIT_DR43	);//0x00	43 0x2B 0x00 Initialization OSC2 Inactive High Byte (125 탎/LSB)
mvSlicDirectRegWrite(lineId, 44,	INIT_DR44	);//0x00	44 0x2C 0x00 Initialization Pulse Metering Active Low Byte (125 탎/LSB)
mvSlicDirectRegWrite(lineId, 45,	INIT_DR45	);//0x00	45 0x2D 0x00 Initialization Pulse Metering Active High Byte (125 탎/LSB)
mvSlicDirectRegWrite(lineId, 46,	INIT_DR46	);//0x00	46 0x2E 0x00 Initialization Pulse Metering Inactive Low Byte (125 탎/LSB)
mvSlicDirectRegWrite(lineId, 47,	INIT_DR47	);//0x00	47 0x2F 0x00 Initialization Pulse Metering Inactive High Byte (125 탎/LSB)
mvSlicDirectRegWrite(lineId, 48,	INIT_DR48	);//0X80	48 0x30 0x00 0x80 Initialization Ringing Osc. Active Timer Low Byte (2 s,125 탎/LSB)
mvSlicDirectRegWrite(lineId, 49,	INIT_DR49	);//0X3E	49 0x31 0x00 0x3E Initialization Ringing Osc. Active Timer High Byte (2 s,125 탎/LSB)
mvSlicDirectRegWrite(lineId, 50,	INIT_DR50	);//0X00	50 0x32 0x00 0x00 Initialization Ringing Osc. Inactive Timer Low Byte (4 s, 125 탎/LSB)
mvSlicDirectRegWrite(lineId, 51,	INIT_DR51	);//0X7D	51 0x33 0x00 0x7D Initialization Ringing Osc. Inactive Timer High Byte (4 s, 125 탎/LSB)
mvSlicDirectRegWrite(lineId, 52,	INIT_DR52	);//0X00	52 0x34 0x00 Normal Oper. FSK Data Bit
mvSlicDirectRegWrite(lineId, 63,	INIT_DR63	);//0X54	63 0x3F 0x54 Initialization Ringing Mode Loop Closure Debounce Interval
mvSlicDirectRegWrite(lineId, 64,	INIT_DR64	);//0x00	64 0x40 0x00 Normal Oper. Mode Byte뾭rimary control
mvSlicDirectRegWrite(lineId, 65,	INIT_DR65	);//0X61	65 0x41 0x61 Initialization External Bipolar Transistor Settings
mvSlicDirectRegWrite(lineId, 66,	INIT_DR66	);//0X03	66 0x42 0x03 Initialization Battery Control
mvSlicDirectRegWrite(lineId, 67,	INIT_DR67	);//0X1F	67 0x43 0x1F Initialization Automatic/Manual Control
mvSlicDirectRegWrite(lineId, 69,	INIT_DR69	);//0X0C	69 0x45 0x0A 0x0C Initialization Loop Closure Debounce Interval (1.25 ms/LSB)
mvSlicDirectRegWrite(lineId, 70,	INIT_DR70	);//0X0A	70 0x46 0x0A Initialization Ring Trip Debounce Interval (1.25 ms/LSB)
mvSlicDirectRegWrite(lineId, 71,	INIT_DR71	);//0X01	71 0x47 0x00 0x01 Initialization Off-Hook Loop Current Limit (20 mA + 3 mA/LSB)
mvSlicDirectRegWrite(lineId, 72,	INIT_DR72	);//0X20	72 0x48 0x20 Initialization On-Hook Voltage (open circuit voltage) = 48 V(1.5 V/LSB)
mvSlicDirectRegWrite(lineId, 73,	INIT_DR73	);//0X02	73 0x49 0x02 Initialization Common Mode VoltageVCM = 3 V(1.5 V/LSB)
mvSlicDirectRegWrite(lineId, 74,	INIT_DR74	);//0X32	74 0x4A 0x32 Initialization VBATH (ringing) = 75 V (1.5 V/LSB)
mvSlicDirectRegWrite(lineId, 75,	INIT_DR75	);//0X10	75 0x4B 0x10 Initialization VBATL (off-hook) = 24 V (TRACK = 0)(1.5 V/LSB)
if (mvSlicChipType(lineId) != 3)
mvSlicDirectRegWrite(lineId, 92,	INIT_DR92	);//0x7f	92 0x5C 0xFF 7F Initialization DCDC Converter PWM Period (61.035 ns/LSB)
else
mvSlicDirectRegWrite(lineId, 92,	INIT_SI3210M_DR92	);//0x7f	92 0x5C 0xFF 7F Initialization DCDC Converter PWM Period (61.035 ns/LSB)


mvSlicDirectRegWrite(lineId, 93,	INIT_DR93	);//0x14	93 0x5D 0x14 0x19 Initialization DCDC Converter Min. Off Time (61.035 ns/LSB)
mvSlicDirectRegWrite(lineId, 96,	INIT_DR96	);//0x00	96 0x60 0x1F Initialization Calibration Control Register 1(written second and starts calibration)
mvSlicDirectRegWrite(lineId, 97,	INIT_DR97	);//0X1F	97 0x61 0x1F Initialization Calibration Control Register 2(written before Register 96)
mvSlicDirectRegWrite(lineId, 98,	INIT_DR98	);//0X10	98 0x62 0x10 Informative Calibration result (see data sheet)
mvSlicDirectRegWrite(lineId, 99,	INIT_DR99	);//0X10	99 0x63 0x10 Informative Calibration result (see data sheet)
mvSlicDirectRegWrite(lineId, 100,	INIT_DR100	);//0X11	100 0x64 0x11 Informative Calibration result (see data sheet)
mvSlicDirectRegWrite(lineId, 101,	INIT_DR101	);//0X11	101 0x65 0x11 Informative Calibration result (see data sheet)
mvSlicDirectRegWrite(lineId, 102,	INIT_DR102	);//0x08	102 0x66 0x08 Informative Calibration result (see data sheet)
mvSlicDirectRegWrite(lineId, 103,	INIT_DR103	);//0x88	103 0x67 0x88 Informative Calibration result (see data sheet)
mvSlicDirectRegWrite(lineId, 104,	INIT_DR104	);//0x00	104 0x68 0x00 Informative Calibration result (see data sheet)
mvSlicDirectRegWrite(lineId, 105,	INIT_DR105	);//0x00	105 0x69 0x00 Informative Calibration result (see data sheet)
mvSlicDirectRegWrite(lineId, 106,	INIT_DR106	);//0x20	106 0x6A 0x20 Informative Calibration result (see data sheet)
mvSlicDirectRegWrite(lineId, 107,	INIT_DR107	);//0x08	107 0x6B 0x08 Informative Calibration result (see data sheet)
mvSlicDirectRegWrite(lineId, 108,	INIT_DR108	);//0xEB	108 0x63 0x00 0xEB Initialization Feature enhancement register
}
   

static void mvSlicClearInterrupts(unsigned int lineId)
{
	mvSlicDirectRegWrite(lineId, 	18	,	INIT_DR18	);//0xff	Normal Oper. Interrupt Register 1 (clear with 0xFF)
	mvSlicDirectRegWrite(lineId, 	19	,	INIT_DR19	);//0xff	Normal Oper. Interrupt Register 2 (clear with 0xFF)
	mvSlicDirectRegWrite(lineId, 	20	,	INIT_DR20	);//0xff	Normal Oper. Interrupt Register 3 (clear with 0xFF)
}

int verifyIndirectReg(unsigned int lineId, unsigned char address, unsigned short should_be_value)
{ 
	int error_flag ;
	unsigned short value;
	
		value = mvSlicIndirectRegRead(lineId, address);
		error_flag = (should_be_value != value);
		
		if ( error_flag )
		{
			mvOsPrintf("\n   iREG %d = %X  should be %X ",address,value,should_be_value );			
		}	
		return error_flag;
}

#ifdef DEBUG

static int mvSlicVerifyIndirectRegisters(unsigned int lineId)
{		
	int error=0;

	if (!Si3215_Flag)
	{

	error |= verifyIndirectReg(lineId   ,    	0	,	INIT_IR0		);	//	0x55C2	DTMF_ROW_0_PEAK
	error |= verifyIndirectReg(lineId   ,    	1	,	INIT_IR1		);	//	0x51E6	DTMF_ROW_1_PEAK
	error |= verifyIndirectReg(lineId   ,    	2	,	INIT_IR2		);	//	0x4B85	DTMF_ROW2_PEAK
	error |= verifyIndirectReg(lineId   ,    	3	,	INIT_IR3		);	//	0x4937	DTMF_ROW3_PEAK
	error |= verifyIndirectReg(lineId   ,    	4	,	INIT_IR4		);	//	0x3333	DTMF_COL1_PEAK
	error |= verifyIndirectReg(lineId   ,    	5	,	INIT_IR5		);	//	0x0202	DTMF_FWD_TWIST
	error |= verifyIndirectReg(lineId   ,    	6	,	INIT_IR6		);	//	0x0202	DTMF_RVS_TWIST
	error |= verifyIndirectReg(lineId   ,    	7	,	INIT_IR7		);	//	0x0198	DTMF_ROW_RATIO
	error |= verifyIndirectReg(lineId   ,    	8	,	INIT_IR8		);	//	0x0198	DTMF_COL_RATIO
	error |= verifyIndirectReg(lineId   ,    	9	,	INIT_IR9		);	//	0x0611	DTMF_ROW_2ND_ARM
	error |= verifyIndirectReg(lineId   ,    	10	,	INIT_IR10		);	//	0x0202	DTMF_COL_2ND_ARM
	error |= verifyIndirectReg(lineId   ,    	11	,	INIT_IR11		);	//	0x00E5	DTMF_PWR_MIN_
	error |= verifyIndirectReg(lineId   ,    	12	,	INIT_IR12		);	//	0x0A1C	DTMF_OT_LIM_TRES
	}
	error |= verifyIndirectReg(lineId   ,    	13	,	INIT_IR13		);	//	0x7b30	OSC1_COEF
	error |= verifyIndirectReg(lineId   ,    	14	,	INIT_IR14		);	//	0x0063	OSC1X
	error |= verifyIndirectReg(lineId   ,    	15	,	INIT_IR15		);	//	0x0000	OSC1Y
	error |= verifyIndirectReg(lineId   ,    	16	,	INIT_IR16		);	//	0x7870	OSC2_COEF
	error |= verifyIndirectReg(lineId   ,    	17	,	INIT_IR17		);	//	0x007d	OSC2X
	error |= verifyIndirectReg(lineId   ,    	18	,	INIT_IR18		);	//	0x0000	OSC2Y
	error |= verifyIndirectReg(lineId   ,    	19	,	INIT_IR19		);	//	0x0000	RING_V_OFF
	error |= verifyIndirectReg(lineId   ,    	20	,	INIT_IR20		);	//	0x7EF0	RING_OSC
	error |= verifyIndirectReg(lineId   ,    	21	,	INIT_IR21		);	//	0x0160	RING_X
	error |= verifyIndirectReg(lineId   ,    	22	,	INIT_IR22		);	//	0x0000	RING_Y
	error |= verifyIndirectReg(lineId   ,    	23	,	INIT_IR23		);	//	0x2000	PULSE_ENVEL
	error |= verifyIndirectReg(lineId   ,    	24	,	INIT_IR24		);	//	0x2000	PULSE_X
	error |= verifyIndirectReg(lineId   ,    	25	,	INIT_IR25		);	//	0x0000	PULSE_Y
	error |= verifyIndirectReg(lineId   ,    	26	,	INIT_IR26		);	//	0x4000	RECV_DIGITAL_GAIN
	error |= verifyIndirectReg(lineId   ,    	27	,	INIT_IR27		);	//	0x4000	XMIT_DIGITAL_GAIN
	error |= verifyIndirectReg(lineId   ,    	28	,	INIT_IR28		);	//	0x1000	LOOP_CLOSE_TRES
	error |= verifyIndirectReg(lineId   ,    	29	,	INIT_IR29		);	//	0x3600	RING_TRIP_TRES
	error |= verifyIndirectReg(lineId   ,    	30	,	INIT_IR30		);	//	0x1000	COMMON_MIN_TRES
	error |= verifyIndirectReg(lineId   ,    	31	,	INIT_IR31		);	//	0x0200	COMMON_MAX_TRES
	error |= verifyIndirectReg(lineId   ,    	32	,	INIT_IR32		);	//	0x7c0  	PWR_ALARM_Q1Q2
	error |= verifyIndirectReg(lineId   ,    	33	,	INIT_IR33		);	//	0x2600	PWR_ALARM_Q3Q4
	error |= verifyIndirectReg(lineId   ,    	34	,	INIT_IR34		);	//	0x1B80	PWR_ALARM_Q5Q6
	error |= verifyIndirectReg(lineId   ,    	35	,	INIT_IR35		);	//	0x8000	LOOP_CLSRE_FlTER
	error |= verifyIndirectReg(lineId   ,    	36	,	INIT_IR36		);	//	0x0320	RING_TRIP_FILTER
	error |= verifyIndirectReg(lineId   ,    	37	,	INIT_IR37		);	//	0x08c	TERM_LP_POLE_Q1Q2
	error |= verifyIndirectReg(lineId   ,    	38	,	INIT_IR38		);	//	0x0100	TERM_LP_POLE_Q3Q4
	error |= verifyIndirectReg(lineId   ,    	39	,	INIT_IR39		);	//	0x0010	TERM_LP_POLE_Q5Q6
	error |= verifyIndirectReg(lineId   ,    	40	,	INIT_IR40		);	//	0x0C00	CM_BIAS_RINGING
	error |= verifyIndirectReg(lineId   ,    	41	,	INIT_IR41		);	//	0x0C00	DCDC_MIN_V
	error |= verifyIndirectReg(lineId   ,    	43	,	INIT_IR43		);	//	0x1000	LOOP_CLOSE_TRES Low
	error |= verifyIndirectReg(lineId   ,    	99	,	INIT_IR99		);	//	0x00DA	FSK 0 FREQ PARAM
	error |= verifyIndirectReg(lineId   ,    	100	,	INIT_IR100		);	//	0x6B60	FSK 0 AMPL PARAM
	error |= verifyIndirectReg(lineId   ,    	101	,	INIT_IR101		);	//	0x0074	FSK 1 FREQ PARAM
	error |= verifyIndirectReg(lineId   ,    	102	,	INIT_IR102		);	//	0x79C0	FSK 1 AMPl PARAM
	error |= verifyIndirectReg(lineId   ,    	103	,	INIT_IR103		);	//	0x1120	FSK 0to1 SCALER
	error |= verifyIndirectReg(lineId   ,    	104	,	INIT_IR104		);	//	0x3BE0	FSK 1to0 SCALER
	
	return error;
}

#endif /* DEBUG */

static void mvSlicEnablePCMhighway(unsigned int lineId)
{
	/* pcm Mode Select */
	/*
	** Bit 0:   tri-State on Positive edge of Pclk
	** Bit 1:   GCI Clock Format, 1 PCLK per data bit
	** Bit 2:   PCM Transfer size (8 bt transfer)
	** Bit 3-4:	PCM Format mu-law
	** Bit 5:   PCM Enable
	*/
	mvSlicDirectRegWrite(lineId, 1,0x28); 
}


/* New Functions */
static MV_STATUS mvSlicPrintInfo(unsigned int lineId)
{
    unsigned int type;
    unsigned char fm;
    char name[10];

    
    fm = mvSlicFamily(lineId);
    type = mvSlicChipType(lineId);
    
	if(fm == MV_SI3210_FAMILY)
	{
		if(type == MV_SI3210)
			strcpy(name, "SI3210");
		else if(type == MV_SI3211)
			strcpy(name, "SI3211");
		else if(type == MV_SI3210M)
			strcpy(name, "SI3210M");
		else 
            strcpy(name, "UNKNOWN");
	}
	else if(fm == MV_SI3215_FAMILY)
	{
		if(type == MV_SI3215)
			strcpy(name, "SI3215");
		else if(type == MV_SI3215M)
			strcpy(name, "SI3215M");
		else
            strcpy(name, "UNKNOWN");

	}
	strcat(name, "\0");
    //mvOsPrintf("FXS type : %s\n",name);

    if(strcmp(name,"UNKNOWN"))
        return MV_OK;
    else
        return MV_NOT_SUPPORTED;

}
MV_STATUS mvSlicInit(unsigned short first, unsigned short last, unsigned short pSlotInfo[][2],
		     unsigned short lines, mv_band_mode_t bandMode, mv_pcm_format_t pcmFormat)
{
	MV_SLIC_LINE_INFO *pSlicLineInfo;
	unsigned int lineId;

	MV_TRC_REC("->%s\n", __FUNCTION__);

	/* Save parameters */
	firstSlic = first;
	lastSlic = last;
	mvBandMode = bandMode;
	mvPcmFormat = pcmFormat;
	totalSLICs = last -first + 1;
	spiMode = mvBoardTdmSpiModeGet();
	calibrated = 0;

	slicLineInfo = (MV_SLIC_LINE_INFO*)mvOsMalloc(sizeof(MV_SLIC_LINE_INFO) * totalSLICs);
	if(!slicLineInfo)
	{
		mvOsPrintf("%s:Error, allocation failed(slicLineInfo) !!!\n",__FUNCTION__);
		return MV_ERROR;
	}
	
	/* Set pointer to first SLIC */
	pSlicLineInfo = (MV_SLIC_LINE_INFO*)(slicLineInfo + firstSlic);

	mvOsPrintf("Start initialization for SLIC modules %u to %u...",firstSlic, lastSlic);

	for(lineId = firstSlic; lineId < (lastSlic+1); lineId++)
	{
		pSlicLineInfo->lineId = lineId;

		if(!mvSlicStart(lineId))
			return MV_ERROR;

		/* Goto next SLIC module */
		pSlicLineInfo++;
	}

	/* Set pointer to first SLIC */
	pSlicLineInfo = (MV_SLIC_LINE_INFO*)(slicLineInfo + firstSlic);

	for(lineId = firstSlic; lineId < (lastSlic+1); lineId++)
	{
		/* Complete starting phase */
		mvSlicStart2(lineId);

		mvSlicEnablePCMhighway(lineId);

		/* print slic info */
		if(mvSlicPrintInfo(lineId) == MV_OK) 
		{
			if(spiMode)
			{
				if(!mvSlicDaisyChainGet(lineId))
				{
					mvOsPrintf("error, line %d is not in daisy chain mode\n",lineId);
					return MV_ERROR;
				}
			}

			mvSlicIntDisable(lineId);

			switch(pcmFormat)
			{
				case MV_PCM_FORMAT_ALAW:
				case MV_PCM_FORMAT_ULAW:
				case MV_PCM_FORMAT_LINEAR:
					mvSlicPcmFormatSet(lineId, pcmFormat);
					break;
				default:
					mvOsPrintf("Error, wrong pcm format(%d)\n", pcmFormat);
					break;
			}

			/* Configure tx/rx sample in SLIC */
			pSlicLineInfo->rxSample = pSlotInfo[lineId][0];
			pSlicLineInfo->txSample = pSlotInfo[lineId][1];
			DBG("Rx sample %d, Tx sample %d\n", pSlicLineInfo->rxSample, pSlicLineInfo->txSample);
			pSlicLineInfo->txSample *= 8;
			pSlicLineInfo->rxSample *= 8;
			mvSlicPcmStartCountRegsSet(lineId);
		}
		else
		{
			mvOsPrintf("%s: error, unknown slic type\n",__FUNCTION__);
			return MV_ERROR;
		}

		/* enable interrupts */
		mvSlicIntEnable(lineId);

		/* Goto next SLIC module */
		pSlicLineInfo++;

		mvSlicPrintFreqRevision(lineId);
	}

	mvOsPrintf("Done\n");
	MV_TRC_REC("<-%s\n", __FUNCTION__);
	return MV_OK;
}

static void mvSlicIntEnable(unsigned int lineId)
{
	MV_TRC_REC("enable slic-%d interrupts\n", lineId);
	mvSlicDirectRegWrite(lineId, INTRPT_MASK1, 0x10);		
	mvSlicDirectRegWrite(lineId, INTRPT_MASK2, 2);
	mvSlicDirectRegWrite(lineId, INTRPT_MASK3, 0);

}

static void mvSlicIntDisable(unsigned int lineId)
{
	MV_TRC_REC("disable slic-%d interrupts\n", lineId);
	mvSlicDirectRegWrite(lineId, INTRPT_MASK1, 0);
	mvSlicDirectRegWrite(lineId, INTRPT_MASK2, 0);
	mvSlicDirectRegWrite(lineId, INTRPT_MASK3, 0);
	mvSlicClearInterrupts(lineId);
}



static int mvSlicDaisyChainGet(unsigned int lineId)
{
    int val;

    val = mvSlicDirectRegRead(lineId, 0);
	
    return (val & 0x80);

}

static void mvSlicPcmFormatSet(unsigned int lineId, mv_pcm_format_t pcmFormat)
{
	int val;

	val = mvSlicDirectRegRead(lineId, PCM_MODE);

	if(pcmFormat == MV_PCM_FORMAT_LINEAR)
		val = val | BIT2; /* set 16-bit transfer */
	else
		val = val & ~BIT2; /* set 8-bit transfer */

	mvSlicDirectRegWrite(lineId, PCM_MODE, (val | (pcmFormat<<3)));
}

static void mvSlicPcmStartCountRegsSet(unsigned int lineId)
{
	MV_SLIC_LINE_INFO *pSlicLineInfo = (MV_SLIC_LINE_INFO*)(slicLineInfo + lineId);

	mvSlicDirectRegWrite(lineId, PCM_XMIT_START_COUNT_LSB, pSlicLineInfo->txSample&0xff);
	mvSlicDirectRegWrite(lineId, PCM_XMIT_START_COUNT_MSB, (pSlicLineInfo->txSample>>8)&0x3);
	mvSlicDirectRegWrite(lineId, PCM_RCV_START_COUNT_LSB,  pSlicLineInfo->rxSample&0xff);
	mvSlicDirectRegWrite(lineId, PCM_RCV_START_COUNT_MSB, (pSlicLineInfo->rxSample>>8)&0x3);

}

static int mvSlicIntCheck(unsigned int lineId)
{
   union {
		unsigned char reg_data[3];
		long interrupt_bits;
	} u ;
	u.interrupt_bits=0;


	u.reg_data[0] = mvSlicDirectRegRead(lineId, 18);
	mvSlicDirectRegWrite(lineId, 18, u.reg_data[0]);

	u.reg_data[1] = mvSlicDirectRegRead(lineId, 19);
	if ((u.reg_data[1] & 0xfc) != 0) {
		mvSlicClearAlarmBits (lineId);
	} 
	mvSlicDirectRegWrite(lineId, 19, u.reg_data[1] );

	u.reg_data[2] = mvSlicDirectRegRead(lineId,  20);
	mvSlicDirectRegWrite(lineId,  20, u.reg_data[2]);

	if((u.reg_data[0] & 0x10) && (0x04  == mvSlicDirectRegRead(lineId, 68)))	
	{
		return 0;
	}

	return (u.interrupt_bits)? 1 : 0;
 
}

unsigned char mvSlicIntGet(unsigned int lineId)
{
	unsigned char state;

	if(!mvSlicIntCheck(lineId))
		return 0;
	
	mvSlicHookStateGet(lineId, &state);

	return	state; 
	
} 


void mvSlicHookStateGet(unsigned short lineId, unsigned char* hookstate)
{
	MV_U8 reg68;

    	reg68 = mvSlicDirectRegRead(lineId, 68);
	
	if((reg68 & 4))
	{
		*hookstate = ON_HOOK;
	}
	else
	{
		*hookstate = OFF_HOOK;
	}

	return;
}

void mvSlicReverseDcPolarity(unsigned char lineId)
{ 
	MV_U8 data;

	/* read value of On-Hook Line Voltage register */
	data = mvSlicDirectRegRead(lineId, OFF_HOOK_V);
	/* toggle the VSGN bit - VTIP뻍RING polarity */
	mvSlicDirectRegWrite(lineId, OFF_HOOK_V, data^0x40);
    
	return;
}

void mvSlicLinefeedControlSet(unsigned char lineId, mv_linefeed_t lfState)
{
	mvSlicDirectRegWrite(lineId, LINE_STATE, lfState);
	return;
}

void mvSlicLinefeedControlGet(unsigned char lineId, mv_linefeed_t* lfState)
{
	*lfState = ((mvSlicDirectRegRead(lineId, LINE_STATE) & 0x70) >> 4);
	return;
}


void mvSlicRelease(void)
{
	if(slicLineInfo)
		mvOsFree(slicLineInfo);
}
