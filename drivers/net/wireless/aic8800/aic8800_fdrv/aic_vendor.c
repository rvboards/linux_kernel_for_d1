#include "aic_vendor.h"
#include "rwnx_defs.h"
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/skbuff.h>
#include <linux/netdevice.h>
#include <linux/inetdevice.h>
#include <linux/rtnetlink.h>
#include <net/netlink.h>

#define KEEP_ALIVE

#define GOOGLE_OUI     0x001A11

#ifdef KEEP_ALIVE
int aic_dev_start_mkeep_alive(struct rwnx_hw *rwnx_hw, struct rwnx_vif *rwnx_vif,
			u8 mkeep_alive_id, u8 *ip_pkt, u16 ip_pkt_len, u8 *src_mac, u8 *dst_mac, u32 period_msec)
{
	u8 *data, *pos;

	data = kzalloc(ip_pkt_len + 14, GFP_KERNEL);
	if (!data)
		return -ENOMEM;

	pos = data;
	memcpy(pos, dst_mac, 6);
	pos += 6;
	memcpy(pos, src_mac, 6);
	pos += 6;
    /* Mapping Ethernet type (ETHERTYPE_IP: 0x0800) */
	*(pos++) = 0x08;
	*(pos++) = 0x00;

	/* Mapping IP pkt */
	memcpy(pos, ip_pkt, ip_pkt_len);
	pos += ip_pkt_len;

	//add send 802.3 pkt(raw data)
	kfree(data);

	return 0;
}


int aic_dev_stop_mkeep_alive(struct rwnx_hw *rwnx_hw, struct rwnx_vif *rwnx_vif, u8 mkeep_alive_id)
{
	int  res = -1;

	/*
	 * The mkeep_alive packet is for STA interface only; if the bss is configured as AP,
	 * dongle shall reject a mkeep_alive request.
	 */
	if (rwnx_vif->wdev.iftype != NL80211_IFTYPE_STATION)
		return res;

	printk("%s execution\n", __func__);

	//add send stop keep alive
	res = 0;
	return res;
}


static int aicwf_vendor_start_mkeep_alive(struct wiphy *wiphy, struct wireless_dev *wdev,
	const void *data, int len)
{
	/* max size of IP packet for keep alive */
	const int MKEEP_ALIVE_IP_PKT_MAX = 256;

	int ret = 0, rem, type;
	u8 mkeep_alive_id = 0;
	u8 *ip_pkt = NULL;
	u16 ip_pkt_len = 0;
	u8 src_mac[6];
	u8 dst_mac[6];
	u32 period_msec = 0;
	const struct nlattr *iter;
	struct rwnx_hw *rwnx_hw = wiphy_priv(wiphy);
	struct rwnx_vif *rwnx_vif = container_of(wdev, struct rwnx_vif, wdev);
	gfp_t kflags = in_atomic() ? GFP_ATOMIC : GFP_KERNEL;
    printk("%s\n", __func__);

	nla_for_each_attr(iter, data, len, rem) {
		type = nla_type(iter);
		switch (type) {
		case MKEEP_ALIVE_ATTRIBUTE_ID:
			mkeep_alive_id = nla_get_u8(iter);
			break;
		case MKEEP_ALIVE_ATTRIBUTE_IP_PKT_LEN:
			ip_pkt_len = nla_get_u16(iter);
			if (ip_pkt_len > MKEEP_ALIVE_IP_PKT_MAX) {
				ret = BADARG;
				goto exit;
			}
			break;
		case MKEEP_ALIVE_ATTRIBUTE_IP_PKT:
			if (!ip_pkt_len) {
				ret = BADARG;
				printk("ip packet length is 0\n");
				goto exit;
			}
			ip_pkt = (u8 *)kzalloc(ip_pkt_len, kflags);
			if (ip_pkt == NULL) {
				ret = NOMEM;
				printk("Failed to allocate mem for ip packet\n");
				goto exit;
			}
			memcpy(ip_pkt, (u8 *)nla_data(iter), ip_pkt_len);
			break;
		case MKEEP_ALIVE_ATTRIBUTE_SRC_MAC_ADDR:
			memcpy(src_mac, nla_data(iter), 6);
			break;
		case MKEEP_ALIVE_ATTRIBUTE_DST_MAC_ADDR:
			memcpy(dst_mac, nla_data(iter), 6);
			break;
		case MKEEP_ALIVE_ATTRIBUTE_PERIOD_MSEC:
			period_msec = nla_get_u32(iter);
			break;
		default:
			printk("Unknown type: %d\n", type);
			ret = BADARG;
			goto exit;
		}
	}

	if (ip_pkt == NULL) {
		ret = BADARG;
		printk("ip packet is NULL\n");
		goto exit;
	}

	ret = aic_dev_start_mkeep_alive(rwnx_hw, rwnx_vif, mkeep_alive_id, ip_pkt, ip_pkt_len, src_mac,
		dst_mac, period_msec);
	if (ret < 0) {
		printk("start_mkeep_alive is failed ret: %d\n", ret);
	}

exit:
	if (ip_pkt) {
		kfree(ip_pkt);
	}

	return ret;
}

