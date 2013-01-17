
#include <generated/autoconf.h>
#include "mvSysHwConfig.h"
#include "ctrlEnv/sys/mvCpuIf.h"
#include "mvCommon.h"
#include "ctrlEnv/mvCtrlEnvSpec.h"
#include <asm/mach/map.h>

/* for putstr */
/* #include <asm/arch/uncompress.h> */

MV_CPU_DEC_WIN* mv_sys_map(void);
u32 mv_pci_mem_size_get(int ifNum);
u32 mv_pci_io_base_get(int ifNum);
u32 mv_pci_io_size_get(int ifNum);
MV_TARGET mv_pci_io_target_get(int ifNum);
u32 mv_pci_mem_base_get(int ifNum);
#if defined(CONFIG_MV_INCLUDE_CESA)
u32 mv_crypto_base_get(void);
#endif
#if defined(CONFIG_MV_INCLUDE_DEVICE_CS1)
u32 mv_device_cs1_base_get(void);
#endif

#include "mvDB_78XX0.h"
#include "mvRD_78XX0_AMC.h"
#include "mvRD_78XX0_H3C.h"
#include "mvRD_78XX0_MASA.h"
#include "mvDB_632X.h"

#include "sysmap_db78xx0.h"
#include "sysmap_rd78xx0_amc.h"
#include "sysmap_rd78xx0_mASA.h"
#include "sysmap_rd78xx0_H3C.h"
#include "sysmap_db632x.h"


static u32 mv_pci_mem_base[] = 
{
	PCI0_MEM0_BASE,
	PCI1_MEM0_BASE,
	PCI2_MEM0_BASE,
	PCI3_MEM0_BASE,
	PCI4_MEM0_BASE, 
	PCI5_MEM0_BASE, 
	PCI6_MEM0_BASE, 
	PCI7_MEM0_BASE, 
};

static u32 mv_pci_io_base[] = 
{
	PCI0_IO_BASE,
	PCI1_IO_BASE,
	PCI2_IO_BASE,
	PCI3_IO_BASE,
	PCI4_IO_BASE, 
	PCI5_IO_BASE, 
	PCI6_IO_BASE, 
	PCI7_IO_BASE, 
};


static MV_TARGET mv_pci_io_target[] = 
{
	PCI0_IO,
	PCI1_IO,
	PCI2_IO,
	PCI3_IO,
	PCI4_IO, 
	PCI5_IO, 
	PCI6_IO, 
	PCI7_IO, 
};

	

u32 mv_pci_mem_base_get(int ifNum)
{
	if (ifNum < mvCtrlPciIfMaxIfGet())	
		return mv_pci_mem_base[ifNum];
	else
		panic("%s: error, PCI %d does not exist\n", __FUNCTION__, ifNum);
}

u32 mv_pci_mem_size_get(int ifNum)
{
	if (ifNum < mvCtrlPciIfMaxIfGet())	
		return PCIx_MEM0_SIZE;
	else
		panic("%s: error, PCI %d does not exist\n", __FUNCTION__, ifNum);
	return 0;
}

u32 mv_pci_io_base_get(int ifNum)
{
	if (ifNum < mvCtrlPciIfMaxIfGet())	
		return mv_pci_io_base[ifNum];
	else
		panic("%s: error, PCI %d does not exist\n", __FUNCTION__, ifNum);
}

u32 mv_pci_io_size_get(int ifNum)
{
	if (ifNum < mvCtrlPciIfMaxIfGet())	
		return PCIx_IO_SIZE;
	else
		panic("%s: error, PCI %d does not exist\n", __FUNCTION__, ifNum);
	return 0;
}

MV_TARGET mv_pci_io_target_get(int ifNum)
{
	if (ifNum < mvCtrlPciIfMaxIfGet())	
		return mv_pci_io_target[ifNum];
	else
		panic("%s: error, PCI %d does not exist\n", __FUNCTION__, ifNum);
	return 0;
}


int mv_is_pci_io_mapped(int ifNum)
{
	switch(mvBoardIdGet()) {
	case DB_78XX0_ID:
	case DB_78200_ID:
	case DB_632X_ID:
	case RD_78XX0_H3C_ID:
		if (ifNum < mvCtrlPciIfMaxIfGet())	
			return 1;
	default:
		return 0;
	}
}

u32 mv_pci_irqnum_get(int ifNum)
{
	if (ifNum < mvCtrlPciIfMaxIfGet())	
	{
		return IRQ_PEX_INT(ifNum);
	}
	return 0;
}

MV_CPU_DEC_WIN* mv_sys_map(void)
{
	switch(mvBoardIdGet()) {
	case DB_78XX0_ID:		
		if (mvBoardIsBootFromNor32())
			return SYSMAP_DB_78XX0_NOR_32BIT;
		else
			return SYSMAP_DB_78XX0;
	case DB_78200_ID:
		return SYSMAP_DB_78200;
	case DB_632X_ID:
		return SYSMAP_DB_623X;
	case RD_78XX0_AMC_ID:
		return SYSMAP_RD_78XX0_AMC; 
	case RD_78XX0_MASA_ID:
                return SYSMAP_RD_78XX0_MASA;
	case RD_78XX0_H3C_ID:
                return SYSMAP_RD_78XX0_H3C;
	default:
		printk("ERROR: can't find system address map\n");
		return NULL;
        }
}

