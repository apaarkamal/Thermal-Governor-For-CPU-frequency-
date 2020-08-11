#define pr_fmt(fmt) KBUILD_MODNAME ": " fmt

#include <linux/cpufreq.h>
#include <linux/init.h>
#include <linux/module.h>
#include "../thermal/thermal_core.h"
#include <linux/kthread.h>
#include <linux/sched.h>
#include <linux/delay.h>

#define CPU_TEMP_THRESHOLD 45000
#define SLEEP_TIME 5000
#define SENSOR_ID  8

static struct task_struct *temperature_monitoring_thread;
struct cpufreq_policy* core_policy;
char this_governor[] = "thermohertz";


static void  change_to_max(struct cpufreq_policy *policy) {
	__cpufreq_driver_target(policy, policy->max, CPUFREQ_RELATION_H);
}

static void change_to_min(struct cpufreq_policy  *policy) {
	__cpufreq_driver_target(policy, policy->min, CPUFREQ_RELATION_L);
}

static bool has_expired(struct cpufreq_policy *policy) {
	char * current_governor = policy -> governor -> name;
	int len_this_governor = strlen(this_governor);
	int len_current_governor = strlen(current_governor);
	if (!strncmp(this_governor, current_governor, min(len_this_governor, len_current_governor))) {
		return true;
	}
	else {
		return false;
	}
}

int temperature_monitor(void* data) {
	struct cpufreq_policy *policy = core_policy;
	while (1) {
		if (has_expired(policy)) {
			break;
		}
		int temperature = get_sensor_temp(SENSOR_ID);
		if (temperature >= CPU_TEMP_THRESHOLD) {
			change_to_min(policy);
		}
		else {
			change_to_max(policy);
		}
		msleep(SLEEP_TIME);
	}
	return 1;
}



static void cpufreq_gov_thermohertz_limits(struct cpufreq_policy *policy) {
	core_policy = policy;
	char  our_thread[] = "temperature_monitoring_thread";
	temperature_monitoring_thread = kthread_create(temperature_monitor, NULL, our_thread);
	if ((temperature_monitoring_thread))
	{
		wake_up_process(temperature_monitoring_thread);
	}
}

static struct cpufreq_governor cpufreq_gov_thermohertz = {
	.name		= "thermohertz",
	.limits		= cpufreq_gov_thermohertz_limits,
	.owner		= THIS_MODULE,
};

static int __init cpufreq_gov_thermohertz_init(void) {
	printk(KERN_ALERT "DEBUG: Passed %s %d \n", __FUNCTION__, __LINE__);
	return cpufreq_register_governor(&cpufreq_gov_thermohertz);
}

static void __exit cpufreq_gov_thermohertz_exit(void) {
	printk(KERN_ALERT "DEBUG: Passed %s %d \n", __FUNCTION__, __LINE__);
	cpufreq_unregister_governor(&cpufreq_gov_thermohertz);
}

MODULE_DESCRIPTION("CPUfreq policy governor 'thermohertz'");
MODULE_LICENSE("GPL");

#ifdef CONFIG_CPU_FREQ_DEFAULT_GOV_THERMOHERTZ
struct cpufreq_governor *cpufreq_default_governor(void) {
	printk(KERN_ALERT "DEBUG: Passed %s %d \n", __FUNCTION__, __LINE__);
	return &cpufreq_gov_thermohertz;
}

core_initcall(cpufreq_gov_thermohertz_init);
#else
module_init(cpufreq_gov_thermohertz_init);
#endif
module_exit(cpufreq_gov_thermohertz_exit);
