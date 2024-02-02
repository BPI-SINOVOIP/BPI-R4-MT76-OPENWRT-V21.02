/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * Copyright (c) 2023 MediaTek Inc. All Rights Reserved.
 *
 * Author: Ren-Ting Wang <ren-ting.wang@mediatek.com>
 */

#ifndef _TOPS_ETH_H_
#define _TOPS_ETH_H_

#include <linux/if_ether.h>

#include "tops/tops_params.h"

int
mtk_tops_eth_encap_param_setup(
			struct sk_buff *skb,
			struct tops_params *params,
			int (*tnl_encap_param_setup)(struct sk_buff *skb,
						     struct tops_params *params));
int mtk_tops_eth_decap_param_setup(struct sk_buff *skb, struct tops_params *params);
int mtk_tops_eth_debug_param_setup(const char *buf, int *ofs,
				   struct tops_params *params);
void mtk_tops_eth_param_dump(struct seq_file *s, struct tops_params *params);
#endif /* _TOPS_ETH_H_ */
