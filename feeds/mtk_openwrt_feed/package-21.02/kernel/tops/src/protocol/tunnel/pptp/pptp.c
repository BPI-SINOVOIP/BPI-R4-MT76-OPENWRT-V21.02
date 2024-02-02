// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (c) 2023 MediaTek Inc. All Rights Reserved.
 *
 * Author: Frank-zj Lin <frank-zj.lin@mediatek.com>
 */

#include <linux/if_pppox.h>
#include <linux/ppp_channel.h>

#include <net/gre.h>
#include <net/pptp.h>
#include <net/sock.h>

#include <pce/cls.h>
#include <pce/netsys.h>
#include <pce/pce.h>

#include "tops/protocol/mac/ppp.h"
#include "tops/protocol/tunnel/pptp/pptp.h"
#include "tops/seq_gen.h"
#include "tops/tunnel.h"

static int pptp_cls_entry_setup(struct tops_tnl_info *tnl_info,
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
	CLS_DESC_MASK_DATA(cdesc, l4_hdr_usr_data, 0x0000FFFF, 0x0000880B);

	return 0;
}

/*
 * If a sequence generator is already allocated for this tunnel (call_id),
 * return with seq_gen_idx set. Otherwise, allocate a new sequence generator
 * and set the starting sequence number.
 */
static int pptp_get_seq_gen_idx(uint16_t call_id, uint32_t seq_start,
				int *seq_gen_idx)
{
	int ret;

	ret = mtk_tops_pptp_seq_get_seq_gen_idx(call_id, seq_gen_idx);
	if (ret)
		ret = mtk_tops_pptp_seq_alloc(call_id, seq_start, seq_gen_idx);

	return ret;
}

static inline bool pptp_gre_offload_valid(struct sk_buff *skb)
{
	struct pptp_gre_header *pptp_gre;
	struct pptp_gre_header pptp_greh;

	pptp_gre = skb_header_pointer(skb, 0, sizeof(struct pptp_gre_header), &pptp_greh);
	if (unlikely(!pptp_gre))
		return false;

	if (pptp_gre->gre_hd.protocol != GRE_PROTO_PPP
	    || pptp_gre->payload_len < sizeof(struct ppp_hdr)
	    || GRE_IS_CSUM(pptp_gre->gre_hd.flags)	/* flag CSUM should be clear */
	    || GRE_IS_ROUTING(pptp_gre->gre_hd.flags)	/* flag ROUTING should be clear */
	    || !GRE_IS_KEY(pptp_gre->gre_hd.flags)	/* flag KEY should be set */
	    || pptp_gre->gre_hd.flags & GRE_FLAGS)	/* flag Recursion Ctrl should be clear */
		return false;

	return true;
}

static inline int pptp_gre_len_evaluate(struct sk_buff *skb)
{
	static const int possible_greh_len[] = {
		sizeof(struct pptp_gre_header) - PPTP_GRE_HDR_ACK_LEN,
		sizeof(struct pptp_gre_header),
	};
	struct pptp_gre_header *pptp_gre;
	int pptp_gre_len;
	int i;

	for (i = 0; i < ARRAY_SIZE(possible_greh_len); i++) {
		pptp_gre_len = possible_greh_len[i];

		skb_push(skb, pptp_gre_len);
		pptp_gre = (struct pptp_gre_header *)skb->data;
		skb_pull(skb, pptp_gre_len);

		if (pptp_gre->gre_hd.protocol == GRE_PROTO_PPP)
			return pptp_gre_len;
	}

	return -EINVAL;
}

static int pptp_tnl_decap_param_setup(struct sk_buff *skb,
				      struct tops_params *params)
{
	struct pptp_gre_header *pptp_gre;
	struct pptp_gre_header pptp_greh;
	struct tops_pptp_params *pptpp;
	struct sock *sk;
	int pptp_gre_len;
	int ret = 0;

	/* ppp */
	skb_push(skb, sizeof(struct ppp_hdr));
	if (unlikely(!mtk_tops_ppp_valid(skb))) {
		ret = -EINVAL;
		goto restore_ppp;
	}

