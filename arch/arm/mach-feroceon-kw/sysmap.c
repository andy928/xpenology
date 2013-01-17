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


#include "mvSysHwConfig.h"
#include "ctrlEnv/sys/mvCpuIf.h"
#include <asm/mach/map.h>

/* for putstr */
/* #include <asm/arch/uncompress.h> */

MV_CPU_DEC_WIN* mv_sys_map(void);

#if defined(CONFIG_MV_INCLUDE_CESA)
u32 mv_crypto_base_get(void);
#endif

#if defined(CONFIG_MV_INTERNAL_REGS_SELECTIVE_MAPPING)

/* need to make sure it is big enough to hold all mapping entries */
#define MEM_TABLE_MAX_ENTRIES	30

/* default mapped entries */
#define MEM_TABLE_ENTRIES	7

/* number of entries to map */
volatile u32 entries = MEM_TABLE_ENTRIES;

struct _mv_internal_regs_map {
	MV_UNIT_ID id;
	u32 index;
	u32 offset;
	u32 size;
};

/* Internal registers mapping table */
struct _mv_internal_regs_map mv_internal_regs_map[] = {
	{DRAM_UNIT_ID,    0, DRAM_BASE,                SZ_64K},
	{CESA_UNIT_ID,    0, MV_CESA_TDMA_REG_BASE,    SZ_64K},
	{USB_UNIT_ID,  	  0, USB_REG_BASE(0),          SZ_64K},
	{XOR_UNIT_ID,  	  0, MV_XOR_REG_BASE,          SZ_64K},
	{ETH_GIG_UNIT_ID, 0, MV_ETH_REG_BASE(0),       SZ_8K},  /* GbE port0 registers */
	{ETH_GIG_UNIT_ID, 1, MV_ETH_REG_BASE(1),       SZ_8K},  /* GbE port1 registers */
	{SATA_UNIT_ID,    0, (SATA_REG_BASE + 0x2000), SZ_8K }, /* SATA port0 registers */
	{SATA_UNIT_ID,    1, (SATA_REG_BASE + 0x4000), SZ_8K }, /* SATA port1 registers */
	{SDIO_UNIT_ID,    0, MV_SDIO_REG_BASE,         SZ_64K},
	{AUDIO_UNIT_ID,   0, AUDIO_REG_BASE(0),        SZ_64K},
	{TS_UNIT_ID,      0, TSU_GLOBAL_REG_BASE,      SZ_16K},
	{TDM_UNIT_ID,     0, TDM_REG_BASE,             SZ_64K}
};

/* AHB to MBUS mapping entry */
struct map_desc AHB_TO_MBUS_MAP[] = {
  {(INTER_REGS_BASE + MAX_AHB_TO_MBUS_REG_BASE), __phys_to_pfn(INTER_REGS_BASE + MAX_AHB_TO_MBUS_REG_BASE),
 		 SZ_64K, MT_DEVICE},
  };

/* WARNING: update of this table requires updating MEM_TABLE_ENTRIES */
struct map_desc  MEM_TABLE[MEM_TABLE_MAX_ENTRIES] = {
  {(INTER_REGS_BASE + MPP_REG_BASE),  __phys_to_pfn(INTER_REGS_BASE + MPP_REG_BASE),  SZ_64K,	      MT_DEVICE},
  {(INTER_REGS_BASE + SATA_REG_BASE), __phys_to_pfn(INTER_REGS_BASE + SATA_REG_BASE), SZ_8K,	      MT_DEVICE},
  {(INTER_REGS_BASE + PEX_IF_BASE(0)),__phys_to_pfn(INTER_REGS_BASE + PEX_IF_BASE(0)),SZ_64K,         MT_DEVICE},
  { PEX0_IO_BASE,   		      __phys_to_pfn(PEX0_IO_BASE),   	 	      PEX0_IO_SIZE,   MT_DEVICE},
  { NFLASH_CS_BASE, 		      __phys_to_pfn(NFLASH_CS_BASE), 	              NFLASH_CS_SIZE, MT_DEVICE},
  { SPI_CS_BASE, 		      __phys_to_pfn(SPI_CS_BASE), 	              SPI_CS_SIZE,    MT_DEVICE},
  { CRYPT_ENG_BASE, 		      __phys_to_pfn(CRYPT_ENG_BASE), 		      CRYPT_ENG_SIZE, MT_DEVICE},
};

