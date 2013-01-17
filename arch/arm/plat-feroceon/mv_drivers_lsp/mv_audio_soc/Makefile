#
# Makefile for the Marvell Audio ALSA Device Driver
#
ifeq ($(CONFIG_ARCH_FEROCEON_ORION),y)
	include $(srctree)/arch/arm/mach-feroceon-orion/config/mvRules.mk
endif
 
ifeq ($(CONFIG_ARCH_FEROCEON_KW),y)
	include $(srctree)/arch/arm/mach-feroceon-kw/config/mvRules.mk
endif

obj-$(CONFIG_SND_MRVL_AUDIO) += snd-mv88fx-soc.o
obj-$(CONFIG_SND_MRVL_AUDIO) += snd-mv88fx-i2s-soc.o
obj-$(CONFIG_SND_MRVL_AUDIO) += snd-mv88fx-pcm-soc.o

snd-mv88fx-i2s-soc-objs               += mv88fx-i2s.o
snd-mv88fx-pcm-soc-objs		      += mv88fx-pcm.o

#snd-mv88fx-soc-objs                  += cs42l51-hal.o
snd-mv88fx-soc-objs                  += mv88fx.o