	/* pptp_gre */
	pptp_gre_len = pptp_gre_len_evaluate(skb);
	if (pptp_gre_len < 0) {
		ret = -EINVAL;
		goto restore_ppp;
	}

	skb_push(skb, pptp_gre_len);
	pptp_gre = skb_header_pointer(skb, 0, pptp_gre_len, &pptp_greh);
	if (unlikely(!pptp_gre)) {
		ret = -EINVAL;
		goto restore_pptp_gre;
	}

	if (unlikely(!pptp_gre_offload_valid(skb))) {
		ret = -EINVAL;
		goto restore_pptp_gre;
	}

	/*
	 * In decap setup, dl_call_id is fetched from the skb and ul_call_id is
	 * fetched from socket struct of ppp device.
	 */
	sk = ppp_netdev_get_sock(skb->dev);
	if (IS_ERR(sk)) {
		ret = PTR_ERR(sk);
		goto restore_pptp_gre;
	}

	params->tunnel.type = TOPS_TUNNEL_PPTP;
	pptpp = &params->tunnel.pptp;
	pptpp->dl_call_id = pptp_gre->call_id;
	pptpp->ul_call_id = htons(pppox_sk(sk)->proto.pptp.dst_addr.call_id);

	ret = mtk_tops_network_decap_param_setup(skb, params);

restore_pptp_gre:
	skb_pull(skb, pptp_gre_len);

restore_ppp:
	skb_pull(skb, sizeof(struct ppp_hdr));

	return ret;
}

static int pptp_tnl_encap_param_setup(struct sk_buff *skb,
				      struct tops_params *params)
{
	struct pptp_gre_header *pptp_gre;
	struct pptp_gre_header pptp_greh;
	struct tops_pptp_params *pptpp;
	uint32_t pptp_gre_len;
	int seq_gen_idx;
	int ret = 0;

	if (unlikely(!pptp_gre_offload_valid(skb)))
		return -EINVAL;

	pptp_gre = skb_header_pointer(skb, 0, sizeof(struct pptp_gre_header), &pptp_greh);
	if (unlikely(!pptp_gre))
		return -EINVAL;

	pptp_gre_len = sizeof(*pptp_gre);
	if (!(GRE_IS_ACK(pptp_gre->gre_hd.flags)))
		pptp_gre_len -= sizeof(pptp_gre->ack);

	skb_pull(skb, pptp_gre_len);

	/* check ppp */
	if (unlikely(!mtk_tops_ppp_valid(skb))) {
		ret = -EINVAL;
		goto restore_pptp_gre;
	}

	ret = pptp_get_seq_gen_idx(ntohs(pptp_gre->call_id),
				   ntohl(pptp_gre->seq), &seq_gen_idx);
	if (ret)
		goto restore_pptp_gre;

	params->tunnel.type = TOPS_TUNNEL_PPTP;
	pptpp = &params->tunnel.pptp;
	pptpp->seq_gen_idx = (u8)seq_gen_idx;
	pptpp->ul_call_id = pptp_gre->call_id;

restore_pptp_gre:
	skb_push(skb, pptp_gre_len);

	return ret;
}

static int pptp_debug_param_fetch_call_id(const char *buf, int *ofs, u16 *call_id)
{
	int nchar = 0;
	int ret;
	u16 c = 0;

	ret = sscanf(buf + *ofs, "%hu %n", &c, &nchar);
	if (ret != 1)
		return -EPERM;

	*call_id = htons(c);

	*ofs += nchar;

	return 0;
}

static int pptp_tnl_debug_param_setup(const char *buf, int *ofs,
				      struct tops_params *params)
{
	struct tops_pptp_params *pptpp;
	int seq_gen_idx;
	int ret;

	pptpp = &params->tunnel.pptp;

	ret = pptp_debug_param_fetch_call_id(buf, ofs, &pptpp->ul_call_id);
	if (ret)
		return ret;

	ret = pptp_debug_param_fetch_call_id(buf, ofs, &pptpp->dl_call_id);
	if (ret)
		return ret;

	ret = pptp_get_seq_gen_idx(ntohs(pptpp->ul_call_id), 0, &seq_gen_idx);
	if (ret)
		return ret;

	pptpp->seq_gen_idx = (u8)seq_gen_idx;

