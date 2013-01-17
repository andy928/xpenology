
#include "voiceband/daa/daa.h"
#include "voiceband/tdm/mvTdm.h"

#undef DEBUG
#ifdef DEBUG
#define DBG(fmt, arg...)	mvOsPrintf(KERN_INFO fmt, ##arg) 
#else 
#define DBG(fmt, arg...)
#endif

/* Static APIs */
static unsigned char mvDaaDirectRegRead(unsigned int lineId, unsigned char address);
static void mvDaaDirectRegWrite(unsigned int lineId, unsigned char address, unsigned char data);
static void mvDaaPrintInfo(unsigned int lineId);
static void mvDaaIntEnable(unsigned int lineId);
static void mvDaaIntDisable(unsigned int lineId);
static void mvDaaPcmStartCountRegsSet(unsigned int lineId);
static int mvDaaIntCheck(unsigned int lineId); 
static void mvDaaCalibrate(unsigned int lineId);
static void mvDaaDigitalHybridSet(unsigned int index, unsigned int lineId);

typedef struct {
	unsigned char lineId;
	unsigned short txSample;
	unsigned short rxSample;
	unsigned char hook;
	unsigned char ring;
	unsigned char reverse_polarity;
	unsigned char drop_out;

} MV_DAA_LINE_INFO;

#define COUNTRY_INDEX   	29 /* SET TO ISRAEL */
#define NUM_OF_REGS		59
#define SPI_READ_CONTROL_BYTE	0x60	
#define SPI_WRITE_CONTROL_BYTE	0x20

static MV_DAA_LINE_INFO* daaLineInfo = NULL;
static int totalDAAs = 0;
static unsigned char spiMode;
static mv_band_mode_t mvBandMode;
static mv_pcm_format_t mvPcmFormat;
static unsigned short firstDaa = 0;
static unsigned short lastDaa = 0;

