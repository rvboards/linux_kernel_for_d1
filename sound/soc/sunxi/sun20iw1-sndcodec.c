/*
 * sound\soc\sunxi\sun20iw1-sndcodec.c
 * (C) Copyright 2021-2026
 * Reuuimlla Technology Co., Ltd. <www.reuuimllatech.com>
 * huangxin <huangxin@Reuuimllatech.com>
 * liushaohua <liushaohua@allwinnertech.com>
 * yumingfengng <yumingfeng@allwinnertech.com>
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
#include <linux/clk.h>
#include <linux/mutex.h>
#include <linux/gpio.h>
#include <linux/io.h>
#include <linux/input.h>
#include <linux/of.h>
#include <linux/delay.h>
#include <sound/pcm.h>
#include <sound/soc.h>
#include <sound/jack.h>
#include <sound/pcm_params.h>
#include <sound/soc-dapm.h>

#include "sun20iw1-codec.h"

#define LOG_ERR(fmt, arg...)	pr_err("[SNDCODEC][%s][%d]:" fmt "\n", __func__, __LINE__, ##arg)
#define LOG_WARN(fmt, arg...)	pr_warn("[SNDCODEC][%s][%d]:" fmt "\n", __func__, __LINE__, ##arg)
#define LOG_INFO(fmt, arg...)	pr_info("[SNDCODEC][%s][%d]:" fmt "\n", __func__, __LINE__, ##arg)

static int mdata_threshold = 0x10;
module_param(mdata_threshold, int, 0644);
MODULE_PARM_DESC(mdata_threshold,
		"SUNXI hmic data threshold");

typedef enum {
	RESUME_IRQ  = 0x0,
	SYSINIT_IRQ = 0x1,
	OTHER_IRQ   = 0x2,
} _jack_irq_times;

enum HPDETECTWAY {
	HP_DETECT_LOW = 0x0,
	HP_DETECT_HIGH = 0x1,
};

enum dectect_jack {
	PLUG_OUT = 0x0,
	PLUG_IN  = 0x1,
};

enum JACK_FUNCTION {
	JACK_ENABLE = 0x1,
	JACK_DISABLE = 0x0,
};

static bool is_irq;
static int switch_state;

struct sunxi_card_priv {
	struct snd_soc_card *card;
	struct snd_soc_component *component;
	struct delayed_work hs_init_work;
	struct delayed_work hs_detect_work;
	struct delayed_work hs_button_work;
	struct delayed_work hs_checkplug_work;
	struct mutex jack_mlock;
	struct snd_soc_jack jack;
	struct timespec64 tv_headset_plugin;	/*4*/
	_jack_irq_times jack_irq_times;
	u32 detect_state;
	u32 jackirq;				/*switch irq*/
	u32 HEADSET_DATA;			/*threshod for switch insert*/
	u32 headset_basedata;
	u32 switch_status;
	u32 key_volup;
	u32 key_voldown;
	u32 key_hook;
	u32 key_voiceassist;
	u32 hp_detect_case;
	u32 jack_func;
};

/*
 * Identify the jack type as Headset/Headphone/None
 */
static int sunxi_check_jack_type(struct snd_soc_jack *jack)
{
	u32 reg_val = 0;
	u32 jack_type = 0, tempdata = 0;
	struct sunxi_card_priv *priv = container_of(jack,
						    struct sunxi_card_priv,
						    jack);

	reg_val = snd_soc_component_read32(priv->component, SUNXI_HMIC_STS);
	tempdata = (reg_val >> HMIC_DATA) & 0x1f;
	priv->headset_basedata = tempdata;

	if (tempdata >= priv->HEADSET_DATA) {
		/* headset:4 */
		jack_type = SND_JACK_HEADSET;
	} else {
		/* headphone:3, disable hbias and adc */
		snd_soc_component_update_bits(priv->component,
				SUNXI_MICBIAS_REG,
				(0x1 << HMICBIASEN),
				(0x0 << HMICBIASEN));
		snd_soc_component_update_bits(priv->component,
				SUNXI_MICBIAS_REG,
				(0x1 << MICADCEN),
				(0x0 << MICADCEN));
		jack_type = SND_JACK_HEADPHONE;
	}

	return jack_type;
}

