/*
 * sunxi gorilla memory controller driver (For AW1855)
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
#include <linux/delay.h>
#include "gorilla-common.h"

struct gorilla_memc {
	struct device *dev;
};

/*
 * initialize sunxi memc
 * @freq: 12 or 24. default is 12
 * @do_traning: default is false
 */
static noinline int gorilla_memc_init(struct gorilla_memc *chip, int freq, bool do_traning)
{
	struct device *dev = chip->dev;
	u32 data = 0;

	dev_dbg(dev, "%s(): BEGIN\n", __func__);

	write_reg_by_address(0x0482300c, 0x00008000);
	write_reg_by_address(0x04810020, 0xffffffff);
	data = read_reg_by_address(0x04810000);
	write_reg_by_address(0x04810000, 0x004319f5);
	write_reg_by_address(0x04823030, 0x00001c70);
	write_reg_by_address(0x04823034, 0x00000000);
	write_reg_by_address(0x04823038, 0x00000018);
	write_reg_by_address(0x0482303c, 0x00000000);
	write_reg_by_address(0x04823058, 0x0c0f180c);
	write_reg_by_address(0x0482305c, 0x00030310);
	write_reg_by_address(0x04823060, 0x04060509);
	write_reg_by_address(0x04823064, 0x0000400c);
	write_reg_by_address(0x04823068, 0x05020305);
	write_reg_by_address(0x0482306c, 0x05050403);
	data = read_reg_by_address(0x04823078);
	write_reg_by_address(0x04823078, 0x90006610);
	write_reg_by_address(0x04823080, 0x02040102);
	write_reg_by_address(0x04823044, 0x01e007c3);
	write_reg_by_address(0x04823050, 0x03c00800);
	write_reg_by_address(0x04823054, 0x24000500);
	//write_reg_by_address(0x04823090, 0x00490069);
	data = read_reg_by_address(0x0481000c);
	write_reg_by_address(0x0481000c, 0x0000018f);
	data = read_reg_by_address(0x04823108);
	write_reg_by_address(0x04823108, 0x000023c0);
	data = read_reg_by_address(0x04823344);
	write_reg_by_address(0x04823344, 0x40020281);
	data = read_reg_by_address(0x048233c4);
	write_reg_by_address(0x048233c4, 0x40020281);
	data = read_reg_by_address(0x04823444);
	write_reg_by_address(0x04823444, 0x40020281);
	data = read_reg_by_address(0x048234c4);
	write_reg_by_address(0x048234c4, 0x40020281);
	data = read_reg_by_address(0x04823208);
	write_reg_by_address(0x04823208, 0x0002034a);
	data = read_reg_by_address(0x04823108);
	write_reg_by_address(0x04823108, 0x00002380);
	data = read_reg_by_address(0x04823060);
	data = read_reg_by_address(0x048230bc);
	write_reg_by_address(0x048230bc, 0x00f00104);
	data = read_reg_by_address(0x0482311c);
	write_reg_by_address(0x0482311c, 0x4c00041f);
	data = read_reg_by_address(0x048230c0);
	write_reg_by_address(0x048230c0, 0x83000081);
	data = read_reg_by_address(0x04823140);
	write_reg_by_address(0x04823140, 0x013b3bbb);
	write_reg_by_address(0x04823000, 0x000001f2);
	write_reg_by_address(0x04823000, 0x000001f3);

	data = read_reg_by_address(0x04823010);
	while (data != 1) {
		data = read_reg_by_address(0x04823010);
	}

	data = read_reg_by_address(0x04810014);
	write_reg_by_address(0x04810014, data);

	data = read_reg_by_address(0x04823018);
	while (data != 1) {
		data = read_reg_by_address(0x04823018);
	}

	data = read_reg_by_address(0x0482310c);
	write_reg_by_address(0x0482310c, 0xc0aa0060);
	data = read_reg_by_address(0x0482308c);
	write_reg_by_address(0x0482308c, 0x80200010);
	data = read_reg_by_address(0x0482308c);
	write_reg_by_address(0x0482308c, 0x00200010);
	write_reg_by_address(0x04810020, 0xffffffff);
	write_reg_by_address(0x04810024, 0x000007ff);
	write_reg_by_address(0x04810028, 0x0000ffff);
	data = read_reg_by_address(0x04810000);
	write_reg_by_address(0x04810000, 0x004319f5);
	data = read_reg_by_address(0x04810000);
	write_reg_by_address(0x04810000, 0x004319f5);
	data = read_reg_by_address(0x04810000);
	write_reg_by_address(0x04823120, 0x00000303);
	data = read_reg_by_address(0x04810000);
	data = read_reg_by_address(0x04810004);

	write_reg_by_address(0x048200a0, 0x10000000);

	dev_dbg(dev, "%s(): END\n", __func__);
	return 0;
}

static int gorilla_memc_probe(struct platform_device *pdev)
{
	struct gorilla_memc *chip;
	struct device *dev = &pdev->dev;
	int err;

	dev_dbg(dev, "%s(): BEGIN\n", __func__);

	chip = devm_kzalloc(dev, sizeof(*chip), GFP_KERNEL);
	if (!chip)
		return -ENOMEM;

	platform_set_drvdata(pdev, chip);
	chip->dev = dev;

	err = gorilla_memc_init(chip, 24, false);
	if (err) {
		dev_err(dev, "gorilla_memc_init() failed\n");
		return err;
	}

	dev_dbg(dev, "%s(): END\n", __func__);
	return 0;
}

static int gorilla_memc_remove(struct platform_device *pdev)
{
	return 0;
}

static const struct of_device_id gorilla_memc_dt_ids[] = {
	{.compatible = "sunxi,gorilla-memc" },
	{ /* sentinel */ },
};
MODULE_DEVICE_TABLE(of, gorilla_memc_dt_ids);

static struct platform_driver gorilla_memc_driver = {
	.probe    = gorilla_memc_probe,
	.remove   = gorilla_memc_remove,
	.driver   = {
		.name  = "gorilla-memc",
		.owner = THIS_MODULE,
		.of_match_table = gorilla_memc_dt_ids,
	},
};
module_platform_driver(gorilla_memc_driver);

MODULE_DESCRIPTION("sunxi gorilla memory controller driver");
MODULE_AUTHOR("Martin <wuyan@allwinnertech.com>");
MODULE_LICENSE("GPL v2");
MODULE_VERSION("1.0.0");
