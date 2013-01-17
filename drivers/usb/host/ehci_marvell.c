/*
 * Copyright (c) 2000-2002 by Dima Epshtein
 * 
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 * or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
 * for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software Foundation,
 * Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

/* #define DRIVER_AUTHOR "Dima Epshtein" */


#ifdef CONFIG_USB_DEBUG
    #define DEBUG
#else
    #undef DEBUG
#endif

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/platform_device.h>

#include "ehci.h"

/* Special bits in Port status regsiter */
#define PORT_SPEED_OFFS		26
#define PORT_SPEED_MASK		0x3
#define PORT_SPEED_FULL		0
#define PORT_SPEED_LOW		1
#define PORT_SPEED_HIGH		2

static int ehci_marvell_setup(struct usb_hcd *hcd);


void 	ehci_marvell_port_status_changed(struct ehci_hcd *ehci)
{
	/* GL USB-19:USB Configuration for LS Eye Pattern Test - DD and KW */
#if defined(CONFIG_ARCH_FEROCEON_KW) || defined(CONFIG_ARCH_FEROCEON_MV78XX0)
	u32 __iomem 	*reg_ptr;
	u32 		port_status, phy_val;

	/* Change PHY Tx Control Register: offset 0x420 = 0x140 + 0x2e0 */
    	reg_ptr = (u32 __iomem *)(((u8 __iomem *)ehci->regs) + 0x2e0);
    	phy_val = ehci_readl(ehci, reg_ptr);

	port_status = ehci_readl(ehci, &ehci->regs->port_status[0]); 	
	if( (port_status & PORT_CONNECT) && 
            (((port_status >> PORT_SPEED_OFFS) & PORT_SPEED_MASK) == PORT_SPEED_LOW) )
	{
		/* If Low speed device is connected - disable autocalibration */
		/* bits[27-30] = 0, bit[26] = 1 */
		phy_val &= ~(0xF << 27);
		phy_val |= (1 << 26);	
	}
	else
	{
		/* Nothing connected or FS/HS devices connected - enable autocalibration */
		phy_val &= ~(1 << 26);
	}
	ehci_writel(ehci, phy_val, reg_ptr);
#endif /* CONFIG_ARCH_FEROCEON_KW || CONFIG_ARCH_FEROCEON_MV78XX0 */
}

static const struct hc_driver ehci_marvell_hc_driver = {
        .description = hcd_name,
        .product_desc = "Marvell Orion EHCI",
        .hcd_priv_size = sizeof(struct ehci_hcd),

        /*
         * generic hardware linkage
         */
        .irq = ehci_irq,
        .flags = HCD_USB2,

        /*
         * basic lifecycle operations
         */
        .reset = ehci_marvell_setup,
        .start = ehci_run,
#ifdef CONFIG_PM
        .bus_suspend = ehci_bus_suspend,
        .bus_resume = ehci_bus_resume,
#endif
        .stop = ehci_stop,
        .shutdown = ehci_shutdown,

        /*
         * managing i/o requests and associated device resources
         */
        .urb_enqueue = ehci_urb_enqueue,
        .urb_dequeue = ehci_urb_dequeue,
        .endpoint_disable = ehci_endpoint_disable,
		.endpoint_reset = ehci_endpoint_reset,

        /*
         * scheduling support
         */
        .get_frame_number = ehci_get_frame,

        /*
         * root hub support
         */
        .hub_status_data = ehci_hub_status_data,
        .hub_control = ehci_hub_control,
        .bus_suspend = ehci_bus_suspend,
        .bus_resume = ehci_bus_resume,
		.clear_tt_buffer_complete = ehci_clear_tt_buffer_complete,
};

static int ehci_marvell_setup(struct usb_hcd *hcd)
{
        struct ehci_hcd *ehci = hcd_to_ehci(hcd);
        int retval;

        /*
         * registers start at offset
         */
        ehci->caps = hcd->regs;
        ehci->regs = hcd->regs +
                HC_LENGTH(*ehci, ehci_readl(ehci, &ehci->caps->hc_capbase));

        /*
         * cache this readonly data; minimize chip reads
         */
        ehci->hcs_params = ehci_readl(ehci, &ehci->caps->hcs_params);

        retval = ehci_halt(ehci);
        if (retval)
                return retval;

        /*
         * data structure init
         */
        retval = ehci_init(hcd);
        if (retval)
                return retval;

	hcd->has_tt = 1;

        ehci->sbrn = 0x20;

        ehci_reset(ehci);

        ehci_port_power(ehci, 0);

        return retval;
}

static int ehci_marvell_probe(struct platform_device *pdev)
{ 
    int                     i, retval; 
    struct usb_hcd          *hcd = NULL; 
 
    hcd = usb_create_hcd (&ehci_marvell_hc_driver, &pdev->dev, dev_name(&pdev->dev));
    if (hcd == NULL) 
    { 
        printk("%s: hcd_alloc failed\n", __FUNCTION__); 
        return -ENOMEM; 
    } 
 
    for(i=0; i<pdev->num_resources; i++)
    {
        if(pdev->resource[i].flags == IORESOURCE_IRQ)
        {
            hcd->irq = pdev->resource[i].start; 
        }
        else if(pdev->resource[i].flags == IORESOURCE_DMA)
        {
            hcd->regs = (void *)pdev->resource[i].start; 
    	    hcd->rsrc_start = pdev->resource[i].start;
    	    hcd->rsrc_len = pdev->resource[i].end - hcd->rsrc_start + 1;
        }
    }     

    retval = usb_add_hcd (hcd, hcd->irq, IRQF_SHARED);
	if (retval != 0)
    {
        printk("%s: usb_add_hcd failed, retval=0x%x\n", __FUNCTION__, retval); 
        return -ENOMEM; 
    }    
 
    return 0; 
} 

static int ehci_marvell_remove(struct platform_device *pdev)
{ 
    struct usb_hcd *hcd = platform_get_drvdata(pdev);

    printk("USB: ehci_marvell_remove\n"); 
   
    usb_remove_hcd (hcd); 
    usb_put_hcd (hcd);

   return 0;
} 
 
 
static struct platform_driver ehci_marvell_driver =  
{ 
    .driver.name = "ehci_marvell", 
    .probe = ehci_marvell_probe, 
    .remove = ehci_marvell_remove,
    .shutdown = usb_hcd_platform_shutdown, 
};  