/* Checks hs insertion by mdet */
static void sunxi_check_hs_plug(struct work_struct *work)
{
	struct sunxi_card_priv *priv = container_of(work,
						    struct sunxi_card_priv,
						    hs_checkplug_work.work);

	mutex_lock(&priv->jack_mlock);
	if (priv->detect_state != PLUG_IN) {
		/* Enable MDET */
		snd_soc_component_update_bits(priv->component, SUNXI_HMIC_CTRL,
				(0x1 << MIC_DET_IRQ_EN),
				(0x1 << MIC_DET_IRQ_EN));
		snd_soc_component_update_bits(priv->component, SUNXI_MICBIAS_REG,
				(0x1 << MICADCEN),
				(0x1 << MICADCEN));
		/* Enable PA */
		/*
		snd_soc_component_update_bits(priv->component, SUNXI_HEADPHONE_REG,
				(0x1 << HPPA_EN),
				(0x1 << HPPA_EN));
		*/
	} else {
		/*
		 * Enable HPPA_EN
		 * FIXME:When the Audio HAL is not at the do_output_standby,
		 * apk not play the music at the same time, we can insert
		 * headset now and click to play music immediately in the apk,
		 * the Audio HAL will write data to the card and not update
		 * the stream routing. Because we also set mute when
		 * the mdet come into force, so that the dapm will not update
		 * and it makes the mute.
		 */
		/*
		snd_soc_component_update_bits(priv->component, SUNXI_HEADPHONE_REG,
				(0x1 << HPPA_EN),
				(0x1 << HPPA_EN));
		*/
	}
	mutex_unlock(&priv->jack_mlock);
}

/* Checks jack insertion and identifies the jack type.*/
static void sunxi_check_hs_detect_status(struct work_struct *work)
{
	int jack_type = 0, reg_val = 0;
	struct sunxi_card_priv *priv = container_of(work,
						    struct sunxi_card_priv,
						    hs_detect_work.work);

	mutex_lock(&priv->jack_mlock);
	if (priv->detect_state == PLUG_IN) {
		/* enable hbias and adc */
		snd_soc_component_update_bits(priv->component,
				SUNXI_MICBIAS_REG,
				(0x1 << HMICBIASEN),
				(0x1 << HMICBIASEN));
		snd_soc_component_update_bits(priv->component,
				SUNXI_MICBIAS_REG,
				(0x1 << MICADCEN),
				(0x1 << MICADCEN));
		msleep(100);
		jack_type = sunxi_check_jack_type(&priv->jack);
		if (jack_type != priv->switch_status) {
			priv->switch_status = jack_type;
			snd_jack_report(priv->jack.jack, jack_type);
			LOG_INFO("plugin --> switch:%d", jack_type);
			switch_state = jack_type;
		}

		/* if SND_JACK_HEADSET,enable mic detect irq */
		if (jack_type == SND_JACK_HEADSET) {
			reg_val = snd_soc_component_read32(priv->component, SUNXI_HMIC_STS);
			priv->headset_basedata = (reg_val >> HMIC_DATA) & 0x1f;
			if (priv->headset_basedata > 3)
				priv->headset_basedata -= 3;

			usleep_range(1000, 2000);
			snd_soc_component_update_bits(priv->component,
				SUNXI_HMIC_CTRL,
				(0x1f << MDATA_THRESHOLD),
				(priv->headset_basedata << MDATA_THRESHOLD));
			snd_soc_component_update_bits(priv->component,
				SUNXI_HMIC_CTRL,
				(0x1 << MIC_DET_IRQ_EN),
				(0x1 << MIC_DET_IRQ_EN));
		} else if (jack_type == SND_JACK_HEADPHONE) {
			/* if is HEADPHONE 3, close mic detect irq */
			snd_soc_component_update_bits(priv->component,
				SUNXI_HMIC_CTRL,
				(0x1 << MIC_DET_IRQ_EN),
				(0x0 << MIC_DET_IRQ_EN));
		}
	} else {
		priv->switch_status = 0;
		snd_jack_report(priv->jack.jack, priv->switch_status);
		switch_state = priv->switch_status;
		LOG_INFO("plugout --> switch:%d", priv->switch_status);
		snd_soc_component_update_bits(priv->component,
				SUNXI_MICBIAS_REG,
				(0x1 << HMICBIASEN),
				(0x0 << HMICBIASEN));
		snd_soc_component_update_bits(priv->component,
				SUNXI_MICBIAS_REG,
				(0x1 << MICADCEN),
				(0x0 << MICADCEN));
		snd_soc_component_update_bits(priv->component, SUNXI_HMIC_CTRL,
				(0x1f << MDATA_THRESHOLD),
				(mdata_threshold << MDATA_THRESHOLD));
		snd_soc_component_update_bits(priv->component, SUNXI_HMIC_CTRL,
				(0x1 << MIC_DET_IRQ_EN),
				(0x0 << MIC_DET_IRQ_EN));
	}
	mutex_unlock(&priv->jack_mlock);
}

