/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * Copyright (c) 2023 MediaTek Inc. All Rights Reserved.
 *
 * Author: Ren-Ting Wang <ren-ting.wang@mediatek.com>
 */

#ifndef _TOPS_PARAMS_H_
#define _TOPS_PARAMS_H_

#include <linux/if_ether.h>
#include <linux/seq_file.h>
#include <linux/types.h>

#include "tops/protocol/network/ip_params.h"
#include "tops/protocol/transport/udp_params.h"
#include "tops/protocol/tunnel/l2tp/l2tp_params.h"
#include "tops/protocol/tunnel/pptp/pptp_params.h"

/* tunnel params flags */
#define TNL_DECAP_ENABLE	(BIT(TNL_PARAMS_DECAP_ENABLE_BIT))
#define TNL_ENCAP_ENABLE	(BIT(TNL_PARAMS_ENCAP_ENABLE_BIT))

#define DEBUG_PROTO_LEN		(21)
#define DEBUG_PROTO_ETH		"eth"
#define DEBUG_PROTO_IP		"ipv4"
#define DEBUG_PROTO_UDP		"udp"
#define DEBUG_PROTO_GRETAP	"gretap"
#define DEBUG_PROTO_L2TP_V2	"l2tpv2"

enum tops_mac_type {
	TOPS_MAC_NONE,
	TOPS_MAC_ETH,

	__TOPS_MAC_TYPE_MAX,
};

enum tops_network_type {
	TOPS_NETWORK_NONE,
	TOPS_NETWORK_IPV4,

	__TOPS_NETWORK_TYPE_MAX,
};

enum tops_transport_type {
	TOPS_TRANSPORT_NONE,
	TOPS_TRANSPORT_UDP,

	__TOPS_TRANSPORT_TYPE_MAX,
};

enum tops_tunnel_type {
	TOPS_TUNNEL_NONE = 0,
	TOPS_TUNNEL_GRETAP,
	TOPS_TUNNEL_PPTP,
	TOPS_TUNNEL_L2TP_V2,
	TOPS_TUNNEL_L2TP_V3 = 5,
	TOPS_TUNNEL_VXLAN,
	TOPS_TUNNEL_NATT,
	TOPS_TUNNEL_CAPWAP_CTRL,
	TOPS_TUNNEL_CAPWAP_DATA,
	TOPS_TUNNEL_CAPWAP_DTLS = 10,
	TOPS_TUNNEL_IPSEC_ESP,
	TOPS_TUNNEL_IPSEC_AH,

	__TOPS_TUNNEL_TYPE_MAX = CONFIG_TOPS_TNL_TYPE_NUM,
};

enum tops_tnl_params_flag {
	TNL_PARAMS_DECAP_ENABLE_BIT,
	TNL_PARAMS_ENCAP_ENABLE_BIT,
};

struct tops_mac_params {
	union {
		struct ethhdr eth;
	};
	enum tops_mac_type type;
};

struct tops_network_params {
	union {
		struct tops_ip_params ip;
	};
	enum tops_network_type type;
};

struct tops_transport_params {
	union {
		struct tops_udp_params udp;
	};
	enum tops_transport_type type;
};

struct tops_tunnel_params {
	union {
		struct tops_l2tp_params l2tp;
		struct tops_pptp_params pptp;
	};
	enum tops_tunnel_type type;
};

struct tops_params {
	struct tops_mac_params mac;
	struct tops_network_params network;
	struct tops_transport_params transport;
	struct tops_tunnel_params tunnel;
};

/* record outer tunnel header data for HW offloading */
struct tops_tnl_params {
	struct tops_params params;
	u8 tops_entry_proto;
	u8 cls_entry;
	u8 cdrt;
	u8 flag; /* bit: enum tops_tnl_params_flag */
} __packed __aligned(16);

int
mtk_tops_encap_param_setup(struct sk_buff *skb,
			   struct tops_params *params,
			   int (*tnl_encap_param_setup)(struct sk_buff *skb,
							struct tops_params *params));
int
mtk_tops_decap_param_setup(struct sk_buff *skb,
			   struct tops_params *params,
			   int (*tnl_decap_param_setup)(struct sk_buff *skb,
							struct tops_params *params));

int mtk_tops_transport_decap_param_setup(struct sk_buff *skb,
					 struct tops_params *params);
int mtk_tops_network_decap_param_setup(struct sk_buff *skb,
				       struct tops_params *params);
int mtk_tops_mac_decap_param_setup(struct sk_buff *skb,
				       struct tops_params *params);

int mtk_tops_debug_param_proto_peek(const char *buf, int ofs, char *out);
int mtk_tops_debug_param_setup(const char *buf, int *ofs,
				   struct tops_params *params);
void mtk_tops_mac_param_dump(struct seq_file *s, struct tops_params *params);
void mtk_tops_network_param_dump(struct seq_file *s, struct tops_params *params);
void mtk_tops_transport_param_dump(struct seq_file *s, struct tops_params *params);

bool mtk_tops_params_match(struct tops_params *p1, struct tops_params *p2);
#endif /* _TOPS_PARAMS_H_ */
