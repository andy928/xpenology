/*
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */
#include <linux/kernel.h>
#include <linux/pci.h>
#include <linux/ptrace.h>
#include <linux/slab.h>
#include <linux/ioport.h>
#include <linux/interrupt.h>
#include <linux/spinlock.h>
#include <linux/init.h>
#ifdef CONFIG_SYNO_MV88F6281
#include <linux/synobios.h>
#endif
                                                                                                                             
#include <mach/hardware.h>
#include <asm/io.h>
#include <asm/irq.h>
#include <asm/system.h>
#include <asm/mach/pci.h>
#include <mach/irqs.h>

#include "ctrlEnv/mvCtrlEnvLib.h"
#include "ctrlEnv/sys/mvSysPex.h"
#include "ctrlEnv/sys/mvCpuIf.h"
#include "ctrlEnv/mvCtrlEnvSpec.h"

#undef DEBUG
#ifdef DEBUG
#	define DB(x) x
#else
#	define DB(x) 
#endif

#define PCI_ERR_NAME_LEN 12
#define MV_MAX_PEX_IF_NUMBER 2

static int __init mv_map_irq_0(struct pci_dev *dev, u8 slot, u8 pin);
static int __init mv_map_irq_1(struct pci_dev *dev, u8 slot, u8 pin);

void mv_pci_error_init(u32 pciIf);
static irqreturn_t pex_error_interrupt(int irq, void *dev_id);

#ifdef CONFIG_SYNO_MV88F6281
extern char gszSynoHWVersion[];
#endif

static struct pex_if_error {
	MV_8 irq_name[PCI_ERR_NAME_LEN];
	MV_U32 ifNumber;
} pex_err[MV_MAX_PEX_IF_NUMBER];

void __init mv_pci_preinit(void)
{
	MV_ADDR_WIN win;
	MV_STATUS retval;
	u32 pciIf;
	u32 maxif = mvCtrlPexMaxIfGet();

	/* Check Power State */
	if (MV_FALSE == mvCtrlPwrClckGet(PEX_UNIT_ID, 0))	
		return;

	for (pciIf = 0; pciIf < maxif; pciIf++) 
	{
#ifdef CONFIG_SYNO_MV88F6281
		//this workaround is for 7042, DS212 does not have one, so it is not necessary.
		//212+ attaches USB 3.0 on PCIe 0x1, so it cannot be reset.
		if ( 0 == strncmp(HW_DS212, gszSynoHWVersion, strlen(HW_DS212))) {
			if( ((0 == strncmp(HW_DS212pv10, gszSynoHWVersion, strlen(HW_DS212pv10))) || (0 == strncmp(HW_DS212pv20, gszSynoHWVersion, strlen(HW_DS212pv20)))) && (0 == pciIf) )
				    goto apply_pcie_workaround;
			else 
					goto skip_pcie_workaround;
		} else if ((0 == strncmp(HW_DS112, gszSynoHWVersion, strlen(HW_DS112))) || (0 == strncmp(HW_DS112pv10, gszSynoHWVersion, strlen(HW_DS112pv10)))) {
			goto skip_pcie_workaround;
		}
apply_pcie_workaround:
		if (!(0x1 & MV_REG_READ(PEX_LINK_STATUS_REG(pciIf)))) {
			/*
			* Synology 6281 PCIe link issue workaround.
			* If we don't reset the PCIe link, the device's register will all be 0xffffffff
			* after the PCIe device work for some times.
			*
			* In DS409slim, 7042 is work as PCIe 4X in 6281. Some board will encounter
			* this problem. When the register are all 0xffffffff, the system is just like hang.
			*
			* Disable PCIe link and wait for that link is really down.
			* And then enable it again.
			* Please refer 6281 functional Spec page 447.
			* PCIe Link control status register bit 4.
			*/
			MV_REG_WRITE(PEX_LINK_CONTROL_REG(pciIf), MV_REG_READ(PEX_LINK_CONTROL_REG(pciIf)) | 0x10);
			while(MV_REG_READ(PEX_LINK_STATUS_REG(pciIf)) & 0x1)
				;
			MV_REG_WRITE(PEX_LINK_CONTROL_REG(pciIf), MV_REG_READ(PEX_LINK_CONTROL_REG(pciIf)) & ~0x10);
			
			if (MV_REG_READ(PEX_LINK_STATUS_REG(pciIf)) & 0x1) {
				printk("PCIe link not enable now\n");
			}

			printk("PCIe link is enable, apply PCIe workaround\n");
		}
skip_pcie_workaround:
#endif

		retval = mvPexInit(pciIf, MV_PEX_ROOT_COMPLEX);

		if(retval == MV_NO_SUCH){
			//printk("pci_init:no such calling mvPexInit for PEX-%x\n",pciIf);
			continue;
		}

		if (retval != MV_OK)
		{
			printk("pci_init:Error calling mvPexInit for PEX-%x\n",pciIf);
			continue;
		}

		/* unmask inter A/B/C/D */
		//printk("writing %x tp %x \n",MV_PCI_MASK_ABCD, MV_PCI_MASK_REG(pciIf) );
		MV_REG_WRITE(MV_PCI_MASK_REG(pciIf), MV_PCI_MASK_ABCD );

#ifdef CONFIG_SYNO_MV88F6281
		//This Error handling can cause Kernel boot error if the CPU is not 6282A1, 
		//The BoardID is checked to avoid this error.
		if(MV_6282_A1_ID == mvCtrlModelRevGet())
		{
			/* init PCI express error handling */
			// There has some board cannot boot if we enable this options. So disable it again.
			//mv_pci_error_init(pciIf);
		}

#else
		/* init PCI express error handling */
		mv_pci_error_init(pciIf);
#endif

		/* remmap IO !! */
		win.baseLow = (pciIf? PEX1_IO_BASE : PEX0_IO_BASE) - IO_SPACE_REMAP;
		win.baseHigh = 0x0;
		win.size = pciIf? PEX1_IO_SIZE : PEX0_IO_SIZE;
		mvCpuIfPciIfRemap((pciIf? PCI_IF1_IO : PCI_IF0_IO), &win);
	}
}

