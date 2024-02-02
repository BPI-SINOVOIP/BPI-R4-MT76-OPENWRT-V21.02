// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (c) 2023 MediaTek Inc. All Rights Reserved.
 *
 * Author: Ren-Ting Wang <ren-ting.wang@mediatek.com>
 */

#include "tops/internal.h"
#include "tops/protocol/mac/eth.h"
#include "tops/protocol/network/ip.h"

#include <mtk_hnat/nf_hnat_mtk.h>

int
mtk_tops_eth_encap_param_setup(
			struct sk_buff *skb,
			struct tops_params *params,
			int (*tnl_encap_param_setup)(struct sk_buff *skb,
						     struct tops_params *params))
{
	struct ethhdr *eth = eth_hdr(skb);

	params->mac.type = TOPS_MAC_ETH;

	memcpy(&params->mac.eth.h_source, eth->h_source, ETH_ALEN);
	memcpy(&params->mac.eth.h_dest, eth->h_dest, ETH_ALEN);
	params->mac.eth.h_proto = htons(ETH_P_IP);

	/*
	 * either has contrusted ethernet header with IP
	 * or the packet is going to do xfrm encryption
	 */
	if ((ntohs(eth->h_proto) == ETH_P_IP)
	    || (!skb_hnat_cdrt(skb) && skb_dst(skb) && dst_xfrm(skb_dst(skb)))) {
		return mtk_tops_ip_encap_param_setup(skb,
						     params,
						     tnl_encap_param_setup);
	}

	TOPS_NOTICE("eth proto not support, proto: 0x%x\n",
		    ntohs(eth->h_proto));

	return -EINVAL;
}

int mtk_tops_eth_decap_param_setup(struct sk_buff *skb, struct tops_params *params)
{
	struct ethhdr *eth;
	struct ethhdr ethh;
	int ret = 0;

	skb_push(skb, sizeof(struct ethhdr));
	eth = skb_header_pointer(skb, 0, sizeof(struct ethhdr), &ethh);
	if (unlikely(!eth)) {
		ret = -EINVAL;
		goto out;
	}

	if (unlikely(ntohs(eth->h_proto) != ETH_P_IP)) {
		TOPS_NOTICE("eth proto not support, proto: 0x%x\n",
			    ntohs(eth->h_proto));
		ret = -EINVAL;
		goto out;
	}

	params->mac.type = TOPS_MAC_ETH;

	memcpy(&params->mac.eth.h_source, eth->h_dest, ETH_ALEN);
	memcpy(&params->mac.eth.h_dest, eth->h_source, ETH_ALEN);
	params->mac.eth.h_proto = htons(ETH_P_IP);

out:
	skb_pull(skb, sizeof(struct ethhdr));

	return ret;
}

static int tops_eth_debug_param_fetch_mac(const char *buf, int *ofs, u8 *mac)
{
	int nchar = 0;
	int ret;

	ret = sscanf(buf + *ofs, "%hhx:%hhx:%hhx:%hhx:%hhx:%hhx %n",
		&mac[0], &mac[1], &mac[2], &mac[3], &mac[4], &mac[5], &nchar);
	if (ret != 6)
		return -EPERM;

	*ofs += nchar;

	return 0;
}

int mtk_tops_eth_debug_param_setup(const char *buf, int *ofs,
				   struct tops_params *params)
{
	char proto[DEBUG_PROTO_LEN] = {0};
	int ret;

	params->mac.type = TOPS_MAC_ETH;

	ret = tops_eth_debug_param_fetch_mac(buf, ofs, params->mac.eth.h_source);
	if (ret)
		return ret;

	ret = tops_eth_debug_param_fetch_mac(buf, ofs, params->mac.eth.h_dest);
	if (ret)
		return ret;

	ret = mtk_tops_debug_param_proto_peek(buf, *ofs, proto);
	if (ret < 0)
		return ret;

	*ofs += ret;

	if (!strcmp(proto, DEBUG_PROTO_IP)) {
		params->mac.eth.h_proto = htons(ETH_P_IP);
		ret = mtk_tops_ip_debug_param_setup(buf, ofs, params);
	} else {
		ret = -EINVAL;
	}

	return ret;
}

void mtk_tops_eth_param_dump(struct seq_file *s, struct tops_params *params)
{
	seq_puts(s, "\tMAC Type: Ethernet ");
	seq_printf(s, "saddr: %pM daddr: %pM\n",
		   params->mac.eth.h_source, params->mac.eth.h_dest);
}
