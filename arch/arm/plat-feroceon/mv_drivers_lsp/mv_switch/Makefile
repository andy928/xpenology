#
# Makefile for the Marvell Key
#
ifeq ($(CONFIG_ARCH_FEROCEON),y)
	include $(srctree)/$(MACHINE)/config/mvRules.mk
endif

ifeq ($(CONFIG_ARCH_ARMADA_XP),y)
	include $(srctree)/arch/arm/mach-armadaxp/config/mvRules.mk
endif
 
obj-$(CONFIG_MV_INCLUDE_SWITCH)	+= mv_switch.o mv_switch_sysfs.o