static struct MV_DAA_MODE {
	char *name;
	int ohs;
	int ohs2;
	int rz;
	int rt;
	int ilim;
	int dcv;
	int mini;
	int acim;
} daaModes[] =
{
	{ "ARGENTINA", 0, 0, 0, 0, 0, 0x3, 0, 0, },
	{ "AUSTRALIA", 1, 0, 0, 0, 0, 0, 0x3, 0x3, },
	{ "AUSTRIA", 0, 1, 0, 0, 1, 0x3, 0, 0x3, },
	{ "BAHRAIN", 0, 0, 0, 0, 1, 0x3, 0, 0x2, },
	{ "BELGIUM", 0, 1, 0, 0, 1, 0x3, 0, 0x2, },
	{ "BRAZIL", 0, 0, 0, 0, 0, 0, 0x3, 0, },
	{ "BULGARIA", 0, 0, 0, 0, 1, 0x3, 0x0, 0x3, },
	{ "CANADA", 0, 0, 0, 0, 0, 0x3, 0, 0, },
	{ "CHILE", 0, 0, 0, 0, 0, 0x3, 0, 0, },
	{ "CHINA", 0, 0, 0, 0, 0, 0, 0x3, 0xf, },
	{ "COLUMBIA", 0, 0, 0, 0, 0, 0x3, 0, 0, },
	{ "CROATIA", 0, 0, 0, 0, 1, 0x3, 0, 0x2, },
	{ "CYRPUS", 0, 0, 0, 0, 1, 0x3, 0, 0x2, },
	{ "CZECH", 0, 0, 0, 0, 1, 0x3, 0, 0x2, },
	{ "DENMARK", 0, 1, 0, 0, 1, 0x3, 0, 0x2, },
	{ "ECUADOR", 0, 0, 0, 0, 0, 0x3, 0, 0, },
	{ "EGYPT", 0, 0, 0, 0, 0, 0, 0x3, 0, },
	{ "ELSALVADOR", 0, 0, 0, 0, 0, 0x3, 0, 0, },
	{ "FINLAND", 0, 1, 0, 0, 1, 0x3, 0, 0x2, },
	{ "FRANCE", 0, 1, 0, 0, 1, 0x3, 0, 0x2, },
	{ "GERMANY", 0, 1, 0, 0, 1, 0x3, 0, 0x3, },
	{ "GREECE", 0, 1, 0, 0, 1, 0x3, 0, 0x2, },
	{ "GUAM", 0, 0, 0, 0, 0, 0x3, 0, 0, },
	{ "HONGKONG", 0, 0, 0, 0, 0, 0x3, 0, 0, },
	{ "HUNGARY", 0, 0, 0, 0, 0, 0x3, 0, 0, },
	{ "ICELAND", 0, 1, 0, 0, 1, 0x3, 0, 0x2, },
	{ "INDIA", 0, 0, 0, 0, 0, 0x3, 0, 0x4, },
	{ "INDONESIA", 0, 0, 0, 0, 0, 0x3, 0, 0, },
	{ "IRELAND", 0, 1, 0, 0, 1, 0x3, 0, 0x2, },
	{ "ISRAEL", 0, 1, 0, 0, 1, 0x3, 0, 0x2, },
	{ "ITALY", 0, 1, 0, 0, 1, 0x3, 0, 0x2, },
	{ "JAPAN", 0, 0, 0, 0, 0, 0, 0x3, 0, },
	{ "JORDAN", 0, 0, 0, 0, 0, 0, 0x3, 0, },
	{ "KAZAKHSTAN", 0, 0, 0, 0, 0, 0x3, 0, },
	{ "KUWAIT", 0, 0, 0, 0, 0, 0x3, 0, 0, },
	{ "LATVIA", 0, 0, 0, 0, 1, 0x3, 0, 0x2, },
	{ "LEBANON", 0, 0, 0, 0, 1, 0x3, 0, 0x2, },
	{ "LUXEMBOURG", 0, 1, 0, 0, 1, 0x3, 0, 0x2, },
	{ "MACAO", 0, 0, 0, 0, 0, 0x3, 0, 0, },
	{ "MALAYSIA", 0, 0, 0, 0, 0, 0, 0x3, 0, },	/* Current loop >= 20ma */
	{ "MALTA", 0, 0, 0, 0, 1, 0x3, 0, 0x2, },
	{ "MEXICO", 0, 0, 0, 0, 0, 0x3, 0, 0, },
	{ "MOROCCO", 0, 0, 0, 0, 1, 0x3, 0, 0x2, },
	{ "NETHERLANDS", 0, 1, 0, 0, 1, 0x3, 0, 0x2, },
	{ "NEWZEALAND", 0, 0, 0, 0, 0, 0x3, 0, 0x4, },
	{ "NIGERIA", 0, 0, 0, 0, 0x1, 0x3, 0, 0x2, },
	{ "NORWAY", 0, 1, 0, 0, 1, 0x3, 0, 0x2, },
	{ "OMAN", 0, 0, 0, 0, 0, 0, 0x3, 0, },
	{ "PAKISTAN", 0, 0, 0, 0, 0, 0, 0x3, 0, },
	{ "PERU", 0, 0, 0, 0, 0, 0x3, 0, 0, },
	{ "PHILIPPINES", 0, 0, 0, 0, 0, 0, 0x3, 0, },
	{ "POLAND", 0, 0, 1, 1, 0, 0x3, 0, 0, },
	{ "PORTUGAL", 0, 1, 0, 0, 1, 0x3, 0, 0x2, },
	{ "ROMANIA", 0, 0, 0, 0, 0, 3, 0, 0, },
	{ "RUSSIA", 0, 0, 0, 0, 0, 0, 0x3, 0, },
	{ "SAUDIARABIA", 0, 0, 0, 0, 0, 0x3, 0, 0, },
	{ "SINGAPORE", 0, 0, 0, 0, 0, 0x3, 0, 0, },
	{ "SLOVAKIA", 0, 0, 0, 0, 0, 0x3, 0, 0x3, },
	{ "SLOVENIA", 0, 0, 0, 0, 0, 0x3, 0, 0x2, },
	{ "SOUTHAFRICA", 1, 0, 1, 0, 0, 0x3, 0, 0x3, },
	{ "SOUTHKOREA", 0, 0, 0, 0, 0, 0x3, 0, 0, },
	{ "SPAIN", 0, 1, 0, 0, 1, 0x3, 0, 0x2, },
	{ "SWEDEN", 0, 1, 0, 0, 1, 0x3, 0, 0x2, },
	{ "SWITZERLAND", 0, 1, 0, 0, 1, 0x3, 0, 0x2, },
	{ "TAIWAN", 0, 0, 0, 0, 0, 0, 0x3, 0, },
	{ "TBR21", 0, 0, 0, 0, 1, 0x3, 0, 0x2/*, 0x7e6c, 0x023a, */},
	/* Austria, Belgium, Denmark, Finland, France, Germany, Greece, Iceland, Ireland, Italy, Luxembourg, Netherlands,
	Norway, Portugal, Spain, Sweden, Switzerland, and UK */
	{ "THAILAND", 0, 0, 0, 0, 0, 0, 0x3, 0, },
	{ "UAE", 0, 0, 0, 0, 0, 0x3, 0, 0, },
	{ "UK", 0, 1, 0, 0, 1, 0x3, 0, 0x5, },
	{ "USA", 0, 0, 0, 0, 0, 0x3, 0, 0, },
	{ "YEMEN", 0, 0, 0, 0, 0, 0x3, 0, 0, },
	{ "FCC", 0, 0, 0, 1, 0, 0x3, 0, 0, }, 	/* US, Canada */

};


