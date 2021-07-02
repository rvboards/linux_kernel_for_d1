/*
 * sound\soc\sunxi\sun8iw20-codec.c
 * (C) Copyright 2021-2026
 * Allwinner Technology Co., Ltd. <www.allwinnertech.com>
 * yumingfeng <yumingfeng@allwinnertech.com>
 * luguofang <luguofang@allwinnertech.com>
 * Dby <Dby@allwinnertech.com>
 *
 * some simple description for this code
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 */
#include <linux/module.h>
#include <linux/init.h>
#include <linux/slab.h>
#include <linux/clk.h>
#include <linux/io.h>
#include <linux/delay.h>
#include <linux/of.h>
#include <linux/of_gpio.h>
#include <linux/of_address.h>
#include <linux/of_device.h>
#include <linux/pm.h>
#include <linux/regulator/consumer.h>
#include <linux/pinctrl/pinctrl-sunxi.h>
#include <linux/platform_device.h>
#include <linux/dma-mapping.h>
#include <linux/dmaengine.h>
#include <linux/reset.h>
#include <asm/dma.h>
#include <sound/pcm.h>
#include <sound/dmaengine_pcm.h>
#include <sound/pcm_params.h>
#include <sound/soc.h>
#include <sound/core.h>
#include <sound/soc-dapm.h>
#include <sound/initval.h>
#include <sound/tlv.h>
#include <linux/sunxi-gpio.h>
#include <linux/sunxi-sid.h>

#include "sun8iw20-codec.h"

#define LOG_ERR(fmt, arg...)	pr_err("[AUDIOCODEC][%s][%d]:" fmt "\n", __func__, __LINE__, ##arg)
#define LOG_WARN(fmt, arg...)	pr_warn("[AUDIOCODEC][%s][%d]:" fmt "\n", __func__, __LINE__, ##arg)
#define LOG_INFO(fmt, arg...)	pr_info("[AUDIOCODEC][%s][%d]:" fmt "\n", __func__, __LINE__, ##arg)

/* digital audio process function */
enum sunxi_hw_dap {
	DAP_HP_EN = 0x1,
	DAP_SPK_EN = 0x2,
	/* DAP_HP_EN | DAP_SPK_EN */
	DAP_HPSPK_EN = 0x3,
};

static const struct sample_rate sample_rate_conv[] = {
	{8000,   5},
	{11025,  4},
	{12000,  4},
	{16000,  3},
	{22050,  2},
	{24000,  2},
	{32000,  1},
	{44100,  0},
	{48000,  0},
	{96000,  7},
	{192000, 6},
};

static const DECLARE_TLV_DB_SCALE(digital_tlv, -7424, 116, 0);
static const DECLARE_TLV_DB_SCALE(dac_vol_tlv, -11925, 75, 0);
static const DECLARE_TLV_DB_SCALE(adc_vol_tlv, -11925, 75, 0);
static const DECLARE_TLV_DB_SCALE(mic_gain_tlv, 0, 100, 0);
static const DECLARE_TLV_DB_SCALE(fmin_gain_tlv, 0, 600, 0);
static const DECLARE_TLV_DB_SCALE(linein_gain_tlv, 0, 600, 0);
static const DECLARE_TLV_DB_SCALE(headphone_gain_tlv, -4200, 600, 0);
static const unsigned int lineout_tlv[] = {
	TLV_DB_RANGE_HEAD(2),
	0, 1, TLV_DB_SCALE_ITEM(0, 0, 1),
	2, 31, TLV_DB_SCALE_ITEM(-4350, 150, 1),
};

static struct reg_label reg_labels[] = {
	REG_LABEL(SUNXI_DAC_DPC),
	REG_LABEL(SUNXI_DAC_VOL_CTRL),
	REG_LABEL(SUNXI_DAC_FIFOC),
	REG_LABEL(SUNXI_DAC_FIFOS),
	REG_LABEL(SUNXI_DAC_TXDATA),
	REG_LABEL(SUNXI_DAC_CNT),
	REG_LABEL(SUNXI_DAC_DG),
	REG_LABEL(SUNXI_ADC_FIFOC),
	REG_LABEL(SUNXI_ADC_VOL_CTRL),
	REG_LABEL(SUNXI_ADC_FIFOS),
	REG_LABEL(SUNXI_ADC_RXDATA),
	REG_LABEL(SUNXI_ADC_CNT),
	REG_LABEL(SUNXI_ADC_DG),
	REG_LABEL(SUNXI_ADC_DIG_CTRL),
	REG_LABEL(SUNXI_VRA1SPEEDUP_DOWN_CTRL),
#ifdef SUNXI_CODEC_DAP_ENABLE
	REG_LABEL(SUNXI_DAC_DAP_CTL),
	REG_LABEL(SUNXI_ADC_DAP_CTL),
#endif
	REG_LABEL(SUNXI_ADC1_REG),
	REG_LABEL(SUNXI_ADC2_REG),
	REG_LABEL(SUNXI_ADC3_REG),
	REG_LABEL(SUNXI_DAC_REG),
	REG_LABEL(SUNXI_MICBIAS_REG),
	REG_LABEL(SUNXI_RAMP_REG),
	REG_LABEL(SUNXI_BIAS_REG),
	REG_LABEL(SUNXI_HMIC_CTRL),
	REG_LABEL(SUNXI_HMIC_STS),
	REG_LABEL(SUNXI_HP2_REG),
	REG_LABEL(SUNXI_POWER_REG),
	REG_LABEL(SUNXI_ADC_CUR_REG),
	REG_LABEL_END,
};

#ifdef SUNXI_CODEC_DAP_ENABLE
static void adcdrc_config(struct snd_soc_component *component)
{
	/* Enable DRC gain Min and Max limit, detect noise, Using Peak Filter */
	snd_soc_component_update_bits(component, SUNXI_ADC_DRC_CTRL,
		((0x1 << ADC_DRC_DELAY_BUF_EN) |
		(0x1 << ADC_DRC_GAIN_MAX_EN) | (0x1 << ADC_DRC_GAIN_MIN_EN) |
		(0x1 << ADC_DRC_NOISE_DET_EN) |
		(0x1 << ADC_DRC_SIGNAL_SEL) |
		(0x1 << ADC_DRC_LT_EN) | (0x1 << ADC_DRC_ET_EN)),
		((0x1 << ADC_DRC_DELAY_BUF_EN) |
		(0x1 << ADC_DRC_GAIN_MAX_EN) | (0x1 << ADC_DRC_GAIN_MIN_EN) |
		(0x1 << ADC_DRC_NOISE_DET_EN) |
		(0x1 << ADC_DRC_SIGNAL_SEL) |
		(0x1 << ADC_DRC_LT_EN) | (0x1 << ADC_DRC_ET_EN)));

	/* Left peak filter attack time */
	snd_soc_component_write(component, SUNXI_ADC_DRC_LPFHAT, (0x000B77F0 >> 16) & 0xFFFF);
	snd_soc_component_write(component, SUNXI_ADC_DRC_LPFLAT, 0x000B77F0 & 0xFFFF);
	/* Right peak filter attack time */
	snd_soc_component_write(component, SUNXI_ADC_DRC_RPFHAT, (0x000B77F0 >> 16) & 0xFFFF);
	snd_soc_component_write(component, SUNXI_ADC_DRC_RPFLAT, 0x000B77F0 & 0xFFFF);
	/* Left peak filter release time */
	snd_soc_component_write(component, SUNXI_ADC_DRC_LPFHRT, (0x00FFE1F8 >> 16) & 0xFFFF);
	snd_soc_component_write(component, SUNXI_ADC_DRC_LPFLRT, 0x00FFE1F8 & 0xFFFF);
	/* Right peak filter release time */
	snd_soc_component_write(component, SUNXI_ADC_DRC_RPFHRT, (0x00FFE1F8 >> 16) & 0xFFFF);
	snd_soc_component_write(component, SUNXI_ADC_DRC_RPFLRT, 0x00FFE1F8 & 0xFFFF);

	/* Left RMS filter attack time */
	snd_soc_component_write(component, SUNXI_ADC_DRC_LPFHAT, (0x00012BB0 >> 16) & 0xFFFF);
	snd_soc_component_write(component, SUNXI_ADC_DRC_LPFLAT, 0x00012BB0 & 0xFFFF);
	/* Right RMS filter attack time */
	snd_soc_component_write(component, SUNXI_ADC_DRC_RPFHAT, (0x00012BB0 >> 16) & 0xFFFF);
	snd_soc_component_write(component, SUNXI_ADC_DRC_RPFLAT, 0x00012BB0 & 0xFFFF);

	/* OPL */
	snd_soc_component_write(component, SUNXI_ADC_DRC_HOPL, (0xFF641741 >> 16) & 0xFFFF);
	snd_soc_component_write(component, SUNXI_ADC_DRC_LOPL, 0xFF641741 & 0xFFFF);
	/* OPC */
	snd_soc_component_write(component, SUNXI_ADC_DRC_HOPC, (0xFC0380F3 >> 16) & 0xFFFF);
	snd_soc_component_write(component, SUNXI_ADC_DRC_LOPC, 0xFC0380F3 & 0xFFFF);
	/* OPE */
	snd_soc_component_write(component, SUNXI_ADC_DRC_HOPE, (0xF608C25F >> 16) & 0xFFFF);
	snd_soc_component_write(component, SUNXI_ADC_DRC_LOPE, 0xF608C25F & 0xFFFF);
	/* LT */
	snd_soc_component_write(component, SUNXI_ADC_DRC_HLT, (0x035269E0 >> 16) & 0xFFFF);
	snd_soc_component_write(component, SUNXI_ADC_DRC_LLT, 0x035269E0 & 0xFFFF);
	/* CT */
	snd_soc_component_write(component, SUNXI_ADC_DRC_HCT, (0x06B3002C >> 16) & 0xFFFF);
	snd_soc_component_write(component, SUNXI_ADC_DRC_LCT, 0x06B3002C & 0xFFFF);
	/* ET */
	snd_soc_component_write(component, SUNXI_ADC_DRC_HET, (0x0A139682 >> 16) & 0xFFFF);
	snd_soc_component_write(component, SUNXI_ADC_DRC_LET, 0x0A139682 & 0xFFFF);
	/* Ki */
	snd_soc_component_write(component, SUNXI_ADC_DRC_HKI, (0x00222222 >> 16) & 0xFFFF);
	snd_soc_component_write(component, SUNXI_ADC_DRC_LKI, 0x00222222 & 0xFFFF);
	/* Kc */
	snd_soc_component_write(component, SUNXI_ADC_DRC_HKC, (0x01000000 >> 16) & 0xFFFF);
	snd_soc_component_write(component, SUNXI_ADC_DRC_LKC, 0x01000000 & 0xFFFF);
	/* Kn */
	snd_soc_component_write(component, SUNXI_ADC_DRC_HKN, (0x01C53EF0 >> 16) & 0xFFFF);
	snd_soc_component_write(component, SUNXI_ADC_DRC_LKN, 0x01C53EF0 & 0xFFFF);
	/* Ke */
	snd_soc_component_write(component, SUNXI_ADC_DRC_HKE, (0x04234F68 >> 16) & 0xFFFF);
	snd_soc_component_write(component, SUNXI_ADC_DRC_LKE, 0x04234F68 & 0xFFFF);

	/* smooth filter attack time */
	snd_soc_component_write(component, SUNXI_ADC_DRC_SFHAT, (0x00017665 >> 16) & 0xFFFF);
	snd_soc_component_write(component, SUNXI_ADC_DRC_SFLAT, 0x00017665 & 0xFFFF);
	/* gain smooth filter release time */
	snd_soc_component_write(component, SUNXI_ADC_DRC_SFHRT, (0x00000F04 >> 16) & 0xFFFF);
	snd_soc_component_write(component, SUNXI_ADC_DRC_SFLRT, 0x00000F04 & 0xFFFF);

	/* gain max setting */
	snd_soc_component_write(component, SUNXI_ADC_DRC_MXGHS, (0x69E0F95B >> 16) & 0xFFFF);
	snd_soc_component_write(component, SUNXI_ADC_DRC_MXGLS, 0x69E0F95B & 0xFFFF);

	/* gain min setting */
	snd_soc_component_write(component, SUNXI_ADC_DRC_MNGHS, (0xF95B2C3F >> 16) & 0xFFFF);
	snd_soc_component_write(component, SUNXI_ADC_DRC_MNGLS, 0xF95B2C3F & 0xFFFF);

	/* smooth filter release and attack time */
	snd_soc_component_write(component, SUNXI_ADC_DRC_EPSHC, (0x00025600 >> 16) & 0xFFFF);
	snd_soc_component_write(component, SUNXI_ADC_DRC_EPSHC, 0x00025600 & 0xFFFF);
}

static void adcdrc_enable(struct snd_soc_component *component, bool on)
{
	struct sunxi_codec_info *sunxi_codec = snd_soc_component_get_drvdata(component);
	struct codec_dap *adc_dap = &sunxi_codec->adc_dap;

	if (on) {
		if (adc_dap->drc_enable++ == 0) {
			snd_soc_component_update_bits(component, SUNXI_ADC_DAP_CTL,
				(0x1 << ADC_DRC0_EN), (0x1 << ADC_DRC0_EN));
			snd_soc_component_update_bits(component, SUNXI_ADC_DAP_CTL,
				(0x1 << ADC_DRC1_EN), (0x1 << ADC_DRC1_EN));
			if (sunxi_codec->adc_dap_enable++ == 0) {
				snd_soc_component_update_bits(component, SUNXI_ADC_DAP_CTL,
					(0x1 << ADC_DAP0_EN), (0x1 << ADC_DAP0_EN));
				snd_soc_component_update_bits(component, SUNXI_ADC_DAP_CTL,
					(0x1 << ADC_DAP1_EN), (0x1 << ADC_DAP1_EN));
			}
		}
	} else {
		if (--adc_dap->drc_enable <= 0) {
			adc_dap->drc_enable = 0;
			if (--sunxi_codec->adc_dap_enable <= 0) {
				sunxi_codec->adc_dap_enable = 0;
				snd_soc_component_update_bits(component, SUNXI_ADC_DAP_CTL,
					(0x1 << ADC_DAP0_EN), (0x0 << ADC_DAP0_EN));
				snd_soc_component_update_bits(component, SUNXI_ADC_DAP_CTL,
					(0x1 << ADC_DAP1_EN), (0x0 << ADC_DAP1_EN));
			}
			snd_soc_component_update_bits(component, SUNXI_ADC_DAP_CTL,
				(0x1 << ADC_DRC0_EN), (0x0 << ADC_DRC0_EN));
			snd_soc_component_update_bits(component, SUNXI_ADC_DAP_CTL,
				(0x1 << ADC_DRC1_EN), (0x0 << ADC_DRC1_EN));
		}
	}
}

