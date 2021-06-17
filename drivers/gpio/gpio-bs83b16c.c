/*
 * drivers/gpio/gpio-bs83b16c.c - Allwinner bs83b16c expender IO driver
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
#include <linux/gpio.h>
#include <linux/i2c.h>
#include <linux/interrupt.h>
#include <linux/irq.h>
#include <linux/irqdomain.h>
#include <linux/module.h>
#include <linux/spinlock.h>
#include <linux/workqueue.h>
#include <linux/io.h>
#include <linux/of.h>
#include <linux/of_gpio.h>

#include <linux/sunxi-gpio.h>
#include <linux/pinctrl/pinconf.h>
#include <linux/pinctrl/pinctrl-sunxi.h>

#include "gpio-bs83b16c.h"

#define BS_DEBUG 1
#define NOOP 1

#define MAX_BANK  3
#define BANK_SIZE 8

/* Match device name */
static const struct i2c_device_id bs83b16c_id[] = {
	{ "bs83b16c", 8 },
	{ }
};
MODULE_DEVICE_TABLE(i2c, bs83b16c_id);

static const struct of_device_id bs83b16c_of_table[] = {
	{ .compatible = "bs83b16c" },
	{ }
};
MODULE_DEVICE_TABLE(of, bs83b16c_of_table);

struct bs83b16c {
	struct gpio_chip        chip;
	struct i2c_client       *client;
	struct mutex		lock;		/* protect 'out' */
	struct mutex		en_lock;
	struct mutex		dis_lock;
	spinlock_t		slock;		/* protect irq demux */

	struct work_struct	work;		/* irq demux work */
	struct work_struct	en_work;
	struct work_struct	dis_work;

	unsigned                out;			/* software latch */
	unsigned		status[MAX_BANK];       /* current status */
	unsigned		int_status[MAX_BANK];	/* interrupt status */
	unsigned int		state_reg[MAX_BANK];	/* gpio state*/
	unsigned int		data_reg[MAX_BANK];	/* gpio data */
	unsigned int		int_reg[MAX_BANK];	/* gpio int */
	unsigned int		pull_reg[MAX_BANK];	/* gpio pull */

	struct irq_domain       *irq_domain;		/* for irq demux  */
	int                     irq;			/* int-gpio real irq number */
	int			en_irq;			/* for pull up gpio */
	int			dis_irq;		/* for pull down gpio */

	int (*write)(struct i2c_client *client, unsigned int reg, unsigned int val);
	int (*read)(struct i2c_client *client, unsigned int reg);
};

static int i2c_write(struct i2c_client *client, unsigned int reg,
				unsigned int val)
{
	return i2c_smbus_write_byte_data(client, reg, val);
}

static int i2c_read(struct i2c_client *client, unsigned int reg)
{
	return i2c_smbus_read_byte_data(client, reg);
}

static int bs83b16c_input(struct gpio_chip *chip, unsigned offset)
{
	struct bs83b16c *gpio = container_of(chip, struct bs83b16c, chip);
	unsigned bit, gpio_out;
	int status;
	u8 bank;

	bit = offset % BANK_SIZE; /* get gpio offset */
	bank = offset / BANK_SIZE; /* get gpio bank */

	mutex_lock(&gpio->lock);
	/* set gpio input*/
	gpio_out = gpio->read(gpio->client, gpio->state_reg[bank]);
	gpio_out |= (1 << bit);

	status = gpio->write(gpio->client, gpio->state_reg[bank], gpio_out);
	mutex_unlock(&gpio->lock);

	return status;
}

