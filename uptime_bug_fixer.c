#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/kprobes.h>
#include <linux/ktime.h>
#include <linux/limits.h>
#include <linux/sched.h>

static char func_name[NAME_MAX] = "scale_rt_power";
module_param_string(func, func_name, NAME_MAX, S_IRUGO);
MODULE_PARM_DESC(func, "Kretprobe that should fix negative scale_rt_powers");

struct my_data {
	ktime_t entry_stamp;
};

static int ret_handler(struct kretprobe_instance *ri, struct pt_regs *regs)
{
	u64 retval = regs_return_value(regs);

#ifdef CONFIG_X86
	if ((long) retval <= 0) {
		regs->ax = 0;
		WARN_ONCE(1, "%s returned %llu and we fixed it to 0\n",
				func_name, retval);
	} 
#endif

	return 0;
}

static struct kretprobe my_kretprobe = {
	.handler		= ret_handler,
	.maxactive		= -1,
};

static int __init kretprobe_init(void)
{
	int ret;

	my_kretprobe.kp.symbol_name = func_name;
	ret = register_kretprobe(&my_kretprobe);
	if (ret < 0) {
		printk(KERN_INFO "register_kretprobe failed, returned %d\n",
				ret);
		return -1;
	}

	mark_tsc_unstable("kernel uptime bug");

	printk(KERN_INFO "Planted return probe at %s: %p\n",
			my_kretprobe.kp.symbol_name, my_kretprobe.kp.addr);

	return 0;
}

static void __exit kretprobe_exit(void)
{
	unregister_kretprobe(&my_kretprobe);
	printk(KERN_INFO "kretprobe at %p unregistered\n",
			my_kretprobe.kp.addr);
}

module_init(kretprobe_init)
module_exit(kretprobe_exit)
MODULE_LICENSE("GPL");
MODULE_AUTHOR("Iliya Polihronov");

