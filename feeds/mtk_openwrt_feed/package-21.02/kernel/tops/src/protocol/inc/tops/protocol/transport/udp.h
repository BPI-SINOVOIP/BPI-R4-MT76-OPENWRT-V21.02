/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * Copyright (c) 2023 MediaTek Inc. All Rights Reserved.
 *
 * Author: Ren-Ting Wang <ren-ting.wang@mediatek.com>
 */

#ifndef _TOPS_UDP_H_
#define _TOPS_UDP_H_

#include "tops/protocol/transport/udp_params.h"
#include "tops/tops_params.h"

int mtk_tops_udp_encap_param_setup(
			struct sk_buff *skb,
			struct tops_params *params,
			int (*tnl_encap_param_setup)(struct sk_buff *skb,
						     struct tops_params *params));
int mtk_tops_udp_decap_param_setup(struct sk_buff *skb, struct tops_params *params);
int mtk_tops_udp_debug_param_setup(const char *buf, int *ofs,
				   struct tops_params *params);
void mtk_tops_udp_param_dump(struct seq_file *s, struct tops_params *params);
#endif /* _TOPS_UDP_H_ */