#else
struct map_desc  MEM_TABLE[] =	{
	/* no use for pex mem remap */	
  /*{ PEX0_MEM_BASE,  		__phys_to_pfn(PEX0_MEM_BASE),   	PEX0_MEM_SIZE,  	MT_DEVICE},*/
  { INTER_REGS_BASE, 		__phys_to_pfn(INTER_REGS_BASE), 	SZ_1M,  	     	MT_DEVICE},
  { PEX0_IO_BASE,   		__phys_to_pfn(PEX0_IO_BASE),   	 	PEX0_IO_SIZE,  		MT_DEVICE},
  { NFLASH_CS_BASE, 		__phys_to_pfn(NFLASH_CS_BASE), 		NFLASH_CS_SIZE, 	MT_DEVICE},
  { SPI_CS_BASE, 		__phys_to_pfn(SPI_CS_BASE), 		SPI_CS_SIZE, 		MT_DEVICE},
  { CRYPT_ENG_BASE, 		      __phys_to_pfn(CRYPT_ENG_BASE), 		      CRYPT_ENG_SIZE, MT_DEVICE},
};
#endif /* CONFIG_MV_INTERNAL_REGS_SELECTIVE_MAPPING */

MV_CPU_DEC_WIN SYSMAP_88F6281[] = {
  	 /* base low        base high    size       	WinNum     enable */
	{{SDRAM_CS0_BASE ,    0,      SDRAM_CS0_SIZE } ,0xFFFFFFFF,DIS},
	{{SDRAM_CS1_BASE ,    0,      SDRAM_CS1_SIZE } ,0xFFFFFFFF,DIS},
	{{SDRAM_CS2_BASE ,    0,      SDRAM_CS2_SIZE } ,0xFFFFFFFF,DIS},
	{{SDRAM_CS3_BASE ,    0,      SDRAM_CS3_SIZE } ,0xFFFFFFFF,DIS},
	{{PEX0_MEM_BASE  ,    0,      PEX0_MEM_SIZE  } ,0x1       ,EN},
	{{PEX0_IO_BASE   ,    0,      PEX0_IO_SIZE   } ,0x0       ,EN},
    {{PEX1_MEM_BASE  ,    0,      PEX1_MEM_SIZE  } ,0x3       ,DIS},
    {{PEX1_IO_BASE   ,    0,      PEX1_IO_SIZE   } ,0x1       ,DIS},
	{{INTER_REGS_BASE,    0,      INTER_REGS_SIZE} ,0x8       ,EN},
	{{NFLASH_CS_BASE,     0,      NFLASH_CS_SIZE } ,0x2	      ,EN}, 
	{{SPI_CS_BASE,        0,      SPI_CS_SIZE    } ,0x5       ,EN},
	{{DEVICE_CS2_BASE,    0,      DEVICE_CS2_SIZE} ,0x6	      ,DIS},
 	{{BOOTDEV_CS_BASE,    0,      BOOTDEV_CS_SIZE} ,0x4	      ,DIS},
	{{CRYPT_ENG_BASE,     0,      CRYPT_ENG_SIZE}  ,0x7  	  ,EN},
	{{TBL_TERM,TBL_TERM, TBL_TERM} ,TBL_TERM  ,TBL_TERM}		
};

MV_CPU_DEC_WIN SYSMAP_88F6282[] = {
         /* base low        base high    size           WinNum     enable */
        {{SDRAM_CS0_BASE ,    0,      SDRAM_CS0_SIZE } ,0xFFFFFFFF,DIS},
        {{SDRAM_CS1_BASE ,    0,      SDRAM_CS1_SIZE } ,0xFFFFFFFF,DIS},
        {{SDRAM_CS2_BASE ,    0,      SDRAM_CS2_SIZE } ,0xFFFFFFFF,DIS},
        {{SDRAM_CS3_BASE ,    0,      SDRAM_CS3_SIZE } ,0xFFFFFFFF,DIS},
        {{PEX0_MEM_BASE  ,    0,      PEX0_MEM_SIZE  } ,0x2       ,EN},
        {{PEX0_IO_BASE   ,    0,      PEX0_IO_SIZE   } ,0x0       ,EN},
        {{PEX1_MEM_BASE  ,    0,      PEX1_MEM_SIZE } ,0x3       ,EN},
        {{PEX1_IO_BASE   ,    0,      PEX1_IO_SIZE  } ,0x1       ,EN},
        {{INTER_REGS_BASE,    0,      INTER_REGS_SIZE} ,0x8       ,EN},
        {{NFLASH_CS_BASE,     0,      NFLASH_CS_SIZE}  ,0x4       ,EN},
        {{SPI_CS_BASE,        0,      SPI_CS_SIZE    } ,0x5       ,EN},
        {{DEVICE_CS2_BASE,    0,      DEVICE_CS2_SIZE}, 0x6       ,DIS},
        {{BOOTDEV_CS_BASE,    0,      BOOTDEV_CS_SIZE}, 0x9       ,DIS},
        {{CRYPT_ENG_BASE,     0,      CRYPT_ENG_SIZE}  ,0x7       ,EN},
        {{TBL_TERM,TBL_TERM, TBL_TERM} ,TBL_TERM  ,TBL_TERM}
};

