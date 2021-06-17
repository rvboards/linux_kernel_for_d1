/*
 * Copyright (C) 2021 Allwinner Technology Limited. All rights reserved.
 * OuJiayu <OuJiayu@allwinnertech.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 */

#ifndef __LINUX_SUNXI_BS83B16C_H
#define __LINUX_SUNXI_BS83B16C_H

#define KEY_FUNC	0
#define LED_FUNC	1

#define DATA_START	0x00
#define DATA_END	0x07

#define	KEY_EN_REG	0x08
#define KEY_HV_INIT	0x09
#define KEY_STATE	0x0a
#define KEY_HV		0x0b

#define LED_EN_REG	0x0c
#define LED_HV_INIT	0x0d
#define LED_STATE	0x0e
#define LED_HV		0x0f

#define DELAY_REG	0x10
#define FILTER_REG	0x11

#define PIN_PB0		8
#define PIN_PC0		16

#define KEY_MAX_COLS	16
#define LED_MAX_CNT	64

struct led_data {
	struct led_classdev cdev;
	unsigned int row;
	unsigned int col;
	u8 *flag;
};

struct sunxi_bs83b16c_pdata {
	struct i2c_client	*client;
	struct input_dev	*input_dev;
	struct work_struct	work;
	spinlock_t		slock;
	struct led_classdev	cdev;
	struct led_data		*leds;

	unsigned int		*keycode;
	unsigned int		func;
	unsigned int		key_row_num;
	unsigned int		key_col_num;
	unsigned int		led_row_num;
	unsigned int		led_col_num;
	int			row_shift;
	int			int_irq;
	u8			flag;
	unsigned int		key_scan_udelay;
	unsigned int		last_key_state[KEY_MAX_COLS];
};

#endif
