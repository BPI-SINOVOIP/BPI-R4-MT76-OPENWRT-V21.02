// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (c) 2023 MediaTek Inc. All Rights Reserved.
 *
 * Author: Ren-Ting Wang <ren-ting.wang@mediatek.com>
 */

#include "tops/internal.h"
#include "tops/protocol/network/ip.h"
#include "tops/protocol/transport/udp.h"

int mtk_tops_ip_encap_param_setup(
			struct sk_buff *skb,
			struct tops_params *params,
			int (*tnl_encap_param_setup)(struct sk_buff *skb,
						     struct tops_params *params))
{
	struct tops_ip_params *ipp = &params->network.ip;
	struct iphdr *ip;
	struct iphdr iph;
	int ret;

	ip = skb_header_pointer(skb, 0, sizeof(struct iphdr), &iph);
	if (unlikely(!ip))
		return -EINVAL;

	if (unlikely(ip->version != IPVERSION)) {
		TOPS_NOTICE("ip ver: 0x%x invalid\n", ip->version);
		return -EINVAL;
	}

	params->network.type = TOPS_NETWORK_IPV4;

	ipp->proto = ip->protocol;
	ipp->sip = ip->saddr;
	ipp->dip = ip->daddr;
	ipp->tos = ip->tos;
	ipp->ttl = ip->ttl;

	skb_pull(skb, sizeof(struct iphdr));

	switch (ip->protocol) {
	case IPPROTO_UDP:
		ret = mtk_tops_udp_encap_param_setup(skb,
						     params,
						     tnl_encap_param_setup);
		break;
	case IPPROTO_GRE:
		ret = tnl_encap_param_setup(skb, params);
		break;
	default:
		ret = -EINVAL;
		break;
	};

	skb_push(skb, sizeof(struct iphdr));

	return ret;
}

int mtk_tops_ip_decap_param_setup(struct sk_buff *skb, struct tops_params *params)
{
	struct tops_ip_params *ipp;
	struct iphdr *ip;
	struct iphdr iph;
	int ret;

	skb_push(skb, sizeof(struct iphdr));
	ip = skb_header_pointer(skb, 0, sizeof(struct iphdr), &iph);
	if (unlikely(!ip)) {
		ret = -EINVAL;
		goto out;
	}

	if (unlikely(ip->version != IPVERSION)) {
		ret = -EINVAL;
		goto out;
	}

	params->network.type = TOPS_NETWORK_IPV4;

	ipp = &params->network.ip;

	ipp->proto = ip->protocol;
	ipp->sip = ip->daddr;
	ipp->dip = ip->saddr;
	ipp->tos = ip->tos;
	/*
	 * if encapsulation parameter is already configured, TTL will remain as
	 * encapsulation's data
	 */
	ipp->ttl = 128;

	ret = mtk_tops_mac_decap_param_setup(skb, params);

out:
	skb_pull(skb, sizeof(struct iphdr));

	return ret;
}

static int tops_ip_debug_param_fetch_ip(const char *buf, int *ofs, u32 *ip)
{
	int nchar = 0;
	int ret = 0;
	u8 tmp[4];

	ret = sscanf(buf + *ofs, "%hhu.%hhu.%hhu.%hhu %n",
		&tmp[3], &tmp[2], &tmp[1], &tmp[0], &nchar);
	if (ret != 4)
		return -EPERM;

	*ip = tmp[3] | tmp[2] << 8 | tmp[1] << 16 | tmp[0] << 24;

	*ofs += nchar;

	return 0;
}

int mtk_tops_ip_debug_param_setup(const char *buf, int *ofs,
				  struct tops_params *params)
{
	char proto[DEBUG_PROTO_LEN] = {0};
	int ret;

	params->network.type = TOPS_NETWORK_IPV4;

	ret = tops_ip_debug_param_fetch_ip(buf, ofs, &params->network.ip.sip);
	if (ret)
		return ret;

	ret = tops_ip_debug_param_fetch_ip(buf, ofs, &params->network.ip.dip);
	if (ret)
		return ret;

	ret = mtk_tops_debug_param_proto_peek(buf, *ofs, proto);
	if (ret < 0)
		return ret;

	if (!strcmp(proto, DEBUG_PROTO_UDP)) {
		params->network.ip.proto = IPPROTO_UDP;
		*ofs += ret;
		ret = mtk_tops_udp_debug_param_setup(buf, ofs, params);
	} else if (!strcmp(proto, DEBUG_PROTO_GRETAP)) {
		params->network.ip.proto = IPPROTO_GRE;
		ret = 0;
	} else {
		ret = -EINVAL;
	}

	return ret;
}

void mtk_tops_ip_param_dump(struct seq_file *s, struct tops_params *params)
{
	struct tops_ip_params *ipp = &params->network.ip;
	u32 sip = params->network.ip.sip;
	u32 dip = params->network.ip.dip;

	seq_puts(s, "\tNetwork Type: IPv4 ");
	seq_printf(s, "sip: %pI4 dip: %pI4 protocol: 0x%02x tos: 0x%02x ttl: %03u\n",
		   &sip, &dip, ipp->proto, ipp->tos, ipp->ttl);
}
