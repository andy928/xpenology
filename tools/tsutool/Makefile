# kernel sources 
srctree  := $(shell /bin/pwd)

ifndef KSRC
KSRC  := $(srctree)/../..
endif

include $(KSRC)/.config

ifdef CONFIG_CPU_BIG_ENDIAN
CC := armeb-mv5sft-linux-gnueabi-gcc
LD := armeb-mv5sft-linux-gnueabi-ld
CFLAGS  += -DCONFIG_CPU_BIG_ENDIAN
else
CC := arm-mv5sft-linux-gnueabi-gcc
LD := arm-mv5sft-linux-gnueabi-ld
endif
CFLAGS  := $(EXTRA_CFLAGS) -static
CFLAGS  += -I$(KSRC)/arch/arm/plat-feroceon/mv_drivers_lsp/mv_tsu/

SRCS := mv_tsu_tool.c
OBJS := mv_tsu_tool.o

TARGET := mvTsuTool

default :
	$(CC) $(CFLAGS) $(SRCS) -o $(TARGET)

clean:
	rm -f $(TARGET) $(OBJS)


all : clean default

