/*
 * drivers/char/sunxi-bs83b16c.c - Allwinner bs83b16c key & led driver
 *
 * Copyright (C) 2021 Allwinner Technology Limited. All rights reserved.
 *       http://www.allwinnertech.com
 * Author : OuJiayu <OuJiayu@allwinnertech.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 */

#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/i2c.h>
#include <linux/interrupt.h>
#include <linux/irq.h>
#include <linux/module.h>
#include <linux/input.h>
#include <linux/delay.h>
#include <linux/io.h>
#include <linux/of.h>
#include <linux/of_gpio.h>
#include <linux/sunxi-gpio.h>
#include <linux/pinctrl/pinctrl-sunxi.h>
#include <linux/input/matrix_keypad.h>
#include <linux/leds.h>
#include <linux/err.h>
#include "sunxi-bs83b16c.h"

static int i2c_write(struct i2c_client *client, unsigned int reg,
		unsigned int val)
{
	return i2c_smbus_write_byte_data(client, reg, val);
}

static int i2c_read(struct i2c_client *client, unsigned int reg)
{
	return i2c_smbus_read_byte_data(client, reg);
}

static inline struct led_data *
		cdev_to_leddata(struct led_classdev *led_cdev)
{
	return container_of(led_cdev, struct led_data, cdev);
}

static inline struct sunxi_bs83b16c_pdata *
		flag_to_pdata(u8 *flag)
{
	return container_of(flag, struct sunxi_bs83b16c_pdata, flag);
}

static void sunxi_bs83b16c_work_func(struct work_struct *work)
{
	struct sunxi_bs83b16c_pdata *pdata = container_of(work,
				struct sunxi_bs83b16c_pdata, work);
	struct input_dev *input_dev = pdata->input_dev;
	struct i2c_client *client = pdata->client;
	unsigned int new_state[KEY_MAX_COLS];
	uint32_t bits_changed;
	int i, j, num, code;

	udelay(pdata->key_scan_udelay * 20);
	memset(new_state, 0, sizeof(new_state));

	/* save new key state */
	for (i = 0; i < pdata->key_row_num; i++) {
		new_state[i] = i2c_read(client, DATA_START + i);
		udelay(pdata->key_scan_udelay);
	}

	/* find the change and input report */
	for (i = 0; i < pdata->key_row_num; i++) {
		bits_changed = pdata->last_key_state[i] ^ new_state[i];
		if (bits_changed == 0)
			continue;

		for (j = 0; j < pdata->key_col_num; j++) {
			if ((bits_changed & (1 << j)) == 0)
				continue;

			num = i * pdata->key_row_num + j;
			code = MATRIX_SCAN_CODE(i, j, pdata->row_shift);
			input_event(input_dev, EV_MSC, MSC_SCAN, code);
			input_report_key(input_dev,
					pdata->keycode[num],
					new_state[i] & (1 << j));
		}

	}
	input_sync(input_dev);
	memcpy(pdata->last_key_state, new_state, sizeof(new_state));

	spin_lock_irq(&pdata->slock);
	enable_irq(pdata->int_irq);
	spin_unlock_irq(&pdata->slock);
}

static irqreturn_t sunxi_bs83b16c_handler(int irq, void *data)
{
	struct sunxi_bs83b16c_pdata *pdata = data;

	disable_irq_nosync(pdata->int_irq);

	schedule_work(&pdata->work);

	return IRQ_HANDLED;
}

static void sunxi_bs83b16c_led_set(struct led_classdev *led_cdev,
			enum led_brightness value)
{
	struct led_data *leds = cdev_to_leddata(led_cdev);
	struct sunxi_bs83b16c_pdata *pdata =
		flag_to_pdata(leds->flag);
	int ret;
	unsigned int data;

	data = i2c_read(pdata->client, DATA_END - leds->row);
	udelay(5*100);

	if (value == LED_OFF) {
		data &= ~(1 << leds->col);
	} else {
		data |= 1 << leds->col;
	}

	ret = i2c_write(pdata->client, DATA_END - leds->row, data);
	if (ret < 0)
		dev_err(&pdata->client->dev, "i2c write led data failed\n");
}

