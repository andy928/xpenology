
struct map_desc DB_632X_MEM_TABLE[] __initdata = {

 { PCI0_MEM0_BASE,  	__phys_to_pfn(PCI0_MEM0_BASE),  PCIx_MEM0_SIZE, MT_DEVICE},
/*
 { PCI1_MEM0_BASE,  	__phys_to_pfn(PCI1_MEM0_BASE),  PCIx_MEM0_SIZE, MT_DEVICE},
 { PCI2_MEM0_BASE,  	__phys_to_pfn(PCI2_MEM0_BASE),  PCIx_MEM0_SIZE, MT_DEVICE},
 { PCI3_MEM0_BASE,  	__phys_to_pfn(PCI3_MEM0_BASE),  PCIx_MEM0_SIZE, MT_DEVICE},
*/
 { INTER_REGS_BASE, 	__phys_to_pfn(INTER_REGS_BASE), _1M,  	      MT_DEVICE },
 { PCI0_IO_BASE,   	__phys_to_pfn(PCI0_IO_BASE),   	PCIx_IO_SIZE,  	MT_DEVICE},
#if !defined(CONFIG_MV6321)
 { PCI1_IO_BASE,   	__phys_to_pfn(PCI1_IO_BASE),   	PCIx_IO_SIZE, 	MT_DEVICE},
#endif
 { SPI_CS_BASE, __phys_to_pfn(SPI_CS_BASE), SPI_CS_SIZE, MT_DEVICE },
 { DEVICE_CS2_BASE, __phys_to_pfn(DEVICE_CS2_SIZE), DEVICE_CS2_SIZE, MT_DEVICE},
 { CRYPTO_ENG_BASE,__phys_to_pfn(CRYPTO_ENG_BASE), CRYPTO_SIZE, MT_DEVICE  },					
};

