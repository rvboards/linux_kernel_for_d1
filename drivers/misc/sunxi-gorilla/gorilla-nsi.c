/*
 * sunxi gorilla IRQ test driver (For AW1855)
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
#include <linux/init.h>
#include <linux/interrupt.h>
#include <linux/io.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/of_device.h>
#include <linux/err.h>
#include <linux/platform_device.h>
#include "gorilla-common.h"

struct gorilla_nsi {
	struct device *dev;
};

static int nsi_init(struct gorilla_nsi *chip)
{
	write_reg_by_address(0x05101614, 0xa);
	write_reg_by_address(0x05101618, 0x1);
	write_reg_by_address(0x05102414, 0xf);
	write_reg_by_address(0x05102418, 0x1);
	write_reg_by_address(0x05100218, 0x1);
	write_reg_by_address(0x05100028, 0xe4);
	write_reg_by_address(0x0510002c, 0x5d);
	write_reg_by_address(0x05102028, 0xf2);
	write_reg_by_address(0x0510202c, 0x9c);
	write_reg_by_address(0x05100228, 0x2d7);
	write_reg_by_address(0x0510022c, 0x1d4);
	write_reg_by_address(0x05102e90, 0x80202020);
	write_reg_by_address(0x05100010, 0x1);
	write_reg_by_address(0x05100210, 0x1);
	write_reg_by_address(0x05102010, 0x1);
	write_reg_by_address(0x05102e94, 0x0);
	write_reg_by_address(0x05102e98, 0x0);
	write_reg_by_address(0x051000C0, 0x1);
	write_reg_by_address(0x051000C8, 0x8342);
	write_reg_by_address(0x051002C0, 0x1);
	write_reg_by_address(0x051002C8, 0x8342);
	write_reg_by_address(0x051016C0, 0x1);
	write_reg_by_address(0x051016C8, 0x8342);
	write_reg_by_address(0x051020C0, 0x1);
	write_reg_by_address(0x051020C8, 0x8342);
	write_reg_by_address(0x05102CC0, 0x1);
	write_reg_by_address(0x05102CC8, 0x8342);

	return 0;
}

static const struct of_device_id gorilla_nsi_dt_ids[] = {
	{.compatible = "sunxi,gorilla-nsi"},
	{ /* sentinel */ },
};
MODULE_DEVICE_TABLE(of, gorilla_nsi_dt_ids);

static int gorilla_nsi_probe(struct platform_device *pdev)
{
	struct gorilla_nsi *chip;
	struct device *dev = &pdev->dev;

	dev_dbg(dev, "%s(): BEGIN\n", __func__);

	/* Initialize private data structure `chip` */
	chip = devm_kzalloc(dev, sizeof(*chip), GFP_KERNEL);
	if (!chip)
		return -ENOMEM;
	platform_set_drvdata(pdev, chip);
	chip->dev = dev;

	/* Initialize hardware */
	nsi_init(chip);

	dev_dbg(dev, "%s(): END\n", __func__);
	return 0;
}

static struct platform_driver gorilla_nsi_driver = {
	.probe    = gorilla_nsi_probe,
	.driver   = {
		.name  = "gorilla-nsi",
		.owner = THIS_MODULE,
		.of_match_table = gorilla_nsi_dt_ids,
	},
};

module_platform_driver(gorilla_nsi_driver);

MODULE_DESCRIPTION("sunxi gorilla NSI driver");
MODULE_AUTHOR("Martin <wuyan@allwinnertech.com>");
MODULE_LICENSE("GPL v2");
MODULE_VERSION("1.0.0");