static int bs83b16c_output(struct gpio_chip *chip, unsigned offset, int value)
{
	struct bs83b16c *gpio = container_of(chip, struct bs83b16c, chip);
	unsigned bit, data, gpio_out;
	int status;
	u8 bank;

	bit = offset % BANK_SIZE; /* get gpio offset */
	bank = offset / BANK_SIZE; /* get gpio bank */

	mutex_lock(&gpio->lock);

	/* set gpio output*/
	gpio_out = gpio->read(gpio->client, gpio->state_reg[bank]);
	gpio_out &= ~(1 << bit);
	status = gpio->write(gpio->client, gpio->state_reg[bank], gpio_out);
	if (status < 0) {
		dev_err(&gpio->client->dev,
				"set gpio %x output mode failed\n", offset);
		return status;
	}

	/* set gpio data */
	data = gpio->read(gpio->client, gpio->data_reg[bank]);
	if (value) {
		data |= (1 << bit);
	} else {
		data &= ~(1 << bit);
	}
	status = gpio->write(gpio->client, gpio->data_reg[bank], data);
	if (status < 0) {
		dev_err(&gpio->client->dev,
				"set gpio %x output data failed\n", offset);
		return status;
	}

	mutex_unlock(&gpio->lock);

	return status;
}

static int bs83b16c_get(struct gpio_chip *chip, unsigned offset)
{
	struct bs83b16c *gpio = container_of(chip, struct bs83b16c, chip);
	unsigned bit;
	int value;
	u8 bank;

	bit = offset % BANK_SIZE; /* get gpio offset */
	bank = offset / BANK_SIZE; /* get gpio bank */

	mutex_lock(&gpio->lock);
	value = gpio->read(gpio->client, gpio->data_reg[bank]);
	mutex_unlock(&gpio->lock);

	return (value < 0) ? 0 : (value & (1 << bit));
}

static void bs83b16c_set(struct gpio_chip *chip, unsigned offset, int value)
{
	bs83b16c_output(chip, offset, value);
}

static int bs83b16c_to_irq(struct gpio_chip *chip, unsigned offset)
{
	struct bs83b16c *gpio = container_of(chip, struct bs83b16c, chip);

	return irq_create_mapping(gpio->irq_domain, offset);
}

static void bs83b16c_irq_demux_work(struct work_struct *work)
{
	struct bs83b16c *gpio = container_of(work, struct bs83b16c, work);
	unsigned long change, status, flags, enabled;
	int i = 0;
	unsigned int pin = 0;

	for (i = 0; i < MAX_BANK; i++) {
		status = gpio->read(gpio->client, gpio->data_reg[i]);

		spin_lock_irqsave(&gpio->slock, flags);

		enabled = gpio->int_status[i];
		change = (gpio->status[i] ^ status) & enabled;

		if (change == 0) {
			spin_unlock_irqrestore(&gpio->slock, flags);
			continue;
		}
		for_each_set_bit(pin, &change, gpio->chip.ngpio / MAX_BANK) {
			generic_handle_irq(irq_find_mapping(gpio->irq_domain,
					(i * BANK_SIZE + pin)));
		}
		gpio->status[i] = status;

		spin_unlock_irqrestore(&gpio->slock, flags);
	}
}

static irqreturn_t bs83b16c_irq_demux(int irq, void *data)
{
	struct bs83b16c *gpio = data;

	/*
	 * can't not read/write data here,
	 * since i2c data access might go to sleep
	 */
	schedule_work(&gpio->work);

	return IRQ_HANDLED;
}

static void bs83b16c_irq_enable_work(struct work_struct *work)
{
	struct bs83b16c *gpio = container_of(work, struct bs83b16c, en_work);
	int bank, bit, ret, offset, i;
	unsigned int pull;

	mutex_lock(&gpio->en_lock);

	offset = gpio->en_irq;
	bank = offset / BANK_SIZE;
	bit = offset % BANK_SIZE;

	pull = gpio->read(gpio->client, gpio->pull_reg[bank]);
	pull |= (1 << bit);

	ret = gpio->write(gpio->client, gpio->pull_reg[bank], pull);
	if (ret < 0)
		dev_err(&gpio->client->dev,
			"pull up gpio offset:%d failed\n", gpio->irq);


	for (i = 0; i < MAX_BANK; i++) {
		gpio->write(gpio->client, gpio->pull_reg[i],
				gpio->int_status[i]);
	}

	mutex_unlock(&gpio->en_lock);
}