static void adchpf_config(struct snd_soc_component *component)
{
	/* HPF */
	snd_soc_component_write(component, SUNXI_ADC_DRC_HHPFC, (0xFFFAC1 >> 16) & 0xFFFF);
	snd_soc_component_write(component, SUNXI_ADC_DRC_LHPFC, 0xFFFAC1 & 0xFFFF);
}

static void adchpf_enable(struct snd_soc_component *component, bool on)
{
	struct sunxi_codec_info *sunxi_codec = snd_soc_component_get_drvdata(component);
	struct codec_dap *adc_dap = &sunxi_codec->adc_dap;

	if (on) {
		if (adc_dap->hpf_enable++ == 0) {
			snd_soc_component_update_bits(component, SUNXI_ADC_DAP_CTL,
				(0x1 << ADC_HPF0_EN), (0x1 << ADC_HPF0_EN));
			snd_soc_component_update_bits(component, SUNXI_ADC_DAP_CTL,
				(0x1 << ADC_HPF1_EN), (0x1 << ADC_HPF1_EN));
			if (sunxi_codec->adc_dap_enable++ == 0) {
				snd_soc_component_update_bits(component, SUNXI_ADC_DAP_CTL,
					(0x1 << ADC_DAP0_EN), (0x1 << ADC_DAP0_EN));
				snd_soc_component_update_bits(component, SUNXI_ADC_DAP_CTL,
					(0x1 << ADC_DAP1_EN), (0x1 << ADC_DAP1_EN));
			}
		}
	} else {
		if (--adc_dap->hpf_enable <= 0) {
			adc_dap->hpf_enable = 0;
			if (--sunxi_codec->adc_dap_enable <= 0) {
				sunxi_codec->adc_dap_enable = 0;
				snd_soc_component_update_bits(component, SUNXI_ADC_DAP_CTL,
					(0x1 << ADC_DAP0_EN), (0x0 << ADC_DAP0_EN));
				snd_soc_component_update_bits(component, SUNXI_ADC_DAP_CTL,
					(0x1 << ADC_DAP1_EN), (0x0 << ADC_DAP1_EN));
			}
			snd_soc_component_update_bits(component, SUNXI_ADC_DAP_CTL,
				(0x1 << ADC_HPF0_EN), (0x0 << ADC_HPF0_EN));
			snd_soc_component_update_bits(component, SUNXI_ADC_DAP_CTL,
				(0x1 << ADC_HPF1_EN), (0x0 << ADC_HPF1_EN));
		}
	}
}

static void dacdrc_config(struct snd_soc_component *component)
{
	/* Left peak filter attack time */
	snd_soc_component_write(component, SUNXI_DAC_DRC_LPFHAT, (0x000B77BF >> 16) & 0xFFFF);
	snd_soc_component_write(component, SUNXI_DAC_DRC_LPFLAT, 0x000B77BF & 0xFFFF);
	/* Right peak filter attack time */
	snd_soc_component_write(component, SUNXI_DAC_DRC_RPFHAT, (0x000B77F0 >> 16) & 0xFFFF);
	snd_soc_component_write(component, SUNXI_DAC_DRC_RPFLAT, 0x000B77F0 & 0xFFFF);

	/* Left peak filter release time */
	snd_soc_component_write(component, SUNXI_DAC_DRC_LPFHRT, (0x00FFE1F8 >> 16) & 0xFFFF);
	snd_soc_component_write(component, SUNXI_DAC_DRC_LPFLRT, 0x00FFE1F8 & 0xFFFF);
	/* Right peak filter release time */
	snd_soc_component_write(component, SUNXI_DAC_DRC_RPFHRT, (0x00FFE1F8 >> 16) & 0xFFFF);
	snd_soc_component_write(component, SUNXI_DAC_DRC_RPFLRT, 0x00FFE1F8 & 0xFFFF);

	/* Left RMS filter attack time */
	snd_soc_component_write(component, SUNXI_DAC_DRC_LRMSHAT, (0x00012BB0 >> 16) & 0xFFFF);
	snd_soc_component_write(component, SUNXI_DAC_DRC_LRMSLAT, 0x00012BB0 & 0xFFFF);
	/* Right RMS filter attack time */
	snd_soc_component_write(component, SUNXI_DAC_DRC_RRMSHAT, (0x00012BB0 >> 16) & 0xFFFF);
	snd_soc_component_write(component, SUNXI_DAC_DRC_RRMSLAT, 0x00012BB0 & 0xFFFF);

	/* smooth filter attack time */
	snd_soc_component_write(component, SUNXI_DAC_DRC_SFHAT, (0x00017665 >> 16) & 0xFFFF);
	snd_soc_component_write(component, SUNXI_DAC_DRC_SFLAT, 0x00017665 & 0xFFFF);
	/* gain smooth filter release time */
	snd_soc_component_write(component, SUNXI_DAC_DRC_SFHRT, (0x00000F04 >> 16) & 0xFFFF);
	snd_soc_component_write(component, SUNXI_DAC_DRC_SFLRT, 0x00000F04 & 0xFFFF);

	/* OPL */
	snd_soc_component_write(component, SUNXI_DAC_DRC_HOPL, (0xFE56CB10 >> 16) & 0xFFFF);
	snd_soc_component_write(component, SUNXI_DAC_DRC_LOPL, 0xFE56CB10 & 0xFFFF);
	/* OPC */
	snd_soc_component_write(component, SUNXI_DAC_DRC_HOPC, (0xFB04612F >> 16) & 0xFFFF);
	snd_soc_component_write(component, SUNXI_DAC_DRC_LOPC, 0xFB04612F & 0xFFFF);
	/* OPE */
	snd_soc_component_write(component, SUNXI_DAC_DRC_HOPE, (0xF608C25F >> 16) & 0xFFFF);
	snd_soc_component_write(component, SUNXI_DAC_DRC_LOPE, 0xF608C25F & 0xFFFF);
	/* LT */
	snd_soc_component_write(component, SUNXI_DAC_DRC_HLT, (0x035269E0 >> 16) & 0xFFFF);
	snd_soc_component_write(component, SUNXI_DAC_DRC_LLT, 0x035269E0 & 0xFFFF);
	/* CT */
	snd_soc_component_write(component, SUNXI_DAC_DRC_HCT, (0x06B3002C >> 16) & 0xFFFF);
	snd_soc_component_write(component, SUNXI_DAC_DRC_LCT, 0x06B3002C & 0xFFFF);
	/* ET */
	snd_soc_component_write(component, SUNXI_DAC_DRC_HET, (0x0A139682 >> 16) & 0xFFFF);
	snd_soc_component_write(component, SUNXI_DAC_DRC_LET, 0x0A139682 & 0xFFFF);
	/* Ki */
	snd_soc_component_write(component, SUNXI_DAC_DRC_HKI, (0x00400000 >> 16) & 0xFFFF);
	snd_soc_component_write(component, SUNXI_DAC_DRC_LKI, 0x00400000 & 0xFFFF);
	/* Kc */
	snd_soc_component_write(component, SUNXI_DAC_DRC_HKC, (0x00FBCDA5 >> 16) & 0xFFFF);
	snd_soc_component_write(component, SUNXI_DAC_DRC_LKC, 0x00FBCDA5 & 0xFFFF);
	/* Kn */
	snd_soc_component_write(component, SUNXI_DAC_DRC_HKN, (0x0179B472 >> 16) & 0xFFFF);
	snd_soc_component_write(component, SUNXI_DAC_DRC_LKN, 0x0179B472 & 0xFFFF);
	/* Ke */
	snd_soc_component_write(component, SUNXI_DAC_DRC_HKE, (0x04234F68 >> 16) & 0xFFFF);
	snd_soc_component_write(component, SUNXI_DAC_DRC_LKE, 0x04234F68 & 0xFFFF);
	/* MXG */
	snd_soc_component_write(component, SUNXI_DAC_DRC_MXGHS, (0x035269E0 >> 16) & 0xFFFF);
	snd_soc_component_write(component, SUNXI_DAC_DRC_MXGLS, 0x035269E0 & 0xFFFF);
	/* MNG */
	snd_soc_component_write(component, SUNXI_DAC_DRC_MNGHS, (0xF95B2C3F >> 16) & 0xFFFF);
	snd_soc_component_write(component, SUNXI_DAC_DRC_MNGLS, 0xF95B2C3F & 0xFFFF);
	/* EPS */
	snd_soc_component_write(component, SUNXI_DAC_DRC_EPSHC, (0x00025600 >> 16) & 0xFFFF);
	snd_soc_component_write(component, SUNXI_DAC_DRC_EPSLC, 0x00025600 & 0xFFFF);
}

static void dacdrc_enable(struct snd_soc_component *component, bool on)
{
	struct sunxi_codec_info *sunxi_codec = snd_soc_component_get_drvdata(component);
	struct codec_dap *dac_dap = &sunxi_codec->dac_dap;

	if (on) {
		if (dac_dap->drc_enable++ == 0) {
			/* detect noise when ET enable */
			snd_soc_component_update_bits(component, SUNXI_DAC_DRC_CTRL,
				(0x1 << DAC_DRC_NOISE_DET_EN),
				(0x1 << DAC_DRC_NOISE_DET_EN));

			/* 0x0:RMS filter; 0x1:Peak filter */
			snd_soc_component_update_bits(component, SUNXI_DAC_DRC_CTRL,
				(0x1 << DAC_DRC_SIGNAL_SEL),
				(0x1 << DAC_DRC_SIGNAL_SEL));

			snd_soc_component_update_bits(component, SUNXI_DAC_DRC_CTRL,
				(0x1 << DAC_DRC_GAIN_MAX_EN),
				(0x1 << DAC_DRC_GAIN_MAX_EN));

			snd_soc_component_update_bits(component, SUNXI_DAC_DRC_CTRL,
				(0x1 << DAC_DRC_GAIN_MIN_EN),
				(0x1 << DAC_DRC_GAIN_MIN_EN));

			/* delay function enable */
			snd_soc_component_update_bits(component, SUNXI_DAC_DRC_CTRL,
				(0x1 << DAC_DRC_DELAY_BUF_EN),
				(0x1 << DAC_DRC_DELAY_BUF_EN));

			snd_soc_component_update_bits(component, SUNXI_DAC_DRC_CTRL,
				(0x1 << DAC_DRC_LT_EN),
				(0x1 << DAC_DRC_LT_EN));
			snd_soc_component_update_bits(component, SUNXI_DAC_DRC_CTRL,
				(0x1 << DAC_DRC_ET_EN),
				(0x1 << DAC_DRC_ET_EN));

			snd_soc_component_update_bits(component, SUNXI_DAC_DAP_CTL,
				(0x1 << DDAP_DRC_EN),
				(0x1 << DDAP_DRC_EN));

			if (sunxi_codec->dac_dap_enable++ == 0)
				snd_soc_component_update_bits(component, SUNXI_DAC_DAP_CTL,
					(0x1 << DDAP_EN), (0x1 << DDAP_EN));
		}
	} else {
		if (--dac_dap->drc_enable <= 0) {
			dac_dap->drc_enable = 0;
			if (--sunxi_codec->dac_dap_enable <= 0) {
				sunxi_codec->dac_dap_enable = 0;
				snd_soc_component_update_bits(component, SUNXI_DAC_DAP_CTL,
					(0x1 << DDAP_EN), (0x0 << DDAP_EN));
			}

			snd_soc_component_update_bits(component, SUNXI_DAC_DAP_CTL,
				(0x1 << DDAP_DRC_EN),
				(0x0 << DDAP_DRC_EN));

			/* detect noise when ET enable */
			snd_soc_component_update_bits(component, SUNXI_DAC_DRC_CTRL,
				(0x1 << DAC_DRC_NOISE_DET_EN),
				(0x0 << DAC_DRC_NOISE_DET_EN));

			/* 0x0:RMS filter; 0x1:Peak filter */
			snd_soc_component_update_bits(component, SUNXI_DAC_DRC_CTRL,
				(0x1 << DAC_DRC_SIGNAL_SEL),
				(0x0 << DAC_DRC_SIGNAL_SEL));

			/* delay function enable */
			snd_soc_component_update_bits(component, SUNXI_DAC_DRC_CTRL,
				(0x1 << DAC_DRC_DELAY_BUF_EN),
				(0x0 << DAC_DRC_DELAY_BUF_EN));

			snd_soc_component_update_bits(component, SUNXI_DAC_DRC_CTRL,
				(0x1 << DAC_DRC_GAIN_MAX_EN),
				(0x0 << DAC_DRC_GAIN_MAX_EN));
			snd_soc_component_update_bits(component, SUNXI_DAC_DRC_CTRL,
				(0x1 << DAC_DRC_GAIN_MIN_EN),
				(0x0 << DAC_DRC_GAIN_MIN_EN));

			snd_soc_component_update_bits(component, SUNXI_DAC_DRC_CTRL,
				(0x1 << DAC_DRC_LT_EN),
				(0x0 << DAC_DRC_LT_EN));
			snd_soc_component_update_bits(component, SUNXI_DAC_DRC_CTRL,
				(0x1 << DAC_DRC_ET_EN),
				(0x0 << DAC_DRC_ET_EN));
		}
	}
}

static void dachpf_config(struct snd_soc_component *component)
{
	/* HPF */
	snd_soc_component_write(component, SUNXI_DAC_DRC_HHPFC, (0xFFFAC1 >> 16) & 0xFFFF);
	snd_soc_component_write(component, SUNXI_DAC_DRC_LHPFC, 0xFFFAC1 & 0xFFFF);
}

