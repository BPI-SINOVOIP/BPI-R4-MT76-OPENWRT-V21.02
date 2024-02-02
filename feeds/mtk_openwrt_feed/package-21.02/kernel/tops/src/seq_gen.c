// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (c) 2023 MediaTek Inc. All Rights Reserved.
 *
 * Author: Frank-zj Lin <frank-zj.lin@mediatek.com>
 */

#include <linux/io.h>
#include <linux/spinlock.h>
#include <linux/types.h>

#include "tops/internal.h"
#include "tops/seq_gen.h"

#define SEQ_GEN_L(idx)	(TOPS_SEQ_GEN_BASE + (idx) * 0x20)
#define SEQ_GEN_H(idx)	(TOPS_SEQ_GEN_BASE + (idx) * 0x20 + 0x10)

static void __iomem *base;

static DECLARE_BITMAP(seq_gen_used, TOPS_SEQ_GEN_IDX_MAX);
static DEFINE_SPINLOCK(seq_gen_used_lock);

static inline u32 seq_gen_read(u32 reg)
{
	return readl(base + reg);
}

static inline void seq_gen_write(u32 reg, u32 val)
{
	writel(val, base + reg);
}

static inline int seq_gen_read_16(u32 seq_gen_idx, u16 *val)
{
	if (seq_gen_idx >= TOPS_SEQ_GEN_IDX_MAX)
		return -EINVAL;

	*val = (u16)seq_gen_read(SEQ_GEN_L(seq_gen_idx));

	return 0;
}

static inline void seq_gen_write_16(u32 seq_gen_idx, u16 val)
{
	if (seq_gen_idx >= TOPS_SEQ_GEN_IDX_MAX)
		return;

	seq_gen_write(SEQ_GEN_L(seq_gen_idx), (u32)val);
}

static inline int seq_gen_read_32(u32 seq_gen_idx, u32 *val)
{
	u32 val_h, val_l;

	if (seq_gen_idx >= TOPS_SEQ_GEN_IDX_MAX)
		return -EINVAL;

	val_l = seq_gen_read(SEQ_GEN_L(seq_gen_idx));
	val_h = seq_gen_read(SEQ_GEN_H(seq_gen_idx));

	if (val_l != 0xFFFF)
		seq_gen_write(SEQ_GEN_H(seq_gen_idx), val_h);

	*val = (val_h << 16) | val_l;

	return 0;
}

static inline void seq_gen_write_32(u32 seq_gen_idx, u32 val)
{
	if (seq_gen_idx >= TOPS_SEQ_GEN_IDX_MAX)
		return;

	seq_gen_write(SEQ_GEN_L(seq_gen_idx), (val & 0xFFFF));
	seq_gen_write(SEQ_GEN_H(seq_gen_idx), (val >> 16));
}

static void mtk_tops_seq_gen_set_16_no_lock(int seq_gen_idx, u16 val)
{
	if (unlikely(!test_bit(seq_gen_idx, seq_gen_used)))
		return;

	seq_gen_write_16(seq_gen_idx, val);
}

static void mtk_tops_seq_gen_set_32_no_lock(int seq_gen_idx, u32 val)
{
	if (unlikely(!test_bit(seq_gen_idx, seq_gen_used)))
		return;

	seq_gen_write_32(seq_gen_idx, val);
}

static int mtk_tops_seq_gen_next_16_no_lock(int seq_gen_idx, u16 *val)
{
	if (unlikely(!val || !test_bit(seq_gen_idx, seq_gen_used)))
		return -EINVAL;

	return seq_gen_read_16(seq_gen_idx, val);
}

static int mtk_tops_seq_gen_next_32_no_lock(int seq_gen_idx, u32 *val)
{
	if (unlikely(!val || !test_bit(seq_gen_idx, seq_gen_used)))
		return -EINVAL;

	return seq_gen_read_32(seq_gen_idx, val);
}

void mtk_tops_seq_gen_set_16(int seq_gen_idx, u16 val)
{
	unsigned long flag;

	spin_lock_irqsave(&seq_gen_used_lock, flag);

	mtk_tops_seq_gen_set_16_no_lock(seq_gen_idx, val);

	spin_unlock_irqrestore(&seq_gen_used_lock, flag);
}

int mtk_tops_seq_gen_next_16(int seq_gen_idx, u16 *val)
{
	unsigned long flag;
	int ret;

	spin_lock_irqsave(&seq_gen_used_lock, flag);

	ret = mtk_tops_seq_gen_next_16_no_lock(seq_gen_idx, val);

	spin_unlock_irqrestore(&seq_gen_used_lock, flag);

	return ret;
}

void mtk_tops_seq_gen_set_32(int seq_gen_idx, u32 val)
{
	unsigned long flag;

	spin_lock_irqsave(&seq_gen_used_lock, flag);

	mtk_tops_seq_gen_set_32_no_lock(seq_gen_idx, val);

	spin_unlock_irqrestore(&seq_gen_used_lock, flag);
}

int mtk_tops_seq_gen_next_32(int seq_gen_idx, u32 *val)
{
	unsigned long flag;
	int ret;

	spin_lock_irqsave(&seq_gen_used_lock, flag);

	ret = mtk_tops_seq_gen_next_32_no_lock(seq_gen_idx, val);

	spin_unlock_irqrestore(&seq_gen_used_lock, flag);

	return ret;
}

static int mtk_tops_seq_gen_alloc_no_lock(int *seq_gen_idx)
{
	if (!seq_gen_idx)
		return -EINVAL;

	*seq_gen_idx = find_first_zero_bit(seq_gen_used, TOPS_SEQ_GEN_IDX_MAX);
	if (*seq_gen_idx == TOPS_SEQ_GEN_IDX_MAX) {
		TOPS_NOTICE("Sequence generator exhausted\n");
		return -ENOMEM;
	}

	set_bit(*seq_gen_idx, seq_gen_used);

	return 0;
}

int mtk_tops_seq_gen_alloc(int *seq_gen_idx)
{
	unsigned long flag;
	int ret;

	spin_lock_irqsave(&seq_gen_used_lock, flag);

	ret = mtk_tops_seq_gen_alloc_no_lock(seq_gen_idx);

	spin_unlock_irqrestore(&seq_gen_used_lock, flag);

	return ret;
}

static void mtk_tops_seq_gen_free_no_lock(int seq_gen_idx)
{
	clear_bit(seq_gen_idx, seq_gen_used);
}

void mtk_tops_seq_gen_free(int seq_gen_idx)
{
	unsigned long flag = 0;

	spin_lock_irqsave(&seq_gen_used_lock, flag);

	mtk_tops_seq_gen_free_no_lock(seq_gen_idx);

	spin_unlock_irqrestore(&seq_gen_used_lock, flag);
}

int mtk_tops_seq_gen_init(struct platform_device *pdev)
{
	struct resource *res;

	res = platform_get_resource_byname(pdev, IORESOURCE_MEM, "tops-base");
	if (!res)
		return -ENXIO;

	base = devm_ioremap(&pdev->dev, res->start, resource_size(res));
	if (!base)
		return -ENOMEM;

	return 0;
}
