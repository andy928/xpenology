#
# Makefile for the Marvell CESA driver
#

ifeq ($(CONFIG_ARCH_FEROCEON_ORION),y)
	include $(srctree)/arch/arm/mach-feroceon-orion/config/mvRules.mk
endif
 
ifeq ($(CONFIG_ARCH_FEROCEON_KW),y)
	include $(srctree)/arch/arm/mach-feroceon-kw/config/mvRules.mk
endif

ifeq ($(CONFIG_ARCH_FEROCEON_MV78XX0),y)
	include $(srctree)/arch/arm/mach-feroceon-mv78xx0/config/mvRules.mk
endif

ifeq ($(CONFIG_MV_CESA_OCF),y)
EXTRA_CFLAGS    += -I$(srctree)/crypto/ocf_20100325
endif

ifeq ($(CONFIG_MV_CESA_OCF),m)
EXTRA_CFLAGS    += -I$(srctree)/crypto/ocf_20100325
endif

obj-$(CONFIG_MV_CESA_TOOL) += cesa_dev.o
obj-$(CONFIG_MV_CESA_OCF)  += cesa_ocf_drv.o

