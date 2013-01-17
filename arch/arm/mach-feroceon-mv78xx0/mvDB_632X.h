
MV_CPU_DEC_WIN SYSMAP_DB_623X[] = {
/*     	base low      base high      size        	WinNum		enable/disable */
	{{SDRAM_CS0_BASE ,    0,      SDRAM_CS0_SIZE }, 0xFFFFFFFF,	EN },/*  0 */
	{{SDRAM_CS1_BASE ,    0,      SDRAM_CS1_SIZE }, 0xFFFFFFFF,	EN },/*  1 */
	{{SDRAM_CS2_BASE ,    0,      SDRAM_CS2_SIZE }, 0xFFFFFFFF,	EN },/*  2 */
	{{SDRAM_CS3_BASE ,    0,      SDRAM_CS3_SIZE }, 0xFFFFFFFF,	EN },/*  3 */
	{{DEVICE_CS0_BASE,    0,      DEVICE_CS0_SIZE}, 10,		DIS },/*  4 */
	{{DEVICE_CS1_BASE,    0,      DEVICE_CS1_SIZE}, 11,		DIS },/*  5 */
	{{DEVICE_CS2_BASE,    0,      DEVICE_CS2_SIZE}, 12,		DIS },/*  6 */
	{{DEVICE_CS3_BASE,    0,      DEVICE_CS3_SIZE}, 12,		DIS },/*  7 */
	{{DEVICE_CS2_BASE,    0,      DEVICE_CS2_SIZE}, 13,		EN },/*  8 */
	{{SPI_CS_BASE,        0,      SPI_CS_SIZE}, 	9,		EN},/*  9 */
	{{PCI0_IO_BASE   ,    0,      PCI0_IO_SIZE   }, 0,		EN },/*  10 */
	{{PCI0_MEM0_BASE ,    0,      PCI0_MEM0_SIZE }, 1,		EN },/* 11 */
	{{PCI1_IO_BASE   ,    0,      PCI1_IO_SIZE   }, 2,		DIS},/* 12 */
	{{PCI1_MEM0_BASE ,    0,      PCI1_MEM0_SIZE }, 3,		DIS},/* 13 */
	{{PCI2_IO_BASE   ,    0,      PCI2_IO_SIZE   }, 4,		DIS},/* 14 */
	{{PCI2_MEM0_BASE ,    0,      PCI2_MEM0_SIZE }, 5,		DIS},/* 15 */
	{{PCI3_IO_BASE   ,    0,      PCI3_IO_SIZE   }, 4,		DIS},/* 16 */
	{{PCI3_MEM0_BASE ,    0,      PCI3_MEM0_SIZE }, 5,		DIS},/* 17 */
#if !defined(CONFIG_MV6321)
	{{PCI4_IO_BASE   ,    0,      PCI4_IO_SIZE   }, 6,		EN},/* 18 */
	{{PCI4_MEM0_BASE ,    0,      PCI4_MEM0_SIZE }, 7,		EN},/* 19 */
#else
	{{PCI4_IO_BASE   ,    0,      PCI4_IO_SIZE   }, 6,		DIS},/* 18 */
	{{PCI4_MEM0_BASE ,    0,      PCI4_MEM0_SIZE }, 7,		DIS},/* 19 */
#endif
	{{PCI5_IO_BASE   ,    0,      PCI5_IO_SIZE   }, 5,		DIS},/* 20 */
	{{PCI5_MEM0_BASE ,    0,      PCI5_MEM0_SIZE }, 5,		DIS},/* 21 */
	{{PCI6_IO_BASE   ,    0,      PCI6_IO_SIZE   }, 6,		DIS},/* 22 */
	{{PCI6_MEM0_BASE ,    0,      PCI6_MEM0_SIZE }, 6,		DIS},/* 23 */
	{{PCI7_IO_BASE   ,    0,      PCI7_IO_SIZE   }, 7,		DIS},/* 24 */
	{{PCI7_MEM0_BASE ,    0,      PCI7_MEM0_SIZE }, 7,		DIS},/* 25 */
	{{CRYPTO_ENG_BASE ,   0,      CRYPTO_SIZE    }, 8,		EN },/* 26 */
	{{INTER_REGS_BASE,    0,      INTER_REGS_SIZE}, 14,		EN },/* 27 */
    	/* Table terminator */
   	{{TBL_TERM, TBL_TERM, TBL_TERM}, TBL_TERM, TBL_TERM}
};