static void dachpf_enable(struct snd_soc_component *component, bool on)
{
	struct sunxi_codec_info *sunxi_codec = snd_soc_component_get_drvdata(component);
	struct codec_dap *dac_dap = &sunxi_codec->dac_dap;

	if (on) {
		if (dac_dap->hpf_enable++ == 0) {
			snd_soc_component_update_bits(component, SUNXI_DAC_DAP_CTL,
				(0x1 << DDAP_HPF_EN), (0x1 << DDAP_HPF_EN));

			if (sunxi_codec->dac_dap_enable++ == 0)
				snd_soc_component_update_bits(component, SUNXI_DAC_DAP_CTL,
					(0x1 << DDAP_EN), (0x1 << DDAP_EN));
		}
	} else {
		if (--dac_dap->hpf_enable <= 0) {
			dac_dap->hpf_enable = 0;
			if (--sunxi_codec->dac_dap_enable <= 0) {
				sunxi_codec->dac_dap_enable = 0;
				snd_soc_component_update_bits(component, SUNXI_DAC_DAP_CTL,
					(0x1 << DDAP_EN), (0x0 << DDAP_EN));
			}

			snd_soc_component_update_bits(component, SUNXI_DAC_DAP_CTL,
				(0x1 << DDAP_HPF_EN),
				(0x0 << DDAP_HPF_EN));
		}
	}
}
#endif

#ifdef SUNXI_CODEC_HUB_ENABLE
static int sunxi_codec_get_hub_mode(struct snd_kcontrol *kcontrol,
			struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_component *component = snd_kcontrol_chip(kcontrol);
	unsigned int reg_val;

	reg_val = snd_soc_component_read32(component, SUNXI_DAC_DPC);

	ucontrol->value.integer.value[0] =
				((reg_val & (0x1 << DAC_HUB_EN)) ? 1 : 0);

	return 0;
}

static int sunxi_codec_set_hub_mode(struct snd_kcontrol *kcontrol,
			struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_component *component = snd_kcontrol_chip(kcontrol);

	switch (ucontrol->value.integer.value[0]) {
	case	0:
		snd_soc_component_update_bits(component, SUNXI_DAC_DPC,
				(0x1 << DAC_HUB_EN), (0x0 << DAC_HUB_EN));
		break;
	case	1:
		snd_soc_component_update_bits(component, SUNXI_DAC_DPC,
				(0x1 << DAC_HUB_EN), (0x1 << DAC_HUB_EN));
		break;
	default:
		return -EINVAL;
	}
	return 0;
}

/* sunxi codec hub mdoe select */
static const char * const sunxi_codec_hub_function[] = {
				"hub_disable", "hub_enable"};

static const struct soc_enum sunxi_codec_hub_mode_enum[] = {
	SOC_ENUM_SINGLE_EXT(ARRAY_SIZE(sunxi_codec_hub_function),
			sunxi_codec_hub_function),
};
#endif

#ifdef SUNXI_CODEC_ADCSWAP_ENABLE
static int sunxi_codec_get_adcswap1_mode(struct snd_kcontrol *kcontrol,
			struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_component *component = snd_kcontrol_chip(kcontrol);
	unsigned int reg_val;

	reg_val = snd_soc_component_read32(component, SUNXI_ADC_DG);

	ucontrol->value.integer.value[0] =
				((reg_val & (0x1 << AD_SWP1)) ? 1 : 0);

	return 0;
}

static int sunxi_codec_set_adcswap1_mode(struct snd_kcontrol *kcontrol,
			struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_component *component = snd_kcontrol_chip(kcontrol);

	switch (ucontrol->value.integer.value[0]) {
	case	0:
		snd_soc_component_update_bits(component, SUNXI_ADC_DG,
				(0x1 << AD_SWP1), (0x0 << AD_SWP1));
		break;
	case	1:
		snd_soc_component_update_bits(component, SUNXI_ADC_DG,
				(0x1 << AD_SWP1), (0x1 << AD_SWP1));
		break;
	default:
		return -EINVAL;
	}

	return 0;
}

static int sunxi_codec_get_adcswap2_mode(struct snd_kcontrol *kcontrol,
					 struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_component *component = snd_kcontrol_chip(kcontrol);
	unsigned int reg_val;

	reg_val = snd_soc_component_read32(component, SUNXI_ADC_DG);

	ucontrol->value.integer.value[0] =
				((reg_val & (0x1 << AD_SWP2)) ? 1 : 0);

	return 0;
}

static int sunxi_codec_set_adcswap2_mode(struct snd_kcontrol *kcontrol,
					 struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_component *component = snd_kcontrol_chip(kcontrol);

	switch (ucontrol->value.integer.value[0]) {
	case	0:
		snd_soc_component_update_bits(component, SUNXI_ADC_DG,
				(0x1 << AD_SWP2), (0x0 << AD_SWP2));
		break;
	case	1:
		snd_soc_component_update_bits(component, SUNXI_ADC_DG,
				(0x1 << AD_SWP2), (0x1 << AD_SWP2));
		break;
	default:
		return -EINVAL;
	}

	return 0;
}

/* sunxi codec adc swap func */
static const char * const sunxi_codec_adcswap_function[] = {
				"Off", "On"};

static const struct soc_enum sunxi_codec_adcswap_func_enum[] = {
	SOC_ENUM_SINGLE_EXT(ARRAY_SIZE(sunxi_codec_adcswap_function),
			sunxi_codec_adcswap_function),
};
#endif

static int sunxi_codec_hpspeaker_event(struct snd_soc_dapm_widget *w,
				struct snd_kcontrol *k, int event)
{
	struct snd_soc_component *component = snd_soc_dapm_to_component(w->dapm);
	struct sunxi_codec_info *sunxi_codec = snd_soc_component_get_drvdata(component);
	struct codec_spk_config *spk_cfg = &(sunxi_codec->spk_config);
	struct codec_hw_config *hw_cfg = &(sunxi_codec->hw_config);

	switch (event) {
	case	SND_SOC_DAPM_POST_PMU:
#ifdef SUNXI_CODEC_DAP_ENABLE
		if (hw_cfg->dacdrc_cfg & DAP_SPK_EN)
			dacdrc_enable(component, 1);
		else if (hw_cfg->dacdrc_cfg & DAP_HP_EN)
			dacdrc_enable(component, 0);

		if (hw_cfg->dachpf_cfg & DAP_SPK_EN)
			dachpf_enable(component, 1);
		else if (hw_cfg->dachpf_cfg & DAP_HP_EN)
			dachpf_enable(component, 0);
#endif
		if (spk_cfg->used) {
			gpio_direction_output(spk_cfg->spk_gpio, 1);
			gpio_set_value(spk_cfg->spk_gpio, spk_cfg->pa_level);
			/* time delay to wait spk pa work fine */
			msleep(spk_cfg->pa_msleep);
		}
		break;
	case	SND_SOC_DAPM_PRE_PMD:
		if (spk_cfg->used)
			gpio_set_value(spk_cfg->spk_gpio, !(spk_cfg->pa_level));
#ifdef SUNXI_CODEC_DAP_ENABLE
		if (hw_cfg->dacdrc_cfg & DAP_SPK_EN)
			dacdrc_enable(component, 0);
		if (hw_cfg->dacdrc_cfg & DAP_SPK_EN)
			dachpf_enable(component, 0);
#endif
		break;
	default:
		break;
	}

	return 0;
}

static int sunxi_codec_headphone_event(struct snd_soc_dapm_widget *w,
			struct snd_kcontrol *k,	int event)
{
	struct snd_soc_component *component = snd_soc_dapm_to_component(w->dapm);
	struct sunxi_codec_info *sunxi_codec = snd_soc_component_get_drvdata(component);
	struct codec_hw_config *hw_cfg = &(sunxi_codec->hw_config);

	switch (event) {
	case SND_SOC_DAPM_POST_PMU:
#ifdef SUNXI_CODEC_DAP_ENABLE
		if (hw_cfg->dacdrc_cfg & DAP_HP_EN)
			dacdrc_enable(component, 1);
		if (hw_cfg->dachpf_cfg & DAP_HP_EN)
			dachpf_enable(component, 1);
#endif
		snd_soc_component_update_bits(component, SUNXI_RAMP_REG,
				(0x1 << RMC_EN), (0x1 << RMC_EN));
		snd_soc_component_update_bits(component, SUNXI_HP2_REG,
				(0x1 << RAMP_OUT_EN), (0x1 << RAMP_OUT_EN));
		snd_soc_component_update_bits(component, SUNXI_HP2_REG,
				(0x1 << HP_DRVOUTEN), (0x1 << HP_DRVOUTEN));
		snd_soc_component_update_bits(component, SUNXI_POWER_REG,
				(0x1 << HPLDO_EN), (0x1 << HPLDO_EN));
		snd_soc_component_update_bits(component, SUNXI_HP2_REG,
				(0x1 << HP_DRVEN), (0x1 << HP_DRVEN));
		break;
	case SND_SOC_DAPM_PRE_PMD:
		snd_soc_component_update_bits(component, SUNXI_HP2_REG,
				(0x1 << HP_DRVEN), (0x0 << HP_DRVEN));
		/* A version of the chip cannot be disable HPLDO and RMC_EN */
		if (sunxi_get_soc_ver() & 0x7) {
			snd_soc_component_update_bits(component, SUNXI_POWER_REG,
					(0x1 << HPLDO_EN), (0x0 << HPLDO_EN));
			snd_soc_component_update_bits(component, SUNXI_RAMP_REG,
					(0x1 << RMC_EN), (0x0 << RMC_EN));
		} else {
			snd_soc_component_update_bits(component, SUNXI_POWER_REG,
					(0x1 << HPLDO_EN), (0x1 << HPLDO_EN));
			snd_soc_component_update_bits(component, SUNXI_RAMP_REG,
					(0x1 << RMC_EN), (0x1 << RMC_EN));
		}
		snd_soc_component_update_bits(component, SUNXI_HP2_REG,
				(0x1 << HP_DRVOUTEN), (0x0 << HP_DRVOUTEN));
		snd_soc_component_update_bits(component, SUNXI_HP2_REG,
				(0x1 << RAMP_OUT_EN), (0x0 << RAMP_OUT_EN));
#ifdef SUNXI_CODEC_DAP_ENABLE
		if (hw_cfg->dacdrc_cfg & DAP_HP_EN)
			dacdrc_enable(component, 0);
		if (hw_cfg->dachpf_cfg & DAP_HP_EN)
			dachpf_enable(component, 0);
#endif
		break;
	}

	return 0;
}

static int sunxi_codec_lineout_event(struct snd_soc_dapm_widget *w,
				  struct snd_kcontrol *k, int event)
{
	struct snd_soc_component *component = snd_soc_dapm_to_component(w->dapm);
	struct sunxi_codec_info *sunxi_codec = snd_soc_component_get_drvdata(component);
	struct codec_spk_config *spk_cfg = &(sunxi_codec->spk_config);
	struct codec_hw_config *hw_cfg = &(sunxi_codec->hw_config);

	switch (event) {
	case	SND_SOC_DAPM_POST_PMU:
#ifdef SUNXI_CODEC_DAP_ENABLE
		if (hw_cfg->dacdrc_cfg & DAP_SPK_EN)
			dacdrc_enable(component, 1);
		if (hw_cfg->dachpf_cfg & DAP_SPK_EN)
			dachpf_enable(component, 1);
#endif
		snd_soc_component_update_bits(component, SUNXI_DAC_REG,
				(0x1 << LINEOUTLEN), (0x1 << LINEOUTLEN));
		snd_soc_component_update_bits(component, SUNXI_DAC_REG,
				(0x1 << LINEOUTREN), (0x1 << LINEOUTREN));
		snd_soc_component_update_bits(component, SUNXI_RAMP_REG,
				(0x1 << RMC_EN), (0x1 << RMC_EN));

		if (spk_cfg->used) {
			gpio_direction_output(spk_cfg->spk_gpio, 1);
			gpio_set_value(spk_cfg->spk_gpio, spk_cfg->pa_level);
			/* time delay to wait spk pa work fine */
			msleep(spk_cfg->pa_msleep);
		}
		break;
	case	SND_SOC_DAPM_PRE_PMD:
		if (spk_cfg->used)
			gpio_set_value(spk_cfg->spk_gpio, !(spk_cfg->pa_level));

		snd_soc_component_update_bits(component, SUNXI_DAC_REG,
				(0x1 << LINEOUTLEN), (0x0 << LINEOUTLEN));
		snd_soc_component_update_bits(component, SUNXI_DAC_REG,
				(0x1 << LINEOUTREN), (0x0 << LINEOUTREN));
		/* A version of the chip cannot be disable RMC_EN */
		if (sunxi_get_soc_ver() & 0x7)
			snd_soc_component_update_bits(component, SUNXI_RAMP_REG,
					(0x1 << RMC_EN), (0x0 << RMC_EN));
		else
			snd_soc_component_update_bits(component, SUNXI_RAMP_REG,
					(0x1 << RMC_EN), (0x1 << RMC_EN));

#ifdef SUNXI_CODEC_DAP_ENABLE
		if (hw_cfg->dacdrc_cfg & DAP_SPK_EN)
			dacdrc_enable(component, 0);
		if (hw_cfg->dachpf_cfg & DAP_SPK_EN)
			dachpf_enable(component, 0);
#endif
		break;
	default:
		break;
	}

	return 0;
}

static int sunxi_codec_playback_event(struct snd_soc_dapm_widget *w,
				struct snd_kcontrol *k, int event)
{
	struct snd_soc_component *component = snd_soc_dapm_to_component(w->dapm);

	switch (event) {
	case	SND_SOC_DAPM_PRE_PMU:
		/* DACL to left channel LINEOUT Mute control 0:mute 1: not mute */
		msleep(30);
		snd_soc_component_update_bits(component, SUNXI_DAC_REG,
				(0x1 << DACLMUTE) | (0x1 << DACRMUTE),
				(0x1 << DACLMUTE) | (0x1 << DACRMUTE));
		snd_soc_component_update_bits(component, SUNXI_DAC_DPC,
				(0x1<<EN_DAC), (0x1<<EN_DAC));
		break;
	case	SND_SOC_DAPM_POST_PMD:
		snd_soc_component_update_bits(component, SUNXI_DAC_DPC,
				(0x1<<EN_DAC), (0x0<<EN_DAC));
		/* DACL to left channel LINEOUT Mute control 0:mute 1: not mute */
		snd_soc_component_update_bits(component, SUNXI_DAC_REG,
				(0x1 << DACLMUTE) | (0x1 << DACRMUTE),
				(0x0 << DACLMUTE) | (0x0 << DACRMUTE));
		break;
	default:
		break;
	}

	return 0;
}