static int sunxi_bs83b16c_led_set_blocking(struct led_classdev *led_cdev,
			enum led_brightness value)
{
	sunxi_bs83b16c_led_set(led_cdev, value);
	return 0;
}

static int sunxi_bs83b16c_set_keypin(struct i2c_client *client,
			struct device_node *np,
			struct sunxi_bs83b16c_pdata *pdata)
{
	struct device *dev = &client->dev;
	int gpio, irq, ret;

	gpio = of_get_named_gpio(np, "int-gpio", 0);
	if (gpio < 0) {
		dev_err(dev, "get int-gpio failed\n");
		return -1;
	}

	ret = pinctrl_pm_select_default_state(dev);
	if (ret) {
		dev_err(dev, "set int gpio pin function failed\n");
		return -1;
	}

	irq = gpio_to_irq(gpio);

	pdata->int_irq = irq;

	ret = request_irq(irq, sunxi_bs83b16c_handler,
			IRQF_TRIGGER_FALLING | IRQF_SHARED,
			dev_name(dev), pdata);
	if (ret) {
		dev_err(dev, "request_irq for int-gpio failed\n");
		return -1;
	}

	return 0;
}

static int sunxi_bs83b16c_led_register(struct sunxi_bs83b16c_pdata *pdata,
			struct i2c_client *client, int i)
{
	struct device *dev = &client->dev;
	struct led_classdev *cdev;
	int ret;

	pdata->leds[i].row = i / pdata->led_row_num;
	pdata->leds[i].col = i % pdata->led_row_num;
	pdata->leds[i].flag = &pdata->flag;
	cdev = &pdata->leds[i].cdev;

	cdev->brightness_set = sunxi_bs83b16c_led_set;
	cdev->brightness_set_blocking = sunxi_bs83b16c_led_set_blocking;
	cdev->name = kzalloc(8, GFP_KERNEL);
	sprintf((char *)cdev->name, "mled%d", i);
	cdev->dev = dev;

	ret = devm_led_classdev_register(dev, cdev);
	if (ret < 0) {
		dev_err(dev, "class register failed %s\n", cdev->name);
		return ret;
	}
	udelay(1*1000);
	return 0;
}

static int sunxi_bs83b16c_led_unregister(struct sunxi_bs83b16c_pdata *pdata)
{
	int i, led_cnt;
	led_cnt = pdata->led_row_num * pdata->led_col_num;

	for (i = 0; i < led_cnt; i++) {
		kfree(pdata->leds[i].cdev.name);
		led_classdev_unregister(&pdata->leds[i].cdev);
		udelay(5*100);
	}
	return 0;
}

static int sunxi_bs83b16c_set_func(struct i2c_client *client,
			struct sunxi_bs83b16c_pdata *pdata)
{
	int ret;
	unsigned int num;

	if ((unsigned int)(pdata->func >> KEY_FUNC) & 1) {
		num = (pdata->key_row_num << 4) | pdata->key_col_num;
		ret = i2c_write(client, KEY_HV_INIT, num);
		if (ret)
			return -1;
	}

	if ((unsigned int)(pdata->func >> LED_FUNC) & 1) {
		num = (pdata->led_row_num << 4) | pdata->led_col_num;
		ret = i2c_write(client, LED_HV_INIT, num);
		if (ret)
			return -1;
	}

	return 0;
}

static int sunxi_bs83b16c_enable_func(struct i2c_client *client,
			struct sunxi_bs83b16c_pdata *pdata)
{
	unsigned int tmp;
	int ret;

	if ((unsigned int)(pdata->func >> KEY_FUNC) & 1) {
		tmp = i2c_read(client, KEY_HV_INIT);
		if (tmp) {
			ret = i2c_write(client, KEY_EN_REG, 1);
			if (ret)
				return -1;
		}
	}

	if ((unsigned int)(pdata->func >> LED_FUNC) & 1) {
		tmp = i2c_read(client, LED_HV_INIT);
		if (tmp) {
			ret = i2c_write(client, LED_EN_REG, 1);
			if (ret)
				return -1;
		}
	}

