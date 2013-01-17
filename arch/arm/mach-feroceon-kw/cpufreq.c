/*
 * arch/arm/mach-kirkwood/cpufreq.c
 *
 * Clock scaling for Kirkwood SoC
 *
 * This file is licensed under the terms of the GNU General Public
 * License version 2. This program is licensed "as is" without any
 * warranty of any kind, whether express or implied.
 */

#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/cpufreq.h>
#include <linux/io.h>
#include "ctrlEnv/mvCtrlEnvLib.h"
#include "cpu/mvCpu.h"


enum kw_cpufreq_range {
	KW_CPUFREQ_LOW 		= 0,
	KW_CPUFREQ_HIGH 	= 1
};

static struct cpufreq_frequency_table kw_freqs[] = {
	{ KW_CPUFREQ_LOW, 0                  },
	{ KW_CPUFREQ_HIGH, 0                  },
	{ 0, CPUFREQ_TABLE_END  }
};


/*
 * Power management function: set or unset powersave mode
 * FIXME: a better place ?
 */
static inline void kw_set_powersave(u8 on)
{
#ifndef MY_ABC_HERE
	printk(KERN_DEBUG "cpufreq: Setting PowerSaveState to %s\n",
		on ? "on" : "off");
#endif 

	if (on)
		mvCtrlPwrSaveOn();
	else
		mvCtrlPwrSaveOff();
}

static int kw_cpufreq_verify(struct cpufreq_policy *policy)
{
	if (unlikely(!cpu_online(policy->cpu)))
		return -ENODEV;

	return cpufreq_frequency_table_verify(policy, kw_freqs);
}

/*
 * Get the current frequency for a given cpu.
 */
static unsigned int kw_cpufreq_get(unsigned int cpu)
{
	unsigned int freq;
	u32 reg;

	if (unlikely(!cpu_online(cpu)))
		return -ENODEV;

	/* To get the current frequency, we have to check if
	* the powersave mode is set. */
	reg = MV_REG_READ(POWER_MNG_CTRL_REG);

	if (reg & PMC_POWERSAVE_EN)
		freq = kw_freqs[KW_CPUFREQ_LOW].frequency;
	else
		freq = kw_freqs[KW_CPUFREQ_HIGH].frequency;

	return freq;
}

/*
 * Set the frequency for a given cpu.
 */
static int kw_cpufreq_target(struct cpufreq_policy *policy,
		unsigned int target_freq, unsigned int relation)
{
	unsigned int index;
	struct cpufreq_freqs freqs;

	if (unlikely(!cpu_online(policy->cpu)))
		return -ENODEV;

	/* Lookup the next frequency */
	if (unlikely(cpufreq_frequency_table_target(policy,
		kw_freqs, target_freq, relation, &index)))
		return -EINVAL;


	freqs.old = policy->cur;
	freqs.new = kw_freqs[index].frequency;
	freqs.cpu = policy->cpu;

#ifndef MY_ABC_HERE 
	printk(KERN_DEBUG "cpufreq: Setting CPU Frequency to %u KHz\n",freqs.new);
#endif

	cpufreq_notify_transition(&freqs, CPUFREQ_PRECHANGE);

	/* Interruptions will be disabled in the low level power mode
	* functions. */
	if (index == KW_CPUFREQ_LOW)
		kw_set_powersave(1);
	else if (index == KW_CPUFREQ_HIGH)
		kw_set_powersave(0);
	else
		return -EINVAL;

	cpufreq_notify_transition(&freqs, CPUFREQ_POSTCHANGE);

	return 0;
}

static int kw_cpufreq_cpu_init(struct cpufreq_policy *policy)
{
	u16 dev_id = mvCtrlModelGet();

	if (unlikely(!cpu_online(policy->cpu)))
		return -ENODEV;

	/*
	 * As 6180 board have different reset sample register values,
	 * the board type must be checked first.
	 */
	if (dev_id == MV_6180_DEV_ID) {

		kw_freqs[KW_CPUFREQ_HIGH].frequency = mvCpuPclkGet()/1000;
		/* At the lowest frequency CPU goes to DDR data clock rate:
		 * (2 x DDR clock rate). */
		kw_freqs[KW_CPUFREQ_LOW].frequency = mvBoardSysClkGet()*2/1000;

	} else if ((dev_id == MV_6281_DEV_ID) ||
		(dev_id == MV_6192_DEV_ID) || (dev_id == MV_6282_DEV_ID) ||
		(dev_id == MV_6702_DEV_ID) || (dev_id == MV_6701_DEV_ID))  {

		kw_freqs[KW_CPUFREQ_HIGH].frequency = mvCpuPclkGet()/1000;
		/* CPU low frequency is the DDR frequency. */
		kw_freqs[KW_CPUFREQ_LOW].frequency  = mvBoardSysClkGet()/1000;

	} else {
		return -ENODEV;
	}

	printk(KERN_DEBUG
		"cpufreq: High frequency: %uKHz - Low frequency: %uKHz\n",
		kw_freqs[KW_CPUFREQ_HIGH].frequency,
		kw_freqs[KW_CPUFREQ_LOW].frequency);

	policy->cpuinfo.transition_latency = 1000000;
	policy->cur = kw_cpufreq_get(0);
	policy->governor = CPUFREQ_DEFAULT_GOVERNOR;

	cpufreq_frequency_table_get_attr(kw_freqs, policy->cpu);

	return cpufreq_frequency_table_cpuinfo(policy, kw_freqs);
}


static int kw_cpufreq_cpu_exit(struct cpufreq_policy *policy)
{
	cpufreq_frequency_table_put_attr(policy->cpu);
	return 0;
}

static struct freq_attr *kw_freq_attr[] = {
	&cpufreq_freq_attr_scaling_available_freqs,
	NULL,
};


static struct cpufreq_driver kw_freq_driver = {
	.owner          = THIS_MODULE,
	.name           = "kw_cpufreq",
	.init           = kw_cpufreq_cpu_init,
	.verify         = kw_cpufreq_verify,
	.exit		= kw_cpufreq_cpu_exit,
	.target         = kw_cpufreq_target,
	.get            = kw_cpufreq_get,
	.attr           = kw_freq_attr,
};


static int __init kw_cpufreq_init(void)
{
	printk(KERN_INFO "cpufreq: Init kirkwood cpufreq driver\n");

	return cpufreq_register_driver(&kw_freq_driver);
}

static void __exit kw_cpufreq_exit(void)
{
	cpufreq_unregister_driver(&kw_freq_driver);
}


MODULE_AUTHOR("Marvell Semiconductors ltd.");
MODULE_DESCRIPTION("CPU frequency scaling for Kirkwood SoC");
MODULE_LICENSE("GPL");
module_init(kw_cpufreq_init);
module_exit(kw_cpufreq_exit);