/*
static int sunxi_codec_adc1_event(struct snd_soc_dapm_widget *w,
				struct snd_kcontrol *k, int event)
{
	struct snd_soc_component *component = snd_soc_dapm_to_component(w->dapm);

	switch (event) {
	case	SND_SOC_DAPM_POST_PMU:
		//mdelay(80);
		snd_soc_component_update_bits(component, SUNXI_ADC1_REG,
				(0x1 << ADC1_EN), (0x1 << ADC1_EN));
		snd_soc_component_update_bits(component, SUNXI_ADC_FIFOC,
				(0x1 << EN_AD), (0x1 << EN_AD));
		break;
	case	SND_SOC_DAPM_POST_PMD:
		snd_soc_component_update_bits(component, SUNXI_ADC_FIFOC,
				(0x1 << EN_AD), (0x0 << EN_AD));
		snd_soc_component_update_bits(component, SUNXI_ADC1_REG,
				(0x1 << ADC1_EN), (0x0 << ADC1_EN));
		break;
	default:
		break;
	}

	return 0;
}
static int sunxi_codec_adc2_event(struct snd_soc_dapm_widget *w,
				struct snd_kcontrol *k, int event)
{
	struct snd_soc_component *component = snd_soc_dapm_to_component(w->dapm);

	switch (event) {
	case	SND_SOC_DAPM_POST_PMU:
		//mdelay(80);
		snd_soc_component_update_bits(component, SUNXI_ADC2_REG,
				(0x1 << ADC2_EN), (0x1 << ADC2_EN));
		snd_soc_component_update_bits(component, SUNXI_ADC_FIFOC,
				(0x1 << EN_AD), (0x1 << EN_AD));
		break;
	case	SND_SOC_DAPM_POST_PMD:
		snd_soc_component_update_bits(component, SUNXI_ADC_FIFOC,
				(0x1 << EN_AD), (0x0 << EN_AD));
		snd_soc_component_update_bits(component, SUNXI_ADC2_REG,
				(0x1 << ADC2_EN), (0x0 << ADC2_EN));
		break;
	default:
		break;
	}

	return 0;
}
static int sunxi_codec_adc3_event(struct snd_soc_dapm_widget *w,
				struct snd_kcontrol *k, int event)
{
	struct snd_soc_component *component = snd_soc_dapm_to_component(w->dapm);

	switch (event) {
	case	SND_SOC_DAPM_POST_PMU:
		snd_soc_component_update_bits(component, SUNXI_ADC3_REG,
				(0x1 << ADC3_EN), (0x1 << ADC3_EN));
		snd_soc_component_update_bits(component, SUNXI_ADC_FIFOC,
				(0x1 << EN_AD), (0x1 << EN_AD));
		break;
	case	SND_SOC_DAPM_POST_PMD:
		snd_soc_component_update_bits(component, SUNXI_ADC_FIFOC,
				(0x1 << EN_AD), (0x0 << EN_AD));
		snd_soc_component_update_bits(component, SUNXI_ADC3_REG,
				(0x1 << ADC3_EN), (0x0 << ADC3_EN));
		break;
	default:
		break;
	}

	return 0;
}
*/

static int sunxi_codec_capture_event(struct snd_soc_dapm_widget *w,
				struct snd_kcontrol *k, int event)
{
	struct snd_soc_component *component = snd_soc_dapm_to_component(w->dapm);

	switch (event) {
	case	SND_SOC_DAPM_POST_PMU:
		//delay 80ms to avoid the capture pop at the beginning
		mdelay(80);
		snd_soc_component_update_bits(component, SUNXI_ADC_FIFOC,
				(0x1 << EN_AD), (0x1 << EN_AD));
		break;
	case	SND_SOC_DAPM_POST_PMD:
		snd_soc_component_update_bits(component, SUNXI_ADC_FIFOC,
				(0x1 << EN_AD), (0x0 << EN_AD));
		break;
	default:
		break;
	}

	return 0;
}

static int sunxi_codec_adc_mixer_event(struct snd_soc_dapm_widget *w,
				struct snd_kcontrol *k, int event)
{
#ifdef SUNXI_CODEC_DAP_ENABLE
	struct snd_soc_component *component = snd_soc_dapm_to_component(w->dapm);
	struct sunxi_codec_info *sunxi_codec = snd_soc_component_get_drvdata(component);
	struct codec_hw_config *hw_cfg = &(sunxi_codec->hw_config);
	unsigned int adcctrl_val = 0;

	switch (event) {
	case SND_SOC_DAPM_POST_PMU:
		if (hw_cfg->adcdrc_cfg & DAP_HP_EN) {
			adcctrl_val = snd_soc_component_read32(component, SUNXI_ADC1_REG);
			if ((adcctrl_val >> MIC1_PGA_EN) & 0x1)
				adcdrc_enable(component, 1);
		} else if (hw_cfg->adcdrc_cfg & DAP_SPK_EN) {
			adcctrl_val = snd_soc_component_read32(component, SUNXI_ADC2_REG);
			if ((adcctrl_val >> MIC2_PGA_EN) & 0x1)
				adcdrc_enable(component, 1);
		}
		break;
	case SND_SOC_DAPM_POST_PMD:
		if ((hw_cfg->adcdrc_cfg & DAP_SPK_EN) ||
			(hw_cfg->adcdrc_cfg & DAP_HP_EN))
			adcdrc_enable(component, 0);
		break;
	default:
		break;
	}
#endif

	return 0;
}

/*
static int sunxi_codec_adc1_mixer_event(struct snd_soc_dapm_widget *w,
					struct snd_kcontrol *k, int event)
{
	struct snd_soc_component *component = snd_soc_dapm_to_component(w->dapm);
	struct sunxi_codec_info *sunxi_codec = snd_soc_component_get_drvdata(component);
	struct codec_hw_config *hw_cfg = &(sunxi_codec->hw_config);
	unsigned int adcctrl_val = 0;

	switch (event) {
	case SND_SOC_DAPM_POST_PMU:
		if (hw_cfg->adcdrc_cfg & DAP_HP_EN) {
			adcctrl_val = snd_soc_component_read32(component, SUNXI_ADC1_REG);
			if ((adcctrl_val >> MIC1_PGA_EN) & 0x1)
				adcdrc_enable(component, 1);
		} else if (hw_cfg->adcdrc_cfg & DAP_SPK_EN) {
			adcctrl_val = snd_soc_component_read32(component, SUNXI_ADC2_REG);
			if ((adcctrl_val >> MIC2_PGA_EN) & 0x1)
				adcdrc_enable(component, 1);
		}

		snd_soc_component_update_bits(component, SUNXI_ADC_DIG_CTRL,
			(0x1 << ADC1_CHANNEL_EN), (0x1 << ADC1_CHANNEL_EN));

		break;
	case SND_SOC_DAPM_POST_PMD:
		if ((hw_cfg->adcdrc_cfg & DAP_SPK_EN) ||
			(hw_cfg->adcdrc_cfg & DAP_HP_EN))
			adcdrc_enable(component, 0);

		snd_soc_component_update_bits(component, SUNXI_ADC_DIG_CTRL,
			(0x1 << ADC1_CHANNEL_EN), (0x0 << ADC1_CHANNEL_EN));

		break;
	default:
		break;
	}

	return 0;
}
static int sunxi_codec_adc2_mixer_event(struct snd_soc_dapm_widget *w,
					struct snd_kcontrol *k, int event)
{
	struct snd_soc_component *component = snd_soc_dapm_to_component(w->dapm);
	struct sunxi_codec_info *sunxi_codec = snd_soc_component_get_drvdata(component);
	struct codec_hw_config *hw_cfg = &(sunxi_codec->hw_config);
	unsigned int adcctrl_val = 0;

	switch (event) {
	case SND_SOC_DAPM_POST_PMU:
		if (hw_cfg->adcdrc_cfg & DAP_HP_EN) {
			adcctrl_val = snd_soc_component_read32(component, SUNXI_ADC1_REG);
			if ((adcctrl_val >> MIC1_PGA_EN) & 0x1)
				adcdrc_enable(component, 1);
		} else if (hw_cfg->adcdrc_cfg & DAP_SPK_EN) {
			adcctrl_val = snd_soc_component_read32(component, SUNXI_ADC2_REG);
			if ((adcctrl_val >> MIC2_PGA_EN) & 0x1)
				adcdrc_enable(component, 1);
		}
		snd_soc_component_update_bits(component, SUNXI_ADC_DIG_CTRL,
			(0x1 << ADC2_CHANNEL_EN), (0x1 << ADC2_CHANNEL_EN));
		break;
	case SND_SOC_DAPM_POST_PMD:
		if ((hw_cfg->adcdrc_cfg & DAP_SPK_EN) ||
			(hw_cfg->adcdrc_cfg & DAP_HP_EN))
			adcdrc_enable(component, 0);
		snd_soc_component_update_bits(component, SUNXI_ADC_DIG_CTRL,
			(0x1 << ADC2_CHANNEL_EN), (0x0 << ADC2_CHANNEL_EN));
		break;
	default:
		break;
	}

	return 0;
}
static int sunxi_codec_adc3_mixer_event(struct snd_soc_dapm_widget *w,
					struct snd_kcontrol *k, int event)
{
	struct snd_soc_component *component = snd_soc_dapm_to_component(w->dapm);
	struct sunxi_codec_info *sunxi_codec = snd_soc_component_get_drvdata(component);
	struct codec_hw_config *hw_cfg = &(sunxi_codec->hw_config);
	unsigned int adcctrl_val = 0;

	switch (event) {
	case SND_SOC_DAPM_POST_PMU:
		if (hw_cfg->adcdrc_cfg & DAP_HP_EN) {
			adcctrl_val = snd_soc_component_read32(component, SUNXI_ADC1_REG);
			if ((adcctrl_val >> MIC1_PGA_EN) & 0x1)
				adcdrc_enable(component, 1);
		} else if (hw_cfg->adcdrc_cfg & DAP_SPK_EN) {
			adcctrl_val = snd_soc_component_read32(component, SUNXI_ADC2_REG);
			if ((adcctrl_val >> MIC2_PGA_EN) & 0x1)
				adcdrc_enable(component, 1);
		}
		snd_soc_component_update_bits(component, SUNXI_ADC_DIG_CTRL,
			(0x1 << ADC3_CHANNEL_EN), (0x1 << ADC3_CHANNEL_EN));
		break;
	case SND_SOC_DAPM_POST_PMD:
		if ((hw_cfg->adcdrc_cfg & DAP_SPK_EN) ||
			(hw_cfg->adcdrc_cfg & DAP_HP_EN))
			adcdrc_enable(component, 0);
		snd_soc_component_update_bits(component, SUNXI_ADC_DIG_CTRL,
			(0x1 << ADC3_CHANNEL_EN), (0x0 << ADC3_CHANNEL_EN));
		break;
	default:
		break;
	}

	return 0;
}
*/

static const struct snd_kcontrol_new sunxi_codec_controls[] = {
#ifdef SUNXI_CODEC_HUB_ENABLE
	SOC_ENUM_EXT("codec hub mode", sunxi_codec_hub_mode_enum[0],
				sunxi_codec_get_hub_mode,
				sunxi_codec_set_hub_mode),
#endif
#ifdef SUNXI_CODEC_ADCSWAP_ENABLE
	SOC_ENUM_EXT("ADC1 ADC2 Swap", sunxi_codec_adcswap_func_enum[0],
		     sunxi_codec_get_adcswap1_mode,
		     sunxi_codec_set_adcswap1_mode),
	SOC_ENUM_EXT("ADC3 ADC4 swap", sunxi_codec_adcswap_func_enum[0],
		     sunxi_codec_get_adcswap2_mode,
		     sunxi_codec_set_adcswap2_mode),
#endif
	/* Digital Volume */
	SOC_SINGLE_TLV("digital volume", SUNXI_DAC_DPC,
		       DVOL, 0x3F, 1, digital_tlv),
	/* DAC Volume */
	SOC_DOUBLE_TLV("DAC volume", SUNXI_DAC_VOL_CTRL,
		       DAC_VOL_L, DAC_VOL_R, 0xFF, 0, dac_vol_tlv),
	/* ADC1 Volume */
	SOC_SINGLE_TLV("ADC1 volume", SUNXI_ADC_VOL_CTRL,
		       ADC1_VOL, 0xFF, 0, adc_vol_tlv),
	/* ADC2 Volume */
	SOC_SINGLE_TLV("ADC2 volume", SUNXI_ADC_VOL_CTRL,
		       ADC2_VOL, 0xFF, 0, adc_vol_tlv),
	/* ADC3 Volume */
	SOC_SINGLE_TLV("ADC3 volume", SUNXI_ADC_VOL_CTRL,
		       ADC3_VOL, 0xFF, 0, adc_vol_tlv),
	/* MIC1 Gain */
	SOC_SINGLE_TLV("MIC1 gain volume", SUNXI_ADC1_REG,
		       ADC1_PGA_GAIN_CTRL, 0x1F, 0, mic_gain_tlv),
	/* MIC2 Gain */
	SOC_SINGLE_TLV("MIC2 gain volume", SUNXI_ADC2_REG,
		       ADC2_PGA_GAIN_CTRL, 0x1F, 0, mic_gain_tlv),
	/* MIC3 Gain */
	SOC_SINGLE_TLV("MIC3 gain volume", SUNXI_ADC3_REG,
		       ADC3_PGA_GAIN_CTRL, 0x1F, 0, mic_gain_tlv),
	/* FMIN_L Gain */
	SOC_SINGLE_TLV("FMINL gain volume", SUNXI_ADC1_REG,
		       FMINLG, 0x1, 0, fmin_gain_tlv),
	/* FMIN_R Gain */
	SOC_SINGLE_TLV("FMINR gain volume", SUNXI_ADC2_REG,
		       FMINRG, 0x1, 0, fmin_gain_tlv),
	/* LINEIN_L Gain */
	SOC_SINGLE_TLV("LINEINL gain volume", SUNXI_ADC1_REG,
		       LINEINLG, 0x1, 0, linein_gain_tlv),
	/* LINEIN_R Gain */
	SOC_SINGLE_TLV("LINEINR gain volume", SUNXI_ADC2_REG,
		       LINEINRG, 0x1, 0, linein_gain_tlv),
	/* LINEOUT Volume */
	SOC_SINGLE_TLV("LINEOUT volume", SUNXI_DAC_REG,
		       LINEOUT_VOL, 0x1F, 0, lineout_tlv),
	/* Headphone Gain */
	SOC_SINGLE_TLV("Headphone Volume", SUNXI_HP2_REG,
		       HEADPHONE_GAIN, 0x7, 0, headphone_gain_tlv),
};

