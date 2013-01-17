#ifndef __HBA_TIMER_H__
#define __HBA_TIMER_H__

#include "hba_header.h"

#define HBA_REQ_TIMER_AFTER_RESET 30
#define HBA_REQ_TIMER 20 
#define HBA_REQ_TIMER_IOCTL (HBA_REQ_TIMER_AFTER_RESET+3)

#define __hba_init_timer(x) \
	do {\
		init_timer(x);\
		(x)->function = NULL;\
	} while (0)
#define __hba_add_timer(x, time, d, func) \
	do {\
		(x)->expires = jiffies + time;\
		(x)->data = (unsigned long)d;\
		(x)->function = (void (*)(unsigned long))func;\
		add_timer(x);\
	} while (0)
#define __hba_mod_timer(x, time, d, func) \
	do {\
		(x)->expires = jiffies + time;\
		(x)->data = (unsigned long)d;\
		(x)->function = (void (*)(unsigned long))func;\
		mod_timer(x, jiffies + time);\
	} while (0)
#define __hba_del_timer(x) \
	do {\
		if ((x)->function) {\
			del_timer(x);\
			(x)->function = NULL;\
		}\
	} while (0)

enum _tag_hba_msg_state{
	MSG_QUEUE_IDLE=0,
	MSG_QUEUE_PROC
};

struct mv_hba_msg {
	MV_PVOID data;
	MV_U32   msg;
	MV_U32   param;
	struct  list_head msg_list;
};

#define MSG_QUEUE_DEPTH 32

struct mv_hba_msg_queue {
	spinlock_t lock;
	struct list_head free;
	struct list_head tasks;
	struct mv_hba_msg msgs[MSG_QUEUE_DEPTH];
};

enum {
	HBA_TIMER_IDLE = 0,
	HBA_TIMER_RUNNING,
	HBA_TIMER_LEAVING,
};
void hba_house_keeper_init(void);
void hba_house_keeper_run(void);
void hba_house_keeper_exit(void);
void hba_msg_insert(void *data, unsigned int msg, unsigned int param);
void hba_init_timer(PMV_Request req);
void hba_remove_timer(PMV_Request req);
void hba_remove_timer_sync(PMV_Request req);
void hba_add_timer(PMV_Request req, int timeout,
		   MV_VOID (*function)(MV_PVOID data));
void hba_wait_eh(void);

#endif /* __HBA_TIMER_H__ */
