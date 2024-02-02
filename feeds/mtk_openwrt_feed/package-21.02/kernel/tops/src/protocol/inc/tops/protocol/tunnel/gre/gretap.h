/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * Copyright (c) 2023 MediaTek Inc. All Rights Reserved.
 *
 * Author: Ren-Ting Wang <ren-ting.wang@mediatek.com>
 */

#ifndef _TOPS_GRETAP_H_
#define _TOPS_GRETAP_H_

#include "tops/tops_params.h"

#if defined(CONFIG_MTK_TOPS_GRETAP)
int mtk_tops_gretap_decap_param_setup(struct sk_buff *skb, struct tops_params *params);
int mtk_tops_gretap_init(void);
void mtk_tops_gretap_deinit(void);
#else /* !defined(CONFIG_MTK_TOPS_GRETAP) */
static inline int mtk_tops_gretap_decap_param_setup(struct sk_buff *skb,
						    struct tops_params *params)
{
	return -ENODEV;
}

static inline int mtk_tops_gretap_init(void)
{
	return 0;
}

static inline void mtk_tops_gretap_deinit(void)
{
}
#endif /* defined(CONFIG_MTK_TOPS_GRETAP) */
#endif /* _TOPS_GRETAP_H_ */