/* lineout controls */
static const char * const lineout_select_text[] = {
	"DAC_SINGLE", "DAC_DIFFER",
};
/* micin controls */
static const char * const micin_select_text[] = {
	"MIC_DIFFER", "MIC_SINGLE",
};

static const struct soc_enum left_lineout_enum =
	SOC_ENUM_SINGLE(SUNXI_DAC_REG, LINEOUTLDIFFEN,
			ARRAY_SIZE(lineout_select_text), lineout_select_text);
static const struct soc_enum right_lineout_enum =
	SOC_ENUM_SINGLE(SUNXI_DAC_REG, LINEOUTRDIFFEN,
			ARRAY_SIZE(lineout_select_text), lineout_select_text);

static const struct soc_enum mic1_input_enum =
	SOC_ENUM_SINGLE(SUNXI_ADC1_REG, MIC1_SIN_EN,
			ARRAY_SIZE(micin_select_text), micin_select_text);
static const struct soc_enum mic2_input_enum =
	SOC_ENUM_SINGLE(SUNXI_ADC2_REG, MIC2_SIN_EN,
			ARRAY_SIZE(micin_select_text), micin_select_text);
static const struct soc_enum mic3_input_enum =
	SOC_ENUM_SINGLE(SUNXI_ADC3_REG, MIC3_SIN_EN,
			ARRAY_SIZE(micin_select_text), micin_select_text);

static const struct snd_kcontrol_new left_lineout_mux =
	SOC_DAPM_ENUM("LINEOUT Output Select", left_lineout_enum);
static const struct snd_kcontrol_new right_lineout_mux =
	SOC_DAPM_ENUM("LINEOUTR Output Select", right_lineout_enum);

static const struct snd_kcontrol_new mic1_input_mux =
	SOC_DAPM_ENUM("MIC1 Input Select", mic1_input_enum);
static const struct snd_kcontrol_new mic2_input_mux =
	SOC_DAPM_ENUM("MIC2 Input Select", mic2_input_enum);
static const struct snd_kcontrol_new mic3_input_mux =
	SOC_DAPM_ENUM("MIC3 Input Select", mic3_input_enum);

static const struct snd_kcontrol_new adc1_input_mixer[] = {
	SOC_DAPM_SINGLE("MIC1 Boost Switch", SUNXI_ADC1_REG, MIC1_PGA_EN, 1, 0),
	SOC_DAPM_SINGLE("FMINL Switch", SUNXI_ADC1_REG, FMINLEN, 1, 0),
	SOC_DAPM_SINGLE("LINEINL Switch", SUNXI_ADC1_REG, LINEINLEN, 1, 0),
};

static const struct snd_kcontrol_new adc2_input_mixer[] = {
	SOC_DAPM_SINGLE("MIC2 Boost Switch", SUNXI_ADC2_REG, MIC2_PGA_EN, 1, 0),
	SOC_DAPM_SINGLE("FMINR Switch", SUNXI_ADC2_REG, FMINREN, 1, 0),
	SOC_DAPM_SINGLE("LINEINR Switch", SUNXI_ADC2_REG, LINEINREN, 1, 0),
};

static const struct snd_kcontrol_new adc3_input_mixer[] = {
	SOC_DAPM_SINGLE("MIC3 Boost Switch", SUNXI_ADC3_REG, MIC3_PGA_EN, 1, 0),
};

/*audio dapm widget */
static const struct snd_soc_dapm_widget sunxi_codec_dapm_widgets[] = {
	SND_SOC_DAPM_AIF_IN_E("DACL", "Playback", 0, SUNXI_DAC_REG,
				DACLEN, 0,
				sunxi_codec_playback_event,
				SND_SOC_DAPM_PRE_PMU | SND_SOC_DAPM_POST_PMD),

	SND_SOC_DAPM_AIF_IN_E("DACR", "Playback", 0, SUNXI_DAC_REG,
				DACREN, 0,
				sunxi_codec_playback_event,
				SND_SOC_DAPM_PRE_PMU | SND_SOC_DAPM_POST_PMD),

	SND_SOC_DAPM_AIF_OUT_E("ADC1", "Capture", 0, SUNXI_ADC1_REG,
				ADC1_EN, 0,
				sunxi_codec_capture_event,
				SND_SOC_DAPM_POST_PMU | SND_SOC_DAPM_POST_PMD),

	SND_SOC_DAPM_AIF_OUT_E("ADC2", "Capture", 0, SUNXI_ADC2_REG,
				ADC2_EN, 0,
				sunxi_codec_capture_event,
				SND_SOC_DAPM_POST_PMU | SND_SOC_DAPM_POST_PMD),

	SND_SOC_DAPM_AIF_OUT_E("ADC3", "Capture", 0, SUNXI_ADC3_REG,
				ADC3_EN, 0,
				sunxi_codec_capture_event,
				SND_SOC_DAPM_POST_PMU | SND_SOC_DAPM_POST_PMD),
	/*
	SND_SOC_DAPM_AIF_OUT_E("ADC1", "Capture", 0, SND_SOC_NOPM, 0, 0,
			       sunxi_codec_adc1_event,
			       SND_SOC_DAPM_POST_PMU | SND_SOC_DAPM_POST_PMD),
	SND_SOC_DAPM_AIF_OUT_E("ADC2", "Capture", 0, SND_SOC_NOPM, 0, 0,
			       sunxi_codec_adc2_event,
			       SND_SOC_DAPM_POST_PMU | SND_SOC_DAPM_POST_PMD),
	SND_SOC_DAPM_AIF_OUT_E("ADC3", "Capture", 0, SND_SOC_NOPM, 0, 0,
			       sunxi_codec_adc3_event,
			       SND_SOC_DAPM_POST_PMU | SND_SOC_DAPM_POST_PMD),
	*/

	SND_SOC_DAPM_MUX("LINEOUTL Output Select", SND_SOC_NOPM, 0, 0,
			 &left_lineout_mux),
	SND_SOC_DAPM_MUX("LINEOUTR Output Select", SND_SOC_NOPM, 0, 0,
			 &right_lineout_mux),

	SND_SOC_DAPM_MUX("MIC1 Input Select", SND_SOC_NOPM, 0, 0,
			 &mic1_input_mux),
	SND_SOC_DAPM_MUX("MIC2 Input Select", SND_SOC_NOPM, 0, 0,
			 &mic2_input_mux),
	SND_SOC_DAPM_MUX("MIC3 Input Select", SND_SOC_NOPM, 0, 0,
			 &mic3_input_mux),

	SND_SOC_DAPM_MIXER_E("ADC1 Input", SND_SOC_NOPM, 0, 0,
			     adc1_input_mixer, ARRAY_SIZE(adc1_input_mixer),
			     sunxi_codec_adc_mixer_event,
			     SND_SOC_DAPM_POST_PMU | SND_SOC_DAPM_POST_PMD),
	SND_SOC_DAPM_MIXER_E("ADC2 Input", SND_SOC_NOPM, 0, 0,
			     adc2_input_mixer, ARRAY_SIZE(adc2_input_mixer),
			     sunxi_codec_adc_mixer_event,
			     SND_SOC_DAPM_POST_PMU | SND_SOC_DAPM_POST_PMD),
	SND_SOC_DAPM_MIXER_E("ADC3 Input", SND_SOC_NOPM, 0, 0,
			     adc3_input_mixer, ARRAY_SIZE(adc3_input_mixer),
			     sunxi_codec_adc_mixer_event,
			     SND_SOC_DAPM_POST_PMU | SND_SOC_DAPM_POST_PMD),

	/*
	SND_SOC_DAPM_PGA("MIC1 PGA", SUNXI_ADC1_REG, MIC1_PGA_EN, 0, NULL, 0),
	SND_SOC_DAPM_PGA("MIC2 PGA", SUNXI_ADC2_REG, MIC2_PGA_EN, 0, NULL, 0),
	SND_SOC_DAPM_PGA("MIC3 PGA", SUNXI_ADC3_REG, MIC3_PGA_EN, 0, NULL, 0),
	*/

	SND_SOC_DAPM_MICBIAS("MainMic Bias", SUNXI_MICBIAS_REG, MMICBIASEN, 0),

	SND_SOC_DAPM_INPUT("MIC1"),
	SND_SOC_DAPM_INPUT("MIC2"),
	SND_SOC_DAPM_INPUT("MIC3"),
	SND_SOC_DAPM_INPUT("FMINL"),
	SND_SOC_DAPM_INPUT("FMINR"),
	SND_SOC_DAPM_INPUT("LINEINL"),
	SND_SOC_DAPM_INPUT("LINEINR"),

	SND_SOC_DAPM_OUTPUT("HPOUTL"),
	SND_SOC_DAPM_OUTPUT("HPOUTR"),
	SND_SOC_DAPM_OUTPUT("LINEOUTL"),
	SND_SOC_DAPM_OUTPUT("LINEOUTR"),

	SND_SOC_DAPM_HP("Headphone", sunxi_codec_headphone_event),
	SND_SOC_DAPM_LINE("LINEOUT", sunxi_codec_lineout_event),
	SND_SOC_DAPM_SPK("HpSpeaker", sunxi_codec_hpspeaker_event),
};

static const struct snd_soc_dapm_route sunxi_codec_dapm_routes[] = {
	/* input route */
	{"MIC1 Input Select", "MIC_SINGLE", "MIC1"},
	{"MIC1 Input Select", "MIC_DIFFER", "MIC1"},
	{"MIC2 Input Select", "MIC_SINGLE", "MIC2"},
	{"MIC2 Input Select", "MIC_DIFFER", "MIC2"},
	{"MIC3 Input Select", "MIC_SINGLE", "MIC3"},
	{"MIC3 Input Select", "MIC_DIFFER", "MIC3"},

	{"ADC1 Input", "MIC1 Boost Switch", "MIC1 Input Select"},
	{"ADC1 Input", "FMINL Switch", "FMINL"},
	{"ADC1 Input", "LINEINL Switch", "LINEINL"},

	{"ADC2 Input", "MIC2 Boost Switch", "MIC2 Input Select"},
	{"ADC2 Input", "FMINR Switch", "FMINR"},
	{"ADC2 Input", "LINEINR Switch", "LINEINR"},

	{"ADC3 Input", "MIC3 Boost Switch", "MIC3 Input Select"},

	{"ADC1", NULL, "ADC1 Input"},
	{"ADC2", NULL, "ADC2 Input"},
	{"ADC3", NULL, "ADC3 Input"},

	/* Output route */
	{"LINEOUTL Output Select", "DAC_SINGLE", "DACL"},
	{"LINEOUTL Output Select", "DAC_DIFFER", "DACL"},

	{"LINEOUTR Output Select", "DAC_SINGLE", "DACR"},
	{"LINEOUTR Output Select", "DAC_DIFFER", "DACR"},

	{"HPOUTL", NULL, "DACL"},
	{"HPOUTR", NULL, "DACR"},

	{"LINEOUTL", NULL, "LINEOUTL Output Select"},
	{"LINEOUTR", NULL, "LINEOUTR Output Select"},

	{"LINEOUT", NULL, "LINEOUTL"},
	{"LINEOUT", NULL, "LINEOUTR"},

	{"Headphone", NULL, "HPOUTL"},
	{"Headphone", NULL, "HPOUTR"},

	{"HpSpeaker", NULL, "HPOUTR"},
	{"HpSpeaker", NULL, "HPOUTL"},
};

static void sunxi_codec_init(struct snd_soc_component *component)
{
	struct sunxi_codec_info *sunxi_codec = snd_soc_component_get_drvdata(component);

	/* In order to ensure that the ADC sampling is normal,
	 * the A chip SOC needs to always open HPLDO and RMC_EN
	 */
	if (!(sunxi_get_soc_ver() & 0x7)) {
		snd_soc_component_update_bits(component, SUNXI_POWER_REG,
				(0x1 << HPLDO_EN), (0x1 << HPLDO_EN));
		snd_soc_component_update_bits(component, SUNXI_RAMP_REG,
				(0x1 << RMC_EN), (0x1 << RMC_EN));
	}

	/* DAC_VOL_SEL default disabled */
	snd_soc_component_update_bits(component, SUNXI_DAC_VOL_CTRL,
			(0x1 << DAC_VOL_SEL), (0x1 << DAC_VOL_SEL));

	/* Enable ADCFDT to overcome niose at the beginning */
	snd_soc_component_update_bits(component, SUNXI_ADC_FIFOC,
			(0x3 << ADCDFEN), (0x3 << ADCDFEN));

	/* Digital VOL defeult setting */
	snd_soc_component_update_bits(component, SUNXI_DAC_DPC,
			0x3F << DVOL,
			sunxi_codec->digital_vol << DVOL);

	/* LINEOUT VOL defeult setting */
	snd_soc_component_update_bits(component, SUNXI_DAC_REG,
			0x1F << LINEOUT_VOL,
			sunxi_codec->lineout_vol << LINEOUT_VOL);

	/* Headphone Gain defeult setting */
	snd_soc_component_update_bits(component, SUNXI_HP2_REG,
			0x7 << HEADPHONE_GAIN,
			sunxi_codec->headphonegain << HEADPHONE_GAIN);

	/* ADCL MIC1 gain defeult setting */
	snd_soc_component_update_bits(component, SUNXI_ADC1_REG,
			0x1F << ADC1_PGA_GAIN_CTRL,
			sunxi_codec->mic1gain << ADC1_PGA_GAIN_CTRL);
	/* ADCR MIC2 gain defeult setting */
	snd_soc_component_update_bits(component, SUNXI_ADC2_REG,
			0x1F << ADC2_PGA_GAIN_CTRL,
			sunxi_codec->mic2gain << ADC2_PGA_GAIN_CTRL);
	/* ADCR MIC3 gain defeult setting */
	snd_soc_component_update_bits(component, SUNXI_ADC3_REG,
			0x1F << ADC3_PGA_GAIN_CTRL,
			sunxi_codec->mic3gain << ADC3_PGA_GAIN_CTRL);

	/* ADC IOP params default setting */
	snd_soc_component_update_bits(component, SUNXI_ADC1_REG,
			0xFF << ADC1_IOPMIC, 0x55 << ADC1_IOPMIC);
	snd_soc_component_update_bits(component, SUNXI_ADC2_REG,
			0xFF << ADC2_IOPMIC, 0x55 << ADC2_IOPMIC);
	snd_soc_component_update_bits(component, SUNXI_ADC3_REG,
			0xFF << ADC3_IOPMIC, 0x55 << ADC3_IOPMIC);

	snd_soc_component_update_bits(component, SUNXI_DAC_REG,
			0x01 << LINEOUTLDIFFEN, 0x01 << LINEOUTLDIFFEN);
	snd_soc_component_update_bits(component, SUNXI_DAC_REG,
			0x01 << LINEOUTRDIFFEN, 0x01 << LINEOUTRDIFFEN);
	snd_soc_component_update_bits(component, SUNXI_ADC1_REG,
			0x01 << MIC1_SIN_EN, 0x00 << MIC1_SIN_EN);
	snd_soc_component_update_bits(component, SUNXI_ADC2_REG,
			0x01 << MIC2_SIN_EN, 0x00 << MIC2_SIN_EN);
	snd_soc_component_update_bits(component, SUNXI_ADC3_REG,
			0x01 << MIC3_SIN_EN, 0x00 << MIC3_SIN_EN);

#ifdef SUNXI_CODEC_DAP_ENABLE
	if (sunxi_codec->hw_config.adcdrc_cfg)
		adcdrc_config(component);
	if (sunxi_codec->hw_config.adchpf_cfg)
		adchpf_config(component);

	if (sunxi_codec->hw_config.dacdrc_cfg)
		dacdrc_config(component);
	if (sunxi_codec->hw_config.dachpf_cfg)
		dachpf_config(component);
#endif
}

