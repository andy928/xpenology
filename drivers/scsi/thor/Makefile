#
#
# Note: This Makefile is for 2.6 kernel only, at present.
#
# V0.0.0.6 Ver.Make

# default to build for the running kernel
ifeq ("x", "x$(KERNEL_SRC)")
	KERNEL_SRC=/lib/modules/$(shell uname -r)/build
endif

# use KERNEL_SRC if not called by our driver disk maker
ifeq ("x", "x$(KERNEL_SOURCE_DIR)")
        KERNEL_SRC_DIR=$(KERNEL_SRC)
else
	KERNEL_SRC_DIR=$(KERNEL_SOURCE_DIR)
endif

ifeq ("x", "x$(KERNEL_BUILD_DIR)")
	KERNEL_BLD_DIR=$(KERNEL_SRC_DIR)
else
        KERNEL_BLD_DIR=$(KERNEL_BUILD_DIR)
endif

#ifneq ($(KERNELRELEASE),)
#	include $(SUBDIRS)/mv_conf.mk
#else
#	include mv_conf.mk
#endif

SUPPORT_THOR=y

ifeq ("xy", "x$(RAID_MODULE)")
	LIB_TYPE=_raid_
endif

# KERNEL_VER := $(shell cat $(KERNEL_BLD_SRC)/include/linux/version.h | grep UTS_RELEASE | cut -c22-24 | head -n 1)

# we may build for 32bit kernel on a 64bit system
CONFIG_64BIT=$(shell [ -f $(KERNEL_BLD_DIR)/.config ] && cat $(KERNEL_BLD_DIR)/.config | grep -m 1 CONFIG_64BIT | awk -F= '{print $$2}')
CONFIG_REGPARM=$(shell [ -f $(KERNEL_BLD_DIR)/.config ] && cat $(KERNEL_BLD_DIR)/.config | grep -m 1 CONFIG_REGPARM | awk -F= '{print $$2}')
CONFIG_4KSTACKS=$(shell [ -f $(KERNEL_BLD_DIR)/.config ] && cat $(KERNEL_BLD_DIR)/.config | grep -m 1 CONFIG_4KSTACKS | awk -F= '{print $$2}')
CONFIG_SUSE_KERNEL=$(shell [ -f $(KERNEL_BLD_DIR)/.config ] && cat $(KERNEL_BLD_DIR)/.config | grep -m 1 CONFIG_SUSE_KERNEL | awk -F= '{print $$2}')

ifeq ($(ARCH), )
ifeq ($(strip $(CONFIG_64BIT)),y)
	ARCH_TYPE=x86_64
		RAID_LIB_NAME=libmv$(LIB_TYPE)64
else	
	ARCH_TYPE=i386 
	ifeq ($(strip $(CONFIG_REGPARM)),y)
		RAID_LIB_NAME=libmv$(LIB_TYPE)32
	else
		RAID_LIB_NAME=libmv$(LIB_TYPE)32_noregparm
	endif
endif
else
	ARCH_TYPE=$(ARCH)
endif

MV_CC     =  $(CROSS_COMPILE)$(CC)
MV_LD     =  $(CROSS_COMPILE)$(LD)

export LD_LIBRARY_PATH

odin_objs_$(SUPPORT_ODIN)   := core/odin/core_exp.o   core/odin/core_init.o   \
		               core/odin/core_ibcd.o  core/odin/core_adpt.o   \
		               core/odin/core_spi.o    \
		               core/odin/core_ses.o   core/odin/core_xor.o    \
		               core/odin/core_api.o   core/odin/scsi2sata.o   \
		               core/odin/core_i2c.o                           \
		               core/odin/ata2scsi.o   core/odin/core_helper.o \
			       core/odin/core_extern.o core/odin/core_swxor.o  \
#    core/odin/consolid.o \
#   core/odin/core_eeprom.o \

thor_objs_$(SUPPORT_THOR)   := core/thor/core_exp.o core/thor/core_init.o\
		   core/thor/core_api.o core/thor/core_xor.o\
		   core/thor/scsi2sata.o core/thor/core_swxor.o core/thor/core_sat.o

#cons_objs_$(SUPPORT_CONSOLIDATE) := core/thor/consolid.o

CORE_OBJS       :=  $(odin_objs_y) $(thor_objs_y) 


COMM_OBJS       := lib/common/com_util.o lib/common/com_u64.o             \
		   lib/common/com_scsi.o lib/common/com_tag.o             \
		   lib/common/com_sgd.o  lib/common/com_nvram.o