/**
* mv_pci_error_init
* DESCRIPTION:	init PCI express error handling
* INPUTS:	pciIf - number of pex device
* OUTPUTS:	N/A
* RETURNS:	N/A
**/
void mv_pci_error_init(u32 pciIf)
{
 	MV_U32      reg_val;

	/* enable PCI express error handling */
	MV_REG_BIT_SET(MV_IRQ_MASK_HIGH_REG, (1 << (IRQ_PEX_ERR(pciIf) - 32)));

	/* init pex_err structure per each pex */
	pex_err[pciIf].ifNumber=pciIf;
	snprintf(pex_err[pciIf].irq_name, PCI_ERR_NAME_LEN, "error_pex%d", pciIf);

	/* register interrupt for PCI express error */
	if( request_irq(IRQ_PEX_ERR(pciIf), pex_error_interrupt, IRQF_DISABLED ,(const char*)pex_err[pciIf].irq_name, &pex_err[pciIf].ifNumber) < 0){
		panic("Could not allocate IRQ for PCI express error!");
	}

	/* init PCI Express Interrupt Mask Register */

	/* get current value of Interrupt Mask Register */
	reg_val = MV_REG_READ(MV_PCI_MASK_REG(pciIf));

	/* set relevant mask to Interrupt Mask Register */
	MV_REG_WRITE(MV_PCI_MASK_REG(pciIf), (reg_val | MV_PCI_MASK_ERR));
}