static void bs83b16c_irq_enable(struct irq_data *data)
{
	struct bs83b16c *gpio = data->domain->host_data;
	int bank = data->hwirq / BANK_SIZE;
	int bit = data->hwirq % BANK_SIZE;

	gpio->int_status[bank] |= (1 << bit);
	gpio->en_irq = data->hwirq;
	schedule_work(&gpio->en_work);
}

static void bs83b16c_irq_disable_work(struct work_struct *work)
{
	struct bs83b16c *gpio = container_of(work, struct bs83b16c, dis_work);
	int bank, bit, ret, offset;
	unsigned int pull;

	mutex_lock(&gpio->dis_lock);

	offset = gpio->dis_irq;
	bank = offset / BANK_SIZE;
	bit = offset % BANK_SIZE;

	pull = gpio->read(gpio->client, gpio->pull_reg[bank]);
	pull &= ~(1 << bit);

	ret = gpio->write(gpio->client, gpio->pull_reg[bank], pull);
	if (ret < 0)
		dev_err(&gpio->client->dev,
			"pull down gpio offset:%d failed\n", gpio->irq);

	mutex_unlock(&gpio->dis_lock);
}

static void bs83b16c_irq_disable(struct irq_data *data)
{
	struct bs83b16c *gpio = data->domain->host_data;
	int bank = data->hwirq / BANK_SIZE;
	int bit = data->hwirq % BANK_SIZE;

	gpio->int_status[bank] &= ~(1 << bit);
	gpio->dis_irq = data->hwirq;
	schedule_work(&gpio->dis_work);
}

#ifdef NOOP
static void noop(struct irq_data *data) { };
#endif

static struct irq_chip bs83b16c_irq_chip = {
	.name = "bs83b16c",
	.irq_enable = bs83b16c_irq_enable,
	.irq_disable = bs83b16c_irq_disable,
#ifdef NOOP
	.irq_ack = noop,
	.irq_mask = noop,
	.irq_unmask = noop,
#else
	.irq_mask = bs83b16c_irq_mask,
	.irq_unmask = bs83b16c_irq_unmask,
	.irq_set_type = bs83b16c_irq_set_type,
#endif
};

static int bs83b16c_irq_domain_map(struct irq_domain *domain, unsigned int virq,
			irq_hw_number_t hw)
{
	irq_set_chip_and_handler(virq,
				&bs83b16c_irq_chip,
				handle_edge_irq);
	return 0;
}

static struct irq_domain_ops bs83b16c_irq_domain_ops = {
	.map = bs83b16c_irq_domain_map,
};

static void bs83b16c_irq_domain_cleanup(struct bs83b16c *gpio)
{
	if (gpio->irq_domain)
		irq_domain_remove(gpio->irq_domain);

	if (gpio->irq)
		free_irq(gpio->irq, gpio);
}

static int bs83b16c_irq_domain_init(struct bs83b16c *gpio,
				struct bs83b16c_platform_data *pdata,
				struct i2c_client *client)
{
	int status;

	gpio->irq_domain = irq_domain_add_linear(client->dev.of_node,
					gpio->chip.ngpio,
					&bs83b16c_irq_domain_ops,
					gpio);
	if (!gpio->irq_domain)
		goto fail;

	/* enable gpio to irq */
	INIT_WORK(&gpio->work, bs83b16c_irq_demux_work);
	INIT_WORK(&gpio->en_work, bs83b16c_irq_enable_work);
	INIT_WORK(&gpio->dis_work, bs83b16c_irq_disable_work);

	/* enable real irq */
	status = request_irq(client->irq, bs83b16c_irq_demux,
			IRQF_TRIGGER_FALLING | IRQF_SHARED,
			dev_name(&client->dev), gpio);
			//IRQF_TRIGGER_FALLING | IRQF_TRIGGER_RISING,
	if (status)
		goto fail;

	gpio->chip.to_irq = bs83b16c_to_irq;
	gpio->irq = client->irq;

	return 0;
fail:
	bs83b16c_irq_domain_cleanup(gpio);
	return -EINVAL;
}

static void get_gpio_default_state(struct bs83b16c *gpio,
			struct i2c_client *client)
{
	int i;