static int aicwf_vendor_stop_mkeep_alive(struct wiphy *wiphy, struct wireless_dev *wdev,
	const void *data, int len)
{
	int ret = 0, rem, type;
	u8 mkeep_alive_id = 0;
	const struct nlattr *iter;
	struct rwnx_hw *rwnx_hw = wiphy_priv(wiphy);
	struct rwnx_vif *rwnx_vif = container_of(wdev, struct rwnx_vif, wdev);

	printk("%s\n", __func__);
	nla_for_each_attr(iter, data, len, rem) {
		type = nla_type(iter);
		switch (type) {
		case MKEEP_ALIVE_ATTRIBUTE_ID:
			mkeep_alive_id = nla_get_u8(iter);
			break;
		default:
			printk("Unknown type: %d\n", type);
			ret = BADARG;
			break;
		}
	}

	ret = aic_dev_stop_mkeep_alive(rwnx_hw, rwnx_vif, mkeep_alive_id);
	if (ret < 0) {
		printk("stop_mkeep_alive is failed ret: %d\n", ret);
	}

	return ret;
}
#endif /* KEEP_ALIVE */

#if LINUX_VERSION_CODE >= KERNEL_VERSION(4, 19, 0)
static const struct nla_policy
aicwf_cfg80211_mkeep_alive_policy[MKEEP_ALIVE_ATTRIBUTE_MAX+1] = {
	[0] = {.type = NLA_UNSPEC },
	[MKEEP_ALIVE_ATTRIBUTE_ID]		= { .type = NLA_U8 },
	[MKEEP_ALIVE_ATTRIBUTE_IP_PKT]		= { .type = NLA_MSECS },
	[MKEEP_ALIVE_ATTRIBUTE_IP_PKT_LEN]	= { .type = NLA_U16 },
	[MKEEP_ALIVE_ATTRIBUTE_SRC_MAC_ADDR]	= { .type = NLA_MSECS,
						    .len  = ETH_ALEN },
	[MKEEP_ALIVE_ATTRIBUTE_DST_MAC_ADDR]	= { .type = NLA_MSECS,
						    .len  = ETH_ALEN },
	[MKEEP_ALIVE_ATTRIBUTE_PERIOD_MSEC]	= { .type = NLA_U32 },
};
#endif

static int aicwf_dump_interface(struct wiphy *wiphy,
				struct wireless_dev *wdev, struct sk_buff *skb,
				const void *data, int data_len,
				unsigned long *storage)
{
	return 0;
}


const struct wiphy_vendor_command aicwf_vendor_cmd[] = {
#ifdef KEEP_ALIVE
	{
		{
			.vendor_id = GOOGLE_OUI,
			.subcmd = WIFI_OFFLOAD_SUBCMD_START_MKEEP_ALIVE
		},
		.flags = WIPHY_VENDOR_CMD_NEED_WDEV | WIPHY_VENDOR_CMD_NEED_NETDEV,
		.doit = aicwf_vendor_start_mkeep_alive,
		.dumpit = aicwf_dump_interface,
#if LINUX_VERSION_CODE >= KERNEL_VERSION(4, 19, 0)
		.policy = aicwf_cfg80211_mkeep_alive_policy,
		.maxattr = MKEEP_ALIVE_ATTRIBUTE_MAX
#endif
	},
	{
		{
			.vendor_id = GOOGLE_OUI,
			.subcmd = WIFI_OFFLOAD_SUBCMD_STOP_MKEEP_ALIVE
		},
		.flags = WIPHY_VENDOR_CMD_NEED_WDEV | WIPHY_VENDOR_CMD_NEED_NETDEV,
		.doit = aicwf_vendor_stop_mkeep_alive,
		.dumpit = aicwf_dump_interface,
#if LINUX_VERSION_CODE >= KERNEL_VERSION(4, 19, 0)
		.policy = aicwf_cfg80211_mkeep_alive_policy,
		.maxattr = MKEEP_ALIVE_ATTRIBUTE_MAX
#endif
	},
#endif
};

static const struct nl80211_vendor_cmd_info aicwf_vendor_events[] = {
};

int aicwf_vendor_init(struct wiphy *wiphy)
{
	wiphy->vendor_commands = aicwf_vendor_cmd;
	wiphy->n_vendor_commands = ARRAY_SIZE(aicwf_vendor_cmd);
	wiphy->vendor_events = aicwf_vendor_events;
	wiphy->n_vendor_events = ARRAY_SIZE(aicwf_vendor_events);
	return 0;
}
