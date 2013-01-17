// Copyright (c) 2000-2008 Synology Inc. All rights reserved.
#include <linux/synolib.h>
#include <linux/module.h>

/**
 * this function is the crond kernel code.
 * 
 * it's used for schedul job to execute
 * 
 * @param work   Should not be NULL. this parm is passing in by kernel code.
 */
static void _SynoTimePeriodHandler(struct work_struct *work)
{
	SYNOASYNCOPERATION	*pSynoAsyncOp = container_of(work, SYNOASYNCOPERATION, sched_work.work);
	unsigned long flags;

	if(!pSynoAsyncOp) {
		printk("%s (%s)-%d pSynoAsyncOp == NULL\n", __FILE__, __FUNCTION__, __LINE__);
		return;
	}

	spin_lock_irqsave(&pSynoAsyncOp->syno_async_lock, flags);
	if(!pSynoAsyncOp->stopAsyncOperation) {		
		/*
		* calling user job
		*/		
		pSynoAsyncOp->period_func(pSynoAsyncOp->period_func_data);

		/*
		* re-schedule
		*/
		if(NULL != pSynoAsyncOp->pwq){
			queue_delayed_work(pSynoAsyncOp->pwq, &pSynoAsyncOp->sched_work, pSynoAsyncOp->period_in_sec*HZ);
		}else{
			schedule_delayed_work(&pSynoAsyncOp->sched_work, pSynoAsyncOp->period_in_sec*HZ);
		}
	}
	spin_unlock_irqrestore(&pSynoAsyncOp->syno_async_lock, flags);
}

static int _SynoAsyncOperationInitParmValidation(SYNOASYNCOPERATION *pSynoAsyncOp,
										  void (*period_func)(void *data), unsigned long period_in_sec)
{
	int res = -1;
	if(!pSynoAsyncOp){
		printk("pSynoAsyncOp can't be NULL 1");
		goto END;
	}

	if(period_in_sec <= 0){
		printk("period_in_sec must > 1");
		goto END;
	}

	if(!period_func) {
		printk("period_func can't be NULL 1");
		goto END;
	}

	res = 0;
END:
	return res;
}

/**
 * Micro kernel crontab initialize function
 * 
 * @param pSynoAsyncOp
 *                   [IN] Please malloc first. Should not be NULL
 * @param pWorkQueue [IN] Please init it before pass in.
 *                   Should not be NULL
 * @param period_func
 *                   [IN] Schedule function, Should not be NULL.
 * @param period_in_sec
 *                   [IN] Scheduled period time.
 *                   Should more than 1 less than 1 week.
 * 
 * @return 0: on success
 *         -1: fail, set SynoDebugFlag=1 for
 *         more information
 */
int SynoAsyncOperationInit(SYNOASYNCOPERATION *pSynoAsyncOp, struct workqueue_struct *pWorkQueue,
							 void (*period_func)(void *data), void *user_data, unsigned long period_in_sec)
{
	int res = -1;

	if(_SynoAsyncOperationInitParmValidation(pSynoAsyncOp, 
											 period_func, period_in_sec) < 0) {
		goto END;
	}

    spin_lock_init(&pSynoAsyncOp->syno_async_lock);

	/*
	* Setting user's period task
	*/
	pSynoAsyncOp->period_func = period_func;
	pSynoAsyncOp->period_func_data = user_data;
	pSynoAsyncOp->period_in_sec = period_in_sec;
	pSynoAsyncOp->stopAsyncOperation = 0;

	/*
	* workqueue setting
	*/
	if(NULL != pWorkQueue) {
		pSynoAsyncOp->pwq = pWorkQueue;
	}else{
		pSynoAsyncOp->pwq = NULL;
	}
	INIT_DELAYED_WORK(&pSynoAsyncOp->sched_work, _SynoTimePeriodHandler);

	if(NULL != pWorkQueue) {
		queue_delayed_work(pSynoAsyncOp->pwq, &pSynoAsyncOp->sched_work, HZ);		
	}else{
		schedule_delayed_work(&pSynoAsyncOp->sched_work, HZ);		
	}
	res = 0;
END:
	return res;
}

void SynoAsyncOperationPause(SYNOASYNCOPERATION *pSynoAsyncOp)
{
	unsigned long flags;

	spin_lock_irqsave(&pSynoAsyncOp->syno_async_lock, flags);

	pSynoAsyncOp->stopAsyncOperation = 1;	
	if(pSynoAsyncOp) {
		cancel_delayed_work(&pSynoAsyncOp->sched_work);		
	}

	spin_unlock_irqrestore(&pSynoAsyncOp->syno_async_lock, flags);
}

void SynoAsyncOperationResume(SYNOASYNCOPERATION *pSynoAsyncOp)
{
	unsigned long flags;

	spin_lock_irqsave(&pSynoAsyncOp->syno_async_lock, flags);

	pSynoAsyncOp->stopAsyncOperation = 0;
	if(NULL != pSynoAsyncOp->pwq) {
		queue_delayed_work(pSynoAsyncOp->pwq, &pSynoAsyncOp->sched_work, HZ);		
	}else{
		schedule_delayed_work(&pSynoAsyncOp->sched_work, HZ);		
	}

	spin_unlock_irqrestore(&pSynoAsyncOp->syno_async_lock, flags);
}

void SynoAsyncOperationModifyPeriod(SYNOASYNCOPERATION *pSynoAsyncOp, unsigned long period_in_sec)
{
	unsigned long flags;	

	spin_lock_irqsave(&pSynoAsyncOp->syno_async_lock, flags);

	pSynoAsyncOp->period_in_sec = period_in_sec;

	spin_unlock_irqrestore(&pSynoAsyncOp->syno_async_lock, flags);
}

void SynoAsyncOperationCleanUp(SYNOASYNCOPERATION *pSynoAsyncOp)
{
	unsigned long flags;

	spin_lock_irqsave(&pSynoAsyncOp->syno_async_lock, flags);

	pSynoAsyncOp->stopAsyncOperation = 1;	
	if(pSynoAsyncOp) {
		cancel_delayed_work_sync(&pSynoAsyncOp->sched_work);
	}
	
	spin_unlock_irqrestore(&pSynoAsyncOp->syno_async_lock, flags);
}

asmlinkage int SynoPrintk(u8 direct_print, const char *fmt, ...)
{
	va_list args;
	int r = 0;

	if(direct_print) {
		va_start(args, fmt);
		r = vprintk(fmt, args);
		va_end(args);
		goto END;
	}else{
#ifdef  MY_ABC_HERE
		if(syno_temperature_debug) {
			va_start(args, fmt);
			r = vprintk(fmt, args);
			va_end(args);
			goto END;
		}
#endif
	}
END:
	return r;
}


struct workqueue_struct *SynoCreateWorkqueue(const char *name){
	return create_workqueue(name);
}

void SynoDestroyWorkqueue(struct workqueue_struct *wq){
	destroy_workqueue(wq);
}

EXPORT_SYMBOL(SynoPrintk);
EXPORT_SYMBOL(SynoAsyncOperationPause);
EXPORT_SYMBOL(SynoAsyncOperationResume);
EXPORT_SYMBOL(SynoAsyncOperationModifyPeriod);
EXPORT_SYMBOL(SynoAsyncOperationInit);
EXPORT_SYMBOL(SynoAsyncOperationCleanUp);
EXPORT_SYMBOL(SynoCreateWorkqueue);
EXPORT_SYMBOL(SynoDestroyWorkqueue);
