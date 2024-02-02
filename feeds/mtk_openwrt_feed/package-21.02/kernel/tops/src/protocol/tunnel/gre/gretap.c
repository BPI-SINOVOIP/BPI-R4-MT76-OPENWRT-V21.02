// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (c) 2023 MediaTek Inc. All Rights Reserved.
 *
 * Author: Ren-Ting Wang <ren-ting.wang@mediatek.com>
 */

#include <net/gre.h>

#include <pce/cls.h>
#include <pce/netsys.h>
#include <pce/pce.h>

#include "tops/internal.h"
#include "tops/protocol/tunnel/gre/gretap.h"
#include "tops/tunnel.h"

static int gretap_cls_entry_setup(struct tops_tnl_info *tnl_info,
				  struct cls_desc *cdesc)
{
	CLS_DESC_DATA(cdesc, fport, PSE_PORT_PPE0);
	CLS_DESC_DATA(cdesc, tport_idx, 0x4);
	CLS_DESC_MASK_DATA(cdesc, tag, CLS_DESC_TAG_MASK, CLS_DESC_TAG_MATCH_L4_HDR);
	CLS_DESC_MASK_DATA(cdesc, dip_match, CLS_DESC_DIP_MATCH, CLS_DESC_DIP_MATCH);
	CLS_DESC_MASK_DATA(cdesc, l4_type, CLS_DESC_L4_TYPE_MASK, IPPROTO_GRE);
	CLS_DESC_MASK_DATA(cdesc, l4_udp_hdr_nez,
			   CLS_DESC_UDPLITE_L4_HDR_NEZ_MASK,
			   CLS_DESC_UDPLITE_L4_HDR_NEZ_MASK);
	CLS_DESC_MASK_DATA(cdesc, l4_valid,
			   CLS_DESC_L4_VALID_MASK,
			   CLS_DESC_VALID_UPPER_HALF_WORD_BIT |
			   CLS_DESC_VALID_LOWER_HALF_WORD_BIT);
	CLS_DESC_MASK_DATA(cdesc, l4_hdr_usr_data, 0x0000FFFF, 0x00006558);

	return 0;
}

static int gretap_tnl_encap_param_setup(struct sk_buff *skb, struct tops_params *params)
{
	params->tunnel.type = TOPS_TUNNEL_GRETAP;

	return 0;
}

static int gretap_tnl_decap_param_setup(struct sk_buff *skb, struct tops_params *params)
{
	struct gre_base_hdr *pgre;
	struct gre_base_hdr greh;
	int ret;

	if (!skb->dev->rtnl_link_ops
	    || strcmp(skb->dev->rtnl_link_ops->kind, "gretap"))
		return -EAGAIN;

	skb_push(skb, sizeof(struct gre_base_hdr));
	pgre = skb_header_pointer(skb, 0, sizeof(struct gre_base_hdr), &greh);
	if (unlikely(!pgre)) {
		ret = -EINVAL;
		goto out;
	}

	if (unlikely(ntohs(pgre->protocol) != ETH_P_TEB)) {
		TOPS_NOTICE("gre: %p protocol unmatched, proto: 0x%x\n",
			    pgre, ntohs(pgre->protocol));
		ret = -EINVAL;
		goto out;
	}

	params->tunnel.type = TOPS_TUNNEL_GRETAP;

	ret = mtk_tops_network_decap_param_setup(skb, params);

out:
	skb_pull(skb, sizeof(struct gre_base_hdr));

	return ret;
}

static int gretap_tnl_debug_param_setup(const char *buf, int *ofs,
					struct tops_params *params)
{
	params->tunnel.type = TOPS_TUNNEL_GRETAP;

	return 0;
}

static bool gretap_tnl_decap_offloadable(struct sk_buff *skb)
{
	struct iphdr *ip = ip_hdr(skb);
	struct gre_base_hdr *pgre;
	struct gre_base_hdr greh;

	if (ip->protocol != IPPROTO_GRE)
		return false;

	pgre = skb_header_pointer(skb, ip_hdr(skb)->ihl * 4,
				  sizeof(struct gre_base_hdr), &greh);
	if (unlikely(!pgre))
		return false;

	if (ntohs(pgre->protocol) != ETH_P_TEB)
		return false;

	return true;
}

static void gretap_tnl_param_dump(struct seq_file *s, struct tops_params *params)
{
	seq_puts(s, "\tTunnel Type: GRETAP\n");
}

static bool gretap_tnl_param_match(struct tops_params *p, struct tops_params *target)
{
	return !memcmp(&p->tunnel, &target->tunnel, sizeof(struct tops_tunnel_params));
}

static struct tops_tnl_type gretap_type = {
	.type_name = "gretap",
	.cls_entry_setup = gretap_cls_entry_setup,
	.tnl_decap_param_setup = gretap_tnl_decap_param_setup,
	.tnl_encap_param_setup = gretap_tnl_encap_param_setup,
	.tnl_debug_param_setup = gretap_tnl_debug_param_setup,
	.tnl_decap_offloadable = gretap_tnl_decap_offloadable,
	.tnl_param_match = gretap_tnl_param_match,
	.tnl_param_dump = gretap_tnl_param_dump,
	.tnl_proto_type = TOPS_TUNNEL_GRETAP,
	.has_inner_eth = true,
};

int mtk_tops_gretap_init(void)
{
	return mtk_tops_tnl_type_register(&gretap_type);
}

void mtk_tops_gretap_deinit(void)
{
	mtk_tops_tnl_type_unregister(&gretap_type);
}
