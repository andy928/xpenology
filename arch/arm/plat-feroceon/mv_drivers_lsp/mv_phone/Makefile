

#
# Makefile for the Marvell Phone Device Driver
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

obj-$(CONFIG_MV_PHONE) += mv_phone.o

MP_OBJS = mp.o ../../mv_hal/voiceband/voiceband.o ../../mv_hal/voiceband/slic/proslic.o ../../mv_hal/voiceband/daa/daa.o tal.o

ifeq ($(CONFIG_MV_INCLUDE_TDM),y)
MP_OBJS += ../../mv_hal/voiceband/tdm/mvTdm.o
endif

obj-$(CONFIG_MV_PHONE_TEST_MODULE) = phone_test.o

mv_phone-objs := $(MP_OBJS)

