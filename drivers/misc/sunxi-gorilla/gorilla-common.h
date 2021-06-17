/*
 * sunxi gorilla common functions
 *
 * Copyright (c) 2020, Martin <wuyan@allwinnertech.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 */
#include <linux/delay.h>
#include <linux/err.h>
#include <linux/init.h>
#include <linux/io.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/of_device.h>
#include <linux/platform_device.h>
#include <linux/types.h>
#include <linux/delay.h>

static noinline int write_paddr(struct device *dev, u64 paddr, u32 val)
{
	void __iomem *vaddr;

	dev_dbg(dev, "%s(): BEGIN: paddr=0x%llx, val=0x%08x\n", __func__, paddr, val);
	dev_dbg(dev, "%s(): ioremap()\n", __func__);
	vaddr = ioremap(paddr, 4);
	if (IS_ERR_OR_NULL(vaddr)) {
		dev_dbg(dev, "%s(): ioremap() failed: paddr=0x%llx\n", __func__, paddr);
		return 0;
	}
	dev_dbg(dev, "%s(): writel()\n", __func__);
	writel(val, vaddr);
	dev_dbg(dev, "%s(): iounmap()\n", __func__);
	iounmap(vaddr);
	dev_dbg(dev, "%s(): END\n", __func__);
	return 0;
}

static noinline u32 __maybe_unused read_paddr(struct device *dev, u64 paddr)
{
	u32 val;
	void __iomem *vaddr;

	dev_dbg(dev, "%s(): BEGIN: paddr=0x%llx\n", __func__, paddr);
	dev_dbg(dev, "%s(): ioremap()\n", __func__);
	vaddr = ioremap(paddr, 4);
	if (IS_ERR_OR_NULL(vaddr)) {
		dev_dbg(dev, "%s(): ioremap() failed: paddr=0x%llx\n", __func__, paddr);
		return 0;
	}
	dev_dbg(dev, "%s(): readl()\n", __func__);
	val = readl(vaddr);
	dev_dbg(dev, "%s(): iounmap()\n", __func__);
	iounmap(vaddr);
	dev_dbg(dev, "%s(): END:   paddr=0x%llx, val=0x%08x\n", __func__, paddr, val);
	return val;
}

#define write_reg_by_address(paddr, val)	write_paddr(chip->dev, paddr, val)
#define read_reg_by_address(paddr)              read_paddr(chip->dev, paddr)
