/*******************************************************************************
Copyright (C) Marvell International Ltd. and its affiliates

This software file (the "File") is owned and distributed by Marvell 
International Ltd. and/or its affiliates ("Marvell") under the following
alternative licensing terms.  Once you have made an election to distribute the
File under one of the following license alternatives, please (i) delete this
introductory statement regarding license alternatives, (ii) delete the two
license alternatives that you have not elected to use and (iii) preserve the
Marvell copyright notice above.


********************************************************************************
Marvell GPL License Option

If you received this File from Marvell, you may opt to use, redistribute and/or 
modify this File in accordance with the terms and conditions of the General 
Public License Version 2, June 1991 (the "GPL License"), a copy of which is 
available along with the File in the license.txt file or by writing to the Free 
Software Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 or 
on the worldwide web at http://www.gnu.org/licenses/gpl.txt. 

THE FILE IS DISTRIBUTED AS-IS, WITHOUT WARRANTY OF ANY KIND, AND THE IMPLIED 
WARRANTIES OF MERCHANTABILITY OR FITNESS FOR A PARTICULAR PURPOSE ARE EXPRESSLY 
DISCLAIMED.  The GPL License provides additional details about this warranty 
disclaimer.
*******************************************************************************/

#include "mvCommon.h"		/* Should be included before mvSysHwConfig */
#include <linux/etherdevice.h>
#include "mvOs.h"
#include "mvSysHwConfig.h"
#include "eth-phy/mvEthPhy.h"
#include "boardEnv/mvBoardEnvLib.h"

#include "msApi.h"
#include "h/platform/gtMiiSmiIf.h"
#include "mv_switch.h"


static GT_QD_DEV qddev;
GT_QD_DEV *qd_dev;
static spinlock_t switch_lock;

static GT_SYS_CONFIG qd_cfg;


static char *mv_str_speed_state(int port)
{
	GT_PORT_SPEED_MODE speed;
	char *speed_str;

	if (gprtGetSpeedMode(qd_dev, port, &speed) != GT_OK) {
		printk(KERN_ERR "gprtGetSpeedMode failed (port %d)\n", port);
		speed_str = "ERR";
	} else {
		if (speed == PORT_SPEED_1000_MBPS)
			speed_str = "1 Gbps";
		else if (speed == PORT_SPEED_100_MBPS)
			speed_str = "100 Mbps";
		else
			speed_str = "10 Mbps";
	}
	return speed_str;
}



static char *mv_str_duplex_state(int port)
{
	GT_BOOL duplex;

	if (gprtGetDuplex(qd_dev, port, &duplex) != GT_OK) {
		printk(KERN_ERR "gprtGetDuplex failed (port %d)\n", port);
		return "ERR";
	} else
		return (duplex) ? "Full" : "Half";
}

static char *mv_str_port_state(GT_PORT_STP_STATE state)
{
	switch (state) {
	case GT_PORT_DISABLE:
		return "Disable";
	case GT_PORT_BLOCKING:
		return "Blocking";
	case GT_PORT_LEARNING:
		return "Learning";
	case GT_PORT_FORWARDING:
		return "Forwarding";
	default:
		return "Invalid";
	}
}


static char *mv_str_link_state(int port)
{
	GT_BOOL link;

	if (gprtGetLinkState(qd_dev, port, &link) != GT_OK) {
		printk(KERN_ERR "gprtGetLinkState failed (port %d)\n", port);
		return "ERR";
	} else
		return (link) ? "Up" : "Down";
}

static char *mv_str_pause_state(int port)
{
	GT_BOOL force, pause;

	if (gpcsGetForcedFC(qd_dev, port, &force) != GT_OK) {
		printk(KERN_ERR "gpcsGetForcedFC failed (port %d)\n", port);
		return "ERR";
	}
	if (force) {
		if (gpcsGetFCValue(qd_dev, port, &pause) != GT_OK)  {
			printk(KERN_ERR "gpcsGetFCValue failed (port %d)\n", port);
			return "ERR";
		}
	} else {
		if (gprtGetPauseEn(qd_dev, port, &pause) != GT_OK) {
			printk(KERN_ERR "gprtGetPauseEn failed (port %d)\n", port);
			return "ERR";
		}
	}
	return (pause) ? "Enable" : "Disable";
}


static char *mv_str_egress_mode(GT_EGRESS_MODE mode)
{
	switch (mode) {
	case GT_UNMODIFY_EGRESS:
		return "Unmodify";
	case GT_UNTAGGED_EGRESS:
		return "Untagged";
	case GT_TAGGED_EGRESS:
		return "Tagged";
	case GT_ADD_TAG:
		return "Add Tag";
	default:
		return "Invalid";
	}
}

