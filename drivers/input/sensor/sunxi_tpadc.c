/*
 * drivers/input/sensor/sunxi_tpadc.c
 *
 * Copyright (C) 2017 Allwinner.
 * liuyu <liuyu@allwinnertech.com>
 *
 * SUNXI TPADC Controller Driver
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 */

#include <linux/module.h>
#include <linux/init.h>
#include <linux/input.h>
#include <linux/delay.h>
#include <linux/slab.h>
#include <linux/interrupt.h>
#include <linux/ioport.h>
#include <asm/irq.h>
#include <asm/io.h>
#include <linux/errno.h>
#include <linux/err.h>
#include <linux/timer.h>
#include <linux/clk.h>
#include <linux/irq.h>
#include <linux/of_platform.h>
#include <linux/of_irq.h>
#include <linux/of_address.h>
#include <linux/pm.h>
#include <linux/err.h>
#include <linux/clkdev.h>
#include <linux/clk-provider.h>
#include <linux/clk/sunxi.h>
#include <linux/reset.h>
#include <linux/gpio.h>
#include <asm/io.h>
#include "sunxi_tpadc.h"

static u32 debug_mask = 1;
#define dprintk(level_mask, fmt, arg...)				\
do {									\
	if (unlikely(debug_mask & level_mask))				\
		pr_warn("%s()%d - "fmt, __func__, __LINE__, ##arg);	\
} while (0)

static void sunxi_tpadc_clk_division(void __iomem *reg_base, unsigned char divider)
{
	u32 reg;

	reg = readl(reg_base + TP_CTRL0_REG);
	reg |= (divider & 0x3) << ADC_CLK_DIVIDER;
	writel(reg, reg_base + TP_CTRL0_REG);
}

static void sunxi_tpadc_fs_set(void __iomem *reg_base, unsigned char divider)
{
	u32 reg;

	reg = readl(reg_base + TP_CTRL0_REG);
	reg |= (divider & 0xf) << FS_DIVIDER;
	writel(reg, reg_base + TP_CTRL0_REG);
}

static void sunxi_tpadc_ta_set(void __iomem *reg_base, int val)
{
	int reg;

	reg = readl(reg_base + TP_CTRL0_REG);
	reg |= val & 0xfff;
	writel(reg, reg_base + TP_CTRL0_REG);
}

static void sunxi_tpadc_clk_select(void __iomem *reg_base, bool on)
{
	u32 reg;

	reg = readl(reg_base + TP_CTRL0_REG);
	if (on)
		reg &= ~TP_CLOCK;
	else
		reg |= TP_CLOCK;
	writel(reg, reg_base + TP_CTRL0_REG);
}

static void sunxi_tpadc_select_adc(void __iomem *reg_base)
{
	int reg;

	reg = readl(reg_base + TP_CTRL1_REG);
	reg |= ADC_ENABLE;
	writel(reg, reg_base + TP_CTRL1_REG);
}

static void sunxi_tpadc_rtp_enable(void __iomem *reg_base)
{
	int reg;

	reg = readl(reg_base + TP_CTRL1_REG);
	reg |= RTP_MODE_ENABLE;
	writel(reg, reg_base + TP_CTRL1_REG);
}

static void sunxi_tpadc_filter_enable(void __iomem *reg_base)
{
	int reg;

	reg = readl(reg_base + TP_CTRL3_REG);
	reg |= RTP_FILTER_ENABLE;
	writel(reg, reg_base + TP_CTRL3_REG);
}
static void sunxi_tpadc_filter_type(void __iomem *reg_base)
{
	int reg;

	reg = readl(reg_base + TP_CTRL3_REG);
	reg |= RTP_FILTER_TYPE;
	writel(reg, reg_base + TP_CTRL3_REG);
}

static void sunxi_tpadc_trig_level(void __iomem *reg_base, int val)
{
	int reg;

	reg = readl(reg_base + TP_INT_FIFOC_REG);
	reg |= (val & 0xf)<<8;
	writel(reg, reg_base + TP_INT_FIFOC_REG);
}
static void sunxi_tpadc_downup_irq(void __iomem *reg_base)
{
	int reg;

	reg = readl(reg_base + TP_INT_FIFOC_REG);
	reg |= 0x03;
	writel(reg, reg_base + TP_INT_FIFOC_REG);
}

static u32 sunxi_tpadc_ch_select(void __iomem *reg_base, enum tp_channel_id id)
{
	u32 reg_val;

	reg_val = readl(reg_base + TP_CTRL1_REG);
	switch (id) {
	case TP_CH_0:
		reg_val |= TP_CH0_SELECT;
		break;
	case TP_CH_1:
		reg_val |= TP_CH1_SELECT;
		break;
	case TP_CH_2:
		reg_val |= TP_CH2_SELECT;
		break;
	case TP_CH_3:
		reg_val |= TP_CH3_SELECT;
		break;
	default:
		pr_err("%s, invalid channel id!", __func__);
		return -EINVAL;
	}
	writel(reg_val, reg_base + TP_CTRL1_REG);

	return 0;
}

static int sunxi_tpadc_ch_deselect(void __iomem *reg_base, enum tp_channel_id id)
{
	u32 reg_val;

	reg_val = readl(reg_base + TP_CTRL1_REG);
	switch (id) {
	case TP_CH_0:
		reg_val &= ~TP_CH0_SELECT;
		break;
	case TP_CH_1:
		reg_val &= ~TP_CH1_SELECT;
		break;
	case TP_CH_2:
		reg_val &= ~TP_CH2_SELECT;
		break;
	case TP_CH_3:
		reg_val &= ~TP_CH3_SELECT;
		break;
	default:
		pr_err("%s, invalid channel id!", __func__);
		return -EINVAL;
	}
	writel(reg_val, reg_base + TP_CTRL1_REG);

	return 0;
}

static void sunxi_tpadc_enable_irq(void __iomem *reg_base)
{
	u32 reg_val;

	reg_val = readl(reg_base + TP_INT_FIFOC_REG);
	reg_val |= TP_DATA_IRQ_EN;
	writel(reg_val, reg_base + TP_INT_FIFOC_REG);

}

static void sunxi_tpadc_disable_irq(void __iomem *reg_base)
{
	u32 reg_val;

	reg_val = readl(reg_base + TP_INT_FIFOC_REG);
	reg_val &= ~TP_DATA_IRQ_EN;
	writel(reg_val, reg_base + TP_INT_FIFOC_REG);
}

static u32 sunxi_tpadc_irq_status(void __iomem *reg_base)
{
	return readl(reg_base + TP_INT_FIFOS_REG);
}

static void sunxi_tpadc_clear_pending(void __iomem *reg_base, int reg_val)
{
	writel(reg_val, reg_base + TP_INT_FIFOS_REG);
}

static u32 sunxi_tpadc_data_read(void __iomem *reg_base)
{
	return readl(reg_base + TP_DATA_REG);
}

static u32 sunxi_tpadc_clk_config(void __iomem *reg_base, u32 clk_in, u32 sclk_freq)
{
	u32 freq, divider = sclk_freq / clk_in;
	u32 val;

	if (divider > 3 && divider < 6)
		divider = 3;
	if (divider > 6)
		divider = 6;

	switch (divider) {
	case 1:
		val = 0x03;
		break;
	case 2:
		val = 0x00;
		break;
	case 3:
		val = 0x01;
		break;
	case 6:
		val = 0x02;
		break;
	default:
		break;
	}
	sunxi_tpadc_clk_division(reg_base, val);
	freq = sclk_freq / divider;

	return freq;
}
static u32 sunxi_tpadc_fs_config(void __iomem *reg_base, u32 clk_in, u32 fs_div)
{
	u32 sample_freq;

	sunxi_tpadc_fs_set(reg_base, fs_div);
	sample_freq = clk_in / (1 << (20 - fs_div));

	return sample_freq;
}

static int sunxi_tpadc_hw_setup(struct device_node *np,
				struct sunxi_tpadc *sunxi_tpadc)
{
	int ret;

	if (!of_device_is_available(np)) {
		TPADC_ERR("sunxi tpadc is disable\n");
		return -EPERM;
	}

	sunxi_tpadc->mem_res = platform_get_resource(sunxi_tpadc->pdev, IORESOURCE_MEM, 0);
	if (sunxi_tpadc->mem_res == NULL) {
		TPADC_ERR("sunxi tpadc failed to get MEM res \n");
		return -ENXIO;
	}

	if (request_mem_region(sunxi_tpadc->mem_res->start,
				resource_size(sunxi_tpadc->mem_res),
				sunxi_tpadc->mem_res->name) == NULL) {
		TPADC_ERR("sunxi_tpadc failed to request mem region\n");
		return -EINVAL;
	}

	sunxi_tpadc->reg_base = ioremap(sunxi_tpadc->mem_res->start,
					resource_size(sunxi_tpadc->mem_res));
	if (sunxi_tpadc->reg_base == NULL) {
		ret = -EIO;
		goto eiomap;
	}

	sunxi_tpadc->irq_num = platform_get_irq(sunxi_tpadc->pdev, 0);
	if (sunxi_tpadc->irq_num < 0) {
		TPADC_ERR("sunxi_tpadc failed to get irq\n");
		ret = -EINVAL;
		goto ereqirq;
	}

	ret = of_property_read_u32(np, "clock-frequency", &sunxi_tpadc->clk_in);
	if (ret) {
		TPADC_ERR("tpadc get clock-frequency property failed! \n");
		return -1;
	}

	return 0;

ereqirq:
	iounmap(sunxi_tpadc->reg_base);

eiomap:
	release_mem_region(sunxi_tpadc->mem_res->start,
				resource_size(sunxi_tpadc->mem_res));

	return ret;
}

static int sunxi_tpadc_request_clk(struct sunxi_tpadc *sunxi_tpadc)
{
	sunxi_tpadc->mod_clk = devm_clk_get(&sunxi_tpadc->pdev->dev, "mod");
	if (IS_ERR_OR_NULL(sunxi_tpadc->mod_clk)) {
		TPADC_ERR("get tpadc mode clock failed \n");
		return -ENXIO;
	}

	sunxi_tpadc->bus_clk = devm_clk_get(&sunxi_tpadc->pdev->dev, "bus");
	if (IS_ERR_OR_NULL(sunxi_tpadc->bus_clk)) {
		TPADC_ERR("get tpadc bus clock failed\n");
		return -1;
	}

	sunxi_tpadc->reset = devm_reset_control_get(&sunxi_tpadc->pdev->dev, NULL);
	if (IS_ERR_OR_NULL(sunxi_tpadc->reset)) {
		TPADC_ERR("get tpadc reset failed\n");
		return -1;
	}

	return 0;
}

static int sunxi_tpadc_clk_init(struct sunxi_tpadc *sunxi_tpadc)
{
	if (reset_control_deassert(sunxi_tpadc->reset)) {
		pr_err("reset control deassert failed! \n");
		return -1;
	}

	if (clk_prepare_enable(sunxi_tpadc->mod_clk)) {
		pr_err("enable tpadc mod clock failed! \n");
		return -1;
	}

	if (clk_prepare_enable(sunxi_tpadc->bus_clk)) {
		pr_err("enable tpadc bus clock failed! \n");
		return -1;
	}
	return 0;
}

static void sunxi_tpadc_clk_exit(struct sunxi_tpadc *sunxi_tpadc)
{
	if (!IS_ERR_OR_NULL(sunxi_tpadc->bus_clk) && __clk_is_enabled(sunxi_tpadc->bus_clk)) {
		clk_disable_unprepare(sunxi_tpadc->bus_clk);
	}
}

static int sunxi_tpadc_hw_init(struct sunxi_tpadc *sunxi_tpadc)
{
	u32 clk;

//	sunxi_tpadc_clk_select(sunxi_tpadc->reg_base, HOSC);
	if (sunxi_tpadc_request_clk(sunxi_tpadc)) {
		TPADC_ERR("request tpadc clk failed!\n");
		return -EPROBE_DEFER;
	}

	if (sunxi_tpadc_clk_init(sunxi_tpadc)) {
		pr_err("[tpadc%d] init tpadc clk failed!\n", sunxi_tpadc->bus_num);
	}

	clk = sunxi_tpadc_clk_config(sunxi_tpadc->reg_base,
					sunxi_tpadc->clk_in,
					OSC_FREQUENCY);
	sunxi_tpadc_fs_config(sunxi_tpadc->reg_base, clk, 0x06);
	sunxi_tpadc_ta_set(sunxi_tpadc->reg_base, 0xff);
	sunxi_tpadc_select_adc(sunxi_tpadc->reg_base);
	sunxi_tpadc_ch_select(sunxi_tpadc->reg_base, 0);
	sunxi_tpadc_ch_select(sunxi_tpadc->reg_base, 2);
	sunxi_tpadc_enable_irq(sunxi_tpadc->reg_base);
	sunxi_tpadc_filter_enable(sunxi_tpadc->reg_base);
	sunxi_tpadc_filter_type(sunxi_tpadc->reg_base);
	sunxi_tpadc_trig_level(sunxi_tpadc->reg_base, 0x03);
	sunxi_tpadc_downup_irq(sunxi_tpadc->reg_base);
	sunxi_tpadc_rtp_enable(sunxi_tpadc->reg_base);

	return 0;
}

static irqreturn_t sunxi_tpadc_handler(int irqno, void *dev_id)
{
	struct sunxi_tpadc *sunxi_tpadc = (struct sunxi_tpadc *)dev_id;
	u32  reg_val;
	u32 x_data, y_data;

	reg_val = sunxi_tpadc_irq_status(sunxi_tpadc->reg_base);
	sunxi_tpadc_clear_pending(sunxi_tpadc->reg_base, reg_val);

	/* the first time read sunxi tpadc data reg, you can get x data
	 * the second time read sunxi tpadc data reg, you can get y dta
	 * and so on*/
	if (reg_val & TP_FIFO_DATA) {
		x_data = sunxi_tpadc_data_read(sunxi_tpadc->reg_base);
		y_data = sunxi_tpadc_data_read(sunxi_tpadc->reg_base);

		input_report_abs(sunxi_tpadc->input_tp, ABS_X, x_data);
		input_report_abs(sunxi_tpadc->input_tp, ABS_Y, y_data);
		input_sync(sunxi_tpadc->input_tp);
	}

	return IRQ_HANDLED;
}

static int sunxi_tpadc_register(struct sunxi_tpadc *st)
{
	int ret;

	st->input_tp = devm_input_allocate_device(&st->pdev->dev);
	if (IS_ERR_OR_NULL(st->input_tp)) {
		TPADC_ERR("not enough memory for input device \n");
		return -ENOMEM;
	}

	st->input_tp->name = "sunxitpadc_touchscren";
	st->input_tp->phys = "sunxitpadc/input0";

	st->input_tp->id.bustype = BUS_HOST;
	st->input_tp->id.vendor = 0x0001;
	st->input_tp->id.product = 0x0001;
	st->input_tp->id.version = 0x0100;

	st->input_tp->evbit[0] =  BIT(EV_SYN) | BIT(EV_KEY) | BIT(EV_ABS);
	__set_bit(BTN_TOUCH, st->input_tp->keybit);
	input_set_abs_params(st->input_tp, ABS_X, 0, TPADC_WIDTH, 0, 0);
	input_set_abs_params(st->input_tp, ABS_Y, 0, TPADC_WIDTH, 0, 0);

	input_set_drvdata(st->input_tp, st);

	ret = input_register_device(st->input_tp);
	if (ret) {
		pr_err("sunxi tpadc register fail \n");
		goto fail;
	}

	return 0;

fail:
	input_unregister_device(st->input_tp);
	input_free_device(st->input_tp);

	return ret;
}

int sunxi_tpadc_probe(struct platform_device *pdev)
{
	struct device_node *np = pdev->dev.of_node;
	struct sunxi_tpadc *sunxi_tpadc;
	int ret = 0;

	dprintk(DEBUG_INIT, "sunxi tpadc  probe start \n");

	sunxi_tpadc = devm_kzalloc(&pdev->dev,
				sizeof(struct sunxi_tpadc),
				GFP_KERNEL);
	if (IS_ERR_OR_NULL(sunxi_tpadc)) {
		TPADC_ERR("TPADC failed to kzalloc sunxi_tpadc struct\n");
		return -ENOMEM;
	}
	sunxi_tpadc->pdev = pdev;

	ret = sunxi_tpadc_hw_setup(pdev->dev.of_node, sunxi_tpadc);
	if (ret) {
		TPADC_ERR("TPADC failed to setup \n");
		return ret;
	}

	ret = sunxi_tpadc_hw_init(sunxi_tpadc);
	if (ret) {
		TPADC_ERR("TPADC failed to init \n");
		goto fail1;
	}

	sunxi_tpadc_register(sunxi_tpadc);

	ret = request_irq(sunxi_tpadc->irq_num,
			sunxi_tpadc_handler,
			IRQF_TRIGGER_RISING,
			"sunxi-tpadc",
			sunxi_tpadc);
	if (ret) {
		TPADC_ERR("sunxi_tpadc request irq failed\n");
		return ret;
	}

	platform_set_drvdata(pdev, sunxi_tpadc);

	dprintk(DEBUG_INIT, "sunxi tpadc probe success\n");

	return 0;

fail1:
	iounmap(sunxi_tpadc->reg_base);
	release_mem_region(sunxi_tpadc->mem_res->start, resource_size(sunxi_tpadc->mem_res));
	return ret;
}

int sunxi_tpadc_remove(struct platform_device *pdev)
{
	struct sunxi_tpadc *sunxi_tpadc = platform_get_drvdata(pdev);

	sunxi_tpadc_disable_irq(sunxi_tpadc->reg_base);
	sunxi_tpadc_ch_deselect(sunxi_tpadc->reg_base, 0);
	sunxi_tpadc_ch_deselect(sunxi_tpadc->reg_base, 2);
	free_irq(sunxi_tpadc->irq_num, sunxi_tpadc);
	iounmap(sunxi_tpadc->reg_base);

	input_unregister_device(sunxi_tpadc->input_tp);
	input_free_device(sunxi_tpadc->input_tp);

	sunxi_tpadc_clk_exit(sunxi_tpadc);

	kfree(sunxi_tpadc);

	return 0;
}

#ifdef CONFIG_PM
static int sunxi_tpadc_suspend(struct device *dev)
{
	struct platform_device *pdev = to_platform_device(dev);
	struct sunxi_tpadc *sunxi_tpadc = platform_get_drvdata(pdev);

	sunxi_tpadc_disable_irq(sunxi_tpadc->reg_base);
	sunxi_tpadc_ch_deselect(sunxi_tpadc->reg_base, 0);
	sunxi_tpadc_ch_deselect(sunxi_tpadc->reg_base, 2);
	disable_irq_nosync(sunxi_tpadc->irq_num);

	return 0;
}

static int sunxi_tpadc_resume(struct device *dev)
{
	struct platform_device *pdev = to_platform_device(dev);
	struct sunxi_tpadc *sunxi_tpadc = platform_get_drvdata(pdev);

	sunxi_tpadc_enable_irq(sunxi_tpadc->reg_base);
	sunxi_tpadc_ch_select(sunxi_tpadc->reg_base, 0);
	sunxi_tpadc_ch_select(sunxi_tpadc->reg_base, 2);
	enable_irq(sunxi_tpadc->irq_num);

	return 0;
}

static const struct dev_pm_ops sunxi_tpadc_dev_pm_ops = {
	.suspend = sunxi_tpadc_suspend,
	.resume = sunxi_tpadc_resume,
};

#define SUNXI_TPADC_DEV_PM_OPS (&sunxi_tpadc_dev_pm_ops)
#else
#define SUNXI_TPADC_DEV_PM_OPS NULL
#endif

static struct of_device_id sunxi_tpadc_of_match[] = {
	{ .compatible = "allwinner,sunxi-tpadc"},
	{},
};
MODULE_DEVICE_TABLE(of, sunxi_tpadc_of_match);

static struct platform_driver sunxi_tpadc_driver = {
	.probe  = sunxi_tpadc_probe,
	.remove = sunxi_tpadc_remove,
	.driver = {
		.name   = "sunxi-tpadc",
		.owner  = THIS_MODULE,
		.pm = SUNXI_TPADC_DEV_PM_OPS,
		.of_match_table = of_match_ptr(sunxi_tpadc_of_match),
	},
};
module_platform_driver(sunxi_tpadc_driver);

MODULE_AUTHOR("liuyu");
MODULE_DESCRIPTION("sunxi-tpadc driver");
MODULE_LICENSE("GPL");