static void sunxi_hs_init_work(struct work_struct *work)
{
	struct sunxi_card_priv *priv = container_of(work,
						    struct sunxi_card_priv,
						    hs_init_work.work);

	mutex_lock(&priv->jack_mlock);
	if (is_irq == true) {
		is_irq = false;
	} else {
		if ((priv->hp_detect_case == HP_DETECT_LOW) ||
		    (priv->jack_irq_times == RESUME_IRQ)) {
			/*
			 * It should be report after resume.
			 * If the headset plugout after suspend, the system
			 * can not know the state, so we should reset here
			 * when resume.
			 */
			LOG_INFO("resume-->report switch");
			priv->switch_status = 0;
			snd_jack_report(priv->jack.jack, priv->switch_status);
			switch_state = 0;
		}
	}
	priv->jack_irq_times = OTHER_IRQ;
	mutex_unlock(&priv->jack_mlock);
}

/* Check for hook release */
static void sunxi_check_hs_button_status(struct work_struct *work)
{
	u32 i = 0;
	struct sunxi_card_priv *priv = container_of(work,
						    struct sunxi_card_priv,
						    hs_button_work.work);

	mutex_lock(&priv->jack_mlock);
	for (i = 0; i < 1; i++) {
		if (priv->key_hook == 0) {
			LOG_INFO("Hook (2)!!");
			priv->switch_status &= ~SND_JACK_BTN_0;
			snd_jack_report(priv->jack.jack, priv->switch_status);
			break;
		}
		/* may msleep 8 */
		msleep(20);
	}
	mutex_unlock(&priv->jack_mlock);
}

