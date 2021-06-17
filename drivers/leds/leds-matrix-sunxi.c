/*
 * drivers/leds/leds-matrix-leds.c - Allwinner LED Matrix Driver
 *
 * Copyright (C) 2021 Allwinner Technology Limited. All rights reserved.
 *	http://www.allwinnertech.com
 * Author : OuJiayu <OuJiayu@allwinnertech.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 */

#include <linux/module.h>
#include <linux/err.h>
#include <linux/leds.h>
#include <linux/io.h>
#include <linux/slab.h>
#include <linux/delay.h>
#include <linux/platform_device.h>
#include <linux/pinctrl/consumer.h>
#include <linux/property.h>
#include <linux/gpio.h>
#include <linux/of.h>
#include <linux/of_gpio.h>
#include <linux/of_platform.h>
#include "leds-matrix-sunxi.h"

struct matrix_led_gdata {
	int *map;
	int *col;
};
struct matrix_led_gdata *gdata;

static inline int sizeof_gpio_leds_priv(int led_cnt)
{
	return sizeof(struct matrix_led_priv) +
		(sizeof(struct matrix_led_data) * led_cnt);
}

static inline struct matrix_led_data *
		cdev_to_matrix_led_data(struct led_classdev *led_cdev)
{
	return container_of(led_cdev, struct matrix_led_data, cdev);
}

static inline struct matrix_led_priv *
		flag_to_matrix_led_priv(u8 *flag)
{
	return container_of(flag, struct matrix_led_priv, flag);
}

static void matrix_led_set(struct led_classdev *led_cdev,
		enum led_brightness value)
{
	struct matrix_led_data *leds =
		cdev_to_matrix_led_data(led_cdev);
	int level;

	if (value == LED_OFF)
		level = 0;
	else
		level = 1;

	if (leds->can_sleep) {
		gpio_set_value_cansleep(leds->row_gpio, !level); //set row
		gpio_set_value_cansleep(leds->col_gpio, level); //set col
	} else {
		gpio_set_value(leds->row_gpio, !level); //set row
		gpio_set_value(leds->col_gpio, level); //set col
	}
}

static int matrix_led_set_blocking(struct led_classdev *led_cdev,
		enum led_brightness value)
{
	matrix_led_set(led_cdev, value);
	return 0;
}

static DECLARE_WAIT_QUEUE_HEAD(matrix_led_wait);

static void matrix_led_set_refresh(struct led_classdev *led_cdev,
		enum led_brightness value)
{
	struct matrix_led_data *leds =
		cdev_to_matrix_led_data(led_cdev);
	struct matrix_led_priv *priv =
		flag_to_matrix_led_priv(leds->flag);
	int row, col, num, level, i;

	if (value == LED_OFF) {
		if (priv->light_cnt > 0)
			priv->light_cnt -= 1;
		level = 0;
	} else {
		priv->light_cnt += 1;
		level = 1;
	}

	for (i = 0; i < priv->pdata->num_row_gpios; i++) {
		if (leds->row_gpio == priv->pdata->row_gpios[i])
			row = i;
	}

	for (i = 0; i < priv->pdata->num_col_gpios; i++) {
		if (leds->col_gpio == priv->pdata->col_gpios[i]) {
			gdata->col[i] = level;
			col = i;
		}
	}
	num = row * priv->pdata->num_row_gpios + col;

	if (level == 1)
		gdata->map[num] = 1;
	else
		gdata->map[num] = 0;

	if (priv->light_cnt >= 1) {
		priv->pdata->wait = 1;
		wake_up_interruptible(&matrix_led_wait);
	} else {
		priv->pdata->wait = 0;
	}

	schedule_delayed_work(&priv->work, 0);
	return;
}

