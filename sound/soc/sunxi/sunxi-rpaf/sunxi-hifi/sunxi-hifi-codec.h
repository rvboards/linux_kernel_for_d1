/*
 * sound\soc\sunxi\hifi-dsp\sunxi-codec.h
 *
 * (C) Copyright 2019-2025
 * allwinner Technology Co., Ltd. <www.allwinnertech.com>
 * yumingfeng <yumingfeng@allwinnertech.com>
 *
 * some simple description for this code
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 */
#ifndef _SUNXI_CODEC_H
#define _SUNXI_CODEC_H
#include "sunxi-hifi-pcm.h"

#define CODEC_TX_FIFO_SIZE 128
#define CODEC_RX_FIFO_SIZE 256

#define LABEL(constant)		{#constant, constant, 0}
#define LABEL_END		{NULL, 0, -1}

struct label {
	const char *name;
	const unsigned int address;
	int value;
};

struct voltage_supply {
	struct regulator *cpvdd;
	struct regulator *avcc;
};

struct sunxi_codec_info {
	struct device *dev;
	struct regmap *regmap;
	struct resource digital_res;
	void __iomem *addr_base;
	struct snd_soc_codec *codec;
	struct msg_mixer_package msg_mixer;
};

#endif /* __SUNXI_CODEC_H */