static irqreturn_t jack_interrupt(int irq, void *dev_id)
{
	struct sunxi_card_priv *priv = dev_id;
	struct timespec64 tv;
	u32 tempdata = 0, regval = 0;
	int jack_state = 0;

	if (priv->jack_irq_times == RESUME_IRQ ||
	    priv->jack_irq_times == SYSINIT_IRQ) {
		LOG_INFO("is_irq is ture");
		is_irq = true;
		priv->jack_irq_times = OTHER_IRQ;
	}

	jack_state = snd_soc_component_read32(priv->component, SUNXI_HMIC_STS);

	/*
	if (priv->detect_state != PLUG_IN) {
		//when headphone half-insertion, MIC_DET IRQ will be trigger.
		if (jack_state & (1 << MIC_DET_ST)) {
			regval = snd_soc_component_read32(priv->component, SUNXI_HMIC_CTRL);
			regval &= ~(0x1 << MIC_DET_IRQ_EN);
			snd_soc_component_write(priv->component, SUNXI_HMIC_CTRL, regval);

			regval = snd_soc_component_read32(priv->component, SUNXI_MICBIAS_REG);
			regval &= ~(0x1 << MICADCEN);
			snd_soc_component_write(priv->component, SUNXI_MICBIAS_REG, regval);

			//clear mic detect status
			regval = snd_soc_component_read32(priv->component, SUNXI_HMIC_STS);
			regval &= ~(0x1 << JACK_DET_IIN_ST);
			regval &= ~(0x1 << JACK_DET_OUT_ST);
			regval |= 0x1 << MIC_DET_ST;
			snd_soc_component_write(priv->component, SUNXI_HMIC_STS, regval);

			//prevent mic detect false trigger
			schedule_delayed_work(&priv->hs_checkplug_work,
				msecs_to_jiffies(700));
		}
	}
	*/

	/* headphone plugin */
	if (jack_state & (1 << JACK_DET_IIN_ST)) {
		regval = jack_state;
		regval &= ~(0x1 << JACK_DET_OUT_ST);
		snd_soc_component_write(priv->component, SUNXI_HMIC_STS, regval);
		priv->detect_state = PLUG_IN;
		/* get jack insert time to set mic det dead time */
		ktime_get_real_ts64(&priv->tv_headset_plugin);
		schedule_delayed_work(&priv->hs_detect_work,
				      msecs_to_jiffies(10));
	}

	/* headphone plugout */
	if (jack_state & (1 << JACK_DET_OUT_ST)) {
		regval = jack_state;
		regval &= ~(0x1 << JACK_DET_IIN_ST);
		snd_soc_component_write(priv->component, SUNXI_HMIC_STS, regval);
		priv->detect_state = PLUG_OUT;
		schedule_delayed_work(&priv->hs_detect_work,
				      msecs_to_jiffies(10));
	}

	/* headphone btn event */
	if ((priv->detect_state == PLUG_IN) &&
	    (jack_state & (1 << MIC_DET_ST))) {
		regval = jack_state;
		regval &= ~(0x1 << JACK_DET_IIN_ST);
		regval &= ~(0x1 << JACK_DET_OUT_ST);
		snd_soc_component_write(priv->component, SUNXI_HMIC_STS, regval);

		/* plugin less 1s, not operate */
		ktime_get_real_ts64(&tv);
		if (abs(tv.tv_sec - priv->tv_headset_plugin.tv_sec) < 1)
			return IRQ_HANDLED;

		tempdata = snd_soc_component_read32(priv->component, SUNXI_HMIC_STS);
		tempdata = (tempdata & 0x1f00) >> 8;
		LOG_ERR("KEY tempdata: %d", tempdata);

		if (tempdata == 2) {
			priv->key_hook = 0;
			priv->key_voldown = 0;
			priv->key_voiceassist = 0;
			priv->key_volup++;
			if (priv->key_volup == 1) {
				LOG_INFO("Volume ++");
				priv->key_volup = 0;
				priv->switch_status |= SND_JACK_BTN_1;
				snd_jack_report(priv->jack.jack,
						priv->switch_status);
				priv->switch_status &= ~SND_JACK_BTN_1;
				snd_jack_report(priv->jack.jack,
						priv->switch_status);
			}
		} else if ((tempdata == 5) || tempdata == 4) {
			priv->key_volup = 0;
			priv->key_hook = 0;
			priv->key_voiceassist = 0;
			priv->key_voldown++;
			if (priv->key_voldown == 1) {
				LOG_INFO("Volume --");
				priv->key_voldown = 0;
				priv->switch_status |= SND_JACK_BTN_2;
				snd_jack_report(priv->jack.jack,
						priv->switch_status);
				priv->switch_status &= ~SND_JACK_BTN_2;
				snd_jack_report(priv->jack.jack,
						priv->switch_status);
			}
		} else if (tempdata == 1) {
			priv->key_volup = 0;
			priv->key_hook = 0;
			priv->key_voldown = 0;
			priv->key_voiceassist++;
			if (priv->key_voiceassist == 1) {
				LOG_INFO("Voice Assistant Open");
				priv->key_voiceassist = 0;
				priv->switch_status |= SND_JACK_BTN_3;
				snd_jack_report(priv->jack.jack,
						priv->switch_status);
				priv->switch_status &= ~SND_JACK_BTN_3;
				snd_jack_report(priv->jack.jack,
						priv->switch_status);
			}
		} else if (tempdata == 0x0) {
			priv->key_volup = 0;
			priv->key_voldown = 0;
			priv->key_voiceassist = 0;
			priv->key_hook++;
			if (priv->key_hook >= 1) {
				priv->key_hook = 0;
				if ((priv->switch_status & SND_JACK_BTN_0)
				    == 0) {
					priv->switch_status |= SND_JACK_BTN_0;
					snd_jack_report(priv->jack.jack,
							priv->switch_status);
					LOG_INFO("Hook (1)");
				}
				schedule_delayed_work(&priv->hs_button_work,
						      msecs_to_jiffies(180));
			}
		} else {
			LOG_ERR("tempdata:0x%x,Key data err:",
				 tempdata);
			priv->key_volup = 0;
			priv->key_voldown = 0;
			priv->key_hook = 0;
			priv->key_voiceassist = 0;
		}
	}

	return IRQ_HANDLED;
}

static const struct snd_kcontrol_new sunxi_card_controls[] = {
	SOC_DAPM_PIN_SWITCH("Headphone"),
	SOC_DAPM_PIN_SWITCH("HpSpeaker"),
	SOC_DAPM_PIN_SWITCH("LINEOUT"),
};

static const struct snd_soc_dapm_widget sunxi_card_dapm_widgets[] = {
	SND_SOC_DAPM_MIC("HeadphoneMic", NULL),
	SND_SOC_DAPM_MIC("Main Mic", NULL),
};