static char *mv_str_frame_mode(GT_FRAME_MODE mode)
{
	switch (mode) {
	case GT_FRAME_MODE_NORMAL:
		return "Normal";
	case GT_FRAME_MODE_DSA:
		return "DSA";
	case GT_FRAME_MODE_PROVIDER:
		return "Provider";
	case GT_FRAME_MODE_ETHER_TYPE_DSA:
		return "EtherType DSA";
	default:
		return "Invalid";
	}
}

static char *mv_str_header_mode(GT_BOOL mode)
{
	switch (mode) {
	case GT_FALSE:
		return "False";
	case GT_TRUE:
		return "True";
	default:
		return "Invalid";
	}
}

int mv_switch_get_free_buffers_num(void)
{
	MV_U16 regVal;

	if (gsysGetFreeQSize(qd_dev, &regVal) != GT_OK) {
		printk(KERN_ERR "gsysGetFreeQSize - FAILED\n");
		return -1;
	}

	return regVal;
}


#define QD_FMT "%10lu %10lu %10lu %10lu %10lu %10lu %10lu\n"
#define QD_CNT(c, f) (GT_U32)c[0].f, (GT_U32)c[1].f, (GT_U32)c[2].f, (GT_U32)c[3].f, (GT_U32)c[4].f, (GT_U32)c[5].f, (GT_U32)c[6].f
#define QD_MAX 7

void mv_switch_stats_print(void)
{
	GT_STATS_COUNTER_SET3 counters[QD_MAX];
	GT_PORT_STAT2 port_stats[QD_MAX];
	int p;

	if (qd_dev == NULL) {
		printk(KERN_ERR "Switch is not initialized\n");
		return;
	}
	memset(counters, 0, sizeof(GT_STATS_COUNTER_SET3) * QD_MAX);

	printk(KERN_ERR "Total free buffers:      %u\n\n", mv_switch_get_free_buffers_num());

	for (p = 0; p < QD_MAX; p++) {
		if (gstatsGetPortAllCounters3(qd_dev, p, &counters[p]) != GT_OK)
			printk(KERN_ERR "gstatsGetPortAllCounters3 for port #%d - FAILED\n", p);

		if (gprtGetPortCtr2(qd_dev, p, &port_stats[p]) != GT_OK)
			printk(KERN_ERR "gprtGetPortCtr2 for port #%d - FAILED\n", p);
	}

	printk(KERN_ERR "PortNum         " QD_FMT, (GT_U32) 0, (GT_U32) 1, (GT_U32) 2, (GT_U32) 3, (GT_U32) 4, (GT_U32) 5,
	       (GT_U32) 6);
	printk(KERN_ERR "-----------------------------------------------------------------------------------------------\n");
	printk(KERN_ERR "InGoodOctetsLo  " QD_FMT, QD_CNT(counters, InGoodOctetsLo));
	printk(KERN_ERR "InGoodOctetsHi  " QD_FMT, QD_CNT(counters, InGoodOctetsHi));
	printk(KERN_ERR "InBadOctets     " QD_FMT, QD_CNT(counters, InBadOctets));
	printk(KERN_ERR "InUnicasts      " QD_FMT, QD_CNT(counters, InUnicasts));
	printk(KERN_ERR "InBroadcasts    " QD_FMT, QD_CNT(counters, InBroadcasts));
	printk(KERN_ERR "InMulticasts    " QD_FMT, QD_CNT(counters, InMulticasts));
	printk(KERN_ERR "inDiscardLo     " QD_FMT, QD_CNT(port_stats, inDiscardLo));
	printk(KERN_ERR "inDiscardHi     " QD_FMT, QD_CNT(port_stats, inDiscardHi));
	printk(KERN_ERR "InFiltered      " QD_FMT, QD_CNT(port_stats, inFiltered));

	printk(KERN_ERR "OutOctetsLo     " QD_FMT, QD_CNT(counters, OutOctetsLo));
	printk(KERN_ERR "OutOctetsHi     " QD_FMT, QD_CNT(counters, OutOctetsHi));
	printk(KERN_ERR "OutUnicasts     " QD_FMT, QD_CNT(counters, OutUnicasts));
	printk(KERN_ERR "OutMulticasts   " QD_FMT, QD_CNT(counters, OutMulticasts));
	printk(KERN_ERR "OutBroadcasts   " QD_FMT, QD_CNT(counters, OutBroadcasts));
	printk(KERN_ERR "OutFiltered     " QD_FMT, QD_CNT(port_stats, outFiltered));

	printk(KERN_ERR "OutPause        " QD_FMT, QD_CNT(counters, OutPause));
	printk(KERN_ERR "InPause         " QD_FMT, QD_CNT(counters, InPause));

	printk(KERN_ERR "Octets64        " QD_FMT, QD_CNT(counters, Octets64));
	printk(KERN_ERR "Octets127       " QD_FMT, QD_CNT(counters, Octets127));
	printk(KERN_ERR "Octets255       " QD_FMT, QD_CNT(counters, Octets255));
	printk(KERN_ERR "Octets511       " QD_FMT, QD_CNT(counters, Octets511));
	printk(KERN_ERR "Octets1023      " QD_FMT, QD_CNT(counters, Octets1023));
	printk(KERN_ERR "OctetsMax       " QD_FMT, QD_CNT(counters, OctetsMax));

	printk(KERN_ERR "Excessive       " QD_FMT, QD_CNT(counters, Excessive));
	printk(KERN_ERR "Single          " QD_FMT, QD_CNT(counters, Single));
	printk(KERN_ERR "Multiple        " QD_FMT, QD_CNT(counters, InPause));
	printk(KERN_ERR "Undersize       " QD_FMT, QD_CNT(counters, Undersize));
	printk(KERN_ERR "Fragments       " QD_FMT, QD_CNT(counters, Fragments));
	printk(KERN_ERR "Oversize        " QD_FMT, QD_CNT(counters, Oversize));
	printk(KERN_ERR "Jabber          " QD_FMT, QD_CNT(counters, Jabber));
	printk(KERN_ERR "InMACRcvErr     " QD_FMT, QD_CNT(counters, InMACRcvErr));
	printk(KERN_ERR "InFCSErr        " QD_FMT, QD_CNT(counters, InFCSErr));
	printk(KERN_ERR "Collisions      " QD_FMT, QD_CNT(counters, Collisions));
	printk(KERN_ERR "Late            " QD_FMT, QD_CNT(counters, Late));
	printk(KERN_ERR "OutFCSErr       " QD_FMT, QD_CNT(counters, OutFCSErr));
	printk(KERN_ERR "Deferred        " QD_FMT, QD_CNT(counters, Deferred));

	gstatsFlushAll(qd_dev);
}


