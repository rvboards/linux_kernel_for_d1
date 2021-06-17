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

static const struct sunxi_desc_pin sun50iw12_pins[] = {
	SUNXI_PIN(SUNXI_PINCTRL_PIN(A, 0),
		SUNXI_FUNCTION(0x0, "gpio_in"),
		SUNXI_FUNCTION(0x1, "gpio_out"),
		SUNXI_FUNCTION(0x2, "gmac0"),		/* TX */
		SUNXI_FUNCTION(0x3, "spi2"),		/* CS */
		SUNXI_FUNCTION(0x4, "jtag"),		/* MS */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 0, 0)),
	SUNXI_PIN(SUNXI_PINCTRL_PIN(A, 1),
		SUNXI_FUNCTION(0x0, "gpio_in"),
		SUNXI_FUNCTION(0x1, "gpio_out"),
		SUNXI_FUNCTION(0x2, "gmac0"),		/* RX */
		SUNXI_FUNCTION(0x3, "spi2"),		/* CLK */
		SUNXI_FUNCTION(0x4, "jtag"),		/* CK */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 0, 1)),
	SUNXI_PIN(SUNXI_PINCTRL_PIN(A, 2),
		SUNXI_FUNCTION(0x0, "gpio_in"),
		SUNXI_FUNCTION(0x1, "gpio_out"),
		SUNXI_FUNCTION(0x2, "gmac0"),		/* RTS */
		SUNXI_FUNCTION(0x3, "spi2"),		/* MOSI */
		SUNXI_FUNCTION(0x4, "jtag"),		/* DO */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 0, 2)),
	SUNXI_PIN(SUNXI_PINCTRL_PIN(A, 3),
		SUNXI_FUNCTION(0x0, "gpio_in"),
		SUNXI_FUNCTION(0x1, "gpio_out"),
		SUNXI_FUNCTION(0x2, "gmac0"),		/* CTS */
		SUNXI_FUNCTION(0x3, "spi2"),		/* MISO */
		SUNXI_FUNCTION(0x4, "jtag"),		/* DI */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 0, 3)),
	SUNXI_PIN(SUNXI_PINCTRL_PIN(A, 4),
		SUNXI_FUNCTION(0x0, "gpio_in"),
		SUNXI_FUNCTION(0x1, "gpio_out"),
		SUNXI_FUNCTION(0x2, "gmac0"),		/* SCK */
		SUNXI_FUNCTION(0x3, "i2s0"),		/* MCLK */
		SUNXI_FUNCTION(0x4, "jtag"),		/* MS_GPU */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 0, 4)),
	SUNXI_PIN(SUNXI_PINCTRL_PIN(A, 5),
		SUNXI_FUNCTION(0x0, "gpio_in"),
		SUNXI_FUNCTION(0x1, "gpio_out"),
		SUNXI_FUNCTION(0x2, "gmac0"),		/* SDA */
		SUNXI_FUNCTION(0x3, "i2s0"),		/* BCLK */
		SUNXI_FUNCTION(0x4, "jtag"),		/* CK_GPU */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 0, 5)),
	SUNXI_PIN(SUNXI_PINCTRL_PIN(A, 6),
		SUNXI_FUNCTION(0x0, "gpio_in"),
		SUNXI_FUNCTION(0x1, "gpio_out"),
		SUNXI_FUNCTION(0x2, "gmac0"),		/* SDA */
		SUNXI_FUNCTION(0x3, "i2s0"),		/* LRCK */
		SUNXI_FUNCTION(0x4, "jtag"),		/* DO_GPU */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 0, 6)),
	SUNXI_PIN(SUNXI_PINCTRL_PIN(A, 7),
		SUNXI_FUNCTION(0x0, "gpio_in"),
		SUNXI_FUNCTION(0x1, "gpio_out"),
		SUNXI_FUNCTION(0x2, "gmac0"),		/* IN */
		SUNXI_FUNCTION(0x3, "i2s0"),		/* DOUT */
		SUNXI_FUNCTION(0x4, "i2s0"),		/* DIN */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 0, 7)),
	SUNXI_PIN(SUNXI_PINCTRL_PIN(A, 8),
		SUNXI_FUNCTION(0x0, "gpio_in"),
		SUNXI_FUNCTION(0x1, "gpio_out"),
		SUNXI_FUNCTION(0x2, "gmac0"),		/* OUT */
		SUNXI_FUNCTION(0x3, "i2s0"),		/* DIN */
		SUNXI_FUNCTION(0x4, "i2s0"),		/* DOUT */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 0, 8)),
	SUNXI_PIN(SUNXI_PINCTRL_PIN(A, 9),
		SUNXI_FUNCTION(0x0, "gpio_in"),
		SUNXI_FUNCTION(0x1, "gpio_out"),
		SUNXI_FUNCTION(0x2, "uart0"),		/* TX */
		SUNXI_FUNCTION(0x3, "i2c0"),		/* SCK */
		SUNXI_FUNCTION(0x4, "jtag"),		/* DI_CPU */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 0, 9)),
	SUNXI_PIN(SUNXI_PINCTRL_PIN(A, 10),
		SUNXI_FUNCTION(0x0, "gpio_in"),
		SUNXI_FUNCTION(0x1, "gpio_out"),
		SUNXI_FUNCTION(0x2, "gmac0"),		/* RX */
		SUNXI_FUNCTION(0x3, "i2c0"),		/* SDA */
		SUNXI_FUNCTION(0x4, "pwm1"),
		SUNXI_FUNCTION_IRQ_BANK(0xe, 0, 10)),
	SUNXI_PIN(SUNXI_PINCTRL_PIN(A, 11),
		SUNXI_FUNCTION(0x0, "gpio_in"),
		SUNXI_FUNCTION(0x1, "gpio_out"),
		SUNXI_FUNCTION(0x2, "gmac0"),		/* TX */
		SUNXI_FUNCTION(0x3, "i2c0"),		/* SCK */
		SUNXI_FUNCTION(0x4, "jtag"),		/* DI_GPU */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 0, 11)),
	SUNXI_PIN(SUNXI_PINCTRL_PIN(A, 12),
		SUNXI_FUNCTION(0x0, "gpio_in"),
		SUNXI_FUNCTION(0x1, "gpio_out"),
		SUNXI_FUNCTION(0x2, "gmac0"),		/* RX */
		SUNXI_FUNCTION(0x3, "twi0"),		/* SDA */
		SUNXI_FUNCTION(0x4, "pwm1"),
		SUNXI_FUNCTION_IRQ_BANK(0xe, 0, 12)),
	SUNXI_PIN(SUNXI_PINCTRL_PIN(A, 13),
		SUNXI_FUNCTION(0x0, "gpio_in"),
		SUNXI_FUNCTION(0x1, "gpio_out"),
		SUNXI_FUNCTION(0x2, "gmac0"),		/* RX */
		SUNXI_FUNCTION(0x3, "twi0"),		/* SDA */
		SUNXI_FUNCTION(0x4, "pwm1"),
		SUNXI_FUNCTION_IRQ_BANK(0xe, 0, 13)),
	SUNXI_PIN(SUNXI_PINCTRL_PIN(A, 14),
		SUNXI_FUNCTION(0x0, "gpio_in"),
		SUNXI_FUNCTION(0x1, "gpio_out"),
		SUNXI_FUNCTION(0x2, "gmac0"),		/* RX */
		SUNXI_FUNCTION(0x3, "twi0"),		/* SDA */
		SUNXI_FUNCTION(0x4, "pwm1"),
		SUNXI_FUNCTION_IRQ_BANK(0xe, 0, 14)),
	SUNXI_PIN(SUNXI_PINCTRL_PIN(A, 15),
		SUNXI_FUNCTION(0x0, "gpio_in"),
		SUNXI_FUNCTION(0x1, "gpio_out"),
		SUNXI_FUNCTION(0x2, "gmac0"),		/* RX */
		SUNXI_FUNCTION(0x3, "twi0"),		/* SDA */
		SUNXI_FUNCTION(0x4, "pwm1"),
		SUNXI_FUNCTION_IRQ_BANK(0xe, 0, 15)),
	SUNXI_PIN(SUNXI_PINCTRL_PIN(A, 16),
		SUNXI_FUNCTION(0x0, "gpio_in"),
		SUNXI_FUNCTION(0x1, "gpio_out"),
		SUNXI_FUNCTION(0x2, "gmac0"),		/* RX */
		SUNXI_FUNCTION(0x3, "twi0"),		/* SDA */
		SUNXI_FUNCTION(0x4, "pwm1"),
		SUNXI_FUNCTION_IRQ_BANK(0x6, 0, 16)),
	SUNXI_PIN(SUNXI_PINCTRL_PIN(A, 17),
		SUNXI_FUNCTION(0x0, "gpio_in"),
		SUNXI_FUNCTION(0x1, "gpio_out"),
		SUNXI_FUNCTION(0x2, "gmac0"),		/* RX */
		SUNXI_FUNCTION(0x3, "twi0"),		/* SDA */
		SUNXI_FUNCTION(0x4, "pwm1"),
		SUNXI_FUNCTION_IRQ_BANK(0xe, 0, 17)),
	SUNXI_PIN(SUNXI_PINCTRL_PIN(A, 18),
		SUNXI_FUNCTION(0x0, "gpio_in"),
		SUNXI_FUNCTION(0x1, "gpio_out"),
		SUNXI_FUNCTION(0x2, "gmac0"),		/* RX */
		SUNXI_FUNCTION(0x3, "twi0"),		/* SDA */
		SUNXI_FUNCTION(0x4, "pwm1"),
		SUNXI_FUNCTION_IRQ_BANK(0xe, 0, 18)),
	/* Hole */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(B, 0),
		SUNXI_FUNCTION(0x0, "gpio_in"),
		SUNXI_FUNCTION(0x1, "gpio_out"),
		SUNXI_FUNCTION(0x2, "uart2"),		/* TX */
		SUNXI_FUNCTION(0x3, "s_twi0"),		/* CS */
		SUNXI_FUNCTION(0x4, "twi0"),		/* MS */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 1, 0)),
	SUNXI_PIN(SUNXI_PINCTRL_PIN(B, 1),
		SUNXI_FUNCTION(0x0, "gpio_in"),
		SUNXI_FUNCTION(0x1, "gpio_out"),
		SUNXI_FUNCTION(0x2, "uart2"),		/* RX */
		SUNXI_FUNCTION(0x3, "s_twi0"),		/* CLK */
		SUNXI_FUNCTION(0x4, "twi0"),		/* CK */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 1, 1)),
	SUNXI_PIN(SUNXI_PINCTRL_PIN(B, 2),
		SUNXI_FUNCTION(0x0, "gpio_in"),
		SUNXI_FUNCTION(0x1, "gpio_out"),
		SUNXI_FUNCTION(0x2, "uart2"),		/* RTS */
		SUNXI_FUNCTION(0x3, "spi2"),		/* MOSI */
		SUNXI_FUNCTION(0x4, "jtag"),		/* DO */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 1, 2)),
	SUNXI_PIN(SUNXI_PINCTRL_PIN(B, 3),
		SUNXI_FUNCTION(0x0, "gpio_in"),
		SUNXI_FUNCTION(0x1, "gpio_out"),
		SUNXI_FUNCTION(0x2, "uart2"),		/* CTS */
		SUNXI_FUNCTION(0x3, "spi2"),		/* NISO */
		SUNXI_FUNCTION(0x4, "jtag"),		/* DI */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 1, 3)),
	SUNXI_PIN(SUNXI_PINCTRL_PIN(B, 4),
		SUNXI_FUNCTION(0x0, "gpio_in"),
		SUNXI_FUNCTION(0x1, "gpio_out"),
		SUNXI_FUNCTION(0x2, "twi1"),		/* SCK */
		SUNXI_FUNCTION(0x3, "i2s0"),		/* MCLK */
		SUNXI_FUNCTION(0x4, "jtag"),		/* MS_GPU */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 1, 4)),
	SUNXI_PIN(SUNXI_PINCTRL_PIN(B, 5),
		SUNXI_FUNCTION(0x0, "gpio_in"),
		SUNXI_FUNCTION(0x1, "gpio_out"),
		SUNXI_FUNCTION(0x2, "twi1"),		/* SDA */
		SUNXI_FUNCTION(0x3, "i2s0"),		/* BCLK */
		SUNXI_FUNCTION(0x4, "jtag"),		/* CK_GPU */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 1, 5)),
	SUNXI_PIN(SUNXI_PINCTRL_PIN(B, 6),
		SUNXI_FUNCTION(0x0, "gpio_in"),
		SUNXI_FUNCTION(0x1, "gpio_out"),
		SUNXI_FUNCTION(0x3, "i2s0"),		/* LRCK */
		SUNXI_FUNCTION(0x4, "jtag"),		/* DO_GPU */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 1, 6)),
	SUNXI_PIN(SUNXI_PINCTRL_PIN(B, 7),
		SUNXI_FUNCTION(0x0, "gpio_in"),
		SUNXI_FUNCTION(0x1, "gpio_out"),
		SUNXI_FUNCTION(0x2, "spdif"),		/* IN */
		SUNXI_FUNCTION(0x3, "i2s0"),		/* DOUT */
		SUNXI_FUNCTION(0x4, "i2s0"),		/* DIN */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 1, 7)),
	SUNXI_PIN(SUNXI_PINCTRL_PIN(B, 8),
		SUNXI_FUNCTION(0x0, "gpio_in"),
		SUNXI_FUNCTION(0x1, "gpio_out"),
		SUNXI_FUNCTION(0x2, "spdif"),		/* OUT */
		SUNXI_FUNCTION(0x3, "i2s0"),		/* DIN */
		SUNXI_FUNCTION(0x4, "i2s0"),		/* DOUT */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 1, 8)),
	SUNXI_PIN(SUNXI_PINCTRL_PIN(B, 9),
		SUNXI_FUNCTION(0x0, "gpio_in"),
		SUNXI_FUNCTION(0x1, "gpio_out"),
		SUNXI_FUNCTION(0x2, "uart0"),		/* TX */
		SUNXI_FUNCTION(0x3, "i2c0"),		/* SCK */
		SUNXI_FUNCTION(0x4, "jtag"),		/* DI_GPU */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 1, 9)),
	SUNXI_PIN(SUNXI_PINCTRL_PIN(B, 10),
		SUNXI_FUNCTION(0x0, "gpio_in"),
		SUNXI_FUNCTION(0x1, "gpio_out"),
		SUNXI_FUNCTION(0x2, "uart0"),		/* RX */
		SUNXI_FUNCTION(0x3, "i2c0"),		/* SDA */
		SUNXI_FUNCTION(0x4, "pwm1"),
		SUNXI_FUNCTION_IRQ_BANK(0xe, 1, 10)),
	SUNXI_PIN(SUNXI_PINCTRL_PIN(B, 11),
		SUNXI_FUNCTION(0x0, "gpio_in"),
		SUNXI_FUNCTION(0x1, "gpio_out"),
		SUNXI_FUNCTION(0x2, "spdif"),		/* OUT */
		SUNXI_FUNCTION(0x3, "i2s0"),		/* DIN */
		SUNXI_FUNCTION(0x4, "i2s0"),		/* DOUT */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 1, 11)),
	SUNXI_PIN(SUNXI_PINCTRL_PIN(B, 12),
		SUNXI_FUNCTION(0x0, "gpio_in"),
		SUNXI_FUNCTION(0x1, "gpio_out"),
		SUNXI_FUNCTION(0x2, "uart0"),		/* TX */
		SUNXI_FUNCTION(0x3, "twi0"),		/* SCK */
		SUNXI_FUNCTION(0x4, "jtag"),		/* DI_GPU */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 1, 12)),
	SUNXI_PIN(SUNXI_PINCTRL_PIN(B, 13),
		SUNXI_FUNCTION(0x0, "gpio_in"),
		SUNXI_FUNCTION(0x1, "gpio_out"),
		SUNXI_FUNCTION(0x2, "uart0"),		/* RX */
		SUNXI_FUNCTION(0x3, "i2c0"),		/* SDA */
		SUNXI_FUNCTION(0x4, "pwm1"),
		SUNXI_FUNCTION_IRQ_BANK(0xe, 1, 13)),
	/* Hole */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(C, 0),
		SUNXI_FUNCTION(0x0, "gpio_in"),
		SUNXI_FUNCTION(0x1, "gpio_out"),
		SUNXI_FUNCTION(0x2, "nand0"),		/* WE */
		SUNXI_FUNCTION(0x3, "mmc2"),		/* DS */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 2, 0)),
	SUNXI_PIN(SUNXI_PINCTRL_PIN(C, 1),
		SUNXI_FUNCTION(0x0, "gpio_in"),
		SUNXI_FUNCTION(0x1, "gpio_out"),
		SUNXI_FUNCTION(0x2, "nand0"),		/* ALE */
		SUNXI_FUNCTION(0x3, "mmc2"),		/* RST */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 2, 1)),
	SUNXI_PIN(SUNXI_PINCTRL_PIN(C, 2),
		SUNXI_FUNCTION(0x0, "gpio_in"),
		SUNXI_FUNCTION(0x1, "gpio_out"),
		SUNXI_FUNCTION(0x2, "nand0"),		/* CLE */
		SUNXI_FUNCTION(0x4, "spi0"),		/* MOSI */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 2, 2)),
	SUNXI_PIN(SUNXI_PINCTRL_PIN(C, 3),
		SUNXI_FUNCTION(0x0, "gpio_in"),
		SUNXI_FUNCTION(0x1, "gpio_out"),
		SUNXI_FUNCTION(0x2, "nand0"),		/* CE1 */
		SUNXI_FUNCTION(0x4, "spi0"),		/* CS */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 2, 3)),
	SUNXI_PIN(SUNXI_PINCTRL_PIN(C, 4),
		SUNXI_FUNCTION(0x0, "gpio_in"),
		SUNXI_FUNCTION(0x1, "gpio_out"),
		SUNXI_FUNCTION(0x2, "nand0"),		/* CE0 */
		SUNXI_FUNCTION(0x4, "spi0"),		/* MISO */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 2, 4)),
	SUNXI_PIN(SUNXI_PINCTRL_PIN(C, 5),
		SUNXI_FUNCTION(0x0, "gpio_in"),
		SUNXI_FUNCTION(0x1, "gpio_out"),
		SUNXI_FUNCTION(0x2, "nand0"),		/* RE */
		SUNXI_FUNCTION(0x3, "mmc2"),		/* CLK */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 2, 5)),
	SUNXI_PIN(SUNXI_PINCTRL_PIN(C, 6),
		SUNXI_FUNCTION(0x0, "gpio_in"),
		SUNXI_FUNCTION(0x1, "gpio_out"),
		SUNXI_FUNCTION(0x2, "nand0"),		/* RE */
		SUNXI_FUNCTION(0x3, "mmc2"),		/* CMD */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 2, 6)),
	SUNXI_PIN(SUNXI_PINCTRL_PIN(C, 7),
		SUNXI_FUNCTION(0x0, "gpio_in"),
		SUNXI_FUNCTION(0x1, "gpio_out"),
		SUNXI_FUNCTION(0x2, "nand0"),		/* RB1 */
		SUNXI_FUNCTION(0x4, "spi0"),		/* CS */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 2, 7)),
	SUNXI_PIN(SUNXI_PINCTRL_PIN(C, 8),
		SUNXI_FUNCTION(0x0, "gpio_in"),
		SUNXI_FUNCTION(0x1, "gpio_out"),
		SUNXI_FUNCTION(0x2, "nand0"),		/* DQ7 */
		SUNXI_FUNCTION(0x3, "mmc2"),		/* D3 */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 2, 8)),
	SUNXI_PIN(SUNXI_PINCTRL_PIN(C, 9),
		SUNXI_FUNCTION(0x0, "gpio_in"),
		SUNXI_FUNCTION(0x1, "gpio_out"),
		SUNXI_FUNCTION(0x2, "nand0"),		/* DQ6 */
		SUNXI_FUNCTION(0x3, "mmc2"),		/* D4 */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 2, 9)),
	SUNXI_PIN(SUNXI_PINCTRL_PIN(C, 10),
		SUNXI_FUNCTION(0x0, "gpio_in"),
		SUNXI_FUNCTION(0x1, "gpio_out"),
		SUNXI_FUNCTION(0x2, "nand0"),		/* DQ5 */
		SUNXI_FUNCTION(0x3, "mmc2"),		/* D0 */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 2, 10)),
	SUNXI_PIN(SUNXI_PINCTRL_PIN(C, 11),
		SUNXI_FUNCTION(0x0, "gpio_in"),
		SUNXI_FUNCTION(0x1, "gpio_out"),
		SUNXI_FUNCTION(0x2, "nand0"),		/* DQ4 */
		SUNXI_FUNCTION(0x3, "mmc2"),		/* D5 */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 2, 11)),
	SUNXI_PIN(SUNXI_PINCTRL_PIN(C, 12),
		SUNXI_FUNCTION(0x0, "gpio_in"),
		SUNXI_FUNCTION(0x1, "gpio_out"),
		SUNXI_FUNCTION(0x2, "nand0"),		/* DQS */
		SUNXI_FUNCTION(0x4, "spi0"),		/* CLK */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 2, 12)),
	SUNXI_PIN(SUNXI_PINCTRL_PIN(C, 13),
		SUNXI_FUNCTION(0x0, "gpio_in"),
		SUNXI_FUNCTION(0x1, "gpio_out"),
		SUNXI_FUNCTION(0x2, "nand0"),		/* DQ3 */
		SUNXI_FUNCTION(0x3, "mmc2"),		/* D1 */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 2, 13)),
	SUNXI_PIN(SUNXI_PINCTRL_PIN(C, 14),
		SUNXI_FUNCTION(0x0, "gpio_in"),
		SUNXI_FUNCTION(0x1, "gpio_out"),
		SUNXI_FUNCTION(0x2, "nand0"),		/* DQ2 */
		SUNXI_FUNCTION(0x3, "mmc2"),		/* D6 */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 2, 14)),
	SUNXI_PIN(SUNXI_PINCTRL_PIN(C, 15),
		SUNXI_FUNCTION(0x0, "gpio_in"),
		SUNXI_FUNCTION(0x1, "gpio_out"),
		SUNXI_FUNCTION(0x2, "nand0"),		/* DQ1 */
		SUNXI_FUNCTION(0x3, "mmc2"),		/* D2 */
		SUNXI_FUNCTION(0x4, "spi0"),		/* WP */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 2, 15)),
	SUNXI_PIN(SUNXI_PINCTRL_PIN(C, 16),
		SUNXI_FUNCTION(0x0, "gpio_in"),
		SUNXI_FUNCTION(0x1, "gpio_out"),
		SUNXI_FUNCTION(0x2, "nand0"),		/* DQ0 */
		SUNXI_FUNCTION(0x3, "mmc2"),		/* D7 */
		SUNXI_FUNCTION(0x4, "spi0"),		/* HOLD */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 2, 16)),
	/* Hole */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(D, 0),
		SUNXI_FUNCTION(0x0, "gpio_in"),
		SUNXI_FUNCTION(0x1, "gpio_out"),
		SUNXI_FUNCTION(0x2, "lcd0"),		/* D2 */
		SUNXI_FUNCTION(0x3, "lvds0"),		/* D0P */
		SUNXI_FUNCTION(0x4, "dsi0"),		/* DP0 */
		SUNXI_FUNCTION(0x5, "eink0"),		/* D0 */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 3, 0)),
	SUNXI_PIN(SUNXI_PINCTRL_PIN(D, 1),
		SUNXI_FUNCTION(0x0, "gpio_in"),
		SUNXI_FUNCTION(0x1, "gpio_out"),
		SUNXI_FUNCTION(0x2, "lcd0"),		/* D3 */
		SUNXI_FUNCTION(0x3, "lvds0"),		/* D0N */
		SUNXI_FUNCTION(0x4, "dsi0"),		/* DM0 */
		SUNXI_FUNCTION(0x5, "eink0"),		/* D1 */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 3, 1)),
	SUNXI_PIN(SUNXI_PINCTRL_PIN(D, 2),
		SUNXI_FUNCTION(0x0, "gpio_in"),
		SUNXI_FUNCTION(0x1, "gpio_out"),
		SUNXI_FUNCTION(0x2, "lcd0"),		/* D4 */
		SUNXI_FUNCTION(0x3, "lvds0"),		/* D1P */
		SUNXI_FUNCTION(0x4, "dsi0"),		/* DP1 */
		SUNXI_FUNCTION(0x5, "eink0"),		/* D2 */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 3, 2)),
	SUNXI_PIN(SUNXI_PINCTRL_PIN(D, 3),
		SUNXI_FUNCTION(0x0, "gpio_in"),
		SUNXI_FUNCTION(0x1, "gpio_out"),
		SUNXI_FUNCTION(0x2, "lcd0"),		/* D5 */
		SUNXI_FUNCTION(0x3, "lvds0"),		/* D1N */
		SUNXI_FUNCTION(0x4, "dsi0"),		/* DM1 */
		SUNXI_FUNCTION(0x5, "eink0"),		/* D3 */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 3, 3)),
	SUNXI_PIN(SUNXI_PINCTRL_PIN(D, 4),
		SUNXI_FUNCTION(0x0, "gpio_in"),
		SUNXI_FUNCTION(0x1, "gpio_out"),
		SUNXI_FUNCTION(0x2, "lcd0"),		/* D6 */
		SUNXI_FUNCTION(0x3, "lvds0"),		/* D2P */
		SUNXI_FUNCTION(0x4, "dsi0"),		/* CKP */
		SUNXI_FUNCTION(0x5, "eink0"),		/* D4 */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 3, 4)),
	SUNXI_PIN(SUNXI_PINCTRL_PIN(D, 5),
		SUNXI_FUNCTION(0x0, "gpio_in"),
		SUNXI_FUNCTION(0x1, "gpio_out"),
		SUNXI_FUNCTION(0x2, "lcd0"),		/* D7 */
		SUNXI_FUNCTION(0x3, "lvds0"),		/* D2N */
		SUNXI_FUNCTION(0x4, "dsi0"),		/* CKM */
		SUNXI_FUNCTION(0x5, "eink0"),		/* D5 */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 3, 5)),
	SUNXI_PIN(SUNXI_PINCTRL_PIN(D, 6),
		SUNXI_FUNCTION(0x0, "gpio_in"),
		SUNXI_FUNCTION(0x1, "gpio_out"),
		SUNXI_FUNCTION(0x2, "lcd0"),		/* D10 */
		SUNXI_FUNCTION(0x3, "lvds0"),		/* CKP / PLL_TEST */
		SUNXI_FUNCTION(0x4, "dsi0"),		/* P2 */
		SUNXI_FUNCTION(0x5, "eink0"),		/* D6 */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 3, 6)),
	SUNXI_PIN(SUNXI_PINCTRL_PIN(D, 7),
		SUNXI_FUNCTION(0x0, "gpio_in"),
		SUNXI_FUNCTION(0x1, "gpio_out"),
		SUNXI_FUNCTION(0x2, "lcd0"),		/* D11 */
		SUNXI_FUNCTION(0x3, "lvds0"),		/* CKN / PLLTEST */
		SUNXI_FUNCTION(0x4, "dsi0"),		/* DM2 */
		SUNXI_FUNCTION(0x5, "eink0"),		/* D7 */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 3, 7)),
	SUNXI_PIN(SUNXI_PINCTRL_PIN(D, 8),
		SUNXI_FUNCTION(0x0, "gpio_in"),
		SUNXI_FUNCTION(0x1, "gpio_out"),
		SUNXI_FUNCTION(0x2, "lcd0"),		/* D12 */
		SUNXI_FUNCTION(0x3, "lvds0"),		/* D3P */
		SUNXI_FUNCTION(0x4, "dsi0"),		/* DP3 */
		SUNXI_FUNCTION(0x5, "eink0"),		/* D8 */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 3, 8)),
	SUNXI_PIN(SUNXI_PINCTRL_PIN(D, 9),
		SUNXI_FUNCTION(0x0, "gpio_in"),
		SUNXI_FUNCTION(0x1, "gpio_out"),
		SUNXI_FUNCTION(0x2, "lcd0"),		/* TX */
		SUNXI_FUNCTION(0x3, "lvds0"),		/* CS */
		SUNXI_FUNCTION(0x4, "dsi0"),		/* MS */
		SUNXI_FUNCTION(0x5, "eink0"),		/* D9 */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 3, 9)),
	SUNXI_PIN(SUNXI_PINCTRL_PIN(D, 10),
		SUNXI_FUNCTION(0x0, "gpio_in"),
		SUNXI_FUNCTION(0x1, "gpio_out"),
		SUNXI_FUNCTION(0x2, "lcd0"),		/* D14 */
		SUNXI_FUNCTION(0x3, "lvds1"),		/* D0P */
		SUNXI_FUNCTION(0x4, "spi1"),		/* CS */
		SUNXI_FUNCTION(0x5, "eink0"),		/* D10 */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 3, 10)),
	SUNXI_PIN(SUNXI_PINCTRL_PIN(D, 11),
		SUNXI_FUNCTION(0x0, "gpio_in"),
		SUNXI_FUNCTION(0x1, "gpio_out"),
		SUNXI_FUNCTION(0x2, "lcd0"),		/* D15 */
		SUNXI_FUNCTION(0x3, "lvds1"),		/* D0N */
		SUNXI_FUNCTION(0x4, "spi1"),		/* CLK */
		SUNXI_FUNCTION(0x5, "eink0"),		/* D11 */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 3, 11)),
	SUNXI_PIN(SUNXI_PINCTRL_PIN(D, 12),
		SUNXI_FUNCTION(0x0, "gpio_in"),
		SUNXI_FUNCTION(0x1, "gpio_out"),
		SUNXI_FUNCTION(0x2, "lcd0"),		/* D18 */
		SUNXI_FUNCTION(0x3, "lvds1"),		/* D1P */
		SUNXI_FUNCTION(0x4, "spi1"),		/* MOSI */
		SUNXI_FUNCTION(0x5, "eink0"),		/* D12 */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 3, 12)),
	SUNXI_PIN(SUNXI_PINCTRL_PIN(D, 13),
		SUNXI_FUNCTION(0x0, "gpio_in"),
		SUNXI_FUNCTION(0x1, "gpio_out"),
		SUNXI_FUNCTION(0x2, "lcd0"),		/* D19 */
		SUNXI_FUNCTION(0x3, "lvds1"),		/* D1N */
		SUNXI_FUNCTION(0x4, "spi1"),		/* MISO */
		SUNXI_FUNCTION(0x5, "eink0"),		/* D13 */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 3, 13)),
	SUNXI_PIN(SUNXI_PINCTRL_PIN(D, 14),
		SUNXI_FUNCTION(0x0, "gpio_in"),
		SUNXI_FUNCTION(0x1, "gpio_out"),
		SUNXI_FUNCTION(0x2, "lcd0"),		/* D20 */
		SUNXI_FUNCTION(0x3, "lvds1"),		/* D2P */
		SUNXI_FUNCTION(0x4, "uart3"),		/* TX */
		SUNXI_FUNCTION(0x5, "eink0"),		/* D14 */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 3, 14)),
	SUNXI_PIN(SUNXI_PINCTRL_PIN(D, 15),
		SUNXI_FUNCTION(0x0, "gpio_in"),
		SUNXI_FUNCTION(0x1, "gpio_out"),
		SUNXI_FUNCTION(0x2, "lcd0"),		/* D21 */
		SUNXI_FUNCTION(0x3, "lvds1"),		/* D2N */
		SUNXI_FUNCTION(0x4, "uart3"),		/* RX */
		SUNXI_FUNCTION(0x5, "eink0"),		/* D15 */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 3, 15)),
	SUNXI_PIN(SUNXI_PINCTRL_PIN(D, 16),
		SUNXI_FUNCTION(0x0, "gpio_in"),
		SUNXI_FUNCTION(0x1, "gpio_out"),
		SUNXI_FUNCTION(0x2, "lcd0"),		/* D22 */
		SUNXI_FUNCTION(0x3, "lvds1"),		/* CKP */
		SUNXI_FUNCTION(0x4, "uart3"),		/* RTS */
		SUNXI_FUNCTION(0x5, "eink0"),		/* OEH */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 3, 16)),
	SUNXI_PIN(SUNXI_PINCTRL_PIN(D, 17),
		SUNXI_FUNCTION(0x0, "gpio_in"),
		SUNXI_FUNCTION(0x1, "gpio_out"),
		SUNXI_FUNCTION(0x2, "lcd0"),		/* D23 */
		SUNXI_FUNCTION(0x3, "lvds1"),		/* CKN */
		SUNXI_FUNCTION(0x4, "uart3"),		/* CTS */
		SUNXI_FUNCTION(0x5, "eink0"),		/* LEH */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 3, 17)),
	SUNXI_PIN(SUNXI_PINCTRL_PIN(D, 18),
		SUNXI_FUNCTION(0x0, "gpio_in"),
		SUNXI_FUNCTION(0x1, "gpio_out"),
		SUNXI_FUNCTION(0x2, "lcd0"),		/* CLK */
		SUNXI_FUNCTION(0x3, "lvds1"),		/* D3P */
		SUNXI_FUNCTION(0x4, "uart4"),		/* TX */
		SUNXI_FUNCTION(0x5, "eink0"),		/* CKH */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 3, 18)),
	SUNXI_PIN(SUNXI_PINCTRL_PIN(D, 19),
		SUNXI_FUNCTION(0x0, "gpio_in"),
		SUNXI_FUNCTION(0x1, "gpio_out"),
		SUNXI_FUNCTION(0x2, "lcd0"),		/* DE */
		SUNXI_FUNCTION(0x3, "lvds1"),		/* D3N */
		SUNXI_FUNCTION(0x4, "uart4"),		/* RX */
		SUNXI_FUNCTION(0x5, "eink0"),		/* STH */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 3, 19)),
	SUNXI_PIN(SUNXI_PINCTRL_PIN(D, 20),
		SUNXI_FUNCTION(0x0, "gpio_in"),
		SUNXI_FUNCTION(0x1, "gpio_out"),
		SUNXI_FUNCTION(0x2, "lcd0"),		/* HSYNC */
		SUNXI_FUNCTION(0x3, "pwm2"),
		SUNXI_FUNCTION(0x4, "uart4"),		/* RTS */
		SUNXI_FUNCTION(0x5, "eink0"),		/* CKV */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 3, 20)),
	SUNXI_PIN(SUNXI_PINCTRL_PIN(D, 21),
		SUNXI_FUNCTION(0x0, "gpio_in"),
		SUNXI_FUNCTION(0x1, "gpio_out"),
		SUNXI_FUNCTION(0x2, "lcd0"),		/* VSYNC */
		SUNXI_FUNCTION(0x3, "pwm3"),
		SUNXI_FUNCTION(0x4, "uart4"),		/* CTS */
		SUNXI_FUNCTION(0x5, "eink0"),		/* MODE */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 3, 21)),
	SUNXI_PIN(SUNXI_PINCTRL_PIN(D, 22),
		SUNXI_FUNCTION(0x0, "gpio_in"),
		SUNXI_FUNCTION(0x1, "gpio_out"),
		SUNXI_FUNCTION(0x2, "pwm1"),
		SUNXI_FUNCTION(0x4, "i2c0"),		/* SCK */
		SUNXI_FUNCTION(0x5, "eink0"),		/* STV */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 3, 22)),
	SUNXI_PIN(SUNXI_PINCTRL_PIN(D, 23),
		SUNXI_FUNCTION(0x0, "gpio_in"),
		SUNXI_FUNCTION(0x1, "gpio_out"),
		SUNXI_FUNCTION(0x2, "pwm0"),
		SUNXI_FUNCTION(0x4, "i2c0"),		/* SDA */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 3, 23)),
	/* Hole */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(F, 0),
		SUNXI_FUNCTION(0x0, "gpio_in"),
		SUNXI_FUNCTION(0x1, "gpio_out"),
		SUNXI_FUNCTION(0x2, "mmc0"),		/* D1 */
		SUNXI_FUNCTION(0x3, "jtag"),		/* MS */
		SUNXI_FUNCTION(0x4, "jtag"),		/* MS_GPU */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 4, 0)),
	SUNXI_PIN(SUNXI_PINCTRL_PIN(F, 1),
		SUNXI_FUNCTION(0x0, "gpio_in"),
		SUNXI_FUNCTION(0x1, "gpio_out"),
		SUNXI_FUNCTION(0x2, "mmc0"),		/* D0 */
		SUNXI_FUNCTION(0x3, "jtag"),		/* DI */
		SUNXI_FUNCTION(0x4, "jtag"),		/* DI_GPU */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 4, 1)),
	SUNXI_PIN(SUNXI_PINCTRL_PIN(F, 2),
		SUNXI_FUNCTION(0x0, "gpio_in"),
		SUNXI_FUNCTION(0x1, "gpio_out"),
		SUNXI_FUNCTION(0x2, "mmc0"),		/* CLK */
		SUNXI_FUNCTION(0x3, "uart0"),		/* TX */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 4, 2)),
	SUNXI_PIN(SUNXI_PINCTRL_PIN(F, 3),
		SUNXI_FUNCTION(0x0, "gpio_in"),
		SUNXI_FUNCTION(0x1, "gpio_out"),
		SUNXI_FUNCTION(0x2, "mmc0"),		/* CMD */
		SUNXI_FUNCTION(0x3, "jtag"),		/* DO */
		SUNXI_FUNCTION(0x4, "jtag"),		/* DO_GPU */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 4, 3)),
	SUNXI_PIN(SUNXI_PINCTRL_PIN(F, 4),
		SUNXI_FUNCTION(0x0, "gpio_in"),
		SUNXI_FUNCTION(0x1, "gpio_out"),
		SUNXI_FUNCTION(0x2, "mmc0"),		/* D3 */
		SUNXI_FUNCTION(0x3, "uart0"),		/* RX */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 4, 4)),
	SUNXI_PIN(SUNXI_PINCTRL_PIN(F, 5),
		SUNXI_FUNCTION(0x0, "gpio_in"),
		SUNXI_FUNCTION(0x1, "gpio_out"),
		SUNXI_FUNCTION(0x2, "mmc0"),		/* D2 */
		SUNXI_FUNCTION(0x3, "jtag"),		/* CK */
		SUNXI_FUNCTION(0x4, "jtag"),		/* CK_GPU */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 4, 5)),
	SUNXI_PIN(SUNXI_PINCTRL_PIN(F, 6),
		SUNXI_FUNCTION(0x0, "gpio_in"),
		SUNXI_FUNCTION(0x1, "gpio_out"),
		SUNXI_FUNCTION_IRQ_BANK(0xe, 4, 6)),
	SUNXI_PIN(SUNXI_PINCTRL_PIN(F, 7),
		SUNXI_FUNCTION(0x0, "gpio_in"),
		SUNXI_FUNCTION(0x1, "gpio_out"),
		SUNXI_FUNCTION_IRQ_BANK(0xe, 4, 7)),
	SUNXI_PIN(SUNXI_PINCTRL_PIN(F, 8),
		SUNXI_FUNCTION(0x0, "gpio_in"),
		SUNXI_FUNCTION(0x1, "gpio_out"),
		SUNXI_FUNCTION(0x2, "mmc0"),		/* D1 */
		SUNXI_FUNCTION(0x3, "jtag"),		/* MS */
		SUNXI_FUNCTION(0x4, "jtag"),		/* MS_GPU */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 4, 8)),
	SUNXI_PIN(SUNXI_PINCTRL_PIN(F, 9),
		SUNXI_FUNCTION(0x0, "gpio_in"),
		SUNXI_FUNCTION(0x1, "gpio_out"),
		SUNXI_FUNCTION(0x2, "mmc0"),		/* D0 */
		SUNXI_FUNCTION(0x3, "jtag"),		/* DI */
		SUNXI_FUNCTION(0x4, "jtag"),		/* DI_GPU */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 4, 9)),
	SUNXI_PIN(SUNXI_PINCTRL_PIN(F, 10),
		SUNXI_FUNCTION(0x0, "gpio_in"),
		SUNXI_FUNCTION(0x1, "gpio_out"),
		SUNXI_FUNCTION(0x2, "mmc0"),		/* CLK */
		SUNXI_FUNCTION(0x3, "uart0"),		/* TX */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 4, 10)),
	SUNXI_PIN(SUNXI_PINCTRL_PIN(F, 11),
		SUNXI_FUNCTION(0x0, "gpio_in"),
		SUNXI_FUNCTION(0x1, "gpio_out"),
		SUNXI_FUNCTION(0x2, "mmc0"),		/* CMD */
		SUNXI_FUNCTION(0x3, "jtag"),		/* DO */
		SUNXI_FUNCTION(0x4, "jtag"),		/* DO_GPU */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 4, 11)),
	SUNXI_PIN(SUNXI_PINCTRL_PIN(F, 12),
		SUNXI_FUNCTION(0x0, "gpio_in"),
		SUNXI_FUNCTION(0x1, "gpio_out"),
		SUNXI_FUNCTION(0x2, "mmc0"),		/* D3 */
		SUNXI_FUNCTION(0x3, "uart0"),		/* RX */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 4, 12)),
	SUNXI_PIN(SUNXI_PINCTRL_PIN(F, 13),
		SUNXI_FUNCTION(0x0, "gpio_in"),
		SUNXI_FUNCTION(0x1, "gpio_out"),
		SUNXI_FUNCTION(0x2, "mmc0"),		/* D2 */
		SUNXI_FUNCTION(0x3, "jtag"),		/* CK */
		SUNXI_FUNCTION(0x4, "jtag"),		/* CK_GPU */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 4, 13)),
	SUNXI_PIN(SUNXI_PINCTRL_PIN(F, 14),
		SUNXI_FUNCTION(0x0, "gpio_in"),
		SUNXI_FUNCTION(0x1, "gpio_out"),
		SUNXI_FUNCTION_IRQ_BANK(0xe, 4, 14)),
	SUNXI_PIN(SUNXI_PINCTRL_PIN(F, 15),
		SUNXI_FUNCTION(0x0, "gpio_in"),
		SUNXI_FUNCTION(0x1, "gpio_out"),
		SUNXI_FUNCTION_IRQ_BANK(0xe, 4, 15)),
	SUNXI_PIN(SUNXI_PINCTRL_PIN(F, 16),
		SUNXI_FUNCTION(0x0, "gpio_in"),
		SUNXI_FUNCTION(0x1, "gpio_out"),
		SUNXI_FUNCTION(0x2, "mmc0"),		/* D1 */
		SUNXI_FUNCTION(0x3, "jtag"),		/* MS */
		SUNXI_FUNCTION(0x4, "jtag"),		/* MS_GPU */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 4, 16)),
	SUNXI_PIN(SUNXI_PINCTRL_PIN(F, 17),
		SUNXI_FUNCTION(0x0, "gpio_in"),
		SUNXI_FUNCTION(0x1, "gpio_out"),
		SUNXI_FUNCTION(0x2, "mmc0"),		/* D0 */
		SUNXI_FUNCTION(0x3, "jtag"),		/* DI */
		SUNXI_FUNCTION(0x4, "jtag"),		/* DI_GPU */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 4, 17)),
	SUNXI_PIN(SUNXI_PINCTRL_PIN(F, 18),
		SUNXI_FUNCTION(0x0, "gpio_in"),
		SUNXI_FUNCTION(0x1, "gpio_out"),
		SUNXI_FUNCTION(0x2, "mmc0"),		/* CLK */
		SUNXI_FUNCTION(0x3, "uart0"),		/* TX */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 4, 18)),
	SUNXI_PIN(SUNXI_PINCTRL_PIN(F, 19),
		SUNXI_FUNCTION(0x0, "gpio_in"),
		SUNXI_FUNCTION(0x1, "gpio_out"),
		SUNXI_FUNCTION(0x2, "mmc0"),		/* CMD */
		SUNXI_FUNCTION(0x3, "jtag"),		/* DO */
		SUNXI_FUNCTION(0x4, "jtag"),		/* DO_GPU */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 4, 19)),
	SUNXI_PIN(SUNXI_PINCTRL_PIN(F, 20),
		SUNXI_FUNCTION(0x0, "gpio_in"),
		SUNXI_FUNCTION(0x1, "gpio_out"),
		SUNXI_FUNCTION(0x2, "mmc0"),		/* D3 */
		SUNXI_FUNCTION(0x3, "uart0"),		/* RX */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 4, 20)),
	SUNXI_PIN(SUNXI_PINCTRL_PIN(F, 21),
		SUNXI_FUNCTION(0x0, "gpio_in"),
		SUNXI_FUNCTION(0x1, "gpio_out"),
		SUNXI_FUNCTION(0x2, "mmc0"),		/* D2 */
		SUNXI_FUNCTION(0x3, "jtag"),		/* CK */
		SUNXI_FUNCTION(0x4, "jtag"),		/* CK_GPU */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 4, 21)),
	SUNXI_PIN(SUNXI_PINCTRL_PIN(F, 22),
		SUNXI_FUNCTION(0x0, "gpio_in"),
		SUNXI_FUNCTION(0x1, "gpio_out"),
		SUNXI_FUNCTION_IRQ_BANK(0xe, 4, 22)),
	SUNXI_PIN(SUNXI_PINCTRL_PIN(F, 23),
		SUNXI_FUNCTION(0x0, "gpio_in"),
		SUNXI_FUNCTION(0x1, "gpio_out"),
		SUNXI_FUNCTION_IRQ_BANK(0xe, 4, 23)),
	/* Hole */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(G, 0),
		SUNXI_FUNCTION(0x0, "gpio_in"),
		SUNXI_FUNCTION(0x1, "gpio_out"),
		SUNXI_FUNCTION(0x2, "mmc1"),		/* CLK */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 5, 0)),
	SUNXI_PIN(SUNXI_PINCTRL_PIN(G, 1),
		SUNXI_FUNCTION(0x0, "gpio_in"),
		SUNXI_FUNCTION(0x1, "gpio_out"),
		SUNXI_FUNCTION(0x2, "mmc1"),		/* CMD */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 5, 1)),
	SUNXI_PIN(SUNXI_PINCTRL_PIN(G, 2),
		SUNXI_FUNCTION(0x0, "gpio_in"),
		SUNXI_FUNCTION(0x1, "gpio_out"),
		SUNXI_FUNCTION(0x2, "mmc1"),		/* D0 */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 5, 2)),
	SUNXI_PIN(SUNXI_PINCTRL_PIN(G, 3),
		SUNXI_FUNCTION(0x0, "gpio_in"),
		SUNXI_FUNCTION(0x1, "gpio_out"),
		SUNXI_FUNCTION(0x2, "mmc1"),		/* D1 */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 5, 3)),
	SUNXI_PIN(SUNXI_PINCTRL_PIN(G, 4),
		SUNXI_FUNCTION(0x0, "gpio_in"),
		SUNXI_FUNCTION(0x1, "gpio_out"),
		SUNXI_FUNCTION(0x2, "mmc1"),		/* D2 */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 5, 4)),
	SUNXI_PIN(SUNXI_PINCTRL_PIN(G, 5),
		SUNXI_FUNCTION(0x0, "gpio_in"),
		SUNXI_FUNCTION(0x1, "gpio_out"),
		SUNXI_FUNCTION(0x2, "mmc1"),		/* D3 */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 5, 5)),
	SUNXI_PIN(SUNXI_PINCTRL_PIN(G, 6),
		SUNXI_FUNCTION(0x0, "gpio_in"),
		SUNXI_FUNCTION(0x1, "gpio_out"),
		SUNXI_FUNCTION(0x2, "uart1"),		/* TX */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 5, 6)),
	SUNXI_PIN(SUNXI_PINCTRL_PIN(G, 7),
		SUNXI_FUNCTION(0x0, "gpio_in"),
		SUNXI_FUNCTION(0x1, "gpio_out"),
		SUNXI_FUNCTION(0x2, "uart1"),		/* RX */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 5, 7)),
	SUNXI_PIN(SUNXI_PINCTRL_PIN(G, 8),
		SUNXI_FUNCTION(0x0, "gpio_in"),
		SUNXI_FUNCTION(0x1, "gpio_out"),
		SUNXI_FUNCTION(0x2, "uart1"),		/* RTS */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 5, 8)),
	SUNXI_PIN(SUNXI_PINCTRL_PIN(G, 9),
		SUNXI_FUNCTION(0x0, "gpio_in"),
		SUNXI_FUNCTION(0x1, "gpio_out"),
		SUNXI_FUNCTION(0x2, "uart1"),		/* TX */
		SUNXI_FUNCTION(0x3, "i2s1"),		/* MCLK */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 5, 9)),
	SUNXI_PIN(SUNXI_PINCTRL_PIN(G, 10),
		SUNXI_FUNCTION(0x0, "gpio_in"),
		SUNXI_FUNCTION(0x1, "gpio_out"),
		SUNXI_FUNCTION(0x3, "i2s1"),		/* BCLK */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 5, 10)),
	SUNXI_PIN(SUNXI_PINCTRL_PIN(G, 11),
		SUNXI_FUNCTION(0x0, "gpio_in"),
		SUNXI_FUNCTION(0x1, "gpio_out"),
		SUNXI_FUNCTION(0x3, "i2s1"),		/* LRCK */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 5, 11)),
	SUNXI_PIN(SUNXI_PINCTRL_PIN(G, 12),
		SUNXI_FUNCTION(0x0, "gpio_in"),
		SUNXI_FUNCTION(0x1, "gpio_out"),
		SUNXI_FUNCTION(0x3, "i2s1"),		/* DOUT */
		SUNXI_FUNCTION(0x4, "i2s1"),		/* DIN */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 5, 12)),
	SUNXI_PIN(SUNXI_PINCTRL_PIN(G, 13),
		SUNXI_FUNCTION(0x0, "gpio_in"),
		SUNXI_FUNCTION(0x1, "gpio_out"),
		SUNXI_FUNCTION(0x3, "i2s1"),		/* DON */
		SUNXI_FUNCTION(0x4, "i2s1"),		/* DOUT */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 5, 13)),
	SUNXI_PIN(SUNXI_PINCTRL_PIN(G, 14),
		SUNXI_FUNCTION(0x0, "gpio_in"),
		SUNXI_FUNCTION(0x1, "gpio_out"),
		SUNXI_FUNCTION(0x3, "i2s1"),		/* DIN */
		SUNXI_FUNCTION(0x4, "i2s1"),		/* DOUT */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 5, 14)),
	/* Hole */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(H, 0),
		SUNXI_FUNCTION(0x0, "gpio_in"),
		SUNXI_FUNCTION(0x1, "gpio_out"),
		SUNXI_FUNCTION(0x2, "i2c0"),		/* SCK */
		SUNXI_FUNCTION(0x5, "emac"),		/* RXD */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 6, 0)),
	SUNXI_PIN(SUNXI_PINCTRL_PIN(H, 1),
		SUNXI_FUNCTION(0x0, "gpio_in"),
		SUNXI_FUNCTION(0x1, "gpio_out"),
		SUNXI_FUNCTION(0x2, "i2c0"),		/* SDA */
		SUNXI_FUNCTION(0x5, "emac"),		/* RXD */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 6, 1)),
	SUNXI_PIN(SUNXI_PINCTRL_PIN(H, 2),
		SUNXI_FUNCTION(0x0, "gpio_in"),
		SUNXI_FUNCTION(0x1, "gpio_out"),
		SUNXI_FUNCTION(0x2, "i2c1"),		/* SCK */
		SUNXI_FUNCTION(0x3, "cpu"),		/* CUR_W */
		SUNXI_FUNCTION(0x5, "emac"),		/* RXTCL/CRS_DV */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 6, 2)),
	SUNXI_PIN(SUNXI_PINCTRL_PIN(H, 3),
		SUNXI_FUNCTION(0x0, "gpio_in"),
		SUNXI_FUNCTION(0x1, "gpio_out"),
		SUNXI_FUNCTION(0x2, "i2c1"),		/* SDA */
		SUNXI_FUNCTION(0x3, "cir"),		/* OUT */
		SUNXI_FUNCTION(0x5, "emac"),		/* CLKIN/RXER */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 6, 3)),
	SUNXI_PIN(SUNXI_PINCTRL_PIN(H, 4),
		SUNXI_FUNCTION(0x0, "gpio_in"),
		SUNXI_FUNCTION(0x1, "gpio_out"),
		SUNXI_FUNCTION(0x2, "uart3"),		/* RX */
		SUNXI_FUNCTION(0x3, "spi1"),		/* CS */
		SUNXI_FUNCTION(0x4, "cpu"),		/* CUR_W */
		SUNXI_FUNCTION(0x5, "emac"),		/* TXD1 */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 6, 4)),
	SUNXI_PIN(SUNXI_PINCTRL_PIN(H, 5),
		SUNXI_FUNCTION(0x0, "gpio_in"),
		SUNXI_FUNCTION(0x1, "gpio_out"),
		SUNXI_FUNCTION(0x2, "uart3"),		/* RX */
		SUNXI_FUNCTION(0x3, "spi1"),		/* CLK */
		SUNXI_FUNCTION(0x4, "ldec"),
		SUNXI_FUNCTION(0x5, "emac"),		/* TXD */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 6, 5)),
	SUNXI_PIN(SUNXI_PINCTRL_PIN(H, 6),
		SUNXI_FUNCTION(0x0, "gpio_in"),
		SUNXI_FUNCTION(0x1, "gpio_out"),
		SUNXI_FUNCTION(0x2, "uart3"),		/* RTS */
		SUNXI_FUNCTION(0x3, "spi1"),		/* MOSI */
		SUNXI_FUNCTION(0x4, "spdif"),		/* IN */
		SUNXI_FUNCTION(0x5, "emac"),		/* TXCK */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 6, 6)),
	SUNXI_PIN(SUNXI_PINCTRL_PIN(H, 7),
		SUNXI_FUNCTION(0x0, "gpio_in"),
		SUNXI_FUNCTION(0x1, "gpio_out"),
		SUNXI_FUNCTION(0x2, "uart3"),		/* CTS */
		SUNXI_FUNCTION(0x3, "spi1"),		/* MISO */
		SUNXI_FUNCTION(0x4, "spdif"),		/* OUT */
		SUNXI_FUNCTION(0x5, "emac"),		/* TXCTL/EN */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 6, 7)),
	SUNXI_PIN(SUNXI_PINCTRL_PIN(H, 8),
		SUNXI_FUNCTION(0x0, "gpio_in"),
		SUNXI_FUNCTION(0x1, "gpio_out"),
		SUNXI_FUNCTION(0x2, "dmic"),		/* CLK */
		SUNXI_FUNCTION(0x3, "spi2"),		/* CS */
		SUNXI_FUNCTION(0x4, "i2s2"),		/* MCLK */
		SUNXI_FUNCTION(0x5, "i2s2"),		/* DIN */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 6, 8)),
	SUNXI_PIN(SUNXI_PINCTRL_PIN(H, 9),
		SUNXI_FUNCTION(0x0, "gpio_in"),
		SUNXI_FUNCTION(0x1, "gpio_out"),
		SUNXI_FUNCTION(0x2, "dimc"),		/* DATA0 */
		SUNXI_FUNCTION(0x3, "spi2"),		/* CLK */
		SUNXI_FUNCTION(0x4, "i2s2"),		/* BCLK */
		SUNXI_FUNCTION(0x5, "mdc"),
		SUNXI_FUNCTION_IRQ_BANK(0xe, 6, 9)),
	SUNXI_PIN(SUNXI_PINCTRL_PIN(H, 10),
		SUNXI_FUNCTION(0x0, "gpio_in"),
		SUNXI_FUNCTION(0x1, "gpio_out"),
		SUNXI_FUNCTION(0x2, "dmic"),		/* DATA1 */
		SUNXI_FUNCTION(0x3, "spi2"),		/* MOSI */
		SUNXI_FUNCTION(0x4, "i2s2"),		/* LRCK */
		SUNXI_FUNCTION(0x5, "mdio"),
		SUNXI_FUNCTION_IRQ_BANK(0xe, 6, 10)),
	SUNXI_PIN(SUNXI_PINCTRL_PIN(H, 11),
		SUNXI_FUNCTION(0x0, "gpio_in"),
		SUNXI_FUNCTION(0x1, "gpio_out"),
		SUNXI_FUNCTION(0x2, "dmic"),		/* DATA2 */
		SUNXI_FUNCTION(0x3, "spi2"),		/* MISO */
		SUNXI_FUNCTION(0x4, "i2s2"),		/* DOUT */
		SUNXI_FUNCTION(0x5, "i2s2"),		/* DIN */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 6, 11)),
	SUNXI_PIN(SUNXI_PINCTRL_PIN(H, 12),
		SUNXI_FUNCTION(0x0, "gpio_in"),
		SUNXI_FUNCTION(0x1, "gpio_out"),
		SUNXI_FUNCTION(0x2, "dmic"),		/* DATA3 */
		SUNXI_FUNCTION(0x3, "i2c3"),		/* SCK */
		SUNXI_FUNCTION(0x5, "i2s2"),		/* DOUT */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 6, 12)),
	SUNXI_PIN(SUNXI_PINCTRL_PIN(H, 13),
		SUNXI_FUNCTION(0x0, "gpio_in"),
		SUNXI_FUNCTION(0x1, "gpio_out"),
		SUNXI_FUNCTION(0x3, "i2c3"),		/* SDA */
		SUNXI_FUNCTION(0x4, "i2s3"),		/* MCLK */
		SUNXI_FUNCTION(0x5, "emac"),		/* 25 */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 6, 13)),
	SUNXI_PIN(SUNXI_PINCTRL_PIN(H, 14),
		SUNXI_FUNCTION(0x0, "gpio_in"),
		SUNXI_FUNCTION(0x1, "gpio_out"),
		SUNXI_FUNCTION(0x4, "i2s3"),		/* BCLK */
		SUNXI_FUNCTION(0x5, "emac"),		/* RXD3 */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 6, 14)),
	SUNXI_PIN(SUNXI_PINCTRL_PIN(H, 15),
		SUNXI_FUNCTION(0x0, "gpio_in"),
		SUNXI_FUNCTION(0x1, "gpio_out"),
		SUNXI_FUNCTION(0x4, "i2s3"),		/* LRCK */
		SUNXI_FUNCTION(0x5, "emac"),		/* RXD2 */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 6, 15)),
	SUNXI_PIN(SUNXI_PINCTRL_PIN(H, 16),
		SUNXI_FUNCTION(0x0, "gpio_in"),
		SUNXI_FUNCTION(0x1, "gpio_out"),
		SUNXI_FUNCTION(0x2, "cir"),		/* IN */
		SUNXI_FUNCTION(0x3, "i2s3"),		/* DOUT */
		SUNXI_FUNCTION(0x4, "i2s3"),		/* DIN */
		SUNXI_FUNCTION(0x5, "ledc"),
		SUNXI_FUNCTION_IRQ_BANK(0xe, 6, 16)),
	SUNXI_PIN(SUNXI_PINCTRL_PIN(H, 17),
		SUNXI_FUNCTION(0x0, "gpio_in"),
		SUNXI_FUNCTION(0x1, "gpio_out"),
		SUNXI_FUNCTION(0x3, "i2s3"),		/* DOUT */
		SUNXI_FUNCTION(0x4, "i2s3"),		/* DIN */
		SUNXI_FUNCTION(0x5, "emac"),		/* TXD3 */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 6, 17)),
	SUNXI_PIN(SUNXI_PINCTRL_PIN(H, 18),
		SUNXI_FUNCTION(0x0, "gpio_in"),
		SUNXI_FUNCTION(0x1, "gpio_out"),
		SUNXI_FUNCTION(0x2, "cir"),		/* OUT */
		SUNXI_FUNCTION(0x3, "i2s3"),		/* DOUT */
		SUNXI_FUNCTION(0x4, "i2s3"),		/* DIN */
		SUNXI_FUNCTION(0x5, "emac"),		/* TXD2 */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 6, 18)),
	SUNXI_PIN(SUNXI_PINCTRL_PIN(H, 19),
		SUNXI_FUNCTION(0x0, "gpio_in"),
		SUNXI_FUNCTION(0x1, "gpio_out"),
		SUNXI_FUNCTION(0x2, "cir"),		/* IN */
		SUNXI_FUNCTION(0x3, "i2s3"),		/* DOUT */
		SUNXI_FUNCTION(0x4, "i2s3"),		/* DIN */
		SUNXI_FUNCTION(0x5, "ledc"),
		SUNXI_FUNCTION_IRQ_BANK(0xe, 6, 19)),
};

