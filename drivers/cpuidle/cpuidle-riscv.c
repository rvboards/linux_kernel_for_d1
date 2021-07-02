// SPDX-License-Identifier: GPL-2.0-only
/*
 *
 * RISCV generic CPU idle driver.
 *
 * Copyright (C) 2020 ALLWINNERTECH.
 * Author: liush <liush@allwinnertech.com>
 */

#include <linux/cpuidle.h>
#include <linux/cpumask.h>
#include <linux/cpu_pm.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/slab.h>
#include <linux/platform_device.h>
#include <asm/cpuidle.h>
#include <asm/sbi.h>

#include "dt_idle_states.h"

#define MAX_IDLE_STATES	2

static int riscv_low_level_suspend_enter(u32 state)
{
	sbi_suspend(state);
	return 0;
}

/* Actual code that puts the SoC in different idle states */
static int riscv_enter_idle(struct cpuidle_device *dev,
			struct cpuidle_driver *drv,
			       int index)
{
	int state = 1;
	return CPU_PM_CPU_IDLE_ENTER_PARAM(riscv_low_level_suspend_enter,
					   index, state);
}

static struct cpuidle_driver riscv_idle_driver = {
	.name			= "riscv_idle",
	.owner			= THIS_MODULE,
	.states[0]		= {
		.enter			= riscv_enter_idle,
		.exit_latency		= 1,
		.target_residency	= 1,
		.name			= "WFI",
		.desc			= "RISCV WFI",
	},
	.state_count = MAX_IDLE_STATES,
};

static const struct of_device_id riscv_idle_state_match[] __initconst = {
	{ .compatible = "riscv,idle-state",
	  .data = riscv_enter_idle},
	{ },
};

/* Initialize CPU idle by registering the idle states */
static int riscv_cpuidle_probe(struct platform_device *dev)
{
	int ret;
	struct cpuidle_driver *drv;

	drv = kmemdup(&riscv_idle_driver, sizeof(*drv), GFP_KERNEL);
	if (!drv)
		return -ENOMEM;
	drv->cpumask = (struct cpumask *)cpumask_of(0);

	ret = dt_init_idle_driver(drv, riscv_idle_state_match, 1);
	if (ret <= 0) {
		ret = ret ? : -ENODEV;
		goto out_kfree_drv;
	}

	ret = cpuidle_register(drv, NULL);
	if (ret)
		goto out_kfree_drv;

	return 0;

out_kfree_drv:
	kfree(drv);
	return ret;
}

const static struct of_device_id riscv_idle_device_ids[] = {
	{.compatible = "riscv,idle"},
	{},
};

static struct platform_driver riscv_cpuidle_driver = {
	.driver = {
		.name = "idle",
		.of_match_table = riscv_idle_device_ids,
	},
	.probe = riscv_cpuidle_probe,
};
builtin_platform_driver(riscv_cpuidle_driver);
