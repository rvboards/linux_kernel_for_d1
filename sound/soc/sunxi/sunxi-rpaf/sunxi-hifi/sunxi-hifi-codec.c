/*
 * sound\soc\sunxi\hifi-dsp\sunxi-codec.c
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

#include <linux/module.h>
#include <linux/slab.h>
#include <linux/clk.h>
#include <linux/io.h>
#include <linux/delay.h>
#include <linux/of.h>
#include <linux/of_gpio.h>
#include <linux/of_address.h>
#include <linux/of_device.h>
#include <linux/pm.h>
#include <linux/pinctrl/consumer.h>
#include <linux/sunxi-gpio.h>
#include <linux/dma-mapping.h>
#include <linux/dmaengine.h>
#include <linux/reset.h>
#include <asm/dma.h>
#include <sound/pcm.h>
#include <sound/pcm_params.h>
#include <sound/dmaengine_pcm.h>
#include <sound/soc.h>
#include <sound/initval.h>
#include <sound/tlv.h>
#include <sound/aw_rpaf/rpmsg_hifi.h>

#include "sunxi-hifi-codec.h"

#define	DRV_NAME	"sunxi-internal-codec"

static int sunxi_hifi_codec_hw_params(struct snd_pcm_substream *substream,
		struct snd_pcm_hw_params *params, struct snd_soc_dai *dai)
{
	return 0;
}

static int sunxi_hifi_codec_startup(struct snd_pcm_substream *substream,
				struct snd_soc_dai *dai)
{
	return 0;
}

static void sunxi_hifi_codec_shutdown(struct snd_pcm_substream *substream,
				struct snd_soc_dai *dai)
{
}

static int sunxi_hifi_codec_prepare(struct snd_pcm_substream *substream,
				struct snd_soc_dai *dai)
{
	return 0;
}

static int sunxi_hifi_codec_trigger(struct snd_pcm_substream *substream,
				int cmd, struct snd_soc_dai *dai)
{
	return 0;
}

static const struct snd_soc_dai_ops sunxi_hifi_codec_dai_ops = {
	.startup	= sunxi_hifi_codec_startup,
	.hw_params	= sunxi_hifi_codec_hw_params,
	.prepare	= sunxi_hifi_codec_prepare,
	.trigger	= sunxi_hifi_codec_trigger,
	.shutdown	= sunxi_hifi_codec_shutdown,
};

static struct snd_soc_dai_driver sunxi_hifi_codec_dai[] = {
	{
		.name	= "sunxi-hifi-codec",
		.playback = {
			.stream_name = "Playback",
			.channels_min = 1,
			.channels_max = 2,
			.rates	= SNDRV_PCM_RATE_8000_192000
				| SNDRV_PCM_RATE_KNOT,
			.formats = SNDRV_PCM_FMTBIT_S16_LE
				| SNDRV_PCM_FMTBIT_S24_LE
				| SNDRV_PCM_FMTBIT_S32_LE,
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
		.ops = &sunxi_hifi_codec_dai_ops,
	},
};

extern int sunxi_mixer_block_send(unsigned int hifi_id,
						struct msg_mixer_package *msg_mixer);
#if 0
static int generic_volume_get(struct snd_kcontrol *kcontrol,
				   struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_component *component = snd_soc_kcontrol_component(kcontrol);
	struct sunxi_codec_info *sunxi_codec = snd_soc_component_get_drvdata(component);
	struct msg_mixer_package *msg_mixer = &sunxi_codec->msg_mixer;
	struct snd_soc_dsp_mixer *soc_mixer = &msg_mixer->soc_mixer;
	int ret;

	soc_mixer->id = kcontrol->id.numid;
	strcpy(soc_mixer->ctl_name, kcontrol->id.name);
	soc_mixer->cmd_val = SND_SOC_DSP_MIXER_READ;

	ret = sunxi_mixer_block_send(SUNXI_HIFI0, msg_mixer);
	if (ret != 0)
		return ret;

	ucontrol->value.integer.value[0] = soc_mixer->value;
	return 0;
}

static int generic_volume_put(struct snd_kcontrol *kcontrol,
		struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_component *component = snd_soc_kcontrol_component(kcontrol);
	struct sunxi_codec_info *sunxi_codec = snd_soc_component_get_drvdata(component);
	struct msg_mixer_package *msg_mixer = &sunxi_codec->msg_mixer;
	struct snd_soc_dsp_mixer *soc_mixer = &msg_mixer->soc_mixer;
	int ret;

	soc_mixer->cmd_val = SND_SOC_DSP_MIXER_WRITE;
	soc_mixer->id = kcontrol->id.numid;
	strcpy(soc_mixer->ctl_name, kcontrol->id.name);
	soc_mixer->value = ucontrol->value.integer.value[0];

	ret = sunxi_mixer_block_send(SUNXI_HIFI0, msg_mixer);
	if (ret != 0)
		return ret;

	ucontrol->value.integer.value[0] = soc_mixer->value;

	return 0;
}

static const struct snd_kcontrol_new sunxi_codec_controls[] = {
	SOC_SINGLE_EXT("MIC1 gain volume", 0, 0, 31, 0, generic_volume_get, generic_volume_put),
	SOC_SINGLE_EXT("MIC2 gain volume", 0, 0, 31, 0, generic_volume_get, generic_volume_put),
	SOC_SINGLE_EXT("MIC3 gain volume", 0, 0, 31, 0, generic_volume_get, generic_volume_put),
	SOC_SINGLE_EXT("MIC4 gain volume", 0, 0, 31, 0, generic_volume_get, generic_volume_put),
	SOC_SINGLE_EXT("MIC5 gain volume", 0, 0, 31, 0, generic_volume_get, generic_volume_put),
	SOC_SINGLE_EXT("Speaker volume", 0, 0, 31, 0, generic_volume_get, generic_volume_put),
};
#endif

static int sunxi_hifi_codec_probe(struct snd_soc_component *component)
{
#if 0
	struct sunxi_codec_info *sunxi_codec = snd_soc_component_get_drvdata(component);
	int ret;
//	sunxi_codec->codec = codec;

	ret = snd_soc_add_component_controls(component, sunxi_codec_controls,
					ARRAY_SIZE(sunxi_codec_controls));
	if (ret)
		pr_err("failed to register codec controls!\n");
#endif
	return 0;
}

static void sunxi_hifi_codec_remove(struct snd_soc_component *component)
{
}

static int sunxi_hifi_codec_suspend(struct snd_soc_component *component)
{
	return 0;
}

static int sunxi_hifi_codec_resume(struct snd_soc_component *component)
{
	return 0;
}

static struct snd_soc_component_driver soc_codec_dev_sunxi = {
	.probe = sunxi_hifi_codec_probe,
	.remove = sunxi_hifi_codec_remove,
	.suspend = sunxi_hifi_codec_suspend,
	.resume = sunxi_hifi_codec_resume,
//	.ignore_pmdown_time = 1,
};

#if 0
static const struct regmap_config sunxi_hifi_codec_regmap_config = {
	.reg_bits = 32,
	.reg_stride = 4,
	.val_bits = 32,
	.max_register = SUNXI_ADC5_REG,
	.cache_type = REGCACHE_NONE,
};
#endif

static int  sunxi_hifi_codec_dev_probe(struct platform_device *pdev)
{
	struct sunxi_codec_info *sunxi_codec;
	struct device_node *np = pdev->dev.of_node;
	struct msg_mixer_package *msg_mixer;
	int ret;

	sunxi_codec = devm_kzalloc(&pdev->dev,
				sizeof(struct sunxi_codec_info), GFP_KERNEL);
	if (!sunxi_codec) {
		dev_err(&pdev->dev, "Can't allocate sunxi codec memory\n");
		ret = -ENOMEM;
		goto err_devm_kzalloc;
	}
	dev_set_drvdata(&pdev->dev, sunxi_codec);
	sunxi_codec->dev = &pdev->dev;

	msg_mixer = &sunxi_codec->msg_mixer;
	init_waitqueue_head(&msg_mixer->tsleep);
	msg_mixer->wakeup_flag = 0;
	/* TODO, config by dts? */
	msg_mixer->soc_mixer.card = 0;
	msg_mixer->soc_mixer.device = 0;
	strcpy(msg_mixer->soc_mixer.driver, "audiocodec");