static const struct snd_soc_dapm_route sunxi_card_routes[] = {
	{"MainMic Bias", NULL, "Main Mic"},
	{"MIC1", NULL, "MainMic Bias"},
	/*{"MIC2", NULL, "HeadphoneMic"},*/
	{"MIC2", NULL, "MainMic Bias"},
	{"MIC3", NULL, "MainMic Bias"},
};

static void sunxi_hs_reg_init(struct sunxi_card_priv *priv)
{
	/*
	snd_soc_component_update_bits(priv->component, SUNXI_HMIC_CTRL,
			(0xffff << 0),
			(0x0 << 0));
	*/
	snd_soc_component_update_bits(priv->component, SUNXI_HMIC_CTRL,
			(0x1f << MDATA_THRESHOLD),
			(0x17 << MDATA_THRESHOLD));
	/*
	snd_soc_component_update_bits(priv->component, SUNXI_HMIC_STS,
			(0xffff << 0),
			(0x6000 << 0));
	*/
	/*
	snd_soc_component_update_bits(priv->component, SUNXI_MICBIAS_REG,
			(0x7f << SELDETADCBF),
			(0x40 << SELDETADCBF));
	*/
	if (priv->hp_detect_case == HP_DETECT_LOW) {
		snd_soc_component_update_bits(priv->component,
				SUNXI_MICBIAS_REG,
				(0x1 << AUTOPLEN),
				(0x1 << AUTOPLEN));
		snd_soc_component_update_bits(priv->component,
				SUNXI_MICBIAS_REG,
				(0x1 << DETMODE),
				(0x0 << DETMODE));
	} else {
		snd_soc_component_update_bits(priv->component,
				SUNXI_MICBIAS_REG,
				(0x1 << AUTOPLEN),
				(0x0 << AUTOPLEN));
		snd_soc_component_update_bits(priv->component,
				SUNXI_MICBIAS_REG,
				(0x1 << DETMODE),
				(0x1 << DETMODE));
	}
	snd_soc_component_update_bits(priv->component, SUNXI_HMIC_CTRL,
			(0xf << HMIC_N),
			(HP_DEBOUCE_TIME << HMIC_N));
	snd_soc_component_update_bits(priv->component, SUNXI_MICBIAS_REG,
			(0x1 << JACKDETEN),
			(0x1 << JACKDETEN));
	snd_soc_component_update_bits(priv->component, SUNXI_HMIC_CTRL,
			(0x1 << JACK_IN_IRQ_EN),
			(0x1 << JACK_IN_IRQ_EN));
	snd_soc_component_update_bits(priv->component, SUNXI_HMIC_CTRL,
			(0x1 << JACK_OUT_IRQ_EN),
			(0x1 << JACK_OUT_IRQ_EN));

	/*
	snd_soc_component_update_bits(priv->component, SUNXI_MICBIAS_REG,
			    (0x1 << HMICBIASEN), (0x1 << HMICBIASEN));
	snd_soc_component_update_bits(priv->component, SUNXI_MICBIAS_REG,
			    (0x1 << MICADCEN), (0x1 << MICADCEN));
	*/

	/*
	snd_soc_component_update_bits(priv->component, SUNXI_HMIC_CTRL,
			(0x1 << MIC_DET_IRQ_EN),
			(0x1 << MIC_DET_IRQ_EN));
	*/

	schedule_delayed_work(&priv->hs_init_work, msecs_to_jiffies(10));
}

static void snd_sunxi_unregister_jack(struct sunxi_card_priv *priv)
{
	/*
	 * Set process button events to false so that the button
	 * delayed work will not be scheduled.
	 */
	cancel_delayed_work_sync(&priv->hs_detect_work);
	cancel_delayed_work_sync(&priv->hs_button_work);
	cancel_delayed_work_sync(&priv->hs_init_work);
	free_irq(priv->jackirq, priv);
}

/*
 * Card initialization
 */
