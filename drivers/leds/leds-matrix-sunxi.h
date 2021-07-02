/*
 * Copyright (C) 2018 Allwinner Technology Limited. All rights reserved.
 * OuJiayu <OuJiayu@allwinnertech.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 */

#ifndef __LINUX_LEDS_MATRIX_SUNXI_H
#define __LINUX_LEDS_MATRIX_SUNXI_H

struct matrix_led_data {
	struct led_classdev cdev;
	unsigned int row_gpio;
	unsigned int col_gpio;
	bool can_sleep;
	u8 *flag;
};

struct matrix_led_priv {
	int led_cnt;
	int light_cnt;
	u8 flag;
	struct delayed_work work;
	struct matrix_led_platform_data *pdata;
	struct matrix_led_data leds[];
};

struct matrix_led_platform_data {
	struct device *dev;
	const unsigned int *row_gpios;
	const unsigned int *col_gpios;
	unsigned int num_row_gpios;
	unsigned int num_col_gpios;
	int delay_us;
	bool refresh; /* use for judge if dts set refresh */
	int wait; /* use for wait event */
};

#endif