#if 0
	/* codec reg_base */
	ret = of_address_to_resource(np, 0, &sunxi_codec->digital_res);
	if (ret) {
		dev_err(&pdev->dev, "parse device node codec resource failed\n");
		ret = -EINVAL;
		goto err_get_resource;
	}

	sunxi_codec->addr_base = of_iomap(np, 0);
	if (sunxi_codec->addr_base == NULL) {
		dev_err(&pdev->dev, "digital register iomap failed\n");
		ret = -EINVAL;
		goto err_get_iomap;
	}

	sunxi_codec->regmap = devm_regmap_init_mmio(&pdev->dev,
				sunxi_codec->addr_base, &sunxi_hifi_codec_regmap_config);
	if (IS_ERR(sunxi_codec->regmap)) {
		dev_err(&pdev->dev, "regmap init failed\n");
		ret = PTR_ERR(sunxi_codec->regmap);
		goto err_devm_regmap_init;
	}
#endif
	ret = snd_soc_register_component(&pdev->dev, &soc_codec_dev_sunxi,
				sunxi_hifi_codec_dai, ARRAY_SIZE(sunxi_hifi_codec_dai));
	if (ret < 0) {
		dev_err(&pdev->dev, "register codec failed\n");
		ret = -EBUSY;
		goto err_register_codec;
	}

	return 0;

err_register_codec:
//err_devm_regmap_init:
//	iounmap(sunxi_codec->addr_base);
//err_get_iomap:
//err_get_resource:
	devm_kfree(&pdev->dev, sunxi_codec);
err_devm_kzalloc:
	of_node_put(np);
	return ret;
}

static int  __exit sunxi_hifi_codec_dev_remove(struct platform_device *pdev)
{
	struct sunxi_codec_info *sunxi_codec = dev_get_drvdata(&pdev->dev);
	struct device_node *np = pdev->dev.of_node;

	snd_soc_unregister_component(&pdev->dev);

//	iounmap(sunxi_codec->addr_base);

	devm_kfree(&pdev->dev, sunxi_codec);
	of_node_put(np);

	return 0;
}

static const struct of_device_id sunxi_hifi_codec_of_match[] = {
	{ .compatible = "allwinner,sunxi-internal-codec", },
	{},
};

static struct platform_driver sunxi_hifi_codec_driver = {
	.driver = {
		.name = DRV_NAME,
		.owner = THIS_MODULE,
		.of_match_table = sunxi_hifi_codec_of_match,
	},
	.probe = sunxi_hifi_codec_dev_probe,
	.remove = __exit_p(sunxi_hifi_codec_dev_remove),
};
module_platform_driver(sunxi_hifi_codec_driver);

MODULE_DESCRIPTION("SUNXI Codec ASoC driver");
MODULE_AUTHOR("yumingfeng <yumingfeng@allwinnertech.com>");
MODULE_LICENSE("GPL");
MODULE_ALIAS("platform:sunxi-internal-codec");