static int sunxi_codec_startup(struct snd_pcm_substream *substream,
				struct snd_soc_dai *dai)
{
#ifdef SUNXI_CODEC_DAP_ENABLE
	struct snd_soc_component *component = dai->component;
	struct sunxi_codec_info *sunxi_codec = snd_soc_component_get_drvdata(component);

	if (substream->stream == SNDRV_PCM_STREAM_CAPTURE) {
		if (sunxi_codec->hw_config.adchpf_cfg)
			adchpf_enable(component, 1);
	}
#endif

	return 0;
}

static int sunxi_codec_hw_params(struct snd_pcm_substream *substream,
		struct snd_pcm_hw_params *params, struct snd_soc_dai *dai)
{
	struct snd_soc_component *component = dai->component;
	struct sunxi_codec_info *sunxi_codec = snd_soc_component_get_drvdata(component);
	int i = 0;

	switch (params_format(params)) {
	case	SNDRV_PCM_FORMAT_S16_LE:
		if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK) {
			snd_soc_component_update_bits(component, SUNXI_DAC_FIFOC,
				(3 << FIFO_MODE), (3 << FIFO_MODE));
			snd_soc_component_update_bits(component, SUNXI_DAC_FIFOC,
				(1 << TX_SAMPLE_BITS), (0 << TX_SAMPLE_BITS));
		} else {
			snd_soc_component_update_bits(component, SUNXI_ADC_FIFOC,
				(1 << RX_FIFO_MODE), (1 << RX_FIFO_MODE));
			snd_soc_component_update_bits(component, SUNXI_ADC_FIFOC,
				(1 << RX_SAMPLE_BITS), (0 << RX_SAMPLE_BITS));
		}
		break;
	case	SNDRV_PCM_FORMAT_S24_LE:
		if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK) {
			snd_soc_component_update_bits(component, SUNXI_DAC_FIFOC,
				(3 << FIFO_MODE), (0 << FIFO_MODE));
			snd_soc_component_update_bits(component, SUNXI_DAC_FIFOC,
				(1 << TX_SAMPLE_BITS), (1 << TX_SAMPLE_BITS));
		} else {
			snd_soc_component_update_bits(component, SUNXI_ADC_FIFOC,
				(1 << RX_FIFO_MODE), (0 << RX_FIFO_MODE));
			snd_soc_component_update_bits(component, SUNXI_ADC_FIFOC,
				(1 << RX_SAMPLE_BITS), (1 << RX_SAMPLE_BITS));
		}
		break;
	default:
		break;
	}

	for (i = 0; i < ARRAY_SIZE(sample_rate_conv); i++) {
		if (sample_rate_conv[i].samplerate == params_rate(params)) {
			if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK) {
				snd_soc_component_update_bits(component, SUNXI_DAC_FIFOC,
					(0x7 << DAC_FS),
					(sample_rate_conv[i].rate_bit << DAC_FS));
			} else {
				if (sample_rate_conv[i].samplerate > 48000)
					return -EINVAL;
				snd_soc_component_update_bits(component, SUNXI_ADC_FIFOC,
					(0x7 << ADC_FS),
					(sample_rate_conv[i].rate_bit<<ADC_FS));
			}
		}
	}

	/* reset the adchpf func setting for different sampling */
	if (substream->stream == SNDRV_PCM_STREAM_CAPTURE) {
		if (sunxi_codec->hw_config.adchpf_cfg) {
			if (params_rate(params) == 16000) {
				snd_soc_component_write(component, SUNXI_ADC_DRC_HHPFC,
						(0x00F623A5 >> 16) & 0xFFFF);

				snd_soc_component_write(component, SUNXI_ADC_DRC_LHPFC,
							0x00F623A5 & 0xFFFF);

			} else if (params_rate(params) == 44100) {
				snd_soc_component_write(component, SUNXI_ADC_DRC_HHPFC,
						(0x00FC60DB >> 16) & 0xFFFF);

				snd_soc_component_write(component, SUNXI_ADC_DRC_LHPFC,
							0x00FC60DB & 0xFFFF);
			} else {
				snd_soc_component_write(component, SUNXI_ADC_DRC_HHPFC,
						(0x00FCABB3 >> 16) & 0xFFFF);

				snd_soc_component_write(component, SUNXI_ADC_DRC_LHPFC,
							0x00FCABB3 & 0xFFFF);
			}
		}
	}

	if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK) {
		switch (params_channels(params)) {
		case 1:
			/* DACL & DACR send same data */
			snd_soc_component_update_bits(component, SUNXI_DAC_FIFOC,
				(0x1 << DAC_MONO_EN), 0x1 << DAC_MONO_EN);
			break;
		case 2:
			snd_soc_component_update_bits(component, SUNXI_DAC_FIFOC,
				(0x1 << DAC_MONO_EN), 0x0 << DAC_MONO_EN);
			break;
		default:
			LOG_WARN("not support channels:%u", params_channels(params));
			return -EINVAL;
		}
	} else {
		switch (params_channels(params)) {
		case 1:
			snd_soc_component_update_bits(component, SUNXI_ADC_DIG_CTRL,
					(0x7<<ADC_CHANNEL_EN), (1<<ADC_CHANNEL_EN));
			break;
		case 2:
			snd_soc_component_update_bits(component, SUNXI_ADC_DIG_CTRL,
					(0x7<<ADC_CHANNEL_EN), (3<<ADC_CHANNEL_EN));
			break;
		case 3:
			snd_soc_component_update_bits(component, SUNXI_ADC_DIG_CTRL,
					(0x7<<ADC_CHANNEL_EN), (7<<ADC_CHANNEL_EN));
			break;
		default:
			LOG_WARN("not support channels:%u", params_channels(params));
			return -EINVAL;
		}
	}

	return 0;
}

static int sunxi_codec_set_sysclk(struct snd_soc_dai *dai,
			int clk_id, unsigned int freq, int dir)
{
	struct snd_soc_component *component = dai->component;
	struct sunxi_codec_info *sunxi_codec = snd_soc_component_get_drvdata(component);

	switch (clk_id) {
	case	0:
		/* set clk source [22.5792MHz * n] to surpport playback */
		if (clk_set_parent(sunxi_codec->dacclk, sunxi_codec->pllaudio0)) {
			LOG_ERR("set parent of dacclk to pllaudio0 failed");
			return -EINVAL;
		}

		if (clk_set_rate(sunxi_codec->dacclk, freq)) {
			LOG_ERR("codec set dac clk rate failed");
			return -EINVAL;
		}
		break;
	case	1:
		/* set clk source [22.5792MHz * n] to surpport capture */
		if (clk_set_parent(sunxi_codec->adcclk, sunxi_codec->pllaudio0)) {
			LOG_ERR("set parent of adcclk to pllaudio0 failed");
			return -EINVAL;
		}

		if (clk_set_rate(sunxi_codec->adcclk, freq)) {
			LOG_ERR("codec set adc clk rate failed");
			return -EINVAL;
		}
		break;
	case	2:
		/* set clk source [24.576MHz * n] to surpport playback */
		if (clk_set_parent(sunxi_codec->dacclk, sunxi_codec->pllaudio1_div5)) {
			LOG_ERR("set parent of dacclk to pllaudio1_div5 failed");
			return -EINVAL;
		}

		if (clk_set_rate(sunxi_codec->dacclk, freq)) {
			LOG_ERR("codec set dac clk rate failed");
			return -EINVAL;
		}
		break;
	case	3:
		/* set clk source [24.576MHz * n] to surpport capture */
		if (clk_set_parent(sunxi_codec->adcclk, sunxi_codec->pllaudio1_div5)) {
			LOG_ERR("set parent of adcclk to pllaudio1_div5 failed");
			return -EINVAL;
		}

		if (clk_set_rate(sunxi_codec->adcclk, freq)) {
			LOG_ERR("codec set adc clk rate failed");
			return -EINVAL;
		}
		break;
	default:
		LOG_ERR("Bad clk params input!");
		return -EINVAL;
	}

	return 0;
}

static void sunxi_codec_shutdown(struct snd_pcm_substream *substream,
				struct snd_soc_dai *dai)
{
#ifdef SUNXI_CODEC_DAP_ENABLE
	struct snd_soc_component *component = dai->component;
	struct sunxi_codec_info *sunxi_codec = snd_soc_component_get_drvdata(component);

	if (substream->stream == SNDRV_PCM_STREAM_CAPTURE) {
		if (sunxi_codec->hw_config.adchpf_cfg)
			adchpf_enable(component, 0);
	}
#endif
}

static int sunxi_codec_prepare(struct snd_pcm_substream *substream,
				struct snd_soc_dai *dai)
{
	struct snd_soc_component *component = dai->component;

	if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK) {
		snd_soc_component_update_bits(component, SUNXI_DAC_FIFOC,
			(1 << FIFO_FLUSH), (1 << FIFO_FLUSH));
		snd_soc_component_write(component, SUNXI_DAC_FIFOS,
			(1 << DAC_TXE_INT | 1 << DAC_TXU_INT | 1 << DAC_TXO_INT));
		snd_soc_component_write(component, SUNXI_DAC_CNT, 0);
	} else {
		snd_soc_component_update_bits(component, SUNXI_ADC_FIFOC,
				(1 << ADC_FIFO_FLUSH), (1 << ADC_FIFO_FLUSH));
		snd_soc_component_write(component, SUNXI_ADC_FIFOS,
				(1 << ADC_RXA_INT | 1 << ADC_RXO_INT));
		snd_soc_component_write(component, SUNXI_ADC_CNT, 0);
	}

	return 0;
}

static int sunxi_codec_trigger(struct snd_pcm_substream *substream,
				int cmd, struct snd_soc_dai *dai)
{
	struct sunxi_codec_info *sunxi_codec = snd_soc_component_get_drvdata(dai->component);

	switch (cmd) {
	case SNDRV_PCM_TRIGGER_START:
	case SNDRV_PCM_TRIGGER_RESUME:
	case SNDRV_PCM_TRIGGER_PAUSE_RELEASE:
		if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK)
			regmap_update_bits(sunxi_codec->regmap, SUNXI_DAC_FIFOC,
				(1 << DAC_DRQ_EN), (1 << DAC_DRQ_EN));
		else
			regmap_update_bits(sunxi_codec->regmap, SUNXI_ADC_FIFOC,
				(1 << ADC_DRQ_EN), (1 << ADC_DRQ_EN));
		break;
	case SNDRV_PCM_TRIGGER_STOP:
	case SNDRV_PCM_TRIGGER_SUSPEND:
	case SNDRV_PCM_TRIGGER_PAUSE_PUSH:
		if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK)
			regmap_update_bits(sunxi_codec->regmap, SUNXI_DAC_FIFOC,
				(1 << DAC_DRQ_EN), (0 << DAC_DRQ_EN));
		else
			regmap_update_bits(sunxi_codec->regmap, SUNXI_ADC_FIFOC,
				(1 << ADC_DRQ_EN), (0 << ADC_DRQ_EN));
		break;
	default:
		return -EINVAL;
	}

	return 0;
}

static const struct snd_soc_dai_ops sunxi_codec_dai_ops = {
	.startup	= sunxi_codec_startup,
	.hw_params	= sunxi_codec_hw_params,
	.shutdown	= sunxi_codec_shutdown,
	.set_sysclk	= sunxi_codec_set_sysclk,
	.trigger	= sunxi_codec_trigger,
	.prepare	= sunxi_codec_prepare,
};

static struct snd_soc_dai_driver sunxi_codec_dai[] = {
	{
		.name	= "sun8iw20codec",
		.playback = {
			.stream_name = "Playback",
			.channels_min = 1,
			.channels_max = 2,
			.rates	= SNDRV_PCM_RATE_8000_192000
				| SNDRV_PCM_RATE_KNOT,
			.formats = SNDRV_PCM_FMTBIT_S16_LE
				| SNDRV_PCM_FMTBIT_S24_LE,
		},
		.capture = {
			.stream_name = "Capture",
			.channels_min = 1,
			.channels_max = 3,
			.rates = SNDRV_PCM_RATE_8000_48000
				| SNDRV_PCM_RATE_KNOT,
			.formats = SNDRV_PCM_FMTBIT_S16_LE
				| SNDRV_PCM_FMTBIT_S24_LE,
		},
		.ops = &sunxi_codec_dai_ops,
	},
};

static int sunxi_codec_probe(struct snd_soc_component *component)
{
	int ret;
	struct snd_soc_dapm_context *dapm = &component->dapm;

	ret = snd_soc_add_component_controls(component, sunxi_codec_controls,
					ARRAY_SIZE(sunxi_codec_controls));
	if (ret)
		LOG_ERR("failed to register codec controls");

	snd_soc_dapm_new_controls(dapm, sunxi_codec_dapm_widgets,
				ARRAY_SIZE(sunxi_codec_dapm_widgets));
	snd_soc_dapm_add_routes(dapm, sunxi_codec_dapm_routes,
				ARRAY_SIZE(sunxi_codec_dapm_routes));

	sunxi_codec_init(component);

	return 0;
}

static void sunxi_codec_remove(struct snd_soc_component *component)
{

}