static unsigned char mvDaaDirectRegRead(unsigned int lineId, unsigned char address)
{
	unsigned char data;

	mvVbSpiRead(lineId, address, MV_LINE_FXO, &data);
	return data;
}

static void mvDaaDirectRegWrite(unsigned int lineId, unsigned char address, unsigned char data)
{
	mvVbSpiWrite(lineId, address, MV_LINE_FXO, data);
}

static void mvDaaPrintInfo(unsigned int lineId)
{
	unsigned char reg11;

	reg11 =  mvDaaDirectRegRead(lineId, DAA_SYSTEM_AND_LINE_SIDE_REV_REG);
	if(reg11)
	{
		DBG("DAA Si3050 Detected.\n");

		switch((reg11 & DAA_LINE_SIDE_ID_MASK))
		{
			case DAA_SI3018 :	DBG("Line Side ID: Si3018\n");
						break;

			case DAA_SI3019 :	DBG("Line Side ID: Si3019\n");
						break;

			default:		DBG("Unkown Line Side ID\n");
						break;
		}
      
		switch((reg11 & DAA_SYSTEM_SIDE_REVISION_MASK))
		{
			case DAA_SYSTEM_SIDE_REV_A:	DBG("System Side Revision: A\n");
							break;
		
			case DAA_SYSTEM_SIDE_REV_B:	DBG("System Side Revision: B\n");
							break;
		
			case DAA_SYSTEM_SIDE_REV_C:	DBG("System Side Revision: C\n");
							break;
		
			case DAA_SYSTEM_SIDE_REV_D:	DBG("System Side Revision: D\n");
							break;
		
			default:			DBG("Unkonwn System Side Revision\n");
							break;
		}

		DBG("DAA Country Settings: %s\n",daaModes[COUNTRY_INDEX].name);
	}
	else
		mvOsPrintf("Error, unable to communicate with DAA\n");

}

