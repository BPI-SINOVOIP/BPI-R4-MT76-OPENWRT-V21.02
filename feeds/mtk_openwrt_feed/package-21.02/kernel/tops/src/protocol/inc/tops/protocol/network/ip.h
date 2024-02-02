/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * Copyright (c) 2023 MediaTek Inc. All Rights Reserved.
 *
 * Author: Ren-Ting Wang <ren-ting.wang@mediatek.com>
 */

#ifndef _TOPS_IP_H_
#define _TOPS_IP_H_

#include <linux/ip.h>
#include <uapi/linux/in.h>

#include "tops/protocol/network/ip_params.h"
#include "tops/tops_params.h"

int mtk_tops_ip_encap_param_setup(
			struct sk_buff *skb,
			struct tops_params *params,
			int (*tnl_encap_param_setup)(struct sk_buff *skb,
						     struct tops_params *params));
int mtk_tops_ip_decap_param_setup(struct sk_buff *skb, struct tops_params *params);
int mtk_tops_ip_debug_param_setup(const char *buf, int *ofs,
				  struct tops_params *params);
void mtk_tops_ip_param_dump(struct seq_file *s, struct tops_params *params);
#endif /* _TOPS_IP_H_ */
