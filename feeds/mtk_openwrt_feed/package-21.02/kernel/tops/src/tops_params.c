// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (c) 2023 MediaTek Inc. All Rights Reserved.
 *
 * Author: Ren-Ting Wang <ren-ting.wang@mediatek.com>
 */

#include "tops/tops_params.h"

#include "tops/protocol/mac/eth.h"
#include "tops/protocol/network/ip.h"
#include "tops/protocol/transport/udp.h"

int
mtk_tops_encap_param_setup(struct sk_buff *skb,
			   struct tops_params *params,
			   int (*tnl_encap_param_setup)(struct sk_buff *skb,
							struct tops_params *params))
{
	return mtk_tops_eth_encap_param_setup(skb, params, tnl_encap_param_setup);
}

int
mtk_tops_decap_param_setup(struct sk_buff *skb,
			   struct tops_params *params,
			   int (*tnl_decap_param_setup)(struct sk_buff *skb,
							struct tops_params *params))
{
	return tnl_decap_param_setup(skb, params);
}

int mtk_tops_transport_decap_param_setup(struct sk_buff *skb,
					 struct tops_params *params)
{
	return mtk_tops_udp_decap_param_setup(skb, params);
}

int mtk_tops_network_decap_param_setup(struct sk_buff *skb,
				       struct tops_params *params)
{
	/* TODO: IPv6 */
	return mtk_tops_ip_decap_param_setup(skb, params);
}

int mtk_tops_mac_decap_param_setup(struct sk_buff *skb,
				   struct tops_params *params)
{
	return mtk_tops_eth_decap_param_setup(skb, params);
}

int mtk_tops_debug_param_proto_peek(const char *buf, int ofs, char *proto)
{
	int nchar = 0;
	int ret;

	if (!proto)
		return -EINVAL;

	ret = sscanf(buf + ofs, "%20s %n", proto, &nchar);
	if (ret != 1)
		return -EPERM;

	return nchar;
}

int mtk_tops_debug_param_setup(const char *buf, int *ofs,
				   struct tops_params *params)
{
	char proto[DEBUG_PROTO_LEN];
	int ret;

	memset(proto, 0, sizeof(proto));

	ret = mtk_tops_debug_param_proto_peek(buf, *ofs, proto);
	if (ret < 0)
		return ret;

	*ofs += ret;

	if (!strcmp(proto, DEBUG_PROTO_ETH))
		return mtk_tops_eth_debug_param_setup(buf, ofs, params);

	/* not support mac protocols other than Ethernet */
	return -EINVAL;
}

void mtk_tops_mac_param_dump(struct seq_file *s, struct tops_params *params)
{
	if (params->mac.type == TOPS_MAC_ETH)
		mtk_tops_eth_param_dump(s, params);
}

void mtk_tops_network_param_dump(struct seq_file *s, struct tops_params *params)
{
	if (params->network.type == TOPS_NETWORK_IPV4)
		mtk_tops_ip_param_dump(s, params);
}

void mtk_tops_transport_param_dump(struct seq_file *s, struct tops_params *params)
{
	if (params->transport.type == TOPS_TRANSPORT_UDP)
		mtk_tops_udp_param_dump(s, params);
}

static bool tops_transport_params_match(struct tops_transport_params *t1,
					struct tops_transport_params *t2)
{
	return !memcmp(t1, t2, sizeof(*t1));
}

static bool tops_network_params_match(struct tops_network_params *n1,
				      struct tops_network_params *n2)
{
	if (n1->type != n2->type)
		return false;

	if (n1->type == TOPS_NETWORK_IPV4)
		return (n1->ip.sip == n2->ip.sip
			&& n1->ip.dip == n2->ip.dip
			&& n1->ip.proto == n2->ip.proto
			&& n1->ip.tos == n2->ip.tos);

	/* TODO: support IPv6 */
	return false;
}

bool mtk_tops_params_match(struct tops_params *p1, struct tops_params *p2)
{
	return (tops_network_params_match(&p1->network, &p2->network)
		&& tops_transport_params_match(&p1->transport, &p2->transport));
}
