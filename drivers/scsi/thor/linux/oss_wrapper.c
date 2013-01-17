#include "hba_header.h"

MV_U8 ossw_add_timer(struct timer_list *timer,
		    u32 msec, 
		    void (*function)(unsigned long),
		    unsigned long data)
{
	u64 jif;

	if(timer_pending(timer))
		del_timer(timer);

	timer->function = function;
	timer->data     = data;

	jif = (u64) (msec * HZ);
	do_div(jif, 1000);         /* wait in unit of second */
	timer->expires = jiffies + 1 + jif;

	add_timer(timer);
	return	0;
}

void ossw_del_timer(struct timer_list *timer)
{
	if (timer->function)
		del_timer(timer);
	timer->function = NULL;
}

u32 ossw_get_time_in_sec(void)
{
	struct timeval tv;

	do_gettimeofday(&tv);
	return (u32) tv.tv_sec;
}

u32 ossw_get_msec_of_time(void)
{
	struct timeval tv;

	do_gettimeofday(&tv);
	return (u32) tv.tv_usec*1000*1000;
}

#if __LEGACY_OSSW__
//reset nmi watchdog before delay
static void mv_touch_nmi_watchdog(void)
{
#ifdef CONFIG_X86_64
	touch_nmi_watchdog();
#endif /* CONFIG_X86_64*/

#if LINUX_VERSION_CODE > KERNEL_VERSION(2, 6, 13)
	touch_softlockup_watchdog();
#endif /* LINUX_VERSION_CODE > KERNEL_VERSION(2, 6, 13) */
}	

void HBA_SleepMillisecond(MV_PVOID ext, MV_U32 msec)
{
	MV_U32	tmp=0;
	MV_U32	mod_msec=2000;
	if (in_softirq()||in_interrupt() || irqs_disabled()){
		mv_touch_nmi_watchdog();
		if (msec<=mod_msec)
			ossw_mdelay(msec);
		else
		{
			for(tmp=0;tmp<msec/mod_msec;tmp++)
			{
				ossw_mdelay(mod_msec);
				mv_touch_nmi_watchdog();
			}
			if (msec%mod_msec)
				ossw_mdelay(msec%mod_msec);
		}
		mv_touch_nmi_watchdog();
	}else {
		set_current_state(TASK_UNINTERRUPTIBLE);
		schedule_timeout( msecs_to_jiffies(msec) );
	}
}


void hba_msleep(MV_U32 msec)
{
	MV_U32	tmp=0;
	MV_U32	mod_msec=2000;
	if (in_softirq()||in_interrupt() || irqs_disabled()){
		mv_touch_nmi_watchdog();
		if (msec<=mod_msec)
			ossw_mdelay(msec);
		else
		{
			for(tmp=0;tmp<msec/mod_msec;tmp++)
			{
				ossw_mdelay(mod_msec);
				mv_touch_nmi_watchdog();
			}
			if (msec%mod_msec)
				ossw_mdelay(msec%mod_msec);
		}
		mv_touch_nmi_watchdog();
	}else {
		set_current_state(TASK_UNINTERRUPTIBLE);
		schedule_timeout( msecs_to_jiffies(msec));
	}
}

#endif

