# for SGlinux builds
-include $(ROOTDIR)/modules/.config

obj-$(CONFIG_OCF_CRYPTOCTEON) += cryptocteon.o

obj ?= .
EXTRA_CFLAGS += -I$(obj)/.. -I$(obj)/

ifdef CONFIG_OCF_CRYPTOCTEON
# you need the cavium crypto component installed
EXTRA_CFLAGS += -I/usr/local/Cavium_Networks/OCTEON-SDK/components/crypto-api/core/cryptolinux
endif

ifdef TOPDIR
-include $(TOPDIR)/Rules.make
endif