	for (i = 0; i < MAX_BANK; i++) {
		 gpio->status[i] = gpio->read(client, gpio->data_reg[i]);
	}

	/* set int reg for 0xFF */
	for (i = 0; i < MAX_BANK; i++) {
		gpio->write(client, gpio->int_reg[i], 0xFF);
	}
}

static struct bs83b16c_platform_data *of_gpio_bs83b16c(struct i2c_client *client)
{
	struct device_node *node = client->dev.of_node;
	struct device *dev = &client->dev;
	struct bs83b16c_platform_data *info;
	int gpio = 0, ret = 0;

	info = devm_kzalloc(&client->dev, sizeof(*info), GFP_KERNEL);
	if (!info)
		return NULL;
	/* get expend io base */
	if (of_property_read_u32(node, "gpio_base", &info->gpio_base)) {
		dev_dbg(&client->dev, "can't not get gpio_base from dts\n");
		info->gpio_base = 1500;
	}

	/* save irq to client */
	gpio = of_get_named_gpio(node, "int-gpio", 0);
	if (gpio < 0) {
		dev_err(dev, "get int-gpio failed\n");
		ret = gpio;
		goto free_info;
	}
	client->irq = gpio_to_irq(gpio);

	/* set int gpio function */
	ret = pinctrl_pm_select_default_state(dev);
	if (ret != 0) {
		dev_err(dev, "set int gpio pin function failed\n");
		goto free_info;
	}
	return info;

free_info:
	return ERR_PTR(ret);
}

#ifdef BS_DEBUG
static irqreturn_t gpio_test_handler(int irq, void *data)
{
	printk("[%s] line=%d\n", __func__, __LINE__);
	return IRQ_HANDLED;
}

static ssize_t gpio_test_show(struct device *dev,
			struct device_attribute *attr, char *buf)
{
	int gpio = 2030;
	int ret, irq;
	unsigned int flags = IRQF_TRIGGER_FALLING | IRQF_TRIGGER_RISING;

	ret = gpio_request(gpio, "gpio_test");
	printk("gpio_request return %d\n", ret);

	ret = gpio_direction_input(gpio);
	printk("gpio_direction_input return %d\n", ret);

	irq = gpio_to_irq(gpio);
	printk("gpio to irq:%d\n", irq);

	ret = request_irq(irq, gpio_test_handler, flags, dev_name(dev), dev);
	printk("request irq return:%d\n", ret);

	disable_irq(irq);
	printk("disable irq\n");

	enable_irq(irq);
	printk("enable irq\n");

	return 0;
}

static DEVICE_ATTR(bsgpio_test, 0444, gpio_test_show, NULL);
#endif