static int sunxi_card_init(struct snd_soc_pcm_runtime *rtd)
{
	struct snd_soc_component *component = rtd->codec_dai->component;
	struct snd_soc_dapm_context *dapm = &component->dapm;

	struct sunxi_card_priv *priv = snd_soc_card_get_drvdata(rtd->card);
	int ret;

	priv->component = rtd->codec_dai->component;

	ret = snd_soc_card_jack_new(rtd->card, "sunxi Audio Jack",
			       SND_JACK_HEADSET | SND_JACK_HEADPHONE |
				   SND_JACK_BTN_0 | SND_JACK_BTN_1 |
				   SND_JACK_BTN_2 | SND_JACK_BTN_3,
			       &priv->jack, NULL, 0);
	if (ret) {
		LOG_ERR("jack creation failed");
		return ret;
	}

	snd_jack_set_key(priv->jack.jack, SND_JACK_BTN_0, KEY_MEDIA);
	snd_jack_set_key(priv->jack.jack, SND_JACK_BTN_1, KEY_VOLUMEUP);
	snd_jack_set_key(priv->jack.jack, SND_JACK_BTN_2, KEY_VOLUMEDOWN);
	snd_jack_set_key(priv->jack.jack, SND_JACK_BTN_3, KEY_VOICECOMMAND);

	snd_soc_dapm_disable_pin(dapm, "HPOUTR");
	snd_soc_dapm_disable_pin(dapm, "HPOUTL");

	snd_soc_dapm_disable_pin(dapm, "LINEOUT");
	snd_soc_dapm_disable_pin(dapm, "HpSpeaker");
	snd_soc_dapm_disable_pin(dapm, "Headphone");

	snd_soc_dapm_sync(dapm);

	LOG_WARN("card init finished");
	return 0;
}

static int sunxi_card_hw_params(struct snd_pcm_substream *substream,
				struct snd_pcm_hw_params *params)
{
	struct snd_soc_pcm_runtime *rtd = substream->private_data;
	struct snd_soc_dai *codec_dai = rtd->codec_dai;
	/* struct snd_soc_card *card = rtd->card; */
	unsigned int freq;
	int ret;
	int stream_flag;

	switch (params_rate(params)) {
	case	8000:
	case	12000:
	case	16000:
	case	24000:
	case	32000:
	case	48000:
	case	96000:
	case	192000:
		freq = 24576000;
		break;
	case	11025:
	case	22050:
	case	44100:
		freq = 22579200;
		break;
	default:
		LOG_ERR("invalid rate setting");
		return -EINVAL;
	}

	/* the substream type: 0->playback, 1->capture */
	stream_flag = substream->stream;
	LOG_INFO("stream_flag: %d", stream_flag);

	/* To surpport playback and capture func in different freq point */
	if (freq == 22579200) {
		if (stream_flag == 0) {
			ret = snd_soc_dai_set_sysclk(codec_dai, 0, freq, 0);
			if (ret < 0) {
				LOG_ERR("set codec dai sysclk faided, freq:%d", freq);
				return ret;
			}
		}
	}

	if (freq == 22579200) {
		if (stream_flag == 1) {
			ret = snd_soc_dai_set_sysclk(codec_dai, 1, freq, 0);
			if (ret < 0) {
				LOG_ERR("set codec dai sysclk faided, freq:%d", freq);
				return ret;
			}
		}
	}

	if (freq == 24576000) {
		if (stream_flag == 0) {
			ret = snd_soc_dai_set_sysclk(codec_dai, 2, freq, 0);
			if (ret < 0) {
				LOG_ERR("set codec dai sysclk faided, freq:%d", freq);
				return ret;
			}
		}
	}

	if (freq == 24576000) {
			if (stream_flag == 1) {
			ret = snd_soc_dai_set_sysclk(codec_dai, 3, freq, 0);
			if (ret < 0) {
				LOG_ERR("set codec dai sysclk faided, freq:%d", freq);
				return ret;
			}
		}
	}

	return 0;
}

static struct snd_soc_ops sunxi_card_ops = {
	.hw_params = sunxi_card_hw_params,
};

SND_SOC_DAILINK_DEFS(sun20iw1p1_dai_link,
	DAILINK_COMP_ARRAY(COMP_CPU("sunxi-dummy-cpudai")),
	DAILINK_COMP_ARRAY(COMP_CODEC("sunxi-internal-codec", "sun20iw1codec")),
	DAILINK_COMP_ARRAY(COMP_PLATFORM("sunxi-dummy-cpudai")));

static struct snd_soc_dai_link sunxi_card_dai_link[] = {
	{
		.name		= "audiocodec",
		.stream_name	= "SUNXI-CODEC",
		.dai_fmt = SND_SOC_DAIFMT_I2S | SND_SOC_DAIFMT_NB_NF
				| SND_SOC_DAIFMT_CBM_CFM,
		.init		= sunxi_card_init,
		.ops		= &sunxi_card_ops,
		SND_SOC_DAILINK_REG(sun20iw1p1_dai_link),
	},
};