	return 0;
}

static bool pptp_tnl_decap_offloadable(struct sk_buff *skb)
{
	struct pptp_gre_header *pptp_gre;
	struct pptp_gre_header pptp_greh;
	struct iphdr *ip;
	int pptp_gre_len;
	int ip_len;
	bool ret = true;

	/* check ip */
	ip = ip_hdr(skb);
	if (ip->protocol != IPPROTO_GRE)
		return false;

	ip_len = ip_hdr(skb)->ihl * 4;

	skb_pull(skb, ip_len);

	/* check gre */
	if (!pptp_gre_offload_valid(skb)) {
		ret = false;
		goto restore_ip;
	}

	pptp_gre = skb_header_pointer(skb, 0, sizeof(struct pptp_gre_header), &pptp_greh);
	if (unlikely(!pptp_gre)) {
		ret = false;
		goto restore_ip;
	}

	pptp_gre_len = sizeof(*pptp_gre);
	if (!(GRE_IS_ACK(pptp_gre->gre_hd.flags)))
		pptp_gre_len -= sizeof(pptp_gre->ack);

	skb_pull(skb, pptp_gre_len);

	/* check ppp */
	if (unlikely(!mtk_tops_ppp_valid(skb))) {
		ret = false;
		goto restore_pptp_gre;
	}

restore_pptp_gre:
	skb_push(skb, pptp_gre_len);

restore_ip:
	skb_push(skb, ip_len);

	return ret;
}

static void pptp_tnl_param_restore(struct tops_params *old, struct tops_params *new)
{
	/* dl_call_id is assigned at decap */
	if (old->tunnel.pptp.dl_call_id)
		new->tunnel.pptp.dl_call_id = old->tunnel.pptp.dl_call_id;

	if (old->tunnel.pptp.ul_call_id)
		new->tunnel.pptp.ul_call_id = old->tunnel.pptp.ul_call_id;

	/* seq_gen_idx is assigned at encap */
	if (old->tunnel.pptp.seq_gen_idx)
		new->tunnel.pptp.seq_gen_idx = old->tunnel.pptp.seq_gen_idx;
}

static bool pptp_tnl_param_match(struct tops_params *p, struct tops_params *target)
{
	/*
	 * Only ul_call_id is guaranteed to be valid for comparison, dl_call_id
	 * may be left empty if no DL traffic had passed yet.
	 */
	return p->tunnel.pptp.ul_call_id == target->tunnel.pptp.ul_call_id;
}

static void pptp_tnl_param_dump(struct seq_file *s, struct tops_params *params)
{
	struct tops_pptp_params *pptpp = &params->tunnel.pptp;

	seq_puts(s, "\tTunnel Type: PPTP ");
	seq_printf(s, "DL Call ID: %05u UL Call ID: %05u SEQ_GEN_IDX: %05u\n",
		   ntohs(pptpp->dl_call_id), ntohs(pptpp->ul_call_id),
		   pptpp->seq_gen_idx);
}

static struct tops_tnl_type pptp_type = {
	.type_name = "pptp",
	.cls_entry_setup = pptp_cls_entry_setup,
	.tnl_decap_param_setup = pptp_tnl_decap_param_setup,
	.tnl_encap_param_setup = pptp_tnl_encap_param_setup,
	.tnl_debug_param_setup = pptp_tnl_debug_param_setup,
	.tnl_decap_offloadable = pptp_tnl_decap_offloadable,
	.tnl_param_restore = pptp_tnl_param_restore,
	.tnl_param_match = pptp_tnl_param_match,
	.tnl_param_dump = pptp_tnl_param_dump,
	.tnl_proto_type = TOPS_TUNNEL_PPTP,
	.has_inner_eth = false,
};

int mtk_tops_pptp_init(void)
{
	int ret = 0;

	ret = mtk_tops_tnl_type_register(&pptp_type);
	if (ret)
		return ret;

	mtk_tops_pptp_seq_init();

	return ret;
}

void mtk_tops_pptp_deinit(void)
{
	mtk_tops_pptp_seq_deinit();

	mtk_tops_tnl_type_unregister(&pptp_type);
}