MV_STATUS mvDaaInit(unsigned short first, unsigned short last, unsigned short pSlotInfo[][2],
		    unsigned short lines, mv_band_mode_t bandMode, mv_pcm_format_t pcmFormat)
{
	unsigned char reg2 = 0, reg24 = 0, reg33 = 0;
	unsigned short newDelay, lineId;
	MV_DAA_LINE_INFO *pDaaLineInfo;
	int lineSideId;

	MV_TRC_REC("->%s\n", __FUNCTION__);

	/* Save parameters */
	firstDaa = first;
	lastDaa = last;
	mvBandMode = bandMode;
	mvPcmFormat = pcmFormat;
	totalDAAs = last -first + 1;
	spiMode = mvBoardTdmSpiModeGet();

	/* Allocate MV_DAA_LINE_INFO array */
	daaLineInfo = (MV_DAA_LINE_INFO*)mvOsMalloc(sizeof(MV_DAA_LINE_INFO) * lines);
	if(!daaLineInfo)
	{
		mvOsPrintf("%s: allocation failed(daaLineInfo) !!!\n",__FUNCTION__);
		return MV_ERROR;
	}

	pDaaLineInfo = (MV_DAA_LINE_INFO*)(daaLineInfo + first);

	mvOsPrintf("Start initialization for DAA modules %u to %u...",first, last);

	for(lineId = first; lineId < (last+1); lineId++)
	{
		pDaaLineInfo->lineId = lineId;
		pDaaLineInfo->hook = 0;
		pDaaLineInfo->ring = 0;
		pDaaLineInfo->reverse_polarity = 0;
		pDaaLineInfo->drop_out = 0;
	
		/* Software reset */
		mvDaaDirectRegWrite(lineId, 1, 0x80);
	
		/* Wait just a bit */	
		mvOsDelay(100);

		mvDaaDigitalHybridSet(COUNTRY_INDEX, lineId);

		/* Configure tx/rx sample in DAA */
		pDaaLineInfo->rxSample = pSlotInfo[lineId][0];
		pDaaLineInfo->txSample = pSlotInfo[lineId][0];
		DBG("Rx sample %d, Tx sample %d\n",pDaaLineInfo->rxSample, pDaaLineInfo->txSample);
		pDaaLineInfo->txSample *= 8;
		pDaaLineInfo->rxSample *= 8;

		mvDaaPcmStartCountRegsSet(lineId);

		/* Enable PCM & linear 16-bit/a-Law 8-bit/u-Law 8-bit */
		switch(pcmFormat)
		{
			case MV_PCM_FORMAT_ALAW:
			case MV_PCM_FORMAT_ULAW:
			case MV_PCM_FORMAT_LINEAR:
				reg33 = (0x20 | (pcmFormat << 3));
				mvDaaDirectRegWrite(lineId, 33, reg33);
				break;
			default:
				mvOsPrintf("Error, wrong pcm format(%d)\n", pcmFormat);
				break;
		}
	
		/* Enable full wave rectifier */
		mvDaaDirectRegWrite(lineId, DAA_INTERNATIONAL_CONTROL_3, DAA_RFWE);

		/* Disable interrupts */
		mvDaaIntDisable(lineId);

		/* Set AOUT/INT pin as hardware interrupt */
		reg2 = mvDaaDirectRegRead(lineId, DAA_CONTROL_2_REG);
		mvDaaDirectRegWrite(lineId, DAA_CONTROL_2_REG, (reg2 | DAA_INTE));
	
		/* Enable ISO-Cap */
		mvDaaDirectRegWrite(lineId, DAA_DAA_CONTROL_2, 0);

		/* Wait 2000ms for ISO-cap to come up */
		newDelay = 2000;
		while(newDelay > 0 && !(mvDaaDirectRegRead(lineId, DAA_SYSTEM_AND_LINE_SIDE_REV_REG) & 0xf0))
		{	
			mvOsDelay(100);
			newDelay -= 100;
		}

		lineSideId = mvDaaDirectRegRead(lineId, DAA_SYSTEM_AND_LINE_SIDE_REV_REG);

		if (!(lineSideId & 0xf0))
		{
			mvOsPrintf("Error: DAA did not bring up ISO link properly!\n");
			return 1;
		}

		/* Perform ADC manual calibration */
		mvDaaCalibrate(lineId);

		/* Enable Ring Validation */
		reg24 = mvDaaDirectRegRead(lineId, DAA_RING_VALIDATION_CONTROL_3);

		mvDaaDirectRegWrite(lineId, DAA_RING_VALIDATION_CONTROL_3 , reg24 | DAA_RNGV);

	
		/* Print DAA info */
		mvDaaPrintInfo(lineId);

		MV_TRC_REC("ISO-Cap is now up, line side: %02x rev %02x\n", 
		        	mvDaaDirectRegRead(lineId, 11) >> 4,
		       		( mvDaaDirectRegRead(lineId, 13) >> 2) & 0xf);
	
	
		/* raise tx gain by 7 dB for NEWZEALAND */
		if (!strcmp(daaModes[COUNTRY_INDEX].name, "NEWZEALAND")) 
		{
			MV_TRC_REC("Adjusting gain\n");
			mvDaaDirectRegWrite(lineId, 38, 0x7);
		}
		/* enable interrupts */
		mvDaaIntEnable(lineId);

		/* Goto next DAA module */
		pDaaLineInfo++;
	}

	mvOsPrintf("Done\n");
	MV_TRC_REC("<-%s\n", __FUNCTION__);

	return MV_OK;
}

