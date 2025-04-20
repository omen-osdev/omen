#include <cpuid.h> //TODO: Implement this
#include <omen/hal/arch/x86/getcpuid.h>
#include <omen/apps/panic/panic.h>
#include <omen/apps/debug/debug.h>

#define CPUID_GETMODEL(cpuinfo) ((cpuinfo[0] & 0xf0) >> 4)
#define CPUID_GETAPIC(cpuinfo) ((cpuinfo[3] & (1 << 9)) != 0)
#define CPUID_GETMSR(cpuinfo) ((cpuinfo[3] & (1 << 5)) != 0)

/* Example: Get CPU's model number */
int get_model(void)
{
    int cpuinfo[4];
    __cpuid(1, cpuinfo[0], cpuinfo[1], cpuinfo[2], cpuinfo[3]);
    return CPUID_GETMODEL(cpuinfo);
}

/* Example: Get APIC capabilities */
int check_apic(void)
{
    int cpuinfo[4];
    __cpuid(1, cpuinfo[0], cpuinfo[1], cpuinfo[2], cpuinfo[3]);
    return (CPUID_GETAPIC(cpuinfo));
}

int check_msr(void)
{
    int cpuinfo[4];
    __cpuid(1, cpuinfo[0], cpuinfo[1], cpuinfo[2], cpuinfo[3]);
    return (CPUID_GETMSR(cpuinfo));
}

int get_maxphyaddr(void)
{
    int cpuinfo[4];
    __cpuid(0x80000008, cpuinfo[0], cpuinfo[1], cpuinfo[2], cpuinfo[3]);
    return (cpuinfo[0] & 0xff);
}

int get_cpu_vendor(void)
{
    int cpuinfo[4];
    __cpuid(0, cpuinfo[0], cpuinfo[1], cpuinfo[2], cpuinfo[3]);
    return (cpuinfo[1]);
}

/*
https://github.com/torvalds/linux/commit/e9ea1e7f53b852147cbd568b0568c7ad97ec21a3
code from linux kernel, in the future might be cool to have

static void set_cpuid_faulting(unsigned int on)
{
	unsigned long long msrval;

	msrval = this_cpu_read(msr_misc_features_shadow);
	msrval &= ~MSR_MISC_FEATURES_ENABLES_CPUID_FAULT;
	msrval |= (on << MSR_MISC_FEATURES_ENABLES_CPUID_FAULT_BIT);
	this_cpu_write(msr_misc_features_shadow, msrval);
	wrmsrl(MSR_MISC_FEATURES_ENABLES, msrval);
}

static void disable_cpuid(void)
{
	preempt_disable();
	if (!test_and_set_thread_flag(TIF_NOCPUID)) {

		set_cpuid_faulting(true);
	}
	preempt_enable();
}

static void enable_cpuid(void)
{
	preempt_disable();
	if (test_and_clear_thread_flag(TIF_NOCPUID)) {

		set_cpuid_faulting(false);
	}
	preempt_enable();
}

*/