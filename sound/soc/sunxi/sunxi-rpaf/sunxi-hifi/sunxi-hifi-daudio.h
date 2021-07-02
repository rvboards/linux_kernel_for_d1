/*
 * sound\soc\sunxi\hifi-dsp\sunxi-daudio.h
 *
 * (C) Copyright 2019-2025
 * Allwinner Technology Co., Ltd. <www.allwinnertech.com>
 * yumingfeng <yumingfeng@allwinertech.com>
 *
 * some simple description for this code
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 */

#ifndef	__SUNXI_HIFI_DAUDIO_H_
#define	__SUNXI_HIFI_DAUDIO_H_

#include "sunxi-hifi-pcm.h"

/*
 * Platform    I2S_count      ARM	DSP
 * sun50iw11	  5	      0-2	3-4
 */

#define DAUDIO_RX_FIFO_SIZE 64
#define DAUDIO_TX_FIFO_SIZE 128

struct daudio_label {
	unsigned long address;
	int value;
};

struct daudio_reg_label {
	const char *name;
	const unsigned int address;
	int value;
};

#define DAUDIO_REG_LABEL(constant) {#constant, constant, 0}
#define DAUDIO_REG_LABEL_END {NULL, -1, 0}

struct sunxi_daudio_mem_info {
	struct resource res;
	void __iomem *membase;
	struct resource *memregion;
	struct regmap *regmap;
};

struct sunxi_daudio_platform_data {
	unsigned int daudio_type;

	unsigned int pcm_lrck_period;
	unsigned int msb_lsb_first:1;
	unsigned int sign_extend:2;
	unsigned int tx_data_mode:2;
	unsigned int rx_data_mode:2;
	unsigned int slot_width_select;
	unsigned int frame_type;
	unsigned int tdm_config;
	unsigned int tdm_num;
	unsigned int mclk_div;
	unsigned int tx_num;
	unsigned int tx_chmap0;
	unsigned int tx_chmap1;
	unsigned int rx_num;
	unsigned int rx_chmap0;
	unsigned int rx_chmap1;
	unsigned int rx_chmap2;
	unsigned int rx_chmap3;

	/* eg:0 snddaudio0, 1 snddaudio1 */
	unsigned int dsp_daudio;
	/* eg:0 sndcodec; 1 snddmic; 2 snddaudio0; */
	unsigned int dsp_card;
	/* default is 0, for reserved */
	unsigned int dsp_device;
};

struct daudio_voltage_supply {
	struct regulator *daudio_regulator;
	const char *regulator_name;
};

struct sunxi_daudio_dts_info {
	struct sunxi_daudio_mem_info mem_info;
	struct sunxi_daudio_platform_data pdata_info;
	struct daudio_voltage_supply vol_supply;

	/* value must be (2^n)Kbyte */
	size_t playback_cma;
	size_t capture_cma;
};

struct sunxi_daudio_info {
	struct device *dev;
	struct sunxi_daudio_dts_info dts_info;
	struct sunxi_dma_params playback_dma_param;
	struct sunxi_dma_params capture_dma_param;
	struct snd_soc_dai *cpu_dai;
	struct daudio_label *reg_label;

	/* for hifi */
	unsigned int capturing;
	unsigned int playing;
	char wq_capture_name[32];
	struct mutex rpmsg_mutex_capture;
	struct workqueue_struct *wq_capture;
	struct work_struct trigger_work_capture;
	char wq_playback_name[32];
	struct mutex rpmsg_mutex_playback;
	struct workqueue_struct *wq_playback;
	struct work_struct trigger_work_playback;

	struct msg_substream_package msg_playback;
	struct msg_substream_package msg_capture;
	struct msg_mixer_package msg_mixer;
	struct msg_debug_package msg_debug;

	struct snd_dsp_component dsp_playcomp;
	struct snd_dsp_component dsp_capcomp;
};

#endif	/* __SUNXI_DAUDIO_H_ */
