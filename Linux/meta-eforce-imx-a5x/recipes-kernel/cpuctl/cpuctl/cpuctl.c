/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * cpuctl client driver 
 *
 * Copyright (c) 2021-2025, eForce Co., Ltd.
 *
 ***************************************************************************
 * @par     History
 *          - rev 1.0 (2021.05.07) yokota
 *            First release
 *          - rev 1.1 (2021.10.28) yokota
 *            update for kernel 5.10
 *          - rev 1.2 (2023.01.27) ogino 
 *            Supported GIC/GICv2.
 *          - rev 1.3 (2025.01.20)
 *            Enable USE_ARM_GIC_V3 macro for iMX8M Plus.
***************************************************************************
 */

#include <linux/init.h>
#include <linux/module.h>
#include <linux/printk.h>
#include <linux/errno.h>
#include <asm/io.h>

#define USE_ARM_GIC_V3

#ifdef USE_ARM_GIC_V3
#include <linux/psci.h>
#include <linux/cpu.h>
#endif /*USE_ARM_GIC_V3*/

#ifndef USE_ARM_GIC_V3
#define GIC_DIST_BASE
#define GIC_DIST_SOFTINT
static void __iomem *reg;
#endif /*USE_ARM_GIC_V3*/

static struct kobject *cpuctl_kobj;

static int __init cpuctl_init(void);
static void __exit cpuctl_exit(void);

#ifdef USE_ARM_GIC_V3
unsigned long long get_mpidr(void)
{
    unsigned long long val;
    asm volatile( "mrs %0, MPIDR_EL1" : "=r"(val) );
    asm volatile( "isb" );
    return val;
}

#if 0
static int cpu_boot(unsigned int cpu)
{
	int err = psci_ops.cpu_on(cpu_logical_map(cpu), target_start_address);
	if (err)
    printk(KERN_ERR "failed to boot CPU%d (%d)\n", cpu, err);
	return err;
}
#endif /* #if 0 */

#endif /* USE_ARM_GIC_V3 */


/* GIC/GICV2 version's sendsgi_show is not used in the RPMSG sample program.
 * but it remains because we don't test behavior when the .show function is NULL.
 */
static ssize_t sendsgi_show(struct kobject *kobj, struct kobj_attribute *attr, char *buf)
{
#ifdef USE_ARM_GIC_V3
	unsigned long long mpidr = (get_mpidr() & (0x000000FF00FFFFFFULL));
	unsigned long long cluster_path = (
            ((mpidr >> 32) << 48) | /* Aff3 */
            ((mpidr >> 16) << 32) | /* Aff2 */
            ((mpidr >> 8) << 16));  /* Aff1 */
	unsigned long long val = 0;
	return sprintf(buf, "%16llx\n", val);
#else /* USE_ARM_GIC_V3 */
	int val = 0;
	return sprintf(buf, "%x\n", val);
#endif /* USE_ARM_GIC_V3 */
}

static ssize_t sendsgi_store(struct kobject *kobj, struct kobj_attribute *attr, const char *buf, size_t count)
{
#ifdef USE_ARM_GIC_V3
	unsigned long long mpidr = (get_mpidr() & (0x000000FF00FFFFFFULL));
	unsigned long long cluster_path = (
            ((mpidr >> 32) << 48) | /* Aff3 */
            ((mpidr >> 16) << 32) | /* Aff2 */
            ((mpidr >> 8) << 16));  /* Aff1 */
	unsigned long long val = 0; 
	sscanf(buf, "%16llx", &val);
	val &= 0x00000000FF00FFFFULL;
	val |= cluster_path;
	asm volatile ("msr S3_0_C12_C11_5, %0" : "+r"((u64)val));
#else /*USE_ARM_GIC_V3*/
	unsigned int val;

	sscanf(buf,"%x", &val);
	writel_relaxed(val, reg + GIC_DIST_SOFTINT);
#endif /*USE_ARM_GIC_V3*/
	return count;
}

#ifdef USE_ARM_GIC_V3
static ssize_t start_address_show(struct kobject *kobj, struct kobj_attribute *attr, char *buf)
{
    return 0;
}

static ssize_t start_address_store(struct kobject *kobj, struct kobj_attribute *attr, const char *buf, size_t count)
{
    return count;
}

static ssize_t stopcpu_store(struct kobject *kobj, struct kobj_attribute *attr, const char *buf, size_t count)
{
    remove_cpu(1);
    return count;
}

static ssize_t startcpu_store(struct kobject *kobj, struct kobj_attribute *attr, const char *buf, size_t count)
{
    add_cpu(1);
    return count;
}
#endif /*USE_ARM_GIC_V3*/

static struct kobj_attribute sendsgi_attr   = __ATTR(sendsgi, 0644, sendsgi_show, sendsgi_store);
#ifdef USE_ARM_GIC_V3
static struct kobj_attribute start_address_attr = __ATTR(start_address, 0644, start_address_show, start_address_store);
static struct kobj_attribute stop_cpu_attr      = __ATTR(stopcpu, 0644, NULL, stopcpu_store);
static struct kobj_attribute start_cpu_attr     = __ATTR(startcpu, 0644, NULL, startcpu_store);
#endif /*USE_ARM_GIC_V3*/

// Driver initialization
static int cpuctl_init(void)
{
	int ret;
	
	cpuctl_kobj = kobject_create_and_add("cpuctl", kernel_kobj);
	ret = sysfs_create_file(cpuctl_kobj, &sendsgi_attr.attr);
#ifdef USE_ARM_GIC_V3
	ret = sysfs_create_file(cpuctl_kobj, &start_address_attr.attr);
	ret = sysfs_create_file(cpuctl_kobj, &stop_cpu_attr.attr);
	ret = sysfs_create_file(cpuctl_kobj, &start_cpu_attr.attr);
#else /*USE_ARM_GIC_V3*/
	reg = ioremap(GIC_DIST_BASE , 4);
#endif /*USE_ARM_GIC_V3*/
	printk(KERN_INFO "cpuctl initialized\n");
	
	return ret;
}

// Driver finalization
static void cpuctl_exit(void)
{
#ifndef USE_ARM_GIC_V3
	iounmap(reg);
#endif /*USE_ARM_GIC_V3*/
	kobject_put(cpuctl_kobj);
	printk(KERN_INFO "cpuctl finalized\n");
}

module_init(cpuctl_init);
module_exit(cpuctl_exit);

MODULE_LICENSE("GPL v2");
MODULE_AUTHOR("Takahisa Yokota<yokota@eforce.co.jp>");
MODULE_DESCRIPTION("cpu ctl driver");
