// SPDX-License-Identifier: GPL-2.0-only
/*
 * Copyright (C) 2012 Regents of the University of California
 */

#include <linux/reboot.h>
#include <linux/pm.h>
#include <asm/sbi.h>

static void default_power_off(void)
{
	sbi_shutdown();
	while (1);
}

void (*pm_power_off)(void) = default_power_off;
EXPORT_SYMBOL(pm_power_off);

void machine_restart(char *cmd)
{
	do_kernel_restart(cmd);
	while (1);
}

void machine_halt(void)
{
	pm_power_off();
}

void machine_power_off(void)
{
	pm_power_off();
}

void machine_shutdown(void)
{
}

static struct notifier_block sbi_srst_reboot_nb;
 void __init sbi_init(void)
{
	pm_power_off = sbi_srst_power_off;
	sbi_srst_reboot_nb.notifier_call = sbi_srst_reboot;
	sbi_srst_reboot_nb.priority = 192;
	register_restart_handler(&sbi_srst_reboot_nb);
}
