#ifndef _AIC_VENDOR_H
#define _AIC_VENDOR_H

#include <linux/types.h>


typedef enum {
    START_MKEEP_ALIVE,
    STOP_MKEEP_ALIVE,
} GetCmdType;


typedef enum {
    /* don't use 0 as a valid subcommand */
    VENDOR_NL80211_SUBCMD_UNSPECIFIED,

    /* define all vendor startup commands between 0x0 and 0x0FFF */
    VENDOR_NL80211_SUBCMD_RANGE_START = 0x0001,
    VENDOR_NL80211_SUBCMD_RANGE_END   = 0x0FFF,

    /* define all GScan related commands between 0x1000 and 0x10FF */
    ANDROID_NL80211_SUBCMD_GSCAN_RANGE_START = 0x1000,
    ANDROID_NL80211_SUBCMD_GSCAN_RANGE_END   = 0x10FF,

    /* define all NearbyDiscovery related commands between 0x1100 and 0x11FF */
    ANDROID_NL80211_SUBCMD_NBD_RANGE_START = 0x1100,
    ANDROID_NL80211_SUBCMD_NBD_RANGE_END   = 0x11FF,

    /* define all RTT related commands between 0x1100 and 0x11FF */
    ANDROID_NL80211_SUBCMD_RTT_RANGE_START = 0x1100,
    ANDROID_NL80211_SUBCMD_RTT_RANGE_END   = 0x11FF,

    ANDROID_NL80211_SUBCMD_LSTATS_RANGE_START = 0x1200,
    ANDROID_NL80211_SUBCMD_LSTATS_RANGE_END   = 0x12FF,

    /* define all Logger related commands between 0x1400 and 0x14FF */
    ANDROID_NL80211_SUBCMD_DEBUG_RANGE_START = 0x1400,
    ANDROID_NL80211_SUBCMD_DEBUG_RANGE_END   = 0x14FF,

    /* define all wifi offload related commands between 0x1600 and 0x16FF */
    ANDROID_NL80211_SUBCMD_WIFI_OFFLOAD_RANGE_START = 0x1600,
    ANDROID_NL80211_SUBCMD_WIFI_OFFLOAD_RANGE_END   = 0x16FF,

    /* define all NAN related commands between 0x1700 and 0x17FF */
    ANDROID_NL80211_SUBCMD_NAN_RANGE_START = 0x1700,
    ANDROID_NL80211_SUBCMD_NAN_RANGE_END   = 0x17FF,

    /* define all Android Packet Filter related commands between 0x1800 and 0x18FF */
    ANDROID_NL80211_SUBCMD_PKT_FILTER_RANGE_START = 0x1800,
    ANDROID_NL80211_SUBCMD_PKT_FILTER_RANGE_END   = 0x18FF,

    /* This is reserved for future usage */

} ANDROID_VENDOR_SUB_COMMAND;

typedef enum {
    WIFI_OFFLOAD_SUBCMD_START_MKEEP_ALIVE = ANDROID_NL80211_SUBCMD_WIFI_OFFLOAD_RANGE_START,
    WIFI_OFFLOAD_SUBCMD_STOP_MKEEP_ALIVE,
} WIFI_OFFLOAD_SUB_COMMAND;


enum mkeep_alive_attributes {
	MKEEP_ALIVE_ATTRIBUTE_ID = 0x1,
	MKEEP_ALIVE_ATTRIBUTE_IP_PKT,
	MKEEP_ALIVE_ATTRIBUTE_IP_PKT_LEN,
	MKEEP_ALIVE_ATTRIBUTE_SRC_MAC_ADDR,
	MKEEP_ALIVE_ATTRIBUTE_DST_MAC_ADDR,
	MKEEP_ALIVE_ATTRIBUTE_PERIOD_MSEC,
	MKEEP_ALIVE_ATTRIBUTE_AFTER_LAST,
	MKEEP_ALIVE_ATTRIBUTE_MAX = MKEEP_ALIVE_ATTRIBUTE_AFTER_LAST - 1
};


#define BADARG			-2	/* Bad Argument */
#define NOMEM			-27	/* No Memory */

typedef struct wl_mkeep_alive_pkt {
	u16	version; /* Version for mkeep_alive */
	u16	length; /* length of fixed parameters in the structure */
	u32	period_msec; /* high bit on means immediate send */
	u16	len_bytes;
	u8	keep_alive_id; /* 0 - 3 for N = 4 */
	u8	data[1];
} wl_mkeep_alive_pkt_t;




#endif /* _AIC_VENDOR_H */