static void mvDaaIntEnable(unsigned int lineId)
{
	MV_TRC_REC("enable daa-%d interrupts\n", lineId);

	/* first, clear source register */
	mvDaaDirectRegWrite(lineId, DAA_INTERRUPT_SOURCE_REG, 0);
	/* enable interrupts in mask register */
	mvDaaDirectRegWrite(lineId, DAA_INTERRUPT_MASK_REG, (DAA_DODM | DAA_RDTM));
}

static void mvDaaIntDisable(unsigned int lineId)
{
	MV_TRC_REC("disable daa-%d interrupts\n", lineId);
	mvDaaDirectRegWrite(lineId, DAA_INTERRUPT_MASK_REG, 0);
}

static void mvDaaPcmStartCountRegsSet(unsigned int lineId)
{
	MV_DAA_LINE_INFO *pDaaLineInfo = (MV_DAA_LINE_INFO*)(daaLineInfo + lineId);
	mvDaaDirectRegWrite(lineId, 34 , pDaaLineInfo->txSample&0xff);
	mvDaaDirectRegWrite(lineId, 35 , (pDaaLineInfo->txSample>>8)&0x3);
	mvDaaDirectRegWrite(lineId, 36 , pDaaLineInfo->rxSample&0xff);
	mvDaaDirectRegWrite(lineId, 37 , (pDaaLineInfo->rxSample>>8)&0x3);

}

static int mvDaaIntCheck(unsigned int lineId)
{
	unsigned char cause , mask, control, cause_and_mask;
	MV_DAA_LINE_INFO *pDaaLineInfo = (MV_DAA_LINE_INFO*)(daaLineInfo + lineId);

	cause = mvDaaDirectRegRead(lineId, DAA_INTERRUPT_SOURCE_REG);
	mask = mvDaaDirectRegRead(lineId, DAA_INTERRUPT_MASK_REG);
	cause_and_mask = cause & mask;

	/* check ring */
	if(cause_and_mask & DAA_RDTI)
	{
		MV_TRC_REC("Ring Detected\n");	
		pDaaLineInfo->ring = 1;
		mvDaaDirectRegWrite(lineId, DAA_INTERRUPT_MASK_REG, (mask & ~DAA_POLM));
		control = mvDaaDirectRegRead(lineId, DAA_DAA_CONTROL_2);
		mvDaaDirectRegWrite(lineId, DAA_DAA_CONTROL_2, (control & (~DAA_PDL)));	
	}

	/* check parallel phone detection */
	if(cause_and_mask & DAA_DODI)
	{
		MV_TRC_REC("Drop Out Detected\n");
		pDaaLineInfo->drop_out = 1;
		control = mvDaaDirectRegRead(lineId, DAA_DAA_CONTROL_2);
		mvDaaDirectRegWrite(lineId, DAA_DAA_CONTROL_2, (control & (~DAA_PDL)));
	}
	/* check reverse polarity */
	if(cause_and_mask & DAA_POLI)
	{
		MV_TRC_REC("Reverse Polarity Detected\n");
		pDaaLineInfo->reverse_polarity = 1;	
	}

	/* clear interrupts */
	mvDaaDirectRegWrite(lineId, DAA_INTERRUPT_SOURCE_REG, (~cause_and_mask));

	
	if(cause_and_mask)	
		return 1;
	else
		return 0;
}