raid_objs_$(RAID_MODULE) := raid/ddf_handler.o       raid/raid0.o         \
			    raid/raid1.o             raid/raid_util_ext.o \
		            raid/raid_api.o          raid/raid_bga.o      \
	                    raid/raid_cmd_handler.o  raid/raid_ddf.o      \
		            raid/raid_eh.o           raid/raid_exp.o      \
			    raid/raid_manage_ext.o   raid/raid_mp.o       \
		            raid/raid_manage.o       raid/raid_res_mgmt.o \
		            raid/raid_tset.o         raid/raid_util.o     \
			    raid/ddf_handler_ext.o   raid/raid_api_ext.o  \
                            raid/raid_cmd_ext.o      raid/raid_ddf_ext.o  \
			    raid/raid_exp_ext.o

cache_objs_$(CACHE_MODULE) := cache/cache_exp.o      cache/cache_mod.o

raid6_objs_$(RAID6_MODULE) := raid/raid6.o math/math_matrix.o

RAID_OBJS       := $(raid_objs_y)  $(raid6_objs_y) $(cache_objs_y)

OSDEP_OBJS      := linux/linux_main.o    linux/hba_exp.o          \
		   linux/hba_mod.o       linux/hba_timer.o        \
		   linux/oss_wrapper.o   linux/linux_iface.o      \
	           linux/res_mgmt.o 	

# for partial source tree building
core_dir_$(SUPPORT_ODIN) :=core/odin
core_dir_$(SUPPORT_THOR) :=core/thor
include_dir := include  include/generic  include/icommon

header_dirs     :=   $(include_dir)

LIBMV_OBJS	:=  $(COMM_OBJS) $(RAID_OBJS)

HBA_OBJS        := $(OSDEP_OBJS) $(CORE_OBJS) $(LIBMV_OBJS) 

INCLUDE_DIR     = -I$(KERNEL_BLD_DIR)/include                          \
		  -I$(KERNEL_BLD_DIR)/include/scsi                     \
		  -I$(KERNEL_BLD_DIR)/drivers/scsi                     \
	 	  -I$(KERNEL_SRC_DIR)/include                          \
	 	  -I$(KERNEL_SRC_DIR)/include/scsi                     \
	 	  -I$(KERNEL_SRC_DIR)/drivers/scsi


ifneq ($(KERNELRELEASE),)
ifeq ($(SUPPORT_ODIN), y)
obj-$(CONFIG_SCSI_MV_THOR)	:=   mv64xx.o
mv64xx-objs   :=   $(HBA_OBJS) 
endif

ifeq ($(SUPPORT_THOR), y)
obj-$(CONFIG_SCSI_MV_THOR)	:=   mv61xx.o
mv61xx-objs	:=   $(HBA_OBJS) 
endif

clean-files   +=   Modules.symvers
clean-files   +=   Module.symvers
EXTRA_CFLAGS  :=   -I$(src)/include    -I$(src)/core            \
		   -I$(src)/raid       -I$(src)/.               \
		   -I$(src)/linux  -I$(src)/math            \
                   -I$(src)/cache      -I$(src)/include/generic \
		   -I$(src)/include/icommon

EXTRA_CFLAGS  +=   -D__MV_LINUX__ $(INCLUDE_DIR)

ifeq ($(CONFIG_64BIT), y)
EXTRA_CFLAGS  +=   -D_64_SYS_
EXTRA_CFLAGS  +=   -D_64_BIT_COMPILER
else
EXTRA_CFLAGS  +=   -D_32_LEGACY_
endif
ifeq ($(CONFIG_4KSTACKS), y)
EXTRA_CFLAGS  +=   -D_4K_STACKS_
endif

ifeq ($(CONFIG_SUSE_KERNEL), y)
EXTRA_CFLAGS  += -DIS_OPENSUSE_SLED_SLES=1
else
EXTRA_CFLAGS  += -DIS_OPENSUSE_SLED_SLES=0
endif

EXTRA_CFLAGS  +=   -include $(src)/mv_config.h

ifeq ($(RAID_MODULE), y)
EXTRA_CFLAGS  +=   -DRAID_DRIVER=1
endif

ifeq ($(RAID6_MODULE), y)
EXTRA_CFLAGS  +=   -DSUPPORT_RAID6=1
endif

ifeq ($(CACHE_MODULE), y)
EXTRA_CFLAGS  +=   -DCACHE_MODULE_SUPPORT=1
endif

ifeq ($(SUPPORT_ODIN), y)
EXTRA_CFLAGS  +=   -I$(src)/core/odin -DPRODUCTNAME_ODIN
endif

ifeq ($(SUPPORT_THOR), y)
EXTRA_CFLAGS  +=   -I$(src)/core/thor -DPRODUCTNAME_THOR
endif


#EXTRA_CFLAGS  += -D__MV_DEBUG__  
EXTRA_CFLAGS  += -Wno-unused-variable  -Wno-pointer-arith
EXTRA_CFLAGS  += -D__LEGACY_OSSW__=1

