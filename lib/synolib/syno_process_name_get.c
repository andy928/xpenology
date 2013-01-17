/* Copyright (c) 2000-2008 Synology Inc. All rights reserved. */
#include <linux/sched.h>
#include <linux/module.h>
#include <linux/jiffies.h>
#include <asm/page.h>

#ifdef MY_ABC_HERE

#define PS_BUF_LEN 32
extern int syno_hibernation_log_sec;
unsigned long glast_jiffies;

extern struct mm_struct *syno_get_task_mm(struct task_struct *task);
extern int syno_access_process_vm(struct task_struct *tsk, unsigned long addr, void *buf, int len, int write);
static int SynoCommGet(struct task_struct *task, char *ptr, int length);
static int SynoUserMMNameGet(struct task_struct *task, char *ptr, int length);
static void SynoProcessNameGet(struct task_struct *task, unsigned char kp, char *buf, int buf_size);

void syno_do_hibernation_log(const char __user *filename)
{	
	char p_cups[PS_BUF_LEN], p_ckps[PS_BUF_LEN];	
	unsigned long timeout;

    if(strstr(filename, "/dev") == NULL &&
	   strstr(filename, "/sys") == NULL &&
	   strstr(filename, "/proc") == NULL &&
	   strstr(filename, "/tmp") == NULL) {
		timeout = glast_jiffies + syno_hibernation_log_sec * HZ;
		if(time_after(jiffies, timeout)) {
			SynoProcessNameGet(current, 0, p_cups, PS_BUF_LEN);
			SynoProcessNameGet(current, 1, p_ckps, PS_BUF_LEN);

			printk("[%s] opened by pid %d [u:(%s), comm:(%s)]\n",
				   filename, current->pid, p_cups, p_ckps);
		}
		glast_jiffies = jiffies;
	}
}
EXPORT_SYMBOL(syno_do_hibernation_log);

/**
 * Process name get
 * 
 * @param task     [IN] task structure, use for get process info.
 *                 Should not be NULL
 * @param kp       [IN]
 *                 0: get user mm task name
 *                 1: get task->comm process name
 *                 hould not be NULL.
 * @param buf      [IN] for copy process name, Should not be NULL.
 * @param buf_size [IN] buf size, Should more than 1.
 */
static void SynoProcessNameGet(struct task_struct *task, unsigned char kp, char *buf, int buf_size)
{
	if(0 >= buf_size) {
		goto END;
	}

	memset(buf, 0 ,buf_size);
	if(kp) {
		if(SynoCommGet(task, buf, buf_size) < 0){
			buf[0] = '\0';
			goto END;
		}
	}else{
		if(SynoUserMMNameGet(task, buf, buf_size) < 0){
			buf[0] = '\0';
			goto END;
		}
	}
END:
	return;
}

static int SynoCommGet(struct task_struct *task, char *ptr, int length)
{
	if(!task){
		return -1;
	}

	strlcpy(ptr, task->comm, length);
	return 0;
}

static int SynoUserMMNameGet(struct task_struct *task, char *ptr, int length)
{
	struct mm_struct *mm;
	int len = 0;
	int res = -1;
	int res_len = -1;
	char buffer[256];
	int buf_size = sizeof(buffer);

	if(!task) {
		return -1;
	}

	mm = syno_get_task_mm(task);
	if(!mm) {
		printk("%s %d get_task_mm_syno == NULL \n", __FUNCTION__, __LINE__);
		goto END;
	}
	
	if(!mm->arg_end) {
		printk("!mm->arg_end \n");
		goto END;
	}

	len = mm->arg_end - mm->arg_start;

	if(len <= 0) {
		printk("len <= 0 \n");
		goto END;
	}

	if(len > PAGE_SIZE) {
		len = PAGE_SIZE;
	}

	if(len > buf_size) {
		len = buf_size;
	}
	
	res_len = syno_access_process_vm(task, mm->arg_start, buffer, len, 0);
	if(res_len <= 0) {
		printk("access_process_vm_syno  fail\n");
		goto END;
	}
	
	if(res_len >= buf_size) {
		res_len = buf_size-1;
	}
	buffer[res_len] = '\0';
	strlcpy(ptr, buffer, length);

	res = 0;	
END:
	if(mm) {
		mmput(mm);
	}
	return res;
}
#endif //MY_ABC_HERE