static void matrix_led_refresh_work(struct work_struct *work)
{
	struct matrix_led_priv *priv;
	struct matrix_led_platform_data *pdata;
	int i = 0, j = 0, flag = 0;
	int col_num;

	priv = container_of(work, struct matrix_led_priv, work.work);
	pdata = priv->pdata;
	col_num = pdata->num_col_gpios;

	while (1) {
		wait_event_interruptible(matrix_led_wait, pdata->wait != 0);
		for (i = 0; i < pdata->num_row_gpios; i++) {
		/* light the row led one by one */
			for (j = 0; j < col_num; j++) {
				gdata->col[j] = gdata->map[i * col_num + j];
				if (gdata->map[i * col_num + j] == 1) {
					flag = 1;
				}
			}
			if (flag == 1) {
				if (!gpio_cansleep(pdata->row_gpios[i]))
					gpio_set_value(pdata->row_gpios[i], 0);
				else
					gpio_set_value_cansleep(pdata->row_gpios[i], 0);
				udelay(pdata->delay_us);
				for (j = 0; j < col_num; j++) {
					if (!gpio_cansleep(pdata->col_gpios[j]))
						gpio_set_value(pdata->col_gpios[j],
							gdata->col[j]);
					else
						gpio_set_value_cansleep(pdata->col_gpios[j],
								gdata->col[j]);
					udelay(pdata->delay_us);
				}
			} else {
				flag = 0;
				continue;
			}
			udelay(50);

		/* turn off the led after light for the purpose of scanning */

			if (!gpio_cansleep(pdata->row_gpios[i]))
				gpio_set_value(pdata->row_gpios[i], 1);
			else
				gpio_set_value_cansleep(pdata->row_gpios[i], 1);
			udelay(pdata->delay_us);
			for (j = 0; j < col_num; j++) {
				if (!gpio_cansleep(pdata->col_gpios[j]))
					gpio_set_value(pdata->col_gpios[j], 0);
				else
					gpio_set_value_cansleep(pdata->col_gpios[j], 0);
				udelay(pdata->delay_us);
			}
			udelay(50);
			flag = 0;
		}
	}
	return;
}

static int matrix_led_set_refresh_blocking(struct led_classdev *led_cdev,
		enum led_brightness value)
{
	matrix_led_set_refresh(led_cdev, value);
	return 0;
}

static int matrix_led_set_default_state(struct matrix_led_platform_data *pdata)
{
	int i = 0, ret = -1;

	/* set row gpio output and high level */
	for (i = 0; i < pdata->num_row_gpios; i++) {
		ret = pinctrl_gpio_direction_output(pdata->row_gpios[i]);
		if (ret < 0) {
			dev_err(pdata->dev,
				"row%d gpio set output failed\n", i);
			return ret;
		}
		gpio_set_value(pdata->row_gpios[i], 1);
	}

	/* set col gpio output and low level */
	for (i = 0; i < pdata->num_col_gpios; i++) {
		ret = pinctrl_gpio_direction_output(pdata->col_gpios[i]);
		if (ret < 0) {
			dev_err(pdata->dev,
				"col%d gpio set output failed\n", i);
			return ret;
		}
		gpio_set_value(pdata->col_gpios[i], 0);
	}

	return 0;
}

static int set_gdata_for_refresh(struct matrix_led_platform_data *pdata)
{
	struct device *dev = pdata->dev;
	int *count, *map;
	int i;
	int led_cnt = pdata->num_col_gpios * pdata->num_row_gpios;

	if (pdata->refresh) {
		gdata = devm_kzalloc(dev, sizeof(*gdata), GFP_KERNEL);
		if (!gdata) {
			dev_err(dev, "could not allocate memory for gdata\n");
			return -1;
		}
	} else {
		if (!gdata)
			gdata = NULL;
		return 0;
	}

	count = devm_kcalloc(dev, pdata->num_col_gpios,
			sizeof(int), GFP_KERNEL);
	if (!count) {
		dev_err(dev, "could not allocate memory for count\n");
		return -1;
	}
	if (pdata->refresh) {
		for (i = 0; i < pdata->num_col_gpios; i++)
			count[i] = 0;
	}
	gdata->col = count;
	map = devm_kcalloc(dev, led_cnt,
			sizeof(int), GFP_KERNEL);
	if (!map) {
		dev_err(dev, "could not allocate memory for map\n");
		goto count_err;
	}
	for (i = 0; i < led_cnt; i++) {
		map[i] = 0;
	}
	gdata->map = map;

	return 0;

count_err:
	kfree(count);
	return -1;
}