MV_CPU_DEC_WIN SYSMAP_88F6180[] = {
  	 /* base low        base high    size       	WinNum     enable */
	{{SDRAM_CS0_BASE ,    0,      SDRAM_CS0_SIZE } ,0xFFFFFFFF,DIS},
	{{SDRAM_CS1_BASE ,    0,      SDRAM_CS1_SIZE } ,0xFFFFFFFF,DIS},
	{{SDRAM_CS2_BASE ,    0,      SDRAM_CS2_SIZE } ,0xFFFFFFFF,DIS},
	{{SDRAM_CS3_BASE ,    0,      SDRAM_CS3_SIZE } ,0xFFFFFFFF,DIS},
	{{PEX0_MEM_BASE  ,    0,      PEX0_MEM_SIZE  } ,0x1       ,EN},
	{{PEX0_IO_BASE   ,    0,      PEX0_IO_SIZE   } ,0x0       ,EN},
    {{PEX1_MEM_BASE  ,    0,      PEX1_MEM_SIZE } ,0x3       ,DIS},
    {{PEX1_IO_BASE   ,    0,      PEX1_IO_SIZE  } ,0x1       ,DIS},
	{{INTER_REGS_BASE,    0,      INTER_REGS_SIZE} ,0x8       ,EN},
	{{NFLASH_CS_BASE,     0,      NFLASH_CS_SIZE } ,0x2	  ,EN}, 
	{{SPI_CS_BASE,        0,      SPI_CS_SIZE}     ,0x5       ,EN}, 
	{{DEVICE_CS2_BASE,    0,      DEVICE_CS2_SIZE}, 0x6	  ,DIS},
 	{{BOOTDEV_CS_BASE,    0,      BOOTDEV_CS_SIZE}, 0x4	  ,DIS},
	{{CRYPT_ENG_BASE,     0,      CRYPT_ENG_SIZE}  ,0x7  	  ,EN},
	{{TBL_TERM,TBL_TERM, TBL_TERM} ,TBL_TERM  ,TBL_TERM}		
};

MV_CPU_DEC_WIN SYSMAP_88F6280[] = {
         /* base low        base high    size           WinNum     enable */
        {{SDRAM_CS0_BASE ,    0,      SDRAM_CS0_SIZE } ,0xFFFFFFFF,DIS},
        {{SDRAM_CS1_BASE ,    0,      SDRAM_CS1_SIZE } ,0xFFFFFFFF,DIS},
        {{SDRAM_CS2_BASE ,    0,      SDRAM_CS2_SIZE } ,0xFFFFFFFF,DIS},
        {{SDRAM_CS3_BASE ,    0,      SDRAM_CS3_SIZE } ,0xFFFFFFFF,DIS},
        {{PEX0_MEM_BASE  ,    0,      PEX0_MEM_SIZE  } ,0x1       ,DIS},
        {{PEX0_IO_BASE   ,    0,      PEX0_IO_SIZE   } ,0x0       ,DIS},
        {{PEX1_MEM_BASE  ,    0,      PEX1_MEM_SIZE } ,0x3       ,DIS},
        {{PEX1_IO_BASE   ,    0,      PEX1_IO_SIZE  } ,0x1       ,DIS},
        {{INTER_REGS_BASE,    0,      INTER_REGS_SIZE} ,0x8       ,EN},
        {{NFLASH_CS_BASE,     0,      NFLASH_CS_SIZE } ,0x2       ,EN},
        {{SPI_CS_BASE,        0,      SPI_CS_SIZE}     ,0x5       ,EN},
        {{DEVICE_CS2_BASE,    0,      DEVICE_CS2_SIZE}, 0x6       ,DIS},
        {{BOOTDEV_CS_BASE,    0,      BOOTDEV_CS_SIZE}, 0x4       ,DIS},
        {{CRYPT_ENG_BASE,     0,      CRYPT_ENG_SIZE}  ,0x7       ,EN},
        {{TBL_TERM,TBL_TERM, TBL_TERM} ,TBL_TERM  ,TBL_TERM}
};