int mv_pci_if_num_to_skip(void)
{
	switch(mvBoardIdGet()) {
	case DB_78XX0_ID:
	case DB_78200_ID:
		return 2; 
	case RD_78XX0_H3C_ID:
		return 3;
	default:
		return -1;
	}
}


#if defined(CONFIG_MV_INCLUDE_CESA)
u32 mv_crypto_base_get(void)
{
	return CRYPTO_ENG_BASE;
}
EXPORT_SYMBOL(mv_crypto_base_get);
#endif



#if !defined(CONFIG_MV_INTERNAL_REGS_SELECTIVE_MAPPING)
void __init mv_map_io(void)
{
        MV_U32 id = mvBoardIdGet();
	switch(id) {
	case DB_78XX0_ID:
		iotable_init(DB_78XX0_MEM_TABLE, ARRAY_SIZE(DB_78XX0_MEM_TABLE));
		break;
	case DB_78200_ID:
		iotable_init(DB_78200_MEM_TABLE, ARRAY_SIZE(DB_78200_MEM_TABLE));
		break;
	case DB_632X_ID:
		iotable_init(DB_632X_MEM_TABLE, ARRAY_SIZE(DB_632X_MEM_TABLE));
		break;
	case RD_78XX0_AMC_ID:
		iotable_init(RD_78XX0_AMC_MEM_TABLE, ARRAY_SIZE(RD_78XX0_AMC_MEM_TABLE));
                break;
	case RD_78XX0_MASA_ID:
                iotable_init(RD_78XX0_MASA_MEM_TABLE, ARRAY_SIZE(RD_78XX0_MASA_MEM_TABLE));
                break;
	case RD_78XX0_H3C_ID:
                iotable_init(RD_78XX0_H3C_MEM_TABLE, ARRAY_SIZE(RD_78XX0_H3C_MEM_TABLE));                
                break;
	default:
		printk("ERROR: can't find system address map for board id 0x%X\n", id);
		BUG();
        }	
}
#endif


/*	Dynamic configuration accroding to setting in power management register.
	If unit is powered down during the boot (bit is 0 in Power Management Register ), 
	internal register space of this SoC unit is not mapped to kernel virtual memory

	Internal registers address map.
	Unit		offset		size	bit
	=========================================
	DRAM:		0			64K		None
	MPP/Device	0x10000		64K		23
	MBus		0x20000		64K		None
	GbE 1.0		0x32000		16K		3
	GbE 1.1		0x36000		16K		4
	Pex 0.0		0x40000		16K		5
	Pex 0.1		0x44000		16K		6
	Pex 0.2		0x48000		16K		7
	Pex 0.3		0x4C000		16K		8
	USB 0		0x50000		4K		17
	USB 1		0x51000		4K		18
	USB 2		0x52000		4K		19
	XOR/IDMA	0x60000		64K		20 & 21
	GbE 0.0		0x72000		16K		1		
	GbE 0.1		0x76000		16K		2
	Pex 1.0		0x80000		16K		9
	Pex 1.1		0x84000		16K		10
	Pex 1.2		0x88000		16K		11
	Pex 1.3		0x8C000		16K		12
	Crypto		0x90000		64K		22
	SATAHC		0xA0000		8K		None
	SATA0		0xA2000		8K		13 & 14
	SATA1		0xA4000		8K		15 & 16
	TDM			0xB0000		64K		24
	
*/


#if defined(CONFIG_MV_INTERNAL_REGS_SELECTIVE_MAPPING)
/* need to make sure it is big enough to hold all mapping entries */
#define MEM_TABLE_MAX_ENTRIES	30

struct _mv_internal_regs_map {
	MV_UNIT_ID id;
	u32 index;
	u32 offset;
	u32 size;
};

u32 __initdata entries;

