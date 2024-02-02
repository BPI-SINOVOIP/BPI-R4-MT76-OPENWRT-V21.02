/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * Copyright (c) 2023 MediaTek Inc. All Rights Reserved.
 *
 * Author: Ren-Ting Wang <ren-ting.wang@mediatek.com>
 */

#ifndef _TOPS_L2TP_V2_H_
#define _TOPS_L2TP_V2_H_

#include "tops/protocol/tunnel/l2tp/l2tp_params.h"

#if defined(CONFIG_MTK_TOPS_L2TP_V2)
int mtk_tops_l2tpv2_init(void);
void mtk_tops_l2tpv2_deinit(void);
#else /* !defined(CONFIG_MTK_TOPS_L2TP_V2) */
static inline int mtk_tops_l2tpv2_init(void)
{
	return 0;
}

static inline void mtk_tops_l2tpv2_deinit(void)
{
}
#endif /* defined(CONFIG_MTK_TOPS_L2TP_V2) */
#endif /* _TOPS_L2TP_V2_H_ */