unsigned int mvDaaIntGet(unsigned short lineId)
{
	unsigned char event = 0;

	if(!mvDaaIntCheck(lineId))
		return 0;

	mvDaaEventGet(&event, lineId);

	return event;
}

void mvDaaPstnStateSet(mv_hook_t hookstate, unsigned short lineId)
{
	MV_DAA_LINE_INFO *pDaaLineInfo = (MV_DAA_LINE_INFO*)(daaLineInfo + lineId);
	unsigned char reg3, reg5;

	if(hookstate == MV_OFF_HOOK)
	{
		/* Set OFFHOOK */
		MV_TRC_REC("DAA goes off-hook\n");
		reg5 = mvDaaDirectRegRead(lineId, DAA_DAA_CONTROL_1);
		mvDaaDirectRegWrite(lineId, DAA_DAA_CONTROL_1,(reg5 | DAA_OH));
		pDaaLineInfo->hook = 1; 	
	}
	else if(hookstate == MV_ON_HOOK)
	{
		MV_TRC_REC("DAA goes on-hook\n");
		/* First disable Drop Out interrupt */
		reg3 = mvDaaDirectRegRead(lineId, DAA_INTERRUPT_MASK_REG);
		mvDaaDirectRegWrite(lineId, DAA_INTERRUPT_MASK_REG, (reg3 & ~DAA_DODM));
		/* Set ONHOOK */
		reg5 = mvDaaDirectRegRead(lineId, DAA_DAA_CONTROL_1);		
		mvDaaDirectRegWrite(lineId, DAA_DAA_CONTROL_1, (reg5 & ~DAA_OH));
		/* Enable Drop Out interrupt */
		mvDaaDirectRegWrite(lineId, DAA_INTERRUPT_MASK_REG, reg3);
		pDaaLineInfo->hook = 0;
	}
	else
		mvOsPrintf("Unknown PSTN state\n");
}

static void mvDaaCalibrate(unsigned int lineId)
{
	unsigned char reg17;

	DBG("Start ADC calibration\n");
	
	/* Set CALD bit */
	reg17 = mvDaaDirectRegRead(lineId, DAA_INTERNATIONAL_CONTROL_2);
	mvDaaDirectRegWrite(lineId, DAA_INTERNATIONAL_CONTROL_2, (reg17 | DAA_CALD));

	/* Set MCAL bit */
	reg17 = mvDaaDirectRegRead(lineId, DAA_INTERNATIONAL_CONTROL_2);
	mvDaaDirectRegWrite(lineId, DAA_INTERNATIONAL_CONTROL_2, (reg17 | DAA_MCAL));

	/* Reset MCAL bit - start ADC calibration */
	reg17 = mvDaaDirectRegRead(lineId, DAA_INTERNATIONAL_CONTROL_2);
	mvDaaDirectRegWrite(lineId, DAA_INTERNATIONAL_CONTROL_2, (reg17 & ~DAA_MCAL));

	/* Must wait at least 256 ms */
	mvOsDelay(256);

	DBG("ADC calibration completed\n");
}