	return 0;
}

static void sunxi_bs83b16c_set_input_code(struct sunxi_bs83b16c_pdata *pdata)
{
	int i = 0;
	int keysize = pdata->key_row_num * pdata->key_col_num;

	for (i = 0; i < keysize; i++) {
		input_set_capability(pdata->input_dev,
				EV_KEY, pdata->keycode[i]);
	}
}

struct sunxi_bs83b16c_pdata *of_get_bs83b16c_pdata(struct i2c_client *client,
			struct device_node *np)
{
	struct sunxi_bs83b16c_pdata *pdata;
	struct device *dev = &client->dev;
	int keysize, i;
	unsigned int *keycode;

	pdata = devm_kzalloc(dev, sizeof(pdata), GFP_KERNEL);
	if (IS_ERR_OR_NULL(pdata)) {
		dev_err(dev, "could not allocate memory for platform data\n");
		return ERR_PTR(-ENOMEM);
	}

	if (of_property_read_u32(np, "func", &pdata->func)) {
		dev_err(dev, "could not read func from dts, please check\n");
		goto free_pdata;
	}

	if (pdata->func == 0) {
		dev_warn(dev, "func = 0, driver does not need to be load\n");
		goto free_pdata;
	}

	if ((unsigned int)(pdata->func >> KEY_FUNC) & 1) {
		if (of_property_read_u32(np, "key_scan_udelay", &pdata->key_scan_udelay))
			pdata->key_scan_udelay = 0;
		of_property_read_u32(np, "key_row_num", &pdata->key_row_num);
		of_property_read_u32(np, "key_col_num", &pdata->key_col_num);
		if (pdata->key_row_num == 0 || pdata->key_col_num == 0) {
			dev_err(dev, "could not read key num from dts\n");
			goto free_pdata;
		}

		keysize = pdata->key_row_num * pdata->key_col_num;
		keycode = devm_kzalloc(dev, sizeof(unsigned int) * keysize,
				GFP_KERNEL);
		if (IS_ERR_OR_NULL(pdata)) {
			dev_err(dev, "could not allocate memory for keycode\n");
			goto free_pdata;
		}

		of_property_read_u32_array(np, "keymap", keycode, keysize);
		if (!keycode) {
			dev_err(dev, "could not read keymap form dts\n");
			goto free_pdata;
		}

		for (i = 0; i < keysize; i++) {
			keycode[i] = keycode[i] & 0x0000ffff;
		}

		pdata->keycode = keycode;
	}

	if ((unsigned int)(pdata->func >> LED_FUNC) & 1) {
		of_property_read_u32(np, "led_row_num", &pdata->led_row_num);
		of_property_read_u32(np, "led_col_num", &pdata->led_col_num);
		if (pdata->led_row_num == 0 || pdata->led_col_num == 0) {
			dev_err(dev, "could not read led num from dts\n");
			goto free_pdata;
		}
	}

	return pdata;

free_pdata:
	kfree(pdata);
	return ERR_PTR(-1);
}