MV_CPU_DEC_WIN SYSMAP_88F6192[] = {
  	 /* base low        base high    size       	WinNum     enable */
	{{SDRAM_CS0_BASE ,    0,      SDRAM_CS0_SIZE } ,0xFFFFFFFF,DIS},
	{{SDRAM_CS1_BASE ,    0,      SDRAM_CS1_SIZE } ,0xFFFFFFFF,DIS},
	{{SDRAM_CS2_BASE ,    0,      SDRAM_CS2_SIZE } ,0xFFFFFFFF,DIS},
	{{SDRAM_CS3_BASE ,    0,      SDRAM_CS3_SIZE } ,0xFFFFFFFF,DIS},
	{{PEX0_MEM_BASE  ,    0,      PEX0_MEM_SIZE  } ,0x1       ,EN},
	{{PEX0_IO_BASE   ,    0,      PEX0_IO_SIZE   } ,0x0       ,EN},
    {{PEX1_MEM_BASE  ,    0,      PEX1_MEM_SIZE } ,0x3       ,DIS},
    {{PEX1_IO_BASE   ,    0,      PEX1_IO_SIZE  } ,0x1       ,DIS},
	{{INTER_REGS_BASE,    0,      INTER_REGS_SIZE} ,0x8       ,EN},
	{{NFLASH_CS_BASE,     0,      NFLASH_CS_SIZE}  ,0x2	  ,EN}, 
	{{SPI_CS_BASE,        0,      SPI_CS_SIZE}     ,0x5       ,EN},
	{{DEVICE_CS2_BASE,    0,      DEVICE_CS2_SIZE}, 0x6	  ,DIS},
 	{{BOOTDEV_CS_BASE,    0,      BOOTDEV_CS_SIZE}, 0x4	  ,DIS},
	{{CRYPT_ENG_BASE,     0,      CRYPT_ENG_SIZE}  ,0x7  	  ,EN},
	{{TBL_TERM,TBL_TERM, TBL_TERM} ,TBL_TERM  ,TBL_TERM}		
};

#if defined(CONFIG_MV_INTERNAL_REGS_SELECTIVE_MAPPING)

void __init mv_build_map_table(void)
{
	u32 unit;
	
	/* prepare consecutive mapping table */
	for(unit = 0; unit < ARRAY_SIZE(mv_internal_regs_map); unit++) {
		if(MV_TRUE == mvCtrlPwrClckGet(mv_internal_regs_map[unit].id, mv_internal_regs_map[unit].index)) {
			MEM_TABLE[entries].virtual = (INTER_REGS_BASE + mv_internal_regs_map[unit].offset);
			MEM_TABLE[entries].pfn = __phys_to_pfn(INTER_REGS_BASE + mv_internal_regs_map[unit].offset);
			MEM_TABLE[entries].length = mv_internal_regs_map[unit].size;
			MEM_TABLE[entries].type = MT_DEVICE;
			entries++;
		}
	}
}

#endif /* CONFIG_MV_INTERNAL_REGS_SELECTIVE_MAPPING */

MV_CPU_DEC_WIN* mv_sys_map(void)
{

	switch(mvBoardIdGet()) {
	case DB_88F6281A_BP_ID:
	case RD_88F6281A_ID:
	case SHEEVA_PLUG_ID: 
#if defined(CONFIG_SYNO_MV88F6281)
	case SYNO_DS409_ID:
	case SYNO_DS109_ID:
	case SYNO_DS409slim_ID:
	case SYNO_DS011_ID:
#endif
		return SYSMAP_88F6281;
	case DB_88F6282A_BP_ID:
	case RD_88F6282A_ID:
#if defined(CONFIG_SYNO_MV88F6281)
	case SYNO_DS211_ID:
	case SYNO_DS411slim_ID:
	case SYNO_RS_6282_ID:
	case SYNO_DS411_ID:
	case SYNO_DS212_ID:
#endif
		return SYSMAP_88F6282;
	case DB_88F6701A_BP_ID:
	case DB_88F6702A_BP_ID:
	case DB_88F6192A_BP_ID:
	case RD_88F6192A_ID:
    case DB_88F6190A_BP_ID:
    case RD_88F6190A_ID:
#if defined(CONFIG_SYNO_MV88F6281)
	case SYNO_6702_1BAY_ID:
#endif
		return SYSMAP_88F6192;
	case DB_88F6180A_BP_ID:
		return SYSMAP_88F6180;
	case DB_88F6280A_BP_ID:
        	return SYSMAP_88F6280;
	default:
		printk("ERROR: can't find system address map\n");
		return NULL;
        }
}


#if defined(CONFIG_MV_INCLUDE_CESA)
u32 mv_crypto_base_get(void)
{
	return CRYPT_ENG_BASE;
}
#endif

void __init mv_map_io(void)
{
#if defined(CONFIG_MV_INTERNAL_REGS_SELECTIVE_MAPPING)
  	/* first, mappping AHB to MBUS entry for mvCtrlPwrClckGet access */
	iotable_init(AHB_TO_MBUS_MAP, ARRAY_SIZE(AHB_TO_MBUS_MAP));

	/* build dynamic mapping table  */
	mv_build_map_table();

	iotable_init(MEM_TABLE, entries);
#else
        iotable_init(MEM_TABLE, ARRAY_SIZE(MEM_TABLE));
#endif /* CONFIG_MV_INTERNAL_REGS_SELECTIVE_MAPPING */	
}