void mvDaaCidStateSet(cid_state_t cid_state, unsigned short lineId)
{
	unsigned char reg5;

	if(cid_state == CID_ON)
	{
		/* Enable on-hook line monitor */
		mvDaaDirectRegWrite(lineId, DAA_DAA_CONTROL_1, DAA_ONHM);
	}
	else if(cid_state == CID_OFF)
	{
		reg5 = mvDaaDirectRegRead(lineId, DAA_DAA_CONTROL_1);
		mvDaaDirectRegWrite(lineId, DAA_DAA_CONTROL_1,(reg5 & ~DAA_ONHM));	
	}
	else
		mvOsPrintf("%s: Unknown CID state\n",__FUNCTION__);
		
}

void mvDaaReversePolaritySet(long enable, unsigned int lineId)
{
	unsigned char mask;

	if(enable)
	{
		mask = mvDaaDirectRegRead(lineId, DAA_INTERRUPT_MASK_REG);
		mvDaaDirectRegWrite(lineId, DAA_INTERRUPT_MASK_REG, (mask | DAA_POLM));
	}
	else
	{
		mask = mvDaaDirectRegRead(lineId, DAA_INTERRUPT_MASK_REG);
		mvDaaDirectRegWrite(lineId, DAA_INTERRUPT_MASK_REG, (mask & ~DAA_POLM));

	}
	
}

void mvDaaEventGet(unsigned char *eventType, unsigned int lineId)
{	
	MV_DAA_LINE_INFO *pDaaLineInfo = (MV_DAA_LINE_INFO*)(daaLineInfo + lineId);

	if(pDaaLineInfo->ring)
	{
		*eventType |= DAA_RDTI;
		pDaaLineInfo->ring = 0;
	}
	
	if(pDaaLineInfo->reverse_polarity)
	{
		*eventType |= DAA_POLI;
		pDaaLineInfo->reverse_polarity = 0;
	}

	if(pDaaLineInfo->drop_out)
	{
		*eventType |= DAA_DODI;
		pDaaLineInfo->drop_out = 0;
	}

	return;
}


static void mvDaaDigitalHybridSet(unsigned int index, unsigned int lineId)
{
	unsigned char reg16 = 0, reg26 = 0, reg30 = 0, reg31 = 0;
	
	/* Set On-hook speed, Ringer impedence, and ringer threshold */
	reg16 |= (daaModes[index].ohs << 6);
	reg16 |= (daaModes[index].rz << 1);
	reg16 |= (daaModes[index].rt);
	mvDaaDirectRegWrite(lineId, 16, reg16);
	
	/* Set DC Termination:
	   Tip/Ring voltage adjust, minimum operational current, current limitation */
	reg26 |= (daaModes[index].dcv << 6);
	reg26 |= (daaModes[index].mini << 4);
	reg26 |= (daaModes[index].ilim << 1);
	mvDaaDirectRegWrite(lineId, 26, reg26);

	/* Set AC Impedence */
	reg30 = (daaModes[index].acim);
	mvDaaDirectRegWrite(lineId, 30, reg30);

	/* Misc. DAA parameters */
	reg31 = 0xa3;
	reg31 |= (daaModes[index].ohs2 << 3);
	mvDaaDirectRegWrite(lineId, 31, reg31);

}


int mvDaaLineVoltageGet(unsigned int lineId)
{
	return (int)(mvDaaDirectRegRead(lineId, DAA_LINE_VOLTAGE_STATUS));

}

void mvDaaRelease(void)
{
	if(daaLineInfo)
		mvOsFree(daaLineInfo);
}

void mvDaaRegsDump(unsigned int lineId)
{
	int i;
	
	for(i=1; i < NUM_OF_REGS; i++)
	   MV_TRC_REC("Register %d: 0x%x\n",i, mvDaaDirectRegRead(lineId, i));
}

