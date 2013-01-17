# $Id: Makefile,v 1.1.1.1 2010/04/15 12:28:02 khchen Exp $
#
# Makefile for the linux kernel.
#

CPPFLAGS_vmlinux.lds := -DDRAM_VIRTUAL_BASE=0x$(CONFIG_ETRAX_DRAM_VIRTUAL_BASE)
extra-y	:= vmlinux.lds

obj-y   := process.o traps.o irq.o ptrace.o setup.o time.o sys_cris.o

obj-$(CONFIG_MODULES)    += crisksyms.o
obj-$(CONFIG_MODULES)	 += module.o
obj-$(CONFIG_SYSTEM_PROFILER)   += profile.o

clean:

