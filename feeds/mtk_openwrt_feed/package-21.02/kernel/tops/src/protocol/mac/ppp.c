// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (c) 2023 MediaTek Inc. All Rights Reserved.
 *
 * Author: Frank-zj Lin <rank-zj.lin@mediatek.com>
 *         Ren-Ting Wang <ren-ting.wang@mediatek.com>
 */

#include "tops/protocol/mac/ppp.h"

bool mtk_tops_ppp_valid(struct sk_buff *skb)
{
	struct ppp_hdr *ppp;
	struct ppp_hdr ppph;

	ppp = skb_header_pointer(skb, 0, sizeof(struct ppp_hdr), &ppph);

	if (unlikely(!ppp))
		return false;

	return (ppp->addr == PPP_ALLSTATIONS &&
		ppp->ctrl == PPP_UI && ntohs(ppp->proto) == PPP_IP);
}
