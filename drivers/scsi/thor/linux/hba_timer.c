#include "hba_header.h"
#include "core_exp.h"

/* how long a time between which should each keeper work be done */
#define KEEPER_SHIFT (HZ >> 1)

static struct mv_hba_msg_queue mv_msg_queue;

#ifndef SUPPORT_WORKQUEUE
static struct task_struct *house_keeper_task;
#endif

static int __msg_queue_state;

static inline int queue_state_get(void)
{
	return __msg_queue_state;
}

static inline void queue_state_set(int state)
{
	__msg_queue_state = state;
}

static void hba_proc_msg(struct mv_hba_msg *pmsg)
{
	PHBA_Extension phba;
	struct scsi_device *psdev;

	/* we don't do things without pmsg->data */
	if (NULL == pmsg->data)
		return;

	phba = (PHBA_Extension) pmsg->data;

	
	MV_DBG(DMSG_HBA, "__MV__ In hba_proc_msg.\n");

	MV_ASSERT(pmsg);

	switch (pmsg->msg) {
	case EVENT_DEVICE_ARRIVAL:
		if (scsi_add_device(phba->host, 0, pmsg->param, 0))
			MV_DBG(DMSG_SCSI, 
			       "__MV__ add scsi disk %d-%d-%d failed.\n",
			       0, pmsg->param, 0);
		else
			MV_DBG(DMSG_SCSI,
			       "__MV__ add scsi disk %d-%d-%d.\n",
			       0, pmsg->param, 0);
		break;
	case EVENT_DEVICE_REMOVAL:
		psdev = scsi_device_lookup(phba->host, 0, pmsg->param, 0);

		if (NULL != psdev) {
			MV_DBG(DMSG_SCSI, 
			       "__MV__ remove scsi disk %d-%d-%d.\n",
			       0, pmsg->param, 0);
			scsi_remove_device(psdev);
			scsi_device_put(psdev);
		} else {
			MV_DBG(DMSG_SCSI,
			       "__MV__ no disk to remove %d-%d-%d\n",
			       0, pmsg->param, 0);
		}
		break;
	case EVENT_HOT_PLUG:
		sata_hotplug(pmsg->data, pmsg->param);
		break;
	default:
		break;
	}
}

static void mv_proc_queue(void)
{
	struct mv_hba_msg *pmsg;
	
	/* work on queue non-stop, pre-empty me! */
	queue_state_set(MSG_QUEUE_PROC);

	while (1) {
		MV_DBG(DMSG_HBA, "__MV__ process queue starts.\n");
		spin_lock_irq(&mv_msg_queue.lock);
		if (list_empty(&mv_msg_queue.tasks)) {
			/* it's important we put queue_state_set here. */
			queue_state_set(MSG_QUEUE_IDLE);
			spin_unlock_irq(&mv_msg_queue.lock);
			MV_DBG(DMSG_HBA, "__MV__ process queue ends.\n");
			break;
		}
		pmsg = list_entry(mv_msg_queue.tasks.next, struct mv_hba_msg, msg_list);
		spin_unlock_irq(&mv_msg_queue.lock);

		hba_proc_msg(pmsg);
		
		/* clean the pmsg before returning it to free?*/
		pmsg->data = NULL;
		spin_lock_irq(&mv_msg_queue.lock);
		list_move_tail(&pmsg->msg_list, &(mv_msg_queue.free));
		spin_unlock_irq(&mv_msg_queue.lock);
		MV_DBG(DMSG_HBA, "__MV__ process queue ends.\n");
	}

}

static inline MV_U32 hba_msg_queue_empty(void)
{
	return list_empty(&(mv_msg_queue.tasks));
}

#ifndef SUPPORT_WORKQUEUE
static int hba_house_keeper(void *data)
{
	set_user_nice(current, -15);
	
	set_current_state(TASK_INTERRUPTIBLE);
	while (!kthread_should_stop()) {

		if (!hba_msg_queue_empty() && 
		    MSG_QUEUE_IDLE == queue_state_get()) {
			set_current_state(TASK_RUNNING);
			mv_proc_queue();
		} else {
			schedule();
			set_current_state(TASK_INTERRUPTIBLE);
		}
	}
	
	return 0;
}

#endif

#ifdef SUPPORT_WORKQUEUE

#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,20)
static void mv_wq_handler(void *work)
#else
static void mv_wq_handler(struct work_struct *work)
#endif
{
	if (hba_msg_queue_empty() ){
		MV_DBG(DMSG_KERN,"__MV__  msg queue is empty.\n");
		return;
	}
	else if (!hba_msg_queue_empty() && 
	    MSG_QUEUE_IDLE == queue_state_get()) {
	    	MV_DBG(DMSG_KERN,"__MV__  msg queue isn't empty.\n");
		mv_proc_queue();
	}
}


