/*
 *
 * linux_iface.c - Kernel/CLI interface
 *
 */

#include "hba_header.h"
#include "hba_mod.h"
#include "hba_timer.h"

#include "linux_iface.h"
#include "linux_main.h"


#define MV_DEVFS_NAME "mv"

#ifdef RAID_DRIVER
static int mv_open(struct inode *inode, struct file *file);
#ifdef HAVE_UNLOCKED_IOCTL
static long mv_ioctl(struct file *file, unsigned int cmd, unsigned long arg);
#else
static int mv_ioctl(struct inode *inode, struct file *file, unsigned int cmd,
		    unsigned long arg);
#endif /* HAVE_UNLOCKED_IOCTL */

#define IOCTL_BUF_LEN (1024*1024)
static unsigned char ioctl_buf[IOCTL_BUF_LEN];

static struct file_operations mv_fops = {
	.owner   =    THIS_MODULE,
	.open    =    mv_open,
#ifdef HAVE_UNLOCKED_IOCTL
	.unlocked_ioctl = mv_ioctl,
#else
	.ioctl   =    mv_ioctl,
#endif
	.release =    NULL
};
#endif /* RAID_DRIVER */

void ioctlcallback(MV_PVOID This, PMV_Request req)
{
	struct hba_extension *hba = (struct hba_extension *) This;

	hba->Io_Count--;
#if LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 11)
	atomic_set(&hba->hba_sync, 0);
#else
	complete(&hba->cmpl);
#endif /* LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 11) */
	res_free_req_to_pool(hba->req_pool, req);
}

#ifdef RAID_DRIVER
static MV_U16 API2Driver_ID(MV_U16 API_ID)
{
	MV_U16	returnID = API_ID;
	returnID &= 0xfff;
	return returnID;
}