static int sunxi_card_suspend(struct snd_soc_card *card)
{
	struct sunxi_card_priv *priv = snd_soc_card_get_drvdata(card);

	disable_irq(priv->jackirq);

	snd_soc_component_update_bits(priv->component, SUNXI_HMIC_CTRL,
			    (0x1 << MIC_DET_IRQ_EN), (0x0 << MIC_DET_IRQ_EN));
	snd_soc_component_update_bits(priv->component, SUNXI_HMIC_CTRL,
			    (0x1 << JACK_IN_IRQ_EN), (0x0 << JACK_IN_IRQ_EN));
	snd_soc_component_update_bits(priv->component, SUNXI_HMIC_CTRL,
			    (0x1 << JACK_OUT_IRQ_EN), (0x0 << JACK_OUT_IRQ_EN));
	snd_soc_component_update_bits(priv->component, SUNXI_MICBIAS_REG, (0x1 << JACKDETEN),
			    (0x0 << JACKDETEN));
	LOG_INFO("suspend");

	return 0;
}

static int sunxi_card_resume(struct snd_soc_card *card)
{
	struct sunxi_card_priv *priv = snd_soc_card_get_drvdata(card);

	enable_irq(priv->jackirq);
	priv->jack_irq_times = RESUME_IRQ;
	priv->detect_state = PLUG_OUT;/*todo..?*/
	sunxi_hs_reg_init(priv);
	LOG_INFO("resume");

	return 0;
}

static struct snd_soc_card snd_soc_sunxi_card = {
	.name			= "audiocodec",
	.owner			= THIS_MODULE,
	.dai_link		= sunxi_card_dai_link,
	.num_links		= ARRAY_SIZE(sunxi_card_dai_link),
	.suspend_post		= sunxi_card_suspend,
	.resume_post		= sunxi_card_resume,

};

static int sunxi_card_dev_probe(struct platform_device *pdev)
{
	int ret = 0;
	u32 temp_val;
	struct sunxi_card_priv *priv = NULL;
	struct device_node *np = pdev->dev.of_node;
	struct snd_soc_card *card = &snd_soc_sunxi_card;
	struct snd_soc_dapm_context *dapm = &card->dapm;

	if (!np) {
		LOG_ERR("can not get dt node for this device");
		return -EINVAL;
	}

	/* dai link */
	sunxi_card_dai_link[0].cpus->dai_name = NULL;
	sunxi_card_dai_link[0].cpus->of_node = of_parse_phandle(np,
					"sunxi,cpudai-controller", 0);
	if (!sunxi_card_dai_link[0].cpus->of_node) {
		LOG_ERR("Property 'sunxi,cpudai-controller' missing or invalid");
		ret = -EINVAL;
		goto err_devm_kfree;
	} else {
		sunxi_card_dai_link[0].platforms->name = NULL;
		sunxi_card_dai_link[0].platforms->of_node =
				sunxi_card_dai_link[0].cpus->of_node;
	}
	sunxi_card_dai_link[0].codecs->name = NULL;
	sunxi_card_dai_link[0].codecs->of_node = of_parse_phandle(np,
						"sunxi,audio-codec", 0);
	if (!sunxi_card_dai_link[0].codecs->of_node) {
		LOG_ERR("Property 'sunxi,audio-codec' missing or invalid");
		ret = -EINVAL;
		goto err_devm_kfree;
	}

	/* register the soc card */
	card->dev = &pdev->dev;

	priv = devm_kzalloc(&pdev->dev, sizeof(struct sunxi_card_priv),
			    GFP_KERNEL);
	if (!priv) {
		LOG_ERR("devm_kzalloc failed %d", ret);
		return -ENOMEM;
	}
	priv->card = card;

	snd_soc_card_set_drvdata(card, priv);

	ret = snd_soc_register_card(card);
	if (ret) {
		LOG_ERR("snd_soc_register_card failed %d", ret);
		goto err_devm_kfree;
	}

	ret = snd_soc_add_card_controls(card, sunxi_card_controls,
					ARRAY_SIZE(sunxi_card_controls));
	if (ret)
		LOG_ERR("failed to register codec controls");

	snd_soc_dapm_new_controls(dapm, sunxi_card_dapm_widgets,
				  ARRAY_SIZE(sunxi_card_dapm_widgets));
	snd_soc_dapm_add_routes(dapm, sunxi_card_routes,
				ARRAY_SIZE(sunxi_card_routes));

	ret = of_property_read_u32(np, "jack_enable", &temp_val);
	if (ret < 0) {
		LOG_ERR("jack_enable missing, use default enable jack");
		priv->jack_func = JACK_ENABLE;
	} else {
		priv->jack_func = temp_val;
	}
	if (priv->jack_func == JACK_DISABLE)
		return 0;

	ret = of_property_read_u32(np, "hp_detect_case", &temp_val);
	if (ret < 0) {
		LOG_ERR("hp_detect_case missing, use default hp_detect_low");
		priv->hp_detect_case = HP_DETECT_LOW;
	} else {
		priv->hp_detect_case = temp_val;
	}

	/* initial the parameters for judge switch state */
	priv->jack_irq_times = SYSINIT_IRQ;
	priv->HEADSET_DATA = 0xA;
	priv->detect_state = PLUG_OUT;
	mutex_init(&priv->jack_mlock);
	INIT_DELAYED_WORK(&priv->hs_detect_work, sunxi_check_hs_detect_status);
	INIT_DELAYED_WORK(&priv->hs_button_work, sunxi_check_hs_button_status);
	INIT_DELAYED_WORK(&priv->hs_init_work, sunxi_hs_init_work);
	INIT_DELAYED_WORK(&priv->hs_checkplug_work, sunxi_check_hs_plug);

	priv->jackirq = platform_get_irq(pdev, 0);
	if (priv->jackirq < 0) {
		LOG_ERR("irq get failed");
		ret = -ENODEV;
	}
	ret = request_irq(priv->jackirq, jack_interrupt, 0, "audio jack irq",
			  priv);

	sunxi_hs_reg_init(priv);

	LOG_INFO("register card finished");
	return 0;

err_devm_kfree:
	devm_kfree(&pdev->dev, priv);
	return ret;
}