#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,20)
static DECLARE_WORK(mv_wq, mv_wq_handler,NULL);
#else
static DECLARE_WORK(mv_wq, mv_wq_handler);
#endif

#endif /* SUPPORT_WORKQUEUE */



static void hba_msg_queue_init(void)
{
	int i;
	
	spin_lock_init(&mv_msg_queue.lock);

/* as we're in init, there should be no need to hold the spinlock*/
	INIT_LIST_HEAD(&(mv_msg_queue.free));
	INIT_LIST_HEAD(&(mv_msg_queue.tasks));

	for (i = 0; i < MSG_QUEUE_DEPTH; i++) {
		list_add_tail(&mv_msg_queue.msgs[i].msg_list, 
			      &mv_msg_queue.free);
	}
	
}


void hba_house_keeper_init(void)
{
	hba_msg_queue_init();

	queue_state_set(MSG_QUEUE_IDLE);
#ifndef SUPPORT_WORKQUEUE
	house_keeper_task = kthread_run(hba_house_keeper, NULL, "399B4F5");

	if (IS_ERR(house_keeper_task)) {
		printk("Error creating kthread, out of memory?\n");
		house_keeper_task = NULL;
	}
#endif
}

void hba_house_keeper_run(void)
{
	/* hey hey my my */
}

void hba_house_keeper_exit(void)
{
#ifndef SUPPORT_WORKQUEUE
	if (house_keeper_task)
		kthread_stop(house_keeper_task);
	house_keeper_task = NULL;
#else
	flush_scheduled_work();
#endif
}

void hba_wait_eh()
{
	while (!hba_msg_queue_empty()) {
		schedule();
	}
}


void hba_msg_insert(void *data, unsigned int msg, unsigned int param)
{
	struct mv_hba_msg *pmsg;
	unsigned long flags;

	MV_DBG(DMSG_HBA, "__MV__ msg insert  %d.\n", msg);

	spin_lock_irqsave(&mv_msg_queue.lock, flags);
	if (list_empty(&mv_msg_queue.free)) {
		/* should wreck some havoc ...*/
		MV_DBG(DMSG_HBA, "-- MV -- Message queue is full.\n");
		spin_unlock_irqrestore(&mv_msg_queue.lock, flags);
		return;
	}

	pmsg = list_entry(mv_msg_queue.free.next, struct mv_hba_msg, msg_list);
	pmsg->data = data;
	pmsg->msg  = msg;

	switch (msg) {
	case EVENT_DEVICE_REMOVAL:
	case EVENT_DEVICE_ARRIVAL:
		pmsg->param = param;
		break;
	default:
		pmsg->param = param;
                /*(NULL==param)?0:*((unsigned int*) param);*/
		break;
	}

	list_move_tail(&pmsg->msg_list, &mv_msg_queue.tasks);
	spin_unlock_irqrestore(&mv_msg_queue.lock, flags);

#ifndef SUPPORT_WORKQUEUE
	if (house_keeper_task)
		wake_up_process(house_keeper_task);
#else
	schedule_work(&mv_wq);
#endif

}
#if 1//def REQUEST_TIMER
void hba_remove_timer(PMV_Request req)
{
	/* should be using del_timer_sync, but ... HBA->lock ... */
	if ( req->eh_timeout.function ) {
		del_timer(&req->eh_timeout);
		req->eh_timeout.function = NULL;
#ifdef SUPPORT_REQUEST_TIMER
		req->err_request_ctx = NULL;
#endif
	}
}

/* for req */
void hba_add_timer(PMV_Request req, int timeout,
		   MV_VOID (*function)(MV_PVOID data))
{
	WARN_ON(timer_pending(&req->eh_timeout)); 
	hba_remove_timer(req);
	req->eh_timeout.data = (unsigned long)req;
	/* timeout is in unit of second */
	req->eh_timeout.expires = jiffies + timeout * HZ;
	req->eh_timeout.function = (void (*)(unsigned long)) function;

	add_timer(&req->eh_timeout);
	return;
}
	

void hba_remove_timer_sync(PMV_Request req)
{
	/* should be using del_timer_sync, but ... HBA->lock ... */
	if ( req->eh_timeout.function ) {
		del_timer_sync(&req->eh_timeout);
		req->eh_timeout.function = NULL;
#ifdef SUPPORT_REQUEST_TIMER
		req->err_request_ctx = NULL;
#endif
	}
}
void hba_init_timer(PMV_Request req)
{
	/*
	 * as we have no init routine for req, we'll do init_timer every 
	 * time it is used until we could uniformly init. all reqs
	 */
	req->eh_timeout.function = NULL;
	init_timer(&req->eh_timeout);
}
#endif

