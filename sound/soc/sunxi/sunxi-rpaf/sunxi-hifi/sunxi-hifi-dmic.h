/*
 * sound\sunxi-rpaf\soc\sunxi\hifi-dsp\sunxi_dmic.h
 *
 * (C) Copyright 2019-2025
 * Allwinner Technology Co., Ltd. <www.allwinnertech.com>
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

#ifndef __SUNXI_HIFI_DMIC_H
#define __SUNXI_HIFI_DMIC_H

#include "sunxi-hifi-pcm.h"

struct sunxi_dmic_mem_info {
	struct resource res;
	void __iomem *membase;
	struct resource *memregion;
	struct regmap	*regmap;
};

struct sunxi_dmic_dts_info {
	struct sunxi_dmic_mem_info mem_info;
	/* eg:0 sndcodec; 1 snddmic; 2 snddaudio0; */
	unsigned int dsp_card;
	/* default is 0, for reserved */
	unsigned int dsp_device;
	/* value must be (2^n)Kbyte */
	size_t capture_cma;
	unsigned int rx_chmap;
	unsigned int data_vol;
};

struct sunxi_dmic_info {
	struct device *dev;
	struct snd_soc_dai_driver dai_drv;
	struct snd_soc_dai *cpu_dai;
	struct sunxi_dmic_dts_info dts_info;
	struct sunxi_dma_params capture_dma_param;
	bool capture_en;
	unsigned int chan_en;

	/* for hifi */
	unsigned int capturing;
	char wq_capture_name[32];
	struct mutex rpmsg_mutex_capture;
	struct workqueue_struct *wq_capture;
	struct work_struct trigger_work_capture;

	struct msg_substream_package msg_capture;
	struct msg_mixer_package msg_mixer;
	struct msg_debug_package msg_debug;
	struct snd_dsp_component dsp_capcomp;
};
#endif /* SUNXI_DMIC_H */