void mv_switch_status_print(void)
{
	
	int p;
	GT_PORT_STP_STATE port_state = -1;
	GT_EGRESS_MODE egress_mode = -1;
	GT_FRAME_MODE frame_mode = -1;
	GT_BOOL header_mode = -1;

	if (qd_dev == NULL) {
		printk(KERN_ERR "Switch is not initialized\n");
		return;
	}
	printk(KERN_ERR "Printing Switch Status:\n");

	printk(KERN_ERR "Port   State     Link   Duplex   Speed    Pause     Egress     Frame    Header\n");
	for (p = 0; p < qd_dev->numOfPorts; p++) {

		if (gstpGetPortState(qd_dev, p, &port_state) != GT_OK)
			printk(KERN_ERR "gstpGetPortState failed\n");

		if (gprtGetEgressMode(qd_dev, p, &egress_mode) != GT_OK)
			printk(KERN_ERR "gprtGetEgressMode failed\n");

		if (gprtGetFrameMode(qd_dev, p, &frame_mode) != GT_OK)
			printk(KERN_ERR "gprtGetFrameMode failed\n");

		if (gprtGetHeaderMode(qd_dev, p, &header_mode) != GT_OK)
			printk(KERN_ERR "gprtGetHeaderMode failed\n");

		printk(KERN_ERR "%2d, %10s,  %4s,  %4s,  %8s,  %7s,  %s,  %s,  %s\n",
		       p, mv_str_port_state(port_state), mv_str_link_state(p),
		       mv_str_duplex_state(p), mv_str_speed_state(p), mv_str_pause_state(p),
		       mv_str_egress_mode(egress_mode), mv_str_frame_mode(frame_mode), mv_str_header_mode(header_mode));
	}

}


int mv_switch_reg_read(int port, int reg, int type, MV_U16 *value)
{
	GT_STATUS status;
	if (qd_dev == NULL) {
		printk(KERN_ERR "Switch is not initialized\n");
		return 1;
	}

	switch (type) {
	case MV_SWITCH_PHY_ACCESS:
		status = gprtGetPhyReg(qd_dev, port, reg, value);
		break;

	case MV_SWITCH_PORT_ACCESS:
		status = gprtGetSwitchReg(qd_dev, port, reg, value);
		break;

	case MV_SWITCH_GLOBAL_ACCESS:
		status = gprtGetGlobalReg(qd_dev, reg, value);
		break;

	case MV_SWITCH_GLOBAL2_ACCESS:
		status = gprtGetGlobal2Reg(qd_dev, reg, value);
		break;

	case MV_SWITCH_SMI_ACCESS:
		/* port means phyAddr */
		status = miiSmiIfReadRegister(qd_dev, port, reg, value);
		break;

	default:
		printk(KERN_ERR "%s Failed: Unexpected access type %d\n", __func__, type);
		return 1;
	}
	if (status != GT_OK) {
		printk(KERN_ERR "%s Failed: status = %d\n", __func__, status);
		return 2;
	}
	return 0;
}