/* Currentlly the PCI config read/write are implemented as read modify write
   to 32 bit.
   TBD: adjust it to realy use 1/2/4 byte(partial) read/write, after the pex
	read config WA will be removed.
*/
static int mv_pci_read_config(struct pci_bus *bus, unsigned int devfn, int where,
                          int size, u32 *val)
{

        MV_U32 bus_num,func,regOff,dev_no,temp;
	MV_U32 localBus;
	struct pci_sys_data *sysdata = (struct pci_sys_data *)bus->sysdata;     
        u32 pciIf = sysdata->mv_controller_num; 
 
	*val = 0xffffffff;

	/* Check Power State */
	if (MV_FALSE == mvCtrlPwrClckGet(PEX_UNIT_ID, 0))
		return 0;

        bus_num = bus->number;
        dev_no = PCI_SLOT(devfn);
 
	/* don't return for our device */
	localBus = mvPexLocalBusNumGet(pciIf);
	if((dev_no == 0) && ( bus_num == localBus))
	{
		DB(printk("PCI %x read from our own dev return 0xffffffff \n", pciIf));
		return 0xffffffff;
	}

        func = PCI_FUNC(devfn); 
        regOff = (MV_U32)where & PXCAR_REG_NUM_MASK;

#if PCI0_IF_PTP
	/* WA: use only the first function of the bridge and te first bus*/
	if( (bus_num == mvPexLocalBusNumGet(pciIf)) && (dev_no == 1) && (func != 0) )
	{
		DB(printk("PCI %x read from bridge func != 0 return 0xffffffff \n",pciIf));
		return 0xffffffff;
	}
#endif
	if ((func == 0)&&(dev_no < 2))
	{
		DB(printk("PCI %x read: bus = %x dev = %x func = %x regOff = %x ",pciIf, bus_num,dev_no,func,regOff));
	}
	

        temp = (u32) mvPexConfigRead(pciIf, bus_num, dev_no, func, regOff);

        switch (size) {
        case 1:
                temp = (temp >>  (8*(where & 0x3))) & 0xff;
                break;
 
        case 2:
                temp = (temp >>  (8*(where & 0x2))) & 0xffff;
                break;
 
        default:
                break;
        }
	
	
	*val = temp;

	if ((func == 0)&&(dev_no < 2))
	{
		DB(printk(" got %x \n",temp));
	}
	
        return 0;
}

static int mv_pci_write_config(struct pci_bus *bus, unsigned int devfn, int where,
                           int size, u32 val)
{
        MV_U32 bus_num,func,regOff,dev_no,temp, mask , shift;
        struct pci_sys_data *sysdata = (struct pci_sys_data *)bus->sysdata;     
        u32 pciIf = sysdata->mv_controller_num;          
 
	bus_num = bus->number;
	dev_no = PCI_SLOT(devfn); 
	func = PCI_FUNC(devfn); 
	regOff = (MV_U32)where & PXCAR_REG_NUM_MASK;

	DB(printk("PCI %x: writing data %x size %x to bus %x dev %x func %x offs %x \n",pciIf, val,size,bus_num,dev_no,func,regOff));
	if( size != 4)
	{
        	temp = (u32) mvPexConfigRead(pciIf, bus_num, dev_no, func, regOff);
	}
	else
	{
		temp = val;
	}

        switch (size) {
        case 1:
		shift = (8*(where & 0x3));
		mask = 0xff;
                break;
 
        case 2:
		shift = (8*(where & 0x2));
                mask = 0xffff; 
                break;
 
        default:
		shift = 0;
		mask = 0xffffffff;
                break;
        }
	
	temp = (temp & (~(mask<<shift))) | ((val & mask) << shift);
	mvPexConfigWrite(pciIf,bus_num,dev_no,func,regOff,temp);

        return 0;

}




static struct pci_ops mv_pci_ops = {
        .read   = mv_pci_read_config,
        .write  = mv_pci_write_config,
};