static const unsigned int sun50iw12_irq_bank_map[] = {
	SUNXI_BANK_OFFSET('A', 'A'),
	SUNXI_BANK_OFFSET('B', 'A'),
	SUNXI_BANK_OFFSET('C', 'A'),
	SUNXI_BANK_OFFSET('D', 'A'),
	SUNXI_BANK_OFFSET('F', 'A'),
	SUNXI_BANK_OFFSET('G', 'A'),
	SUNXI_BANK_OFFSET('H', 'A'),
};

static const struct sunxi_pinctrl_desc sun50iw12_pinctrl_data = {
	.pins = sun50iw12_pins,
	.npins = ARRAY_SIZE(sun50iw12_pins),
	.irq_banks = ARRAY_SIZE(sun50iw12_irq_bank_map),
	.irq_bank_map = sun50iw12_irq_bank_map,
	.io_bias_cfg_variant = BIAS_VOLTAGE_PIO_POW_MODE_CTL,
	.pf_power_source_switch = true,
	.hw_type = SUNXI_PCTL_HW_TYPE_1,
};

static int sun50iw12_pinctrl_probe(struct platform_device *pdev)
{
	return sunxi_pinctrl_init(pdev, &sun50iw12_pinctrl_data);
}

static struct of_device_id sun50iw12_pinctrl_match[] = {
	{ .compatible = "allwinner,sun50iw12-pinctrl", },
	{}
};
MODULE_DEVICE_TABLE(of, sun50iw12_pinctrl_match);

static struct platform_driver sun50iw12_pinctrl_driver = {
	.probe	= sun50iw12_pinctrl_probe,
	.driver	= {
		.name		= "sun50iw12-pinctrl",
		.of_match_table	= sun50iw12_pinctrl_match,
	},
};
module_platform_driver(sun50iw12_pinctrl_driver);

MODULE_DESCRIPTION("Allwinner sun50iw12 pio pinctrl driver");
MODULE_LICENSE("GPL");
