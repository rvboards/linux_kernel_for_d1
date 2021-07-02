obj-$(CONFIG_SND_SUNXI_HIFI) += rpmsg_hifi.o

# SUNXI Platform Support
snd-soc-sunxi-hifi-objs := sunxi-hifi-pcm.o
snd-soc-sunxi-hifi-simple-card-objs := sunxi-hifi-simple-card.o
snd-soc-sunxi-hifi-daudio-objs := sunxi-daudio.o
snd-soc-sunxi-hifi-dmic-objs := sunxi-dmic.o

obj-$(CONFIG_SND_SUNXI_SOC_HIFI) += snd-soc-sunxi-hifi.o
obj-$(CONFIG_SND_SUNXI_SOC_HIFI_DAUDIO) += snd-soc-sunxi-hifi-daudio.o
obj-$(CONFIG_SND_SUNXI_SOC_HIFI_DMIC) += snd-soc-sunxi-hifi-dmic.o

# SUNXI Codec Embedded Support
obj-$(CONFIG_SND_SUNXI_SOC_HIFI_CODEC) += sunxi-hifi-codec.o
obj-$(CONFIG_SND_SUNXI_SOC_HIFI_CPUDAI) += sunxi-hifi-cpudai.o

# SUNXI Audio Simple Card Support
obj-$(CONFIG_SND_SUNXI_SOC_HIFI_CODEC) += sunxi-hifi-sndcodec.o
obj-$(CONFIG_SND_SUNXI_SOC_HIFI_SIMPLE_CARD) += snd-soc-sunxi-hifi-simple-card.o

# add the dma_flush range interface for arm64
obj-$(CONFIG_ARM64) += rpaf_flush_cache.o