/*
 * NOTE:This function is used to parse the device tree.In the device tree,
 * what you need to configure are as following:
 * sunxi_matrix_leds {
 *		compatible = <>;
 *		row-gpios = <
 *			&pio PX X GPIO_ACTIVE_HIGH
 *			&pio PX X GPIO_ACTIVE_HIGH>;
 *		col-gpios = <
 *			&pio PX X GPIO_ACTIVE_LOW
 *			&pio PX X GPIO_ACTIVE_LOW>;
 *		status = "okay";
 * };
 * Configure the correct GPIO pin according to the hardware.
 */
static struct matrix_led_platform_data
	*sunxi_matrix_led_parse_dt(struct device *dev, struct device_node *np)
{
	struct matrix_led_platform_data *pdata;
	unsigned int *gpios;
	int nrow, ncol, i, ret;

	pdata = devm_kzalloc(dev, sizeof(*pdata), GFP_KERNEL);
	if (IS_ERR(pdata) || !pdata) {
		dev_err(dev, "could not allocate memory for platform data\n");
		return ERR_PTR(-ENOMEM);
	}

	of_property_read_u32(np, "gpio-delay-us", &pdata->delay_us);

	pdata->num_row_gpios = nrow = of_gpio_named_count(np, "row-gpios");
	pdata->num_col_gpios = ncol = of_gpio_named_count(np, "col-gpios");

	if (pdata->num_row_gpios <= 0 || pdata->num_col_gpios <= 0) {
		dev_err(dev, "number of leds rows/cols not specified\n");
		ret = -EINVAL;
		goto free_pdata;
	}

	gpios = devm_kcalloc(dev, pdata->num_row_gpios + pdata->num_col_gpios,
			sizeof(unsigned int), GFP_KERNEL);
	if (!gpios) {
		dev_err(dev, "could not allocate memory for gpios\n");
		ret = -ENOMEM;
		goto free_pdata;
	}
	for (i = 0; i < nrow; i++) {
		ret = of_get_named_gpio(np, "row-gpios", i);
		if (ret < 0) {
			goto free_mem;
		}
		gpios[i] = ret;
	}
	for (i = 0; i < ncol; i++) {
		ret = of_get_named_gpio(np, "col-gpios", i);
		if (ret < 0)
			goto free_mem;
		gpios[nrow + i] = ret;
	}
	pdata->row_gpios = gpios;
	pdata->col_gpios = &gpios[pdata->num_row_gpios];

	return pdata;

free_mem:
	kfree(gpios);
free_pdata:
	kfree(pdata);
	return ERR_PTR(ret);
}

static int
sunxi_matrix_classdev_register(struct matrix_led_platform_data *pdata,
		struct matrix_led_priv *priv,
		struct matrix_led_data *leds, int i)
{
	int row, col, row_ret, col_ret, ret;
	struct device *dev = pdata->dev;

	row = i / pdata->num_row_gpios;
	col = i % pdata->num_row_gpios;

	leds->row_gpio = pdata->row_gpios[row];
	leds->col_gpio = pdata->col_gpios[col];

	row_ret = gpio_cansleep(leds->row_gpio);
	col_ret = gpio_cansleep(leds->col_gpio);

	if (!row_ret && !col_ret) {
		if (!pdata->refresh)
			leds->cdev.brightness_set = matrix_led_set;
		else
			leds->cdev.brightness_set = matrix_led_set_refresh;
		leds->can_sleep = false;
	} else if (row_ret && col_ret) {
		if (!pdata->refresh)
			leds->cdev.brightness_set_blocking =
				matrix_led_set_blocking;
		else
			leds->cdev.brightness_set_blocking =
				matrix_led_set_refresh_blocking;
		leds->can_sleep = true;
	} else {
		dev_err(dev, "please set the gpio in the same soc\n");
		return -EINVAL;
	}

	leds->cdev.name = kzalloc(8, GFP_KERNEL);
	sprintf((char *)leds->cdev.name, "mled%d", i);
	leds->cdev.dev = dev;

