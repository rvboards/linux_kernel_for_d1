/*
 * Use extra memories as normal storage media
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
#define DEBUG	 /* Enable dev_dbg */
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

struct gorilla_extra_mem {
	struct device *dev;
	void __iomem *base;
};

static int gorilla_extra_mem_probe(struct platform_device *pdev)
{
	struct gorilla_extra_mem *chip;
	struct device *dev = &pdev->dev;
	struct resource *res;

	dev_dbg(dev, "%s(): BEGIN\n", __func__);

	chip = devm_kzalloc(dev, sizeof(*chip), GFP_KERNEL);
	if (!chip)
		return -ENOMEM;

	platform_set_drvdata(pdev, chip);
	chip->dev = dev;

	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if (!res) {
		dev_err(dev, "Fail to get IORESOURCE_MEM\n");
		return -EINVAL;
	}
	chip->base = devm_ioremap_resource(dev, res);
	if (IS_ERR(chip->base)) {
		dev_err(dev, "Fail to map IO resource\n");
		return PTR_ERR(chip->base);
	}

	dev_dbg(dev, "%s(): END\n", __func__);
	return 0;
}

static int gorilla_extra_mem_remove(struct platform_device *pdev)
{
	return 0;
}

static const struct of_device_id gorilla_extra_mem_dt_ids[] = {
	{ .compatible = "sunxi,gorilla-extra-mem" },
	{ /* sentinel */ },
};
MODULE_DEVICE_TABLE(of, gorilla_extra_mem_dt_ids);

static struct platform_driver gorilla_extra_mem_driver = {
	.probe    = gorilla_extra_mem_probe,
	.remove   = gorilla_extra_mem_remove,
	.driver   = {
		.name  = "gorilla-extra-mem",
		.owner = THIS_MODULE,
		.of_match_table = gorilla_extra_mem_dt_ids,
	},
};
module_platform_driver(gorilla_extra_mem_driver);

MODULE_DESCRIPTION("sunxi gorilla extra memory driver");
MODULE_AUTHOR("Martin <wuyan@allwinnertech.com>");
MODULE_LICENSE("GPL v2");
MODULE_VERSION("1.0.0");
