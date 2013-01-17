#
# Makefile for the Marvell egiga ethernet driver
#

PATH_MVSATA422  := drivers/ata/mvSata_4_2_2
FILE_MVRULES    := mvRules.mk
DIR_CORE_DRIVER := CoreDriver

SYNO_HAL_SATA_PATH   = -I$(PATH_MVSATA422)/$(DIR_CORE_DRIVER)
SYNO_COMMON_PATH     = -I$(PATH_MVSATA422)/common
SYNO_OSSERV_PATH     = -I$(PATH_MVSATA422)/linux_oss

# Namely for powerpc platform. Actually for all non-SOC chip.
PLAT_DIR = mach-powerpc

ifeq ($(CONFIG_ARCH_FEROCEON_ORION),y)
	include $(TOPDIR)/arch/arm/mach-feroceon-orion/config/mvRules.mk
endif

ifeq ($(CONFIG_ARCH_FEROCEON_KW),y)
        PLAT_DIR = mach-feroceon-kw
endif

ifeq ($(CONFIG_ARCH_FEROCEON_MV78XX0),y)
	include $(TOPDIR)/arch/arm/mach-feroceon-mv78xx0/config/mvRules.mk
endif

SYNO_CONFIG_PATH = -I$(PATH_MVSATA422)/$(PLAT_DIR)
include $(PATH_MVSATA422)/$(PLAT_DIR)/$(FILE_MVRULES)

IAL_OBJS        := mvLinuxIalLib.o mvLinuxIalHt.o mvLinuxIalOs.o mvIALCommon.o mvIALCommonUtils.o mvLinuxIalSmart.o
SAL_OBJS        := mvScsiAtaLayer.o
CORE_OBJS       := $(DIR_CORE_DRIVER)/mvSata.o $(DIR_CORE_DRIVER)/mvStorageDev.o $(DIR_CORE_DRIVER)/mvLog.o

obj-$(CONFIG_SCSI_MVSATA_BSP422) := mvsata.o
mvsata-y        := $(IAL_OBJS) $(SAL_OBJS) $(CORE_OBJS)



INCLUDE_DIRS    := -Idrivers/scsi
EXTRA_CFLAGS += -DLINUX -D__LINUX__ $(INCLUDE_DIRS)