static int bs83b16c_probe(struct i2c_client *client,
			const struct i2c_device_id *id)
{
	struct bs83b16c_platform_data *pdata;
	struct bs83b16c *gpio;
	struct device *dev = &client->dev;
	int ret, i;

	pdata = client->dev.platform_data;
	if (!pdata) {
		dev_dbg(&client->dev, "no platform data\n");
		pdata = of_gpio_bs83b16c(client);
		if (IS_ERR(pdata)) {
			dev_err(dev, "get dts for bs83b16c platform data failed\n");
		}
	}

	/* Allocate, initialize, and register this gpio_chip. */
	gpio = devm_kzalloc(&client->dev, sizeof(*gpio), GFP_KERNEL);
	if (!gpio) {
		dev_err(&client->dev, "bs83b16c struct allocate failed\n");
		return -ENOMEM;
	}

	/* check i2c dev */
	if (!i2c_check_functionality(client->adapter,
				I2C_FUNC_SMBUS_BYTE_DATA)) {
		dev_err(&client->dev, "SMBUS Byte Data not Supported\n");
		ret = -EIO;
		goto free_gpio;
	}

	mutex_init(&gpio->lock);
	mutex_init(&gpio->en_lock);
	spin_lock_init(&gpio->slock);

	gpio->chip.base			= pdata ? pdata->gpio_base : -1;
	gpio->chip.parent		= &client->dev;
	gpio->chip.can_sleep            = 1;
	gpio->chip.owner                = THIS_MODULE;
	gpio->chip.get                  = bs83b16c_get;
	gpio->chip.set                  = bs83b16c_set;
	gpio->chip.direction_input      = bs83b16c_input;
	gpio->chip.direction_output     = bs83b16c_output;
	gpio->chip.ngpio                = (id->driver_data * MAX_BANK);

	/* set reg for gpio bank */
	for (i = 0; i < MAX_BANK; i++) {
		gpio->state_reg[i] = (unsigned)(PA_STATE + i);
		gpio->data_reg[i]  = (unsigned)(PA_DATA + i);
		gpio->int_reg[i]   = (unsigned)(PA_INT + i);
		gpio->pull_reg[i]  = (unsigned)(PA_PULL + i);
	}

	gpio->write			= i2c_write;
	gpio->read			= i2c_read;

	gpio->chip.label		= client->name;
	gpio->client			= client;

	i2c_set_clientdata(client, gpio);

	gpio->out = pdata ? ~pdata->n_latch : ~0;
	//gpio->status = gpio->out;

	ret = gpiochip_add(&gpio->chip);
	if (ret < 0) {
		dev_err(&client->dev, "gpiochip_add failed\n");
		goto free_gpio;
	}

	if (pdata && pdata->setup) {
		ret = pdata->setup(client, gpio->chip.base,
				gpio->chip.ngpio, pdata->context);
		if (ret < 0) {
			dev_warn(&client->dev, "setup --> %d\n", ret);
		}
	}

	/* enable gpio_to_irq() if platform has settings */
	if (pdata && client->irq) {
		ret = bs83b16c_irq_domain_init(gpio, pdata, client);
		if (ret < 0) {
			dev_err(&client->dev, "irq domain init failed\n");
			goto fail;
		}
	}

	/* set gpio->status */
	get_gpio_default_state(gpio, client);
#if BS_DEBUG
	device_create_file(dev, &dev_attr_bsgpio_test);
#endif
	dev_info(&client->dev, "probe success\n");

	return 0;

fail:
	dev_dbg(&client->dev, "probe error %d for '%s'\n",
			ret, client->name);

	if (pdata && client->irq)
		bs83b16c_irq_domain_cleanup(gpio);

free_gpio:
	kfree(gpio);
	return ret;
}

static int bs83b16c_remove(struct i2c_client *client)
{
	struct bs83b16c *gpio = i2c_get_clientdata(client);
	struct bs83b16c_platform_data *pdata = client->dev.platform_data;
	int ret = 0;

	if (pdata && pdata->teardown) {
		ret = pdata->teardown(client, gpio->chip.base,
				gpio->chip.ngpio, pdata->context);
		if (ret < 0) {
			dev_err(&client->dev, "%s --> %d\n",
					"teardown", ret);
			return ret;
		}
	}

	if (pdata && client->irq)
		bs83b16c_irq_domain_cleanup(gpio);

	gpiochip_remove(&gpio->chip);

	return ret;
}

static struct i2c_driver bs83b16c_driver = {
	.driver = {
		.name = "bs83b16c",
		.owner = THIS_MODULE,
		.of_match_table =  of_match_ptr(bs83b16c_of_table),
	},
	.probe = bs83b16c_probe,
	.remove = bs83b16c_remove,
	.id_table = bs83b16c_id,
};

static int __init bs83b16c_init(void)
{
	return i2c_add_driver(&bs83b16c_driver);
}

/* register after i2c postcore initcall and before
 * subsys initcalls that may rely on these GPIOs
 */
subsys_initcall_sync(bs83b16c_init);

static void __exit bs83b16c_exit(void)
{
	i2c_del_driver(&bs83b16c_driver);
}
module_exit(bs83b16c_exit);

MODULE_LICENSE("GPL");
MODULE_VERSION("1.0.0");
MODULE_DESCRIPTION("GPIO expander driver for bs83b16c");
MODULE_AUTHOR("OuJiayu@allwinnertech.com");
