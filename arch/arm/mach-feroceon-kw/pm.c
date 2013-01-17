#include <linux/suspend.h>
#include "device/mvDeviceRegs.h"
#include "eth-phy/mvEthPhy.h"
#include <linux/workqueue.h>
#include <linux/version.h>

#define WOL_PORT_NUM		0
#define MV_SYS_MANAG_INTERRUPT_REG	0xF1072004

extern u8 mvMacAddr[CONFIG_MV_ETH_PORTS_NUM][6];

/* mv_usb_work_struct - workqueue structure using for creating and run work tasks */
static struct work_struct mv_soft_reset_work_struct;

/**
* mv_soft_reset_work_routine
* DESCRIPTION:	soft reset button interrupt  routine
* INPUTS:	@*ignored - pointer to work structure, ignored
* RETURNS:	IRQ_HANDLED
**/
#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,25)
static int mv_soft_reset_work_routine(struct work_struct *ignored)
#else
static void mv_soft_reset_work_routine(struct work_struct *ignored)
#endif
{
	MV_32 ret_val=0;
	MV_8 *argv[] = {"/sbin/reboot", "", NULL};
	/* call reboot from user space */
	ret_val=call_usermodehelper( argv[0], argv, NULL, UMH_WAIT_PROC );
#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,25)
	return ret_val;
#endif
}

/* initialize a work-struct with work task - mv_soft_reset_work_routine */
static DECLARE_WORK(mv_soft_reset_work_struct, mv_soft_reset_work_routine);

/**
* soft_reset_interrupt
* DESCRIPTION:	soft reset button interrupt  routine
* INPUTS:	@irq: irq - ignored
		@dev_id: device id - ignored
* OUTPUTS:	kernel error message
* RETURNS:	IRQ_HANDLED
* NOTES:	pressing on soft reset(sw2 in kw55) button performs several interrupts
**/
static irqreturn_t soft_reset_interrupt(int irq, void *dev_id)
{
	MV_32 ret_val=1;

	/*check if workqueue list is empty*/
	if(list_empty(&mv_soft_reset_work_struct.entry)) {
		/* call schedule work */
		ret_val = schedule_work(&mv_soft_reset_work_struct);
 		if(ret_val == 0 ) {
			mvOsPrintf("ERROR: %d, schedule_work() failed\n",ret_val);
		}
	}
	return IRQ_HANDLED;
}

/**
* mv_init_soft_reset
* DESCRIPTION:	init soft reset functionality for soft reset button interrupt
* INPUTS:	N/A
* OUTPUTS:	kernel error message
* RETURNS:	N/A
**/
void mv_init_soft_reset(void)
{
	/* register interrupt for soft reset button */
	if( request_irq(IRQ_SOFT_RESET_BUTTON, soft_reset_interrupt, IRQF_DISABLED ,"sr_button", NULL) < 0){
		panic("Could not allocate IRQ for soft reset button!");
	}
}

void kw_wol_configure(void)
{
	MV_U16 phyAddr = mvBoardPhyAddrGet(WOL_PORT_NUM);	
	/* go to LED config page */
	mvEthPhyRegWrite(phyAddr, 0x16, 0x3);
	mvOsDelay(10);
	/* go to LED config page */
	mvEthPhyRegWrite(phyAddr, 0x12, 0x4905);
	mvOsDelay(10);
	/* go to LED config page */
	mvEthPhyRegWrite(phyAddr, 0x10, 0x181B);
	mvOsDelay(10);
	/* go to LED config page */
	mvEthPhyRegWrite(phyAddr, 0x16, 0x0);
	mvOsDelay(10);
	/* go to LED config page */
	mvEthPhyRegWrite(phyAddr, 0x0, 0x8000);
	mvOsDelay(10);
	/* go to LED config page */
	mvEthPhyRegWrite(phyAddr, 0x12, 0x80);
	mvOsDelay(10);
	/* go to LED config page */
	mvEthPhyRegWrite(phyAddr, 0x16, 0x11);
	mvOsDelay(10);
	/* go to LED config page */
	mvEthPhyRegWrite(phyAddr, 0x17, ((mvMacAddr[WOL_PORT_NUM][5] << 8) | (mvMacAddr[WOL_PORT_NUM][4])));
	mvOsDelay(10);
	/* go to LED config page */
	mvEthPhyRegWrite(phyAddr, 0x18, ((mvMacAddr[WOL_PORT_NUM][3] << 8) | (mvMacAddr[WOL_PORT_NUM][2])));
	mvOsDelay(10);
	/* go to LED config page */
	mvEthPhyRegWrite(phyAddr, 0x19, ((mvMacAddr[WOL_PORT_NUM][1] << 8) | (mvMacAddr[WOL_PORT_NUM][0])));
	mvOsDelay(10);
	/* go to LED config page */
	mvEthPhyRegWrite(phyAddr, 0x10, 0x4500);	
	mvOsDelay(10);
	/* go to LED config page */
	mvEthPhyRegWrite(phyAddr, 0x16, 0x3);
	mvOsDelay(10);
	/* configure SMI (system management interrupt) register */
	MV_MEMIO_LE32_WRITE(MV_SYS_MANAG_INTERRUPT_REG, 0x02404185);
	mvOsDelay(10);
}

static void mv_enter_standby(void)
{
	/* halt the system */
	kernel_halt();
	/* configure the Eth Phy to be in WoL mode */
	kw_wol_configure();

}

static int mv_pm_valid_standby(suspend_state_t state)
{
	int ret = 0;

	switch (state) {
	/* it is not real suspend to DRAM, since we completly shutdown */
	case PM_SUSPEND_MEM:
		mv_enter_standby();
		break;
	default:
		ret = -EINVAL;
	}

	return ret;
}

static struct platform_suspend_ops mv_pm_ops = {

	.valid		= mv_pm_valid_standby,
	.begin		= NULL,
	.prepare	= NULL,
	.prepare_late	= NULL,
	.enter		= NULL,
	.wake		= NULL,
	.finish		= NULL,
	.end		= NULL,
	.recover	= NULL
};

int __init mv_pm_init(void)
{
	printk(KERN_INFO "Marvell Kirkwood Power Management Initializing\n");

	/* supported for RD 88F6282A only */
	if (mvBoardIdGet() != RD_88F6282A_ID) {
            return 0;
	}
	suspend_set_ops(&mv_pm_ops);
	mv_init_soft_reset();
	return 0;
}

late_initcall(mv_pm_init);
