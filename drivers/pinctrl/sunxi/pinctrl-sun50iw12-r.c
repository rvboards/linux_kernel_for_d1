// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (c) 2020 frank@allwinnertech.com
 */

#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/of.h>
#include <linux/of_device.h>
#include <linux/pinctrl/pinctrl.h>

#include "pinctrl-sunxi.h"

static const struct sunxi_desc_pin sun50iw12_r_pins[] = {
	SUNXI_PIN(SUNXI_PINCTRL_PIN(L, 0),
		SUNXI_FUNCTION(0x0, "gpio_in"),
		SUNXI_FUNCTION(0x1, "gpio_out"),
		SUNXI_FUNCTION(0x2, "s_i2c0"),		/* SCK */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 0, 0)),
	SUNXI_PIN(SUNXI_PINCTRL_PIN(L, 1),
		SUNXI_FUNCTION(0x0, "gpio_in"),
		SUNXI_FUNCTION(0x1, "gpio_out"),
		SUNXI_FUNCTION(0x2, "s_i2c0"),		/* SDA */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 0, 1)),
	SUNXI_PIN(SUNXI_PINCTRL_PIN(L, 2),
		SUNXI_FUNCTION(0x0, "gpio_in"),
		SUNXI_FUNCTION(0x1, "gpio_out"),
		SUNXI_FUNCTION(0x2, "s_uart0"),		/* TX */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 0, 2)),
	SUNXI_PIN(SUNXI_PINCTRL_PIN(L, 3),
		SUNXI_FUNCTION(0x0, "gpio_in"),
		SUNXI_FUNCTION(0x1, "gpio_out"),
		SUNXI_FUNCTION(0x2, "s_uart0"),		/* RX */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 0, 3)),
	SUNXI_PIN(SUNXI_PINCTRL_PIN(L, 4),
		SUNXI_FUNCTION(0x0, "gpio_in"),
		SUNXI_FUNCTION(0x1, "gpio_out"),
		SUNXI_FUNCTION(0x2, "s_jtag0"),		/* MS */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 0, 4)),
	SUNXI_PIN(SUNXI_PINCTRL_PIN(L, 5),
		SUNXI_FUNCTION(0x0, "gpio_in"),
		SUNXI_FUNCTION(0x1, "gpio_out"),
		SUNXI_FUNCTION(0x2, "s_jtag0"),		/* CK */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 0, 5)),
	SUNXI_PIN(SUNXI_PINCTRL_PIN(L, 6),
		SUNXI_FUNCTION(0x0, "gpio_in"),
		SUNXI_FUNCTION(0x1, "gpio_out"),
		SUNXI_FUNCTION(0x2, "s_jtag0"),		/* DO */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 0, 6)),
	SUNXI_PIN(SUNXI_PINCTRL_PIN(L, 7),
		SUNXI_FUNCTION(0x0, "gpio_in"),
		SUNXI_FUNCTION(0x1, "gpio_out"),
		SUNXI_FUNCTION(0x2, "s_jtag0"),		/* DI */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 0, 7)),
	SUNXI_PIN(SUNXI_PINCTRL_PIN(L, 8),
		SUNXI_FUNCTION(0x0, "gpio_in"),
		SUNXI_FUNCTION(0x1, "gpio_out"),
		SUNXI_FUNCTION(0x2, "s_i2c1"),		/* SCK */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 0, 8)),
	SUNXI_PIN(SUNXI_PINCTRL_PIN(L, 9),
		SUNXI_FUNCTION(0x0, "gpio_in"),
		SUNXI_FUNCTION(0x1, "gpio_out"),
		SUNXI_FUNCTION(0x2, "s_i2c1"),		/* SDA */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 0, 9)),
	SUNXI_PIN(SUNXI_PINCTRL_PIN(L, 10),
		SUNXI_FUNCTION(0x0, "gpio_in"),
		SUNXI_FUNCTION(0x1, "gpio_out"),
		SUNXI_FUNCTION(0x2, "s_pwm"),		/* S_PWM0 */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 0, 10)),
	SUNXI_PIN(SUNXI_PINCTRL_PIN(L, 11),
		SUNXI_FUNCTION(0x0, "gpio_in"),
		SUNXI_FUNCTION(0x1, "gpio_out"),
		SUNXI_FUNCTION(0x2, "s_cpu"),		/* CUR_W */
		SUNXI_FUNCTION(0x3, "s_cir"),		/* IN */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 0, 11)),
};

static const struct sunxi_pinctrl_desc sun50iw12_r_pinctrl_data = {
	.pins = sun50iw12_r_pins,
	.npins = ARRAY_SIZE(sun50iw12_r_pins),
	.pin_base = SUNXI_PIN_BASE('L'),
	.irq_banks = 1,
	.hw_type = SUNXI_PCTL_HW_TYPE_1,
};

static int sun50iw12_r_pinctrl_probe(struct platform_device *pdev)
{
	return sunxi_pinctrl_init(pdev, &sun50iw12_r_pinctrl_data);
}

static struct of_device_id sun50iw12_r_pinctrl_match[] = {
	{ .compatible = "allwinner,sun50iw12-r-pinctrl", },
	{}
};
MODULE_DEVICE_TABLE(of, sun50iw12_r_pinctrl_match);

static struct platform_driver sun50iw12_r_pinctrl_driver = {
	.probe	= sun50iw12_r_pinctrl_probe,
	.driver	= {
		.name		= "sun50iw12-r-pinctrl",
		.of_match_table	= sun50iw12_r_pinctrl_match,
	},
};
module_platform_driver(sun50iw12_r_pinctrl_driver);

MODULE_DESCRIPTION("Allwinner sun50iw12 R_PIO pinctrl driver");
MODULE_LICENSE("GPL");
