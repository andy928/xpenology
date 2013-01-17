#ifndef HBA_INTERNAL_H
#define HBA_INTERNAL_H

#include "hba_header.h"

typedef struct _Timer_Module
{
	MV_PVOID context;
	MV_VOID (*routine) (MV_PVOID);
} Timer_Module, *PTimer_Module;


struct hba_extension {
/* Must be the first */
/* self-descriptor */
	struct mv_mod_desc *desc;
	/* Device extention */
	MV_PVOID host_data;
	
	
	struct list_head        next;
	struct pci_dev          *dev;
	struct cdev             cdev;

#ifdef THOR_DRIVER
	struct timer_list	timer;
	struct _Timer_Module    Timer_Module;
#endif
	//spinlock_t              lock;

	struct Scsi_Host	*host;
	struct completion       cmpl;

#if LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 11)
	atomic_t                hba_sync;
#endif /* LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 11) */

	MV_U32                  State;	
	MV_BOOLEAN              Is_Dump;
	MV_U8                   Io_Count;
	MV_U16                  Max_Io;

#ifdef SUPPORT_EVENT
	struct list_head        Stored_Events;
	struct list_head        Free_Events;
	MV_U32	                SequenceNumber;
	MV_U8                   Num_Stored_Events;
#endif /* SUPPORT_EVENT */
	MV_PVOID                req_pool;

	MV_U8                   Memory_Pool[1];

#ifdef SUPPORT_TASKLET
	struct tasklet_struct mv_tasklet;
#endif
	
};

#define DRIVER_STATUS_IDLE      1    /* The first status */
#define DRIVER_STATUS_STARTING  2    /* Begin to start all modules */
#define DRIVER_STATUS_STARTED   3    /* All modules are all settled. */
#define DRIVER_STATUS_SHUTDOWN   4    /* All modules are all settled. */

#endif /* HBA_INTERNAL_H */
