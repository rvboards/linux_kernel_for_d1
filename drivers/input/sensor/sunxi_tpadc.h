/*
 * drivers/input/sensor/sunxi_tpadc.h
 *
 * Copyright (C) 2017 Allwinner.
 * fuzhaoke <yangshounan@allwinnertech.com>
 *
 * SUNXI tPADC Controller Driver Header
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 */

#ifndef SUNXI_GPADC_H
#define SUNXI_GPADC_H

/*abs_x and abs_y*/
#define TPADC_CHANNEL_NUM	2
#define TPADC_DEV_NAME		("sunxi-tpadc")
#define TPADC_WIDTH		4095  /*12bit 0XFFF*/

/* TPADC register offset */
#define TP_CTRL0_REG		0x00
#define TP_CTRL1_REG		0x04
#define TP_CTRL2_REG		0x08
#define TP_CTRL3_REG		0x0c
#define TP_INT_FIFOC_REG	0x10
#define	TP_INT_FIFOS_REG	0x14
#define TP_CDAT_REG		0x1c
#define TP_DATA_REG		0x24

#define ADC_CLK_DIVIDER		20
#define FS_DIVIDER		16
#define TP_CLOCK		(1<<22)
#define ADC_ENABLE		(1<<4)
#define TP_DATA_IRQ_EN		(1<<16)
#define TP_FIFO_DATA		(1<<16)
#define TP_UP_PENDING		(1<<1)
#define RTP_MODE_ENABLE		(1<<5)
#define RTP_FILTER_ENABLE	(1<<2)
#define RTP_FILTER_TYPE	(0x2<<0)

#define OSC_FREQUENCY		24000000

#define HOSC			1
#define TP_IO_INPUT_MODE	0xfffff

#define TP_CH3_SELECT		(1 << 3) /* channale 3 select enable,  0:disable, 1:enable */
#define TP_CH2_SELECT		(1 << 2) /* channale 2 select enable,  0:disable, 1:enable */
#define TP_CH1_SELECT		(1 << 1) /* channale 1 select enable,  0:disable, 1:enable */
#define TP_CH0_SELECT		(1 << 0) /* channale 0 select enable,  0:disable, 1:enable */

/* For debug */
#define TPADC_ERR(fmt, arg...) pr_err("%s()%d - "fmt, __func__, __LINE__, ##arg)

enum tp_channel_id {
	TP_CH_0 = 0,
	TP_CH_1,
	TP_CH_2,
	TP_CH_3,
};

enum {
	DEBUG_INIT    = 1U << 0,
	DEBUG_SUSPEND = 1U << 1,
	DEBUG_DATA    = 1U << 2,
	DEBUG_INFO    = 1U << 3,
};

/**
 * struct sunxi_tpadc - interface to tpadc for AW
 * @dev: device interface to this driver
 * @input_channel[] : channel0 : x data , channel2 : y data
 * timer :
 * mem_res : parsing the address
 * reg_addr : tpadc start reg
 * irq_num : irq num
 * clk_in : tpadc controller frequency
*/
struct sunxi_tpadc {
	struct platform_device	*pdev;
	struct input_dev *input_tp;
	struct timer_list timer;
	struct resource	*mem_res;
	void __iomem *reg_base;
	int irq_num;
	int bus_num;
	struct device *dev;
	struct clk *bus_clk;
	struct clk *mod_clk;
	struct reset_control *reset;
	int clk_in;
};

#endif