else

# Why use SUBDIRS? for backward compatibility
all:
	$(MAKE) ARCH=$(ARCH_TYPE) CC=$(MV_CC) LD=$(MV_LD) CROSS_COMPILE=$(CROSS_COMPILE) V=$(V) -C $(KERNEL_BLD_DIR) SUBDIRS=`pwd` modules

lib: all
	@mkdir -p raid_lib
	@$(LD) -r -o $(RAID_LIB_NAME).o $(LIBMV_OBJS)
	@cp -rf $(RAID_LIB_NAME).o ./raid_lib/$(RAID_LIB_NAME).obj

partial: 
	@echo
	@echo -n "Creating partial source tree ... "
	@rm -rf partial
	@mkdir -p partial partial/lib partial/include partial/$(core_dir_y)
	@cp Makefile.partial partial/Makefile
	@cp patch.kbuild partial/
	@cp patch.sh partial/
	@cp Makefile.kbuild partial/
	@cp -uf ./raid_lib/* ./partial/lib/
	@cp -uf ./readme.partial ./partial/readme
	@cp -uf linux/*.[ch] ./partial
	@cp -uf $(core_dir_y)/*.[ch] ./partial/$(core_dir_y)
	@cp -uf include/com_list.c ./partial/include
	@cp -uf mv_config.h ./partial
	@cp -uf mv_odin.h ./partial	
	@cp -uf mv_thor.h ./partial
	@cp -uf mv_conf.mk ./partial
	@cp -uf mv_include.h ./partial/include
	@for i in $(header_dirs); do cp -uf $$i/*.h  ./partial/include; done
	@if [ "x$$?" == "x0" ];then echo done.;else echo failed.;fi
	@echo

kbuild:
	-@cat patch.kbuild | patch -p1 -N -d $(KERNEL_SRC_DIR)/drivers/scsi
	@./patch.sh $(KERNEL_SRC_DIR)/drivers/scsi a
	@mkdir -p $(KERNEL_SRC_DIR)/drivers/scsi/mv
	@cp -r * $(KERNEL_SRC_DIR)/drivers/scsi/mv
	@cp -f Makefile.kbuild $(KERNEL_SRC_DIR)/drivers/scsi/mv/Makefile
	@echo done.

ukbuild:
	-@cat patch.kbuild | patch -p1 -N -R -d $(KERNEL_SRC_DIR)/drivers/scsi
	@./patch.sh $(KERNEL_SRC_DIR)/drivers/scsi
	@rm -rf $(KERNEL_SRC_DIR)/drivers/scsi/mv
	@echo Patch has been successfully rewinded.

nonraid:
	@echo -n "Creating non-RAID source tree ... "
	@rm -rf thor odin norse
	@mkdir norse
	@mkdir -p norse/linux norse/lib/common norse/core
	@mkdir -p norse/include/icommon norse/include/generic
	@
	@cp -uf linux/*.[ch] norse/linux
	@cp -uf lib/common/*.[ch] norse/lib/common
	@cp -uf include/*.[ch] norse/include/
	@cp -uf include/icommon/*.[ch] norse/include/icommon
	@cp -uf include/generic/*.[ch] norse/include/generic
	@cp -uf patch.* norse
	@cp -uf Makefile.kbuild norse
	@cp -uf Makefile.nonraid norse/Makefile
	@cp -uf mv_config.h norse
	@cp -uf mv_include.h norse
	@
	@if [ "x${SUPPORT_THOR}" == "xy" ]; then mkdir norse/core/thor; cp core/thor/*.[ch] norse/core/thor;cp mv_thor.h norse;fi
	@if [ "x${SUPPORT_THOR}" == "xy" ]; then echo "SUPPORT_THOR=y" > norse/mv_conf.mk;mv norse thor;fi
	@
	@if [ "x${SUPPORT_ODIN}" == "xy" ]; then mkdir norse/core/odin; cp core/odin/*.[ch] norse/core/odin;cp mv_odin.h norse;fi
	@if [ "x${SUPPORT_ODIN}" == "xy" ]; then echo "SUPPORT_ODIN=y" > norse/mv_conf.mk;mv norse odin;fi
	@echo "done"

clean:  
	$(MAKE) ARCH=$(ARCH_TYPE) CC=$(MV_CC) LD=$(MV_LD) CROSS_COMPILE=$(CROSS_COMPILE) V=$(V) -C $(KERNEL_BLD_DIR) SUBDIRS=`pwd` clean
	@rm -rf partial
	@rm -rf raid_lib
	@rm -rf thor odin norse
	@rm -rf *.o *.obj
endif