static int __exit sunxi_card_dev_remove(struct platform_device *pdev)
{
	struct snd_soc_card *card = platform_get_drvdata(pdev);
	struct sunxi_card_priv *priv = snd_soc_card_get_drvdata(card);

	snd_soc_component_update_bits(priv->component, SUNXI_HMIC_CTRL,
			    (0x1 << JACK_IN_IRQ_EN), (0x0 << JACK_IN_IRQ_EN));
	snd_soc_component_update_bits(priv->component, SUNXI_HMIC_CTRL,
			    (0x1 << JACK_OUT_IRQ_EN), (0x0 << JACK_OUT_IRQ_EN));
	snd_soc_component_update_bits(priv->component, SUNXI_MICBIAS_REG, (0x1 << JACKDETEN),
			    (0x0 << JACKDETEN));
	snd_soc_component_update_bits(priv->component, SUNXI_MICBIAS_REG,
				(0x1 << HMICBIASEN), (0x0 << HMICBIASEN));
	snd_sunxi_unregister_jack(priv);

	snd_soc_unregister_card(card);
	devm_kfree(&pdev->dev, priv);

	LOG_WARN("unregister card finished");

	return 0;
}

static const struct of_device_id sunxi_card_of_match[] = {
	{ .compatible = "allwinner,sunxi-codec-machine", },
	{},
};

static struct platform_driver sunxi_machine_driver = {
	.driver = {
		.name = "sunxi-codec-machine",
		.owner = THIS_MODULE,
		.pm = &snd_soc_pm_ops,
		.of_match_table = sunxi_card_of_match,
	},
	.probe = sunxi_card_dev_probe,
	.remove = __exit_p(sunxi_card_dev_remove),
};

static int __init sunxi_machine_driver_init(void)
{
	return platform_driver_register(&sunxi_machine_driver);
}
module_init(sunxi_machine_driver_init);

static void __exit sunxi_machine_driver_exit(void)
{
	platform_driver_unregister(&sunxi_machine_driver);
}
module_exit(sunxi_machine_driver_exit);

module_param_named(switch_state, switch_state, int, S_IRUGO | S_IWUSR);

MODULE_DESCRIPTION("SUNXI Codec Machine ASoC driver");
MODULE_AUTHOR("Dby <Dby@allwinnertech.com>");
MODULE_LICENSE("GPL");
MODULE_ALIAS("platform:sunxi-codec-machine");
