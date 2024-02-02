/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * Copyright (c) 2023 MediaTek Inc. All Rights Reserved.
 *
 * Author: Frank-zj Lin <frank-zj.lin@mediatek.com>
 */

#ifndef _TOPS_SEQ_GEN_H_
#define _TOPS_SEQ_GEN_H_

#include <linux/platform_device.h>

#define TOPS_SEQ_GEN_BASE	0x880100	/* PKT_ID_GEN reg base */
#define TOPS_SEQ_GEN_IDX_MAX	16		/* num of PKT_ID_GEN reg */

void mtk_tops_seq_gen_set_16(int seq_gen_idx, u16 val);
int mtk_tops_seq_gen_next_16(int seq_gen_idx, u16 *val);
void mtk_tops_seq_gen_set_32(int seq_gen_idx, u32 val);
int mtk_tops_seq_gen_next_32(int seq_gen_idx, u32 *val);
int mtk_tops_seq_gen_alloc(int *seq_gen_idx);
void mtk_tops_seq_gen_free(int seq_gen_idx);
int mtk_tops_seq_gen_init(struct platform_device *pdev);
#endif /* _TOPS_SEQ_GEN_H_ */