/* Internal registers mapping table */
struct _mv_internal_regs_map mv_internal_regs_map[] __initdata = {	
	{CESA_UNIT_ID,    0, MV_CESA_TDMA_REG_BASE,    SZ_64K},
	{PEX_UNIT_ID, 	  0, PEX_IF_BASE0,           SZ_16K},
	{PEX_UNIT_ID, 	  1, PEX_IF_BASE1,           SZ_16K},
	{PEX_UNIT_ID, 	  2, PEX_IF_BASE2,           SZ_16K},
	{PEX_UNIT_ID, 	  3, PEX_IF_BASE3,           SZ_16K},
	{PEX_UNIT_ID, 	  4, PEX_IF_BASE4,           SZ_16K},
	{PEX_UNIT_ID, 	  5, PEX_IF_BASE5,           SZ_16K},
	{PEX_UNIT_ID, 	  6, PEX_IF_BASE6,           SZ_16K},
	{PEX_UNIT_ID, 	  7, PEX_IF_BASE7,           SZ_16K},
	{USB_UNIT_ID,  	  0, USB_REG_BASE0,          SZ_4K},
	{USB_UNIT_ID,  	  1, USB_REG_BASE1,          SZ_4K},
	{USB_UNIT_ID,  	  2, USB_REG_BASE2,          SZ_4K},
	{XOR_UNIT_ID,  	  0, MV_XOR_REG_BASE,        SZ_64K},
	{ETH_GIG_UNIT_ID, 0, MV_ETH_REG_BASE0,       SZ_16K},
	{ETH_GIG_UNIT_ID, 1, MV_ETH_REG_BASE1,       SZ_16K},
	{ETH_GIG_UNIT_ID, 2, MV_ETH_REG_BASE2,       SZ_16K},
	{ETH_GIG_UNIT_ID, 3, MV_ETH_REG_BASE3,       SZ_16K},
	{SATA_UNIT_ID,    0, (SATA_REG_BASE + 0x2000), SZ_8K},
	{SATA_UNIT_ID,    1, (SATA_REG_BASE + 0x4000), SZ_8K},
	{TDM_UNIT_ID,     0, TDM_REG_BASE,             SZ_64K}
};

struct map_desc  MEM_TABLE[MEM_TABLE_MAX_ENTRIES] __initdata  = {
/*Internal register which are always mapped*/
  {(INTER_REGS_BASE + DRAM_BASE),  __phys_to_pfn(INTER_REGS_BASE + DRAM_BASE),  SZ_64K, MT_DEVICE},
  {(INTER_REGS_BASE + DEVICE_BUS_BASE),  __phys_to_pfn(INTER_REGS_BASE + DEVICE_BUS_BASE),  SZ_64K, MT_DEVICE},
  {(INTER_REGS_BASE + SATA_REG_BASE), __phys_to_pfn(INTER_REGS_BASE + SATA_REG_BASE), SZ_8K, MT_DEVICE},
/*Peripherals*/
  { PCI0_MEM0_BASE,  	__phys_to_pfn(PCI0_MEM0_BASE),  PCIx_MEM0_SIZE, MT_DEVICE},
  { PCI0_IO_BASE,   	__phys_to_pfn(PCI0_IO_BASE),   	PCIx_IO_SIZE,  	MT_DEVICE},
  { DEVICE_CS1_BASE, __phys_to_pfn(DEVICE_CS1_BASE), DEVICE_CS1_SIZE, MT_DEVICE },
#if !defined(CONFIG_MV78XX0_Z0)
  { SPI_CS_BASE, __phys_to_pfn(SPI_CS_BASE), SPI_CS_SIZE, MT_DEVICE },
#endif 
   { CRYPTO_ENG_BASE, __phys_to_pfn(CRYPTO_ENG_BASE), CRYPTO_SIZE, MT_DEVICE},
  { 0xFFFFFFFF, 	0xFFFFFFFF, 		      0, MT_DEVICE},  
};

void __init mv_build_map_table(void)
{
	u32 unit;
	/* number of entries to map */	
	for (entries = 0; entries < ARRAY_SIZE(MEM_TABLE); entries++) {
		if (0xFFFFFFFF == MEM_TABLE[entries].virtual)
			break;
	}
	/* prepare consecutive mapping table */
	for(unit = 0; unit < ARRAY_SIZE(mv_internal_regs_map); unit++) {
		
		if (MV_TRUE == mvCtrlPwrClckGet(mv_internal_regs_map[unit].id, mv_internal_regs_map[unit].index)) {
			MEM_TABLE[entries].virtual = (INTER_REGS_BASE + mv_internal_regs_map[unit].offset);
			MEM_TABLE[entries].pfn = __phys_to_pfn(INTER_REGS_BASE + mv_internal_regs_map[unit].offset);
			MEM_TABLE[entries].length = mv_internal_regs_map[unit].size;
			MEM_TABLE[entries].type = MT_DEVICE;
			entries++;
		}
		else
		{
			printk ("unit %d not mapped!\n", unit);
		}
	}
}

void __init mv_map_io(void)
{
	struct map_desc ahb_to_mbus_map = {
		.length = SZ_64K,
		.type = MT_DEVICE
	};
	ahb_to_mbus_map.virtual = INTER_REGS_BASE + CPU_IF_BASE(whoAmI());
	ahb_to_mbus_map.pfn =  __phys_to_pfn(INTER_REGS_BASE + CPU_IF_BASE(whoAmI())),
	/* first, mappping AHB to MBUS entry for mvCtrlPwrClckGet access */
	iotable_init(&ahb_to_mbus_map, 1);

	/* build dynamic mapping table  */
	mv_build_map_table();

	iotable_init(MEM_TABLE, entries);
}

#endif /* CONFIG_MV_INTERNAL_REGS_SELECTIVE_MAPPING */


