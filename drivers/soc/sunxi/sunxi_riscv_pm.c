/*
 * drivers/soc/sunxi_riscv_pm.c
 *
 * Copyright(c) 2019-2020 Allwinnertech Co., Ltd.
 *         http://www.allwinnertech.com
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#include <linux/module.h>
#include <linux/suspend.h>
#include <asm/sbi.h>

static int sunxi_riscv_enter(suspend_state_t state)
{
	sbi_suspend(0);

	return 0;
}

static struct platform_suspend_ops sunxi_riscv_pm = {
	.valid = suspend_valid_only_mem,
	.enter = sunxi_riscv_enter,
};

int sunxi_riscv_pm_register(void)
{
	suspend_set_ops(&sunxi_riscv_pm);

	return 0;
}

void sunxi_riscv_pm_unregister(void)
{
}

module_init(sunxi_riscv_pm_register);
module_exit(sunxi_riscv_pm_unregister);
