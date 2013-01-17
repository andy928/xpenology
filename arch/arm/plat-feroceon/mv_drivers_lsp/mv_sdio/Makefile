#
# Makefile for the Marvell Audio ALSA Device Driver
#
ifeq ($(CONFIG_ARCH_FEROCEON_ORION),y)
	include $(srctree)/arch/arm/mach-feroceon-orion/config/mvRules.mk
endif
 
ifeq ($(CONFIG_ARCH_FEROCEON_KW),y)
	include $(srctree)/arch/arm/mach-feroceon-kw/config/mvRules.mk
endif

obj-$(CONFIG_MMC_MVSDMMC)	+= mvsdmmc.o
