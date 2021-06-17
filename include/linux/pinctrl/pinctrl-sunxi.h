/*
 * include/linux/pinctrl/pinctrl-sunxi.h
 *
 * (C) Copyright 2015-2020
 * Allwinner Technology Co., Ltd. <www.allwinnertech.com>
 * Martin Wu <wuyan@allwinnertech.com>
 *
 * sunxi pinctrl apis
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 */

#ifndef __API_PINCTRL_SUNXI_H
#define __API_PINCTRL_SUNXI_H

#include <linux/pinctrl/pinconf-generic.h>

/*
 * PIN_CONFIG_PARAM_MAX - max available value of 'enum pin_config_param'.
 * see include/linux/pinctrl/pinconf-generic.h
 */
#define PIN_CONFIG_PARAM_MAX    PIN_CONFIG_PERSIST_STATE
enum sunxi_pin_config_param {
	SUNXI_PINCFG_TYPE_FUNC = PIN_CONFIG_PARAM_MAX + 1,
	SUNXI_PINCFG_TYPE_DAT,
	SUNXI_PINCFG_TYPE_PUD,
	SUNXI_PINCFG_TYPE_DRV,
};
#endif
