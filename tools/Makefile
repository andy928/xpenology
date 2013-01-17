# kernel sources 
srctree  := $(shell /bin/pwd)

ifndef KSRC
KSRC  := $(srctree)/..
endif

include $(KSRC)/.config

CFLAGS  := $(EXTRA_CFLAGS)
CFLAGS  += -I$(KSRC)/arch/arm/plat-feroceon/mv_drivers_lsp/mv_cesa/ \
	   -I$(KSRC)/arch/arm/plat-feroceon/mv_drivers_lsp/mv_network/mv_ethernet/

ifdef CONFIG_CPU_BIG_ENDIAN
CC := armeb-mv5sft-linux-gnueabi-gcc 
LD := armeb-mv5sft-linux-gnueabi-ld
CFLAGS  += -DCONFIG_CPU_BIG_ENDIAN
else
CC := arm-mv5sft-linux-gnueabi-gcc
LD := arm-mv5sft-linux-gnueabi-ld
endif


SUBDIRS += tsutool

TARGETS=mv_shell mv_eth_tool

ifeq ($(CONFIG_MV_GATEWAY),y)
CFLAGS += -DCONFIG_MV_GATEWAY
endif

ifeq ($(CONFIG_MV_ETHERNET),y)
CFLAGS += -DCONFIG_MV_ETHERNET
endif

ifeq ($(CONFIG_MV_CPU_PERF_CNTRS),y)
CFLAGS += -DCONFIG_MV_CPU_PERF_CNTRS
endif

ifeq ($(CONFIG_MV_CESA),y)
TARGETS += mv_cesa_tool
endif

ifeq ($(CONFIG_MV_PHONE),y)
TARGETS += mv_voip_tool
endif

ifeq ($(CONFIG_MV_INCLUDE_USB),y)
TARGETS += testusb
endif



all : Makefile $(TARGETS)
	@for subdir in $(SUBDIRS); do \
                (cd $$subdir && $(MAKE) $@) || exit 1; \
        done
	
mv_shell : mv_shell.c 
	$(CC) $(CFLAGS) -static mv_shell.c -o mv_shell

mv_eth_tool: mv_eth_tool.c
	$(CC) $(CFLAGS) -static mv_eth_tool.c -o mv_eth_tool

mv_set_coal : mv_set_coal.c
	$(CC) $(CFLAGS) mv_set_coal.c -o mv_set_coal

mv_voip_tool: mv_voip_tool.c
	$(CC) $(CFLAGS) -static mv_voip_tool.c -o mv_voip_tool

mv_cesa_tool:mv_cesa_tool.c
	$(CC) $(CFLAGS) -static mv_cesa_tool.c -o mv_cesa_tool

testusb: testusb.c
	$(CC) $(CFLAGS) -static testusb.c -o testusb -lpthread

clean:
	rm -f $(TARGETS) *.o
	@for subdir in $(SUBDIRS); do \
                (cd $$subdir && $(MAKE) $@) || exit 1; \
        done

