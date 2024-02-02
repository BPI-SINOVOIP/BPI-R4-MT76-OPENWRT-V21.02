// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (c) 2023 MediaTek Inc. All Rights Reserved.
 *
 * Author: Ren-Ting Wang <ren-ting.wang@mediatek.com>
 */

#include "tops/protocol/network/ip.h"
#include "tops/protocol/transport/udp.h"

int mtk_tops_udp_encap_param_setup(
			struct sk_buff *skb,
			struct tops_params *params,
			int (*tnl_encap_param_setup)(struct sk_buff *skb,
						     struct tops_params *params))
{
	struct tops_udp_params *udpp = &params->transport.udp;
	struct udphdr *udp;
	struct udphdr udph;
	int ret;

	udp = skb_header_pointer(skb, 0, sizeof(struct udphdr), &udph);
	if (unlikely(!udp))
		return -EINVAL;

	params->transport.type = TOPS_TRANSPORT_UDP;

	udpp->sport = udp->source;
	udpp->dport = udp->dest;

	skb_pull(skb, sizeof(struct udphdr));

	/* udp must be the end of a tunnel */
	ret = tnl_encap_param_setup(skb, params);

	skb_push(skb, sizeof(struct udphdr));

	return ret;
}

int mtk_tops_udp_decap_param_setup(struct sk_buff *skb, struct tops_params *params)
{
	struct tops_udp_params *udpp = &params->transport.udp;
	struct udphdr *udp;
	struct udphdr udph;
	int ret;

	skb_push(skb, sizeof(struct udphdr));
	udp = skb_header_pointer(skb, 0, sizeof(struct udphdr), &udph);
	if (unlikely(!udp)) {
		ret = -EINVAL;
		goto out;
	}

	params->transport.type = TOPS_TRANSPORT_UDP;

	udpp->sport = udp->dest;
	udpp->dport = udp->source;

	ret = mtk_tops_network_decap_param_setup(skb, params);

out:
	skb_pull(skb, sizeof(struct udphdr));

	return ret;
}

static int tops_udp_debug_param_fetch_port(const char *buf, int *ofs, u16 *port)
{
	int nchar = 0;
	int ret;
	u16 p = 0;

	ret = sscanf(buf + *ofs, "%hu %n", &p, &nchar);
	if (ret != 1)
		return -EPERM;

	*port = htons(p);

	*ofs += nchar;

	return 0;
}

int mtk_tops_udp_debug_param_setup(const char *buf, int *ofs,
				   struct tops_params *params)
{
	int ret;

	params->transport.type = TOPS_TRANSPORT_UDP;

	ret = tops_udp_debug_param_fetch_port(buf, ofs, &params->transport.udp.sport);
	if (ret)
		return ret;

	ret = tops_udp_debug_param_fetch_port(buf, ofs, &params->transport.udp.dport);
	if (ret)
		return ret;

	return ret;
}

void mtk_tops_udp_param_dump(struct seq_file *s, struct tops_params *params)
{
	struct tops_udp_params *udpp = &params->transport.udp;

	seq_puts(s, "\tTransport Type: UDP ");
	seq_printf(s, "sport: %05u dport: %05u\n",
		   ntohs(udpp->sport), ntohs(udpp->dport));
}