static int sunxi_bs83b16c_probe(struct i2c_client *client,
			const struct i2c_device_id *id)
{
	struct device *dev = &client->dev;
	struct sunxi_bs83b16c_pdata *pdata;
	struct device_node *np = dev->of_node;
	struct input_dev *input_dev;
	int ret, led_cnt, i;
	ssize_t size;

	if (!np) {
		dev_err(dev, "device lacks DT data\n");
		return -ENODEV;
	}

	pdata = of_get_bs83b16c_pdata(client, np);
	if (IS_ERR(pdata)) {
		dev_err(dev, "get bs83b16c platform data failed\n");
		return -1;
	}

	input_dev = input_allocate_device();
	if (!input_dev) {
		ret = -ENOMEM;
		goto pdata_err;
	}
	pdata->input_dev = input_dev;
	pdata->client = client;
	pdata->flag = 1;

	input_dev->name = client->name;
	input_dev->id.bustype   = BUS_HOST;
	input_dev->dev.parent   = dev;

	/* check i2c dev */
	if (!i2c_check_functionality(client->adapter,
				I2C_FUNC_SMBUS_BYTE_DATA)) {
		dev_err(dev, "SMBUS Byte Data not Supported\n");
		ret = -EIO;
		goto pdata_err;
	}

	INIT_WORK(&pdata->work, sunxi_bs83b16c_work_func);
	spin_lock_init(&pdata->slock);

	if ((unsigned int)(pdata->func >> KEY_FUNC) & 1) {
		pdata->row_shift = get_count_order(pdata->key_col_num);
		for (i = 0; i < pdata->key_row_num; i++)
			pdata->last_key_state[i] = 0;

		ret = sunxi_bs83b16c_set_keypin(client, np, pdata);
		if (ret < 0) {
			dev_err(dev, "sunxi_bs83b16c_set_keypin failed\n");
			goto pdata_err;
		}
	}

	if ((unsigned int)(pdata->func >> LED_FUNC) & 1) {
		led_cnt = pdata->led_row_num * pdata->led_col_num;
		size = sizeof(struct led_data) * led_cnt;
		pdata->leds = devm_kzalloc(dev, size, GFP_KERNEL);
		if (!pdata->leds) {
			dev_err(dev, "could not allocate for leds data\n");
			ret = -ENOMEM;
			goto pdata_err;
		}

		for (i = 0; i < led_cnt; i++) {
			ret = sunxi_bs83b16c_led_register(pdata, client, i);
			if (ret < 0) {
				dev_err(dev,
					"sunxi_bs83b16c_led_register failed\n");
				goto pdata_err;
			}
		}
	}

	ret = sunxi_bs83b16c_set_func(client, pdata);
	if (ret < 0) {
		dev_err(dev, "i2c set mcu func failed\n");
		goto led_err;
	}

	input_set_capability(input_dev, EV_MSC, MSC_SCAN);
	input_set_drvdata(input_dev, pdata);
	sunxi_bs83b16c_set_input_code(pdata);

	ret = input_register_device(pdata->input_dev);
	if (ret) {
		dev_err(dev, "input_register_device failed\n");
		goto input_err;
	}

	ret = sunxi_bs83b16c_enable_func(client, pdata);
	if (ret < 0) {
		dev_err(dev, "i2c enable func failed\n");
		goto input_err;
	}

	i2c_set_clientdata(client, pdata);

	return 0;

input_err:
	input_free_device(input_dev);
led_err:
	sunxi_bs83b16c_led_unregister(pdata);
pdata_err:
	kfree(pdata);
	return ret;
}

static int sunxi_bs83b16c_remove(struct i2c_client *client)
{
	struct sunxi_bs83b16c_pdata *pdata = i2c_get_clientdata(client);

	if ((unsigned int)(pdata->func >> KEY_FUNC) & 1)
		input_unregister_device(pdata->input_dev);
	if ((unsigned int)(pdata->func >> LED_FUNC) & 1)
		sunxi_bs83b16c_led_unregister(pdata);

	kfree(pdata);

	return 0;
}

static const struct of_device_id sunxi_bs83b16c_dt_ids[] = {
	{.compatible = "allwinner,sunxi-bs83b16c"},
	{},
};

static struct i2c_driver sunxi_bs83b16c_driver = {
	.driver = {
		.name = "sunxi_bs83b16c",
		.owner = THIS_MODULE,
		.of_match_table = sunxi_bs83b16c_dt_ids,
	},
	.probe = sunxi_bs83b16c_probe,
	.remove = sunxi_bs83b16c_remove,
};

static int __init sunxi_bs83b16c_init(void)
{
	return i2c_add_driver(&sunxi_bs83b16c_driver);
}
late_initcall(sunxi_bs83b16c_init);

static void __exit sunxi_bs83b16c_exit(void)
{
	i2c_del_driver(&sunxi_bs83b16c_driver);
}
module_exit(sunxi_bs83b16c_exit);

MODULE_LICENSE("GPL");
MODULE_VERSION("1.0.0");
MODULE_DESCRIPTION("BS83B16C MCU driver for matrix key and matrix led");
MODULE_AUTHOR("OuJiayu@allwinnertech.com");