static LD_Info ldinfo[LDINFO_NUM] = {{0}};
static int mv_proc_ld_info(struct Scsi_Host *host)
{
	struct hba_extension *hba;
	PMV_Request req;
	MV_U8 Cdb[MAX_CDB_SIZE]; 
	MV_U16 LD_ID = 0XFF;
	unsigned long flags;

	Cdb[0] = APICDB0_LD;
	Cdb[1] = APICDB1_LD_GETINFO;
	Cdb[2] = LD_ID & 0xff;
	Cdb[3] = API2Driver_ID(LD_ID)>>8;
	
	hba = __mv_get_ext_from_host(host);
	req = res_get_req_from_pool(hba->req_pool);
	if (req == NULL) {
		return -1; /*FAIL.*/
	}

	req->Cmd_Initiator = hba;
	req->Org_Req = req;
	req->Device_Id = VIRTUAL_DEVICE_ID;
	req->Cmd_Flag = 0;
	req->Req_Type = REQ_TYPE_OS;

	if (SCSI_IS_READ(Cdb[0]))
		req->Cmd_Flag |= CMD_FLAG_DATA_IN;
	if (SCSI_IS_READ(Cdb[0]) || SCSI_IS_WRITE(Cdb[0]))
		req->Cmd_Flag |= CMD_FLAG_DMA;
	
	req->Data_Transfer_Length = LDINFO_NUM*sizeof(LD_Info);
	memset(ldinfo, 0, LDINFO_NUM*sizeof(LD_Info));
	req->Data_Buffer = ldinfo;
	SGTable_Init(&req->SG_Table, 0);
	memcpy(req->Cdb, Cdb, MAX_CDB_SIZE);
	memset(req->Context, 0, sizeof(MV_PVOID)*MAX_POSSIBLE_MODULE_NUMBER);

	req->LBA.value = 0;    
	req->Sector_Count = 0; 
	req->Completion = ioctlcallback;
	req->Req_Type = REQ_TYPE_INTERNAL;

	spin_lock_irqsave(&hba->desc->hba_desc->global_lock, flags);
	hba->Io_Count++;
#if LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 11)
	atomic_set(&hba->hba_sync, 1);
#endif /* LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 11) */
	hba->desc->ops->module_sendrequest(hba->desc->extension, req);
	spin_unlock_irqrestore(&hba->desc->hba_desc->global_lock, flags);

#if LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 11)
	if ( !__hba_wait_for_atomic_timeout(&hba->hba_sync, 
					    HBA_REQ_TIMER_IOCTL*HZ) ) {
#else
	if (wait_for_completion_timeout(&hba->cmpl, 
					HBA_REQ_TIMER_IOCTL*HZ) == 0) {
#endif /* LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 11) */
		MV_DBG(DMSG_HBA, "__MV__ ioctl req timed out.\n");
	        return -1; /*FAIL.*/
	}

	return 0;/*SUCCESS.*/
}


static char* mv_ld_status(int status)
{
	switch (status) {
	case LD_STATUS_FUNCTIONAL:
		return "online";
	case LD_STATUS_DEGRADE:
		return "degraded";		
	case LD_STATUS_DELETED:
		return "deleted";
	case LD_STATUS_PARTIALLYOPTIMAL:
		return "partially optimal";
	case LD_STATUS_OFFLINE:
		return "offline";
	default:
		return "unknown";
	}
}

static char* mv_ld_raid_mode(int status)
{
	switch (status) {
	case LD_MODE_RAID0:
		return "RAID0";
	case LD_MODE_RAID1:
		return "RAID1";
	case LD_MODE_RAID10:
		return "RAID10";
	case LD_MODE_RAID1E:
		return "RAID1E";
	case LD_MODE_RAID5:
		return "RAID5";
	case LD_MODE_RAID50:
		return "RAID50";
	case LD_MODE_RAID6:
		return "RAID6";
	case LD_MODE_RAID60:
		return "RAID60";
	case LD_MODE_JBOD:
		return "JBOD";
	default:
		return "unknown";	
	}
}

static char* mv_ld_bga_status(int status)
{
	switch (status) {
	case LD_BGA_STATE_RUNNING:
		return "running";
	case LD_BGA_STATE_ABORTED:
		return "aborted";
	case LD_BGA_STATE_PAUSED:
		return "paused";
	case LD_BGA_STATE_AUTOPAUSED:
		return "auto paused";
	case LD_BGA_STATE_DDF_PENDING:
		return "DDF pending";
	default:
		return "N/A";
	}
}

static int mv_ld_get_status(struct Scsi_Host *host, MV_U16 ldid, LD_Status *ldstatus)
{
	struct hba_extension *hba;
	PMV_Request req;
	MV_U8 Cdb[MAX_CDB_SIZE]; 
	MV_U16 LD_ID = ldid;/*0XFF;*/
	unsigned long flags;

	hba = __mv_get_ext_from_host(host);
	Cdb[0] = APICDB0_LD;
	Cdb[1] = APICDB1_LD_GETSTATUS;
	Cdb[2] = LD_ID & 0xff;
	Cdb[3] = API2Driver_ID(LD_ID)>>8;
	
	req = res_get_req_from_pool(hba->req_pool);
	if (req == NULL)
		return -1;

	req->Cmd_Initiator = hba;
	req->Org_Req = req;
	req->Device_Id = VIRTUAL_DEVICE_ID;
	req->Cmd_Flag = 0;
	req->Req_Type = REQ_TYPE_OS;

	if (SCSI_IS_READ(Cdb[0]))
		req->Cmd_Flag |= CMD_FLAG_DATA_IN;
	if ( SCSI_IS_READ(Cdb[0]) || SCSI_IS_WRITE(Cdb[0]) )
		req->Cmd_Flag |= CMD_FLAG_DMA;

	/* Data Buffer */
	req->Data_Transfer_Length = sizeof(LD_Status);
	memset(ldstatus, 0, sizeof(LD_Status));
	ldstatus->Status = LD_STATUS_INVALID;
	req->Data_Buffer = ldstatus;
	
	SGTable_Init(&req->SG_Table, 0);
	memcpy(req->Cdb, Cdb, MAX_CDB_SIZE);
	memset(req->Context, 0, sizeof(MV_PVOID)*MAX_POSSIBLE_MODULE_NUMBER);
	req->LBA.value = 0;    
	req->Sector_Count = 0; 
	req->Completion = ioctlcallback;
	req->Req_Type = REQ_TYPE_INTERNAL;
	req->Scsi_Status = REQ_STATUS_PENDING;

	spin_lock_irqsave(&hba->desc->hba_desc->global_lock, flags);
	hba->Io_Count++;
#if LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 11)
	atomic_set(&hba->hba_sync, 1);
#endif /* LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 11) */
	hba->desc->ops->module_sendrequest(hba->desc->extension, req);
	spin_unlock_irqrestore(&hba->desc->hba_desc->global_lock, flags);

#if LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 11)
	if (!__hba_wait_for_atomic_timeout(&hba->hba_sync, 
					    HBA_REQ_TIMER_IOCTL*HZ)) {
#else
	if (!wait_for_completion_timeout(&hba->cmpl, 
					  HBA_REQ_TIMER_IOCTL*HZ)) {
#endif /* LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 11) */
		MV_DBG(DMSG_HBA, "__MV__ ioctl req timed out.\n");
	        return -1; /*FAIL.*/
	}

	return 0;/*SUCCESS.*/
}

static int mv_ld_show_status(char *buf, PLD_Status pld_status)
{
	char *str, *str1;
	int ret = 0;

	if ( LD_BGA_STATE_RUNNING == pld_status->BgaState)
	{
		if (LD_BGA_REBUILD == pld_status->Bga)
			str = "rebuilding";
		else if (LD_BGA_INIT_QUICK == pld_status->Bga ||
		          LD_BGA_INIT_BACK == pld_status->Bga)
			str = "initializing";
		else if (LD_BGA_CONSISTENCY_CHECK == pld_status->Bga || 
		          LD_BGA_CONSISTENCY_FIX == pld_status->Bga)
			str = "synchronizing";
		else if (LD_BGA_MIGRATION == pld_status->Bga)
			str = "migration";
		else
			str = "unknown bga action";
		ret = sprintf(buf, "  %s is %d%% done", str, pld_status->BgaPercentage);
	}
	else if ((LD_BGA_STATE_ABORTED == pld_status->BgaState) ||
	         (LD_BGA_STATE_PAUSED == pld_status->BgaState) || 
	         (LD_BGA_STATE_AUTOPAUSED == pld_status->BgaState))
	{
		if (LD_BGA_REBUILD == pld_status->Bga)
			str = "rebuilding";
		else if (LD_BGA_INIT_QUICK == pld_status->Bga ||
		         LD_BGA_INIT_BACK == pld_status->Bga)
			str = "initializing";
		else if (LD_BGA_CONSISTENCY_CHECK == pld_status->Bga ||
		          LD_BGA_CONSISTENCY_FIX == pld_status->Bga)
			str = "synchronizing";
		else if (LD_BGA_MIGRATION == pld_status->Bga)
			str = "migration";
		else
			str = "unknown bga action";

		if (LD_BGA_STATE_ABORTED == pld_status->BgaState)
			str1 = "aborted";
		else if (LD_BGA_STATE_PAUSED == pld_status->BgaState)
			str1 = "paused";
		else if (LD_BGA_STATE_AUTOPAUSED == pld_status->BgaState)
			str1 = "auto paused";
		else
			str1 = "aborted";
		ret = sprintf(buf, "  %s is %s", str, str1);
	}
	return ret;
}
#endif /*RAID_DRIVER*/

int mv_linux_proc_info(struct Scsi_Host *pSHost, char *pBuffer, 
		       char **ppStart,off_t offset, int length, int inout)
{
	int len = 0;
	int datalen = 0;/*use as a temp flag.*/
#ifdef RAID_DRIVER
	int i = 0;
	int j = 0;
	int ret = -1;
	LD_Status ld_status;
	char *tmp = NULL;
	int tmplen = 0;
#endif	
	if (!pSHost || !pBuffer)
		return (-ENOSYS);

	if (inout == 1) {
		/* User write is not supported. */
		return (-ENOSYS);
	}

	len = sprintf(pBuffer,"Marvell %s Driver , Version %s\n",
		      mv_product_name, mv_version_linux);
	
#ifdef RAID_DRIVER
	if (mv_proc_ld_info(pSHost) == -1) {
		len = sprintf(pBuffer,
			      "Driver is busy, please try later.\n");
		goto out;
	} else {
		for (i = 0; i < MAX_LD_SUPPORTED_PERFORMANCE; i++) {
			if (ldinfo[i].Status != LD_STATUS_INVALID) {
				if (ldinfo[i].Status == LD_STATUS_OFFLINE
				        && ldinfo[i].BGAStatus == LD_BGA_STATE_RUNNING) {
					ldinfo[i].BGAStatus = LD_BGA_STATE_AUTOPAUSED;
				}
				if (ldinfo[i].Status == LD_STATUS_MISSING) {
					ldinfo[i].Status = LD_STATUS_OFFLINE;
				}
			} else {
				break;
			}
		}
	}
	
	len += sprintf(pBuffer+len,"Index RAID\tStatus  \t\tBGA Status\n");
	for (i = 0 ; i < LDINFO_NUM ; i++) {
		if (ldinfo[i].Size.value == 0) {
			if (i == 0) {
				len += sprintf(pBuffer+len,"NO Logical Disk\n");
			}
			break;
		}

		len += sprintf(pBuffer+len,
			"%-5d %s\t%s",
			ldinfo[i].ID,
			mv_ld_raid_mode(ldinfo[i].RaidMode),
			mv_ld_status(ldinfo[i].Status)
			);

		tmplen = 24 -strlen(mv_ld_status(ldinfo[i].Status));
		while (j < tmplen) {
			len += sprintf(pBuffer+len, "%s", " ");
			j++;
		}
		j = 0;

		len += sprintf(pBuffer+len, "%s", mv_ld_bga_status(ldinfo[i].BGAStatus));

		if (ldinfo[i].BGAStatus != LD_BGA_STATE_NONE) {
			ret = mv_ld_get_status(pSHost,ldinfo[i].ID,&ld_status);
			if (ret == 0) {
				if (ld_status.Status != LD_STATUS_INVALID) {
					if (ld_status.Status == LD_STATUS_MISSING)
						ld_status.Status = LD_STATUS_OFFLINE;
					ld_status.BgaState = ldinfo[i].BGAStatus;
				}
				len += mv_ld_show_status(pBuffer+len,&ld_status);
				ret = -1;
			}
		}

		tmp = NULL;
		tmplen = 0;
		len += sprintf(pBuffer+len,"\n");
	}

out:
#endif
		
	datalen = len - offset;
	if (datalen < 0) {
		datalen = 0;
		*ppStart = pBuffer + len;
	} else {
		*ppStart = pBuffer + offset;
	}
	return datalen;
} 

/*
 *Character Device Interface.
 */
#ifdef RAID_DRIVER
static int mv_open(struct inode *inode, struct file *file)
{
	unsigned int minor_number;
	int retval = -ENODEV;
	unsigned long flags = 0;
	
	spin_lock_irqsave(&inode->i_lock, flags);
	minor_number = MINOR(inode->i_rdev);
	if (minor_number >= __mv_get_adapter_count()) {
		printk("MV : No such device.\n");
		goto out;
	}
	retval = 0;
out:
	spin_unlock_irqrestore(&inode->i_lock, flags);
	return retval;
}

static dev_t _mv_major = 0;

int mv_register_chdev(struct hba_extension *hba)
{
	int ret = 0;
	dev_t num;

	/*
	 * look for already-allocated major number first, if not found or
	 * fail to register, try allocate one .
	 *
	 */
	if (_mv_major)
	{
		ret = register_chrdev_region(MKDEV(_mv_major,
						   hba->desc->hba_desc->id),
					     1,
					     MV_DEVFS_NAME);
		num = MKDEV(_mv_major, hba->desc->hba_desc->id);
	}
	if(ret)
	{
		MV_DBG(DMSG_KERN, "registered chrdev (%d, %d) failed.\n",
	       	_mv_major, hba->desc->hba_desc->id);
		return	ret;
	}
	if (ret || !_mv_major) {
		ret = alloc_chrdev_region(&num,
					  hba->desc->hba_desc->id,
					  1,
					  MV_DEVFS_NAME);
		if (ret)
		{
			MV_DBG(DMSG_KERN, "allocate chrdev (%d, %d) failed.\n",
		       	MAJOR(num), hba->desc->hba_desc->id);
			return ret;
		}
		else
			_mv_major = MAJOR(num);
	}

	memset(&hba->cdev, 0, sizeof(struct cdev));
	cdev_init(&hba->cdev, &mv_fops);
	hba->cdev.owner = THIS_MODULE;
	hba->desc->hba_desc->dev_no = num;
	MV_DBG(DMSG_KERN, "registered chrdev (%d, %d).\n",
	       MAJOR(num), MINOR(num));
	ret = cdev_add(&hba->cdev, num, 1);

	return ret;
}

void mv_unregister_chdev(struct hba_extension *hba)
{
	cdev_del(&hba->cdev);
	unregister_chrdev_region(hba->desc->hba_desc->dev_no, 1);
}

#ifdef HAVE_UNLOCKED_IOCTL
static long mv_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
#else
static int mv_ioctl(struct inode *inode, struct file *file, unsigned int cmd, 
		    unsigned long arg)
#endif /* HAVE_UNLOCKED_IOCTL */ 
{
	struct hba_extension	*hba;
	PMV_Request    req = NULL;
	int error = 0;
	int ret   = 0;
	int sptdwb_size = sizeof(SCSI_PASS_THROUGH_DIRECT_WITH_BUFFER);
	int console_id  = VIRTUAL_DEVICE_ID;
	unsigned long flags;
	PSCSI_PASS_THROUGH_DIRECT_WITH_BUFFER psptdwb = NULL;
	int mv_device_count;
	struct scsi_idlun idlun;
	
#ifdef HAVE_UNLOCKED_IOCTL
	hba = container_of(file->f_dentry->d_inode->i_cdev,
			   struct hba_extension, cdev);
#else
	hba = container_of(inode->i_cdev,
			   struct hba_extension, cdev);
#endif /* HAVE_UNLOCKED_IOCTL */ 
	/*if the driver is shutdown ,any process shouldn't call mv_ioctl*/
	if(hba != NULL) {
		if(DRIVER_STATUS_SHUTDOWN== hba->State ) {
			MV_DBG(DMSG_IOCTL, "Marvell : Driver had been rmmoded ,Invalid operation.\n");
			return -EPERM;
		}	
	} else {
		MV_DBG(DMSG_IOCTL, "Marvell : Driver is not installed ,Invalid operation.\n");
		return -EPERM;
	}
	mv_device_count =  __mv_get_adapter_count();
		if (cmd >= API_IOCTL_MAX) {
			MV_DBG(DMSG_IOCTL, "Marvell : Invalid ioctl command.\n");
			return -EBADF;
	}
	
	if (cmd == API_IOCTL_GET_VIRTURL_ID) {
		if (copy_to_user((void *)arg,(void *)&console_id,sizeof(int)) != 0 ) {
			MV_DBG(DMSG_IOCTL, "Marvell : Get VIRTUAL_DEVICE_ID Error.\n");
			return -EIO;
		}
		return 0;
	}

	if (cmd == API_IOCTL_GET_HBA_COUNT) {
		if (copy_to_user((void *)arg,(void *)&mv_device_count,sizeof(unsigned int)) != 0) {
			MV_DBG(DMSG_IOCTL, "Marvell : Get Device Number Error.\n");
			return -EIO;
		}
		return 0;
	}

	if (cmd == API_IOCTL_LOOKUP_DEV) {
		if( copy_from_user(&idlun, (void *)arg, sizeof(struct scsi_idlun)) !=0) {
			MV_DBG(DMSG_IOCTL, "Marvell : Get Device idlun Error.\n");
			return -EIO;
		}
		
		/*To check host no, if fail, return EFAULT (Bad address) */
		if (hba->host->host_no != ((idlun.dev_id) >> 24)) {
			MV_DBG(DMSG_IOCTL, "Marvell : lookup device host number error .\n");
			return EFAULT;
		}
		return 0;
	}
	

	psptdwb = kmalloc(sptdwb_size, GFP_ATOMIC);
	
	if ( NULL == psptdwb ) 
		return -ENOMEM;

	error = copy_from_user(psptdwb, (void *)arg, sptdwb_size);

	if (error) {
		ret = -EIO;
		goto clean_psp;
	}

	if (psptdwb->sptd.DataTransferLength) {
		if (psptdwb->sptd.DataTransferLength > IOCTL_BUF_LEN) {
			MV_DBG(DMSG_IOCTL, "__MV__ not enough buf space.\n");
			ret = -ENOMEM;
			goto clean_psp;
		}
			
		psptdwb->sptd.DataBuffer = ioctl_buf;
		memset(ioctl_buf, 0, psptdwb->sptd.DataTransferLength);
		
		error = copy_from_user( psptdwb->sptd.DataBuffer,
					((PSCSI_PASS_THROUGH_DIRECT_WITH_BUFFER)arg)->sptd.DataBuffer,
					psptdwb->sptd.DataTransferLength);
		if (error) {
			ret = -EIO;
			goto clean_pspbuf;
		}
	} else {
		psptdwb->sptd.DataBuffer = NULL;
	}
	
	/*Translate request to MV_REQUEST*/
	req = res_get_req_from_pool(hba->req_pool);
	if (NULL == req) {
		ret = -ENOMEM;
		goto clean_pspbuf;
	}

	req->Cmd_Initiator = hba;
	req->Org_Req = req;
	req->Scsi_Status = psptdwb->sptd.ScsiStatus;
	req->Device_Id = psptdwb->sptd.TargetId;
	req->Cmd_Flag = 0;
	req->Req_Type = REQ_TYPE_INTERNAL;

	if (psptdwb->sptd.DataTransferLength == 0) {
		req->Cmd_Flag |= CMD_FLAG_NON_DATA;
	} else {
		if (SCSI_IS_READ(psptdwb->sptd.Cdb[0]))
			req->Cmd_Flag |= CMD_FLAG_DATA_IN;
		if (SCSI_IS_READ(psptdwb->sptd.Cdb[0]) || SCSI_IS_WRITE(psptdwb->sptd.Cdb[0]))
			req->Cmd_Flag |= CMD_FLAG_DMA;
	}

	req->Data_Transfer_Length = psptdwb->sptd.DataTransferLength;
	req->Data_Buffer = psptdwb->sptd.DataBuffer;
	req->Sense_Info_Buffer = psptdwb->Sense_Buffer;

	SGTable_Init(&req->SG_Table, 0);
	
	memcpy(req->Cdb, psptdwb->sptd.Cdb, MAX_CDB_SIZE);
	memset(req->Context, 0, sizeof(MV_PVOID)*MAX_POSSIBLE_MODULE_NUMBER);
	req->LBA.value = 0;    
	req->Sector_Count = 0; 
	req->Completion = ioctlcallback;

	spin_lock_irqsave(&hba->desc->hba_desc->global_lock, flags);
    	hba->Io_Count++;
#if LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 11)
	atomic_set(&hba->hba_sync, 1);
#endif /* LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 11) */
	hba->desc->ops->module_sendrequest(hba->desc->extension, req);
	spin_unlock_irqrestore(&hba->desc->hba_desc->global_lock, flags);

#if LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 11)
	if (!__hba_wait_for_atomic_timeout(&hba->hba_sync, 
					    HBA_REQ_TIMER_IOCTL*HZ)) {
#else
	if (!wait_for_completion_timeout(&hba->cmpl, 
					  HBA_REQ_TIMER_IOCTL*HZ)) {
#endif /* LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 11) */
		MV_DBG(DMSG_HBA, "__MV__ ioctl req timed out.\n");
	        ret = -EIO;
	        goto clean_pspbuf;
	}

	if (psptdwb->sptd.DataTransferLength) {
		error = copy_to_user(((PSCSI_PASS_THROUGH_DIRECT_WITH_BUFFER)arg)->sptd.DataBuffer,
				     psptdwb->sptd.DataBuffer,
				     psptdwb->sptd.DataTransferLength);
		if (error) {
			ret = -EIO;
			goto clean_pspbuf;
		}
	}

	error = copy_to_user((void*)arg, psptdwb, sptdwb_size);
	
	if (error) {
		ret = -EIO;
		goto clean_pspbuf;
	}

	if (req->Scsi_Status)
		ret = req->Scsi_Status;
	
	if (req->Scsi_Status == REQ_STATUS_INVALID_PARAMETER)
		ret = -EPERM;
	
clean_pspbuf:
/* use static buf instead.*/
clean_psp:
	kfree(psptdwb);
	return ret;
}
#else  /* RAID_DRIVER */
inline int mv_register_chdev(struct hba_extension *hba) {return 0;}
inline void mv_unregister_chdev(struct hba_extension *hba) {}
#endif /* RAID_DRIVER */