	ret = devm_led_classdev_register(dev, &leds->cdev);
	if (ret < 0) {
		dev_err(dev, "class register failed %s\n", leds->cdev.name);
		return ret;
	}

	/* prepare to get priv data*/
	leds->flag = &priv->flag;

	return 0;
}

static int
sunxi_matrix_classdev_unregister(struct matrix_led_priv *priv)
{
	int i;
	for (i = 0; i < priv->led_cnt; i++) {
		kfree(priv->leds[i].cdev.name);
		led_classdev_unregister(&priv->leds[i].cdev);
	}
	return 0;
}

static int sunxi_matrix_led_probe(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct matrix_led_platform_data *pdata;
	struct matrix_led_priv *priv;
	struct device_node *np = dev->of_node;
	int ret, i, led_cnt;

	if (!np) {
		dev_err(dev, "device lacks DT data\n");
		return -ENODEV;
	}

	pdata = sunxi_matrix_led_parse_dt(dev, np);
	if (IS_ERR(pdata)) {
		return PTR_ERR(pdata);
	}
	pdata->dev = dev;

	/* If you want to use the function of array LED lighting at the same time,
	 * please set refresh-leds in dts.Otherwise,
	 * the LEDs between rows will affect each other.*/
	led_cnt = (pdata->num_row_gpios * pdata->num_col_gpios);

	priv = devm_kzalloc(dev, sizeof_gpio_leds_priv(led_cnt),
			GFP_KERNEL);
	if (!priv) {
		ret = -ENOMEM;
		goto pdata_err;
	}
	priv->led_cnt = led_cnt;

	if (of_get_property(np, "refresh-leds", NULL))
		pdata->refresh = true;
	else
		pdata->refresh = false;

	for (i = 0; i < led_cnt; i++) {
		ret = sunxi_matrix_classdev_register(pdata, priv,
					&priv->leds[i], i);
		if (ret < 0) {
			goto priv_err;
		}
	}

	ret = set_gdata_for_refresh(pdata);
	if (ret < 0) {
		dev_err(dev, "set gdata for refresh failed\n");
		goto priv_err;
	}

	ret = matrix_led_set_default_state(pdata);
	if (ret < 0) {
		dev_err(dev, "matrix_led_set_default_state failed\n");
		goto priv_err;
	}

	INIT_DELAYED_WORK(&priv->work, matrix_led_refresh_work);
	priv->pdata = pdata;
	platform_set_drvdata(pdev, priv);

	dev_info(dev, "sunxi matrix led probe success!\n");
	return 0;

priv_err:
	kfree(priv);
pdata_err:
	kfree(pdata);
	return ret;
}

static int sunxi_matrix_led_remove(struct platform_device *pdev)
{
	struct matrix_led_priv *priv;
	int i;

	priv = platform_get_drvdata(pdev);

	flush_delayed_work(&priv->work);
	for (i = 0; i < priv->led_cnt; i++) {
		struct matrix_led_data *leds = &priv->leds[i];
		matrix_led_set(&leds->cdev, LED_OFF);
	}
	sunxi_matrix_classdev_unregister(priv);

	if (!gdata)
		kfree(gdata);

	kfree(priv);

	return 0;
}

static const struct of_device_id sunxi_matrix_led_dt_ids[] = {
	{.compatible = "allwinner,sunxi-matrix-leds"},
	{},
};

static struct platform_driver sunxi_matrix_led_driver = {
	.probe          = sunxi_matrix_led_probe,
	.remove         = sunxi_matrix_led_remove,
	.driver         = {
		.name   = "sunxi-matrix-leds",
		.owner  = THIS_MODULE,
		.of_match_table = sunxi_matrix_led_dt_ids,
	},
};

module_platform_driver(sunxi_matrix_led_driver);

MODULE_ALIAS("sunxi matrix gpio leds driver");
MODULE_ALIAS("platform : matrix leds dirver");
MODULE_LICENSE("GPL v2");
MODULE_VERSION("1.0.0");
MODULE_AUTHOR("OuJiayu@allwinnertech.com");
MODULE_DESCRIPTION("Sunxi matrix GPIO LED driver");