static int save_audio_reg(struct sunxi_codec_info *sunxi_codec)
{
	int i = 0;

	while (reg_labels[i].name != NULL) {
		regmap_read(sunxi_codec->regmap, reg_labels[i].address,
			&reg_labels[i].value);
		i++;
	}

	return i;
}

static int echo_audio_reg(struct sunxi_codec_info *sunxi_codec)
{
	int i = 0;

	while (reg_labels[i].name != NULL) {
		regmap_write(sunxi_codec->regmap, reg_labels[i].address,
					reg_labels[i].value);
		i++;
	}

	return i;
}

static int sunxi_codec_suspend(struct snd_soc_component *component)
{
	struct sunxi_codec_info *sunxi_codec = snd_soc_component_get_drvdata(component);
	struct codec_spk_config *spk_cfg = &(sunxi_codec->spk_config);

	pr_debug("Enter %s\n", __func__);
	save_audio_reg(sunxi_codec);

	if (spk_cfg->used)
		gpio_set_value(spk_cfg->spk_gpio, !(spk_cfg->pa_level));

	if (sunxi_codec->vol_supply.avcc)
		regulator_disable(sunxi_codec->vol_supply.avcc);

	if (sunxi_codec->vol_supply.hpvcc)
		regulator_disable(sunxi_codec->vol_supply.hpvcc);

	clk_disable_unprepare(sunxi_codec->dacclk);
	clk_disable_unprepare(sunxi_codec->adcclk);
	clk_disable_unprepare(sunxi_codec->pllaudio0);
	clk_disable_unprepare(sunxi_codec->pllaudio1_div5);
	clk_disable_unprepare(sunxi_codec->codec_clk_bus);
	reset_control_assert(sunxi_codec->codec_clk_rst);

	pr_debug("End %s\n", __func__);

	return 0;
}

static int sunxi_codec_resume(struct snd_soc_component *component)
{
	struct sunxi_codec_info *sunxi_codec = snd_soc_component_get_drvdata(component);
	struct codec_spk_config *spk_cfg = &(sunxi_codec->spk_config);
	unsigned int ret;

	pr_debug("Enter %s\n", __func__);

	if (sunxi_codec->vol_supply.avcc) {
		ret = regulator_enable(sunxi_codec->vol_supply.avcc);
		if (ret)
			LOG_ERR("resume avcc enable failed");
	}

	if (sunxi_codec->vol_supply.hpvcc) {
		ret = regulator_enable(sunxi_codec->vol_supply.hpvcc);
		if (ret)
			LOG_ERR("resume hpvcc enable failed");
	}

	/* 22579200 * n */
	if (clk_set_rate(sunxi_codec->pllaudio0, 22579200)) {
		LOG_ERR("resume codec source set pllaudio0 rate failed");
		return -EBUSY;
	}

	/* 24576000 * 25 */
	if (clk_set_rate(sunxi_codec->pllaudio1_div5, 614400000)) {
		LOG_ERR("resume codec source set pllaudio1_div5 rate failed");
		return -EBUSY;
	}

	if (reset_control_deassert(sunxi_codec->codec_clk_rst)) {
		LOG_ERR("resume deassert the codec reset failed");
		return -EBUSY;
	}

	if (clk_prepare_enable(sunxi_codec->codec_clk_bus)) {
		LOG_ERR("enable codec bus clk failed, resume exit");
		return -EBUSY;
	}

	if (clk_prepare_enable(sunxi_codec->pllaudio0)) {
		LOG_ERR("enable pllaudio0 failed, resume exit");
		return -EBUSY;
	}

	if (clk_prepare_enable(sunxi_codec->pllaudio1_div5)) {
		LOG_ERR("enable pllaudio1_div5 failed, resume exit");
		return -EBUSY;
	}

	if (clk_prepare_enable(sunxi_codec->dacclk)) {
		LOG_ERR("enable dacclk failed, resume exit");
		return -EBUSY;
	}

	if (clk_prepare_enable(sunxi_codec->adcclk)) {
		LOG_ERR("enable  adcclk failed, resume exit");
		clk_disable_unprepare(sunxi_codec->adcclk);
		return -EBUSY;
	}

	if (spk_cfg->used) {
		gpio_direction_output(spk_cfg->spk_gpio, 1);
		gpio_set_value(spk_cfg->spk_gpio, !(spk_cfg->pa_level));
	}

	sunxi_codec_init(component);
	echo_audio_reg(sunxi_codec);

	pr_debug("End %s\n", __func__);

	return 0;
}

static unsigned int sunxi_codec_read(struct snd_soc_component *component,
					unsigned int reg)
{
	struct sunxi_codec_info *sunxi_codec = snd_soc_component_get_drvdata(component);
	unsigned int reg_val;

	regmap_read(sunxi_codec->regmap, reg, &reg_val);

	return reg_val;
}

static int sunxi_codec_write(struct snd_soc_component *component,
				unsigned int reg, unsigned int val)
{
	struct sunxi_codec_info *sunxi_codec = snd_soc_component_get_drvdata(component);

	regmap_write(sunxi_codec->regmap, reg, val);

	return 0;
};

static struct snd_soc_component_driver soc_codec_dev_sunxi = {
	.probe = sunxi_codec_probe,
	.remove = sunxi_codec_remove,
	.suspend = sunxi_codec_suspend,
	.resume = sunxi_codec_resume,
	.read = sunxi_codec_read,
	.write = sunxi_codec_write,
};

/* audiocodec reg dump about */
static ssize_t show_audio_reg(struct device *dev,
			struct device_attribute *attr, char *buf)
{
	struct sunxi_codec_info *sunxi_codec = dev_get_drvdata(dev);
	int cnt = 0, i = 0, j = 0;
	unsigned int reg_val, reg_val_tmp;
	unsigned int size = ARRAY_SIZE(reg_labels);
	char *reg_name_play = "REG NAME";
	char *reg_offset_play = "OFFSET";
	char *reg_val_play = "VALUE";

	cnt += sprintf(buf + cnt,
		       "%-30s|%-6s|%-10s"
		       "|31-28|27-24|23-20|19-16|15-12|11-08|07-04|03-00|"
		       "save_value\n",
		       reg_name_play, reg_offset_play, reg_val_play);

	while ((i < size) && (reg_labels[i].name != NULL)) {
		regmap_read(sunxi_codec->regmap,
			    reg_labels[i].address, &reg_val);

		cnt += sprintf(buf + cnt,
			       "%-30s|0x%4x|0x%8x",
			       reg_labels[i].name,
			       reg_labels[i].address,
			       reg_val);
		for (j = 7; j >= 0; j--) {
			reg_val_tmp = reg_val >> (j * 4);
			cnt += sprintf(buf + cnt,
				       "|%c%c%c%c ",
				       (((reg_val_tmp) & 0x08ull) ? '1' : '0'),
				       (((reg_val_tmp) & 0x04ull) ? '1' : '0'),
				       (((reg_val_tmp) & 0x02ull) ? '1' : '0'),
				       (((reg_val_tmp) & 0x01ull) ? '1' : '0')
				       );
		}
		cnt += sprintf(buf + cnt, "|0x%8x\n", reg_labels[i].value);

		i++;
	}

	return cnt;
}

/* ex:
 * param 1: 0 read;1 write
 * param 2: reg value;
 * param 3: write value;
	read:
		echo 0,0x00> audio_reg
	write:
		echo 1,0x00,0xa > audio_reg
*/
static ssize_t store_audio_reg(struct device *dev, struct device_attribute *attr,
				const char *buf, size_t count)
{
	int ret;
	int rw_flag;
	int input_reg_val = 0;
	int input_reg_offset = 0;
	struct sunxi_codec_info *sunxi_codec = dev_get_drvdata(dev);

	ret = sscanf(buf, "%d,0x%x,0x%x", &rw_flag,
			&input_reg_offset, &input_reg_val);
	LOG_INFO("ret:%d, reg_offset:%d, reg_val:0x%x",
		 ret, input_reg_offset, input_reg_val);

	if (!(rw_flag == 1 || rw_flag == 0)) {
		LOG_ERR("not rw_flag");
		ret = count;
		goto out;
	}

	if (input_reg_offset > SUNXI_BIAS_REG) {
		LOG_ERR("the reg offset[0x%03x] > SUNXI_BIAS_REG[0x%03x]",
			input_reg_offset, SUNXI_BIAS_REG);
		ret = count;
		goto out;
	}

	if (rw_flag) {
		regmap_write(sunxi_codec->regmap,
				input_reg_offset, input_reg_val);
	} else {
		regmap_read(sunxi_codec->regmap,
				input_reg_offset, &input_reg_val);
		LOG_INFO("\n\n Reg[0x%x] : 0x%08x\n\n",
				input_reg_offset, input_reg_val);
	}
	ret = count;
out:
	return ret;
}

static DEVICE_ATTR(audio_reg, 0644, show_audio_reg, store_audio_reg);

static struct attribute *audio_debug_attrs[] = {
	&dev_attr_audio_reg.attr,
	NULL,
};

static struct attribute_group audio_debug_attr_group = {
	.name   = "audio_reg_debug",
	.attrs  = audio_debug_attrs,
};

/* regmap configuration */
static const struct regmap_config sunxi_codec_regmap_config = {
	.reg_bits = 32,
	.reg_stride = 4,
	.val_bits = 32,
	.max_register = SUNXI_ADC_CUR_REG,
	.cache_type = REGCACHE_NONE,
};
static const struct snd_pcm_hardware snd_rockchip_hardware = {
	.info			= SNDRV_PCM_INFO_MMAP |
				  SNDRV_PCM_INFO_MMAP_VALID |
				  SNDRV_PCM_INFO_PAUSE |
				  SNDRV_PCM_INFO_RESUME |
				  SNDRV_PCM_INFO_INTERLEAVED,
	.period_bytes_min	= 32,
	.period_bytes_max	= 8192,
	.periods_min		= 1,
	.periods_max		= 52,
	.buffer_bytes_max	= 64 * 1024,
	.fifo_size		= 32,
};

/*
static int sunxi_codec_regulator_init(struct platform_device *pdev,
				struct sunxi_codec_info *sunxi_codec)
{
	int ret = -EFAULT;

	sunxi_codec->vol_supply.avcc = regulator_get(&pdev->dev, "avcc");
	if (IS_ERR(sunxi_codec->vol_supply.avcc)) {
		LOG_ERR("get audio avcc failed");
		goto err_regulator;
	} else {
		ret = regulator_set_voltage(sunxi_codec->vol_supply.avcc,
							1800000, 1800000);
		if (ret) {
			LOG_ERR("avcc set vol failed");
			goto err_regulator_avcc;
		}

		ret = regulator_enable(sunxi_codec->vol_supply.avcc);
		if (ret) {
			LOG_ERR("avcc enable failed");
			goto err_regulator_avcc;
		}
	}

	sunxi_codec->vol_supply.hpvcc = regulator_get(&pdev->dev, "hpvcc");
	if (IS_ERR(sunxi_codec->vol_supply.hpvcc)) {
		LOG_ERR("get audio hpvcc failed");
		goto err_regulator_avcc;
	} else {
		ret = regulator_set_voltage(sunxi_codec->vol_supply.hpvcc,
							1800000, 1800000);
		if (ret) {
			LOG_ERR("hpvcc set vol failed");
			goto err_regulator_hpvcc;
		}

		ret = regulator_enable(sunxi_codec->vol_supply.hpvcc);
		if (ret) {
			LOG_ERR("hpvcc enable failed");
			goto err_regulator_hpvcc;
		}
	}
	return 0;

err_regulator_hpvcc:
	regulator_put(sunxi_codec->vol_supply.hpvcc);
err_regulator_avcc:
	regulator_put(sunxi_codec->vol_supply.avcc);
err_regulator:
	return ret;
}
*/

static int sunxi_codec_clk_init(struct device_node *np,
				struct platform_device *pdev,
				struct sunxi_codec_info *sunxi_codec)
{
	int ret = -EBUSY;
	/* get the parent clk and the module clk */
	sunxi_codec->dacclk = of_clk_get_by_name(np, "audio_clk_dac");
	if (IS_ERR(sunxi_codec->dacclk))
		LOG_ERR("audio_clk_dac clk get failed");
	sunxi_codec->adcclk = of_clk_get_by_name(np, "audio_clk_adc");
	if (IS_ERR(sunxi_codec->adcclk))
		LOG_ERR("audio_clk_adc clk get failed");
	sunxi_codec->pllaudio0 = of_clk_get_by_name(np, "pll_audio0");
	if (IS_ERR(sunxi_codec->pllaudio0))
		LOG_ERR("pll_audio0 clk get failed");
	sunxi_codec->pllaudio1_div5 = of_clk_get_by_name(np, "pll_audio1_div5");
	if (IS_ERR(sunxi_codec->pllaudio1_div5))
		LOG_ERR("pll_audio1_div5 clk get failed");
	sunxi_codec->codec_clk_bus = of_clk_get_by_name(np, "audio_clk_bus");
	if (IS_ERR(sunxi_codec->codec_clk_bus))
		LOG_ERR("audio_clk_bus clk get failed");
	sunxi_codec->codec_clk_rst = devm_reset_control_get(&pdev->dev, NULL);

	if (reset_control_deassert(sunxi_codec->codec_clk_rst)) {
		LOG_ERR("reset clk deassert failed");
		goto err_devm_kfree;
	}

	if (clk_set_parent(sunxi_codec->dacclk, sunxi_codec->pllaudio0)) {
		LOG_ERR("set parent of dacclk to pllaudio0 failed");
		goto err_devm_kfree;
	}

	if (clk_set_parent(sunxi_codec->adcclk, sunxi_codec->pllaudio0)) {
		LOG_ERR("set parent of adcclk to pllaudio0 failed");
		goto err_devm_kfree;
	}

	/* 22579200 * n */
	if (clk_set_rate(sunxi_codec->pllaudio0, 22579200)) {
		LOG_ERR("pllaudio0 set rate failed");
		goto err_devm_kfree;
	}

	/* 24576000 * n */
	if (clk_set_rate(sunxi_codec->pllaudio1_div5, 614400000)) {
		LOG_ERR("pllaudio1_div5 set rate failed");
		goto err_devm_kfree;
	}

	if (clk_prepare_enable(sunxi_codec->codec_clk_bus)) {
		LOG_ERR("codec clk bus enable failed");
	}

	if (clk_prepare_enable(sunxi_codec->pllaudio0)) {
		LOG_ERR("pllaudio0 enable failed");
		goto err_bus_kfree;
	}