int mv_switch_reg_write(int port, int reg, int type, MV_U16 value)
{
	GT_STATUS status;
	if (qd_dev == NULL) {
		printk(KERN_ERR "Switch is not initialized\n");
		return 1;
	}

	switch (type) {
	case MV_SWITCH_PHY_ACCESS:
		status = gprtSetPhyReg(qd_dev, port, reg, value);
		break;

	case MV_SWITCH_PORT_ACCESS:
		status = gprtSetSwitchReg(qd_dev, port, reg, value);
		break;

	case MV_SWITCH_GLOBAL_ACCESS:
		status = gprtSetGlobalReg(qd_dev, reg, value);
		break;

	case MV_SWITCH_GLOBAL2_ACCESS:
		status = gprtSetGlobal2Reg(qd_dev, reg, value);
		break;

	case MV_SWITCH_SMI_ACCESS:
		/* port means phyAddr */
		status = miiSmiIfWriteRegister(qd_dev, port, reg, value);
		break;

	default:
		printk(KERN_ERR "%s Failed: Unexpected access type %d\n", __func__, type);
		return 1;
	}
	if (status != GT_OK) {
		printk(KERN_ERR "%s Failed: status = %d\n", __func__, status);
		return 2;
	}
	return 0;
}


GT_BOOL readMiiWrap(GT_QD_DEV* dev, unsigned int portNumber, unsigned int MIIReg, unsigned int* value)
{
    unsigned long   flags;
    unsigned short  tmp;
    MV_STATUS       status;

	spin_lock_irqsave(&switch_lock, flags);
    status = mvEthPhyRegRead(portNumber, MIIReg , &tmp);
	spin_unlock_irqrestore(&switch_lock, flags);

    *value = tmp;

    if (status == MV_OK)
        return GT_TRUE;

    return GT_FALSE;
}


GT_BOOL writeMiiWrap(GT_QD_DEV* dev, unsigned int portNumber, unsigned int MIIReg, unsigned int data)
{
    unsigned long   flags;
    unsigned short  tmp;
    MV_STATUS       status;

	spin_lock_irqsave(&switch_lock, flags);
    tmp = (unsigned short)data;
    status = mvEthPhyRegWrite(portNumber, MIIReg, tmp);

	spin_unlock_irqrestore(&switch_lock, flags);

    if (status == MV_OK)
        return GT_TRUE;

    return GT_FALSE;
} 



int mv_switch_load(unsigned int port)
{                              
	printk(KERN_ERR "  o Loading Switch QuarterDeck driver\n");

	if (qd_dev) {
		printk(KERN_ERR "    o %s: Already initialized\n", __func__);
		return 0;
	}

	memset((char *)&qd_cfg, 0, sizeof(GT_SYS_CONFIG));
	spin_lock_init(&switch_lock);

	/* init config structure for qd package */ 
	qd_cfg.BSPFunctions.readMii   = readMiiWrap;
	qd_cfg.BSPFunctions.writeMii  = writeMiiWrap;
	qd_cfg.BSPFunctions.semCreate = NULL;
	qd_cfg.BSPFunctions.semDelete = NULL;
	qd_cfg.BSPFunctions.semTake   = NULL;
	qd_cfg.BSPFunctions.semGive   = NULL;
	qd_cfg.initPorts = GT_TRUE;
	qd_cfg.cpuPortNum = mvBoardSwitchCpuPortGet(port);
    if (mvBoardSmiScanModeGet(port) == 1) {
        qd_cfg.mode.baseAddr = 0;
        qd_cfg.mode.scanMode = SMI_MANUAL_MODE;
    }
    else if (mvBoardSmiScanModeGet(port) == 2) {
        qd_cfg.mode.baseAddr = mvBoardPhyAddrGet(port); /* was 0xA; */
        qd_cfg.mode.scanMode = SMI_MULTI_ADDR_MODE;
    }
	
	/* load switch sw package */ 
	if (qdLoadDriver(&qd_cfg, &qddev) != GT_OK) {
		printk(KERN_ERR "qdLoadDriver failed (mv_switch_load)\n");
		return -1;
	}
	printk("qdLoadDriver OK in %s\n",__func__);
	qd_dev = &qddev;
	return 0;	
}