int __init mv_pci_setup(int nr, struct pci_sys_data *sys)
{
        struct resource *res;

        switch (nr) {
        case 0:
                sys->map_irq = mv_map_irq_0;
                break;
        case 1:
                sys->map_irq = mv_map_irq_1;
                break;
        default:
		printk("mv_pci_setup: nr(%d) out of scope\n",nr);
                return 0;
        }

	res = kmalloc(sizeof(struct resource) * 2, GFP_KERNEL);
        if (!res)
                panic("PCI: unable to alloc resources");
                                                                                                                             
        memset(res, 0, sizeof(struct resource) * 2);
                                                                                                                             
	if(!nr) {                                                                                                                             
        res[0].start = PEX0_IO_BASE - IO_SPACE_REMAP;
        res[0].end   = PEX0_IO_BASE - IO_SPACE_REMAP + PEX0_IO_SIZE - 1;
        	res[0].name  = "PEX0 IO";
        res[0].flags = IORESOURCE_IO;
                                                                                                                             
        res[1].start = PEX0_MEM_BASE;
        res[1].end   = PEX0_MEM_BASE + PEX0_MEM_SIZE - 1;
        	res[1].name  = "PEX0 Memory";
        res[1].flags = IORESOURCE_MEM;
	} else {
        	res[0].start = PEX1_IO_BASE - IO_SPACE_REMAP;
        	res[0].end   = PEX1_IO_BASE - IO_SPACE_REMAP + PEX1_IO_SIZE - 1;
        	res[0].name  = "PEX1 IO";
        	res[0].flags = IORESOURCE_IO;
                                                                                                                             
        	res[1].start = PEX1_MEM_BASE;
        	res[1].end   = PEX1_MEM_BASE + PEX1_MEM_SIZE - 1;
        	res[1].name  = "PEX1 Memory";
        	res[1].flags = IORESOURCE_MEM;
	}
     
        if (request_resource(&ioport_resource, &res[0]))
	{	
		printk ("IO Request resource failed - Pci If %x\n",nr);
	}
	if (request_resource(&iomem_resource, &res[1]))
	{	
		printk ("Memory Request resource failed - Pci If %x\n",nr);
	}
 
        sys->resource[0] = &res[0];
        sys->resource[1] = &res[1];
        sys->resource[2] = NULL;
        sys->io_offset   = 0x0;
	sys->mv_controller_num = nr;
 
        return 1;

}

struct pci_bus *mv_pci_scan_bus(int nr, struct pci_sys_data *sys)
{
	struct pci_ops *ops = &mv_pci_ops;
	struct pci_bus *bus;

	bus = pci_scan_bus(sys->busnr, ops, sys);

	if(sys->mv_controller_num == 0)
        	mvPexLocalBusNumSet(1, 
                	bus->number + bus->subordinate - bus->secondary + 1); 
 
	return bus;
}

/**
* pex_error_interrupt
* DESCRIPTION: PCI express error interrupt  routine
* INPUTS:  @irq: irq number
       @dev_id: device id - ignored
* OUTPUTS: kernel error message
* RETURNS: IRQ_HANDLED
**/
static irqreturn_t pex_error_interrupt(int irq, void *dev_id)
{
   MV_U32  reg_val;
   MV_U32 ifPexNumber=*(MV_U32 *)dev_id;

   /* get current value of Interrupt Cause Register */
   reg_val = MV_REG_READ(MV_PCI_IRQ_CAUSE_REG(ifPexNumber));
   printk(KERN_ERR "PCI express error: irq - %d, Pex number: %d, Interrupt Cause Register va  lue: %x  \n", irq, ifPexNumber, reg_val);

   return IRQ_HANDLED;
}

static int __init mv_map_irq_0(struct pci_dev *dev, u8 slot, u8 pin)
{
	return IRQ_PEX0_INT;
}

static int __init mv_map_irq_1(struct pci_dev *dev, u8 slot, u8 pin)
{
        return IRQ_PEX1_INT;
}


static struct hw_pci mv_pci __initdata = {
	.swizzle        	= pci_std_swizzle,
        .map_irq                = mv_map_irq_0,
        .setup                  = mv_pci_setup,
        .scan                   = mv_pci_scan_bus,
        .preinit                = mv_pci_preinit,
};
 
static int __init mv_pci_init(void)
{
#if defined(MV_INCLUDE_CLK_PWR_CNTRL)
	 /*Check pex power state */
	MV_U32 pexPower;
	pexPower = mvCtrlPwrClckGet(PEX_UNIT_ID,0);
	if (pexPower == MV_FALSE)
	{
		printk("\nWarning PCI-E is Powered Off\n");
		return 0;
	}
#endif

    mv_pci.nr_controllers = mvCtrlPexMaxIfGet(); 
    pci_common_init(&mv_pci);

    return 0;
}


subsys_initcall(mv_pci_init);