	if (clk_prepare_enable(sunxi_codec->pllaudio1_div5)) {
		LOG_ERR("pllaudio1_div5 enable failed");
		goto err_pllaudio0_kfree;
	}

	if (clk_prepare_enable(sunxi_codec->dacclk)) {
		LOG_ERR("dacclk enable failed");
		goto err_pllaudio1_div5_kfree;
	}

	if (clk_prepare_enable(sunxi_codec->adcclk)) {
		LOG_ERR("adcclk enable failed");
		goto err_dacclk_kfree;
	}
	return 0;

err_dacclk_kfree:
	clk_disable_unprepare(sunxi_codec->dacclk);
err_pllaudio1_div5_kfree:
	clk_disable_unprepare(sunxi_codec->pllaudio1_div5);
err_pllaudio0_kfree:
	clk_disable_unprepare(sunxi_codec->pllaudio0);
err_bus_kfree:
	clk_disable_unprepare(sunxi_codec->codec_clk_bus);
err_devm_kfree:
	return ret;
}

static int sunxi_codec_parse_params(struct device_node *np,
				struct platform_device *pdev,
				struct sunxi_codec_info *sunxi_codec)
{
	int ret = 0;
	unsigned int temp_val;

	/* get the special property form the board.dts */
	ret = of_property_read_u32(np, "digital_vol", &temp_val);
	if (ret < 0) {
		LOG_WARN("digital volume get failed, use default vol");
		sunxi_codec->digital_vol = 0;
	} else {
		sunxi_codec->digital_vol = temp_val;
	}

	/* lineout volume */
	ret = of_property_read_u32(np, "lineout_vol", &temp_val);
	if (ret < 0) {
		LOG_WARN("lineout volume get failed, use default vol");
		sunxi_codec->lineout_vol = 0;
	} else {
		sunxi_codec->lineout_vol = temp_val;
	}

	/* headphone volume */
	ret = of_property_read_u32(np, "headphonegain", &temp_val);
	if (ret < 0) {
		LOG_WARN("headphonegain volume get failed, use default vol");
		sunxi_codec->headphonegain = 0;
	} else {
		sunxi_codec->headphonegain = temp_val;
	}

	/* mic gain for capturing */
	ret = of_property_read_u32(np, "mic1gain", &temp_val);
	if (ret < 0) {
		LOG_WARN("mic1gain get failed, use default vol");
		sunxi_codec->mic1gain = 32;
	} else {
		sunxi_codec->mic1gain = temp_val;
	}
	ret = of_property_read_u32(np, "mic2gain", &temp_val);
	if (ret < 0) {
		LOG_WARN("mic2gain get failed, use default vol");
		sunxi_codec->mic2gain = 32;
	} else {
		sunxi_codec->mic2gain = temp_val;
	}
	ret = of_property_read_u32(np, "mic3gain", &temp_val);
	if (ret < 0) {
		sunxi_codec->mic3gain = 32;
		LOG_WARN("mic3gain get failed, use default vol");
	} else {
		sunxi_codec->mic3gain = temp_val;
	}

	/* Pa's delay time(ms) to work fine */
	ret = of_property_read_u32(np, "pa_msleep_time", &temp_val);
	if (ret < 0) {
		LOG_WARN("pa_msleep get failed, use default vol");
		sunxi_codec->spk_config.pa_msleep = 160;
	} else {
		sunxi_codec->spk_config.pa_msleep = temp_val;
	}

	/* PA/SPK enable property */
	ret = of_property_read_u32(np, "pa_level", &temp_val);
	if (ret < 0) {
		LOG_WARN("pa_level get failed, use default vol");
		sunxi_codec->spk_config.pa_level = 1;
	} else {
		sunxi_codec->spk_config.pa_level = temp_val;
	}
	ret = of_property_read_u32(np, "pa_pwr_level", &temp_val);
	if (ret < 0) {
		LOG_WARN("pa_pwr_level get failed, use default vol");
		sunxi_codec->spk_pwr_config.pa_level = 1;
	} else {
		sunxi_codec->spk_pwr_config.pa_level = temp_val;
	}

	LOG_INFO("digital_vol:%d, lineout_vol:%d, mic1gain:%d, mic2gain:%d"
		 " pa_msleep:%d, pa_level:%d, pa_pwr_level:%d\n",
		 sunxi_codec->digital_vol,
		 sunxi_codec->lineout_vol,
		 sunxi_codec->mic1gain,
		 sunxi_codec->mic2gain,
		 sunxi_codec->spk_config.pa_msleep,
		 sunxi_codec->spk_config.pa_level,
		 sunxi_codec->spk_pwr_config.pa_level);

#ifdef SUNXI_CODEC_DAP_ENABLE
	/* ADC/DAC DRC/HPF func enable property */
	ret = of_property_read_u32(np, "adcdrc_cfg", &temp_val);
	if (ret < 0) {
		LOG_ERR("adcdrc_cfg configs missing or invalid");
	} else {
		sunxi_codec->hw_config.adcdrc_cfg = temp_val;
	}

	ret = of_property_read_u32(np, "adchpf_cfg", &temp_val);
	if (ret < 0) {
		LOG_ERR("adchpf_cfg configs missing or invalid");
	} else {
		sunxi_codec->hw_config.adchpf_cfg = temp_val;
	}

	ret = of_property_read_u32(np, "dacdrc_cfg", &temp_val);
	if (ret < 0) {
		LOG_ERR("dacdrc_cfg configs missing or invalid");
	} else {
		sunxi_codec->hw_config.dacdrc_cfg = temp_val;
	}

	ret = of_property_read_u32(np, "dachpf_cfg", &temp_val);
	if (ret < 0) {
		LOG_ERR("dachpf_cfg configs missing or invalid");
	} else {
		sunxi_codec->hw_config.dachpf_cfg = temp_val;
	}

	LOG_INFO("adcdrc_cfg:%d, adchpf_cfg:%d, dacdrc_cfg:%d, dachpf:%d",
		 sunxi_codec->hw_config.adcdrc_cfg,
		 sunxi_codec->hw_config.adchpf_cfg,
		 sunxi_codec->hw_config.dacdrc_cfg,
		 sunxi_codec->hw_config.dachpf_cfg);
#endif
	/* get the gpio number to control external speaker */
	ret = of_get_named_gpio(np, "gpio-spk", 0);
	if (ret >= 0) {
		sunxi_codec->spk_config.used = 1;
		sunxi_codec->spk_config.spk_gpio = ret;
		if (!gpio_is_valid(sunxi_codec->spk_config.spk_gpio)) {
			sunxi_codec->spk_config.used = 0;
			LOG_ERR("gpio-spk set failed, SPK not work!");
		} else {
			ret = devm_gpio_request(&pdev->dev,
				sunxi_codec->spk_config.spk_gpio, "SPK");
			if (ret) {
				sunxi_codec->spk_config.used = 0;
				LOG_ERR("gpio-spk set failed, SPK not work!");
			}
		}
	} else {
		sunxi_codec->spk_config.used = 0;
	}

	ret = of_get_named_gpio(np, "gpio-spk-pwr", 0);
	if (ret >= 0) {
		sunxi_codec->spk_pwr_config.used = 1;
		sunxi_codec->spk_pwr_config.spk_gpio = ret;
		if (!gpio_is_valid(sunxi_codec->spk_pwr_config.spk_gpio)) {
			sunxi_codec->spk_pwr_config.used = 0;
			LOG_ERR("gpio-spk-pwr set failed, SPK not work!");
		} else {
			ret = devm_gpio_request(&pdev->dev,
				sunxi_codec->spk_pwr_config.spk_gpio, "SPK POWER");
			if (ret) {
				sunxi_codec->spk_pwr_config.used = 0;
				LOG_ERR("gpio-spk-pwr set failed, SPK not work!");
			} else {
				gpio_direction_output(sunxi_codec->spk_pwr_config.spk_gpio, 1);
				gpio_set_value(sunxi_codec->spk_pwr_config.spk_gpio,
						sunxi_codec->spk_pwr_config.pa_level);
			}
		}
	} else {
		sunxi_codec->spk_pwr_config.used = 0;
	}

	return 0;
}

static int sunxi_internal_codec_probe(struct platform_device *pdev)
{
	struct sunxi_codec_info *sunxi_codec;
	struct resource res;
	struct resource *memregion = NULL;
	struct device_node *np = pdev->dev.of_node;
	int ret;

	if (IS_ERR_OR_NULL(np)) {
		LOG_ERR("pdev->dev.of_node is err");
		ret = -EFAULT;
		goto err_node_put;
	}

	sunxi_codec = devm_kzalloc(&pdev->dev,
				sizeof(struct sunxi_codec_info), GFP_KERNEL);
	if (!sunxi_codec) {
		LOG_ERR("Can't allocate sunxi codec memory");
		ret = -ENOMEM;
		goto err_node_put;
	}
	dev_set_drvdata(&pdev->dev, sunxi_codec);
	sunxi_codec->dev = &pdev->dev;

	/*
	if (sunxi_codec_regulator_init(pdev, sunxi_codec) != 0) {
		LOG_ERR("Failed to init sunxi audio regulator");
		ret = -EFAULT;
		goto err_devm_kfree;
	}
	*/

	if (sunxi_codec_parse_params(np, pdev, sunxi_codec) != 0) {
		LOG_ERR("Failed to parse sunxi audio params");
		ret = -EFAULT;
		goto err_devm_kfree;
	}

	if (sunxi_codec_clk_init(np, pdev, sunxi_codec) != 0) {
		LOG_ERR("Failed to init sunxi audio clk");
		ret = -EFAULT;
		goto err_devm_kfree;
	}

	/* codec reg_base */
	/* get the true codec addr form np0 to avoid the build warning */
	ret = of_address_to_resource(np, 0, &res);
	if (ret) {
		LOG_ERR("Failed to get sunxi resource");
		return -EINVAL;
		goto err_moduleclk_disable;
	}

	memregion = devm_request_mem_region(&pdev->dev, res.start,
				resource_size(&res), "sunxi-internal-codec");
	if (!memregion) {
		LOG_ERR("sunxi memory region already claimed");
		ret = -EBUSY;
		goto err_moduleclk_disable;
	}

	sunxi_codec->digital_base = devm_ioremap(&pdev->dev,
					res.start, resource_size(&res));
	if (!sunxi_codec->digital_base) {
		LOG_ERR("sunxi digital_base ioremap failed");
		ret = -EBUSY;
		goto err_moduleclk_disable;
	}

	sunxi_codec->regmap = devm_regmap_init_mmio(&pdev->dev,
				sunxi_codec->digital_base,
				&sunxi_codec_regmap_config);
	if (IS_ERR_OR_NULL(sunxi_codec->regmap)) {
		LOG_ERR("regmap init failed");
		ret = PTR_ERR(sunxi_codec->regmap);
		goto err_moduleclk_disable;
	}

	/* CODEC DAI cfg and register */
	ret = devm_snd_soc_register_component(&pdev->dev, &soc_codec_dev_sunxi,
				sunxi_codec_dai, ARRAY_SIZE(sunxi_codec_dai));
	if (ret < 0) {
		LOG_ERR("register codec failed");
		goto err_moduleclk_disable;
	}

	ret  = sysfs_create_group(&pdev->dev.kobj, &audio_debug_attr_group);
	if (ret) {
		LOG_WARN("failed to create attr group");
		goto err_moduleclk_disable;
	}

	LOG_INFO("codec probe finished");

	return 0;


err_moduleclk_disable:
	clk_disable_unprepare(sunxi_codec->dacclk);
	clk_disable_unprepare(sunxi_codec->adcclk);
	clk_disable_unprepare(sunxi_codec->pllaudio0);
	clk_disable_unprepare(sunxi_codec->pllaudio1_div5);
err_devm_kfree:
	devm_kfree(&pdev->dev, sunxi_codec);
err_node_put:
	of_node_put(np);
	return ret;
}

static int  __exit sunxi_internal_codec_remove(struct platform_device *pdev)
{
	struct sunxi_codec_info *sunxi_codec = dev_get_drvdata(&pdev->dev);
	struct codec_spk_config *spk_cfg = &(sunxi_codec->spk_config);

	if (spk_cfg->used) {
		devm_gpio_free(&pdev->dev,
					sunxi_codec->spk_config.spk_gpio);
	}

	if (sunxi_codec->vol_supply.avcc) {
		regulator_disable(sunxi_codec->vol_supply.avcc);
		regulator_put(sunxi_codec->vol_supply.avcc);
	}

	if (sunxi_codec->vol_supply.hpvcc) {
		regulator_disable(sunxi_codec->vol_supply.hpvcc);
		regulator_put(sunxi_codec->vol_supply.hpvcc);
	}

	sysfs_remove_group(&pdev->dev.kobj, &audio_debug_attr_group);
	snd_soc_unregister_component(&pdev->dev);

	clk_disable_unprepare(sunxi_codec->dacclk);
	clk_put(sunxi_codec->dacclk);
	clk_disable_unprepare(sunxi_codec->adcclk);
	clk_put(sunxi_codec->adcclk);
	clk_disable_unprepare(sunxi_codec->pllaudio0);
	clk_put(sunxi_codec->pllaudio0);
	clk_disable_unprepare(sunxi_codec->pllaudio1_div5);
	clk_put(sunxi_codec->pllaudio1_div5);
	clk_disable_unprepare(sunxi_codec->codec_clk_bus);
	clk_put(sunxi_codec->codec_clk_bus);
	reset_control_assert(sunxi_codec->codec_clk_rst);

	devm_iounmap(&pdev->dev, sunxi_codec->digital_base);
	devm_kfree(&pdev->dev, sunxi_codec);
	platform_set_drvdata(pdev, NULL);

	LOG_INFO("codec remove finished");

	return 0;
}

static const struct of_device_id sunxi_internal_codec_of_match[] = {
	{ .compatible = "allwinner,sunxi-internal-codec", },
	{},
};

static struct platform_driver sunxi_internal_codec_driver = {
	.driver = {
		.name = "sunxi-internal-codec",
		.owner = THIS_MODULE,
		.of_match_table = sunxi_internal_codec_of_match,
	},
	.probe = sunxi_internal_codec_probe,
	.remove = __exit_p(sunxi_internal_codec_remove),
};
module_platform_driver(sunxi_internal_codec_driver);

MODULE_DESCRIPTION("SUNXI Codec ASoC driver");
MODULE_AUTHOR("Dby <Dby@allwinnertech.com>");
MODULE_LICENSE("GPL");
MODULE_ALIAS("platform:sunxi-internal-codec");
