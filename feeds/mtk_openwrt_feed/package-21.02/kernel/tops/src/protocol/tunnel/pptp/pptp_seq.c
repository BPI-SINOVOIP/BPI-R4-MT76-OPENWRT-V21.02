// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (c) 2023 MediaTek Inc. All Rights Reserved.
 *
 * Author: Frank-zj Lin <frank-zj.lin@mediatek.com>
 */

#include <linux/hashtable.h>
#include <linux/spinlock.h>
#include <linux/slab.h>

#include <net/pptp.h>

#include "tops/hwspinlock.h"
#include "tops/seq_gen.h"

#define PPTP_SEQ_HT_BITS	4

struct pptp_seq {
	struct hlist_node hlist;
	int seq_gen_idx;
	uint16_t call_id;
};

static DEFINE_HASHTABLE(pptp_seq_ht, PPTP_SEQ_HT_BITS);
static DEFINE_SPINLOCK(pptp_seq_ht_lock);

static struct pptp_seq *mtk_tops_pptp_seq_find_no_lock(uint16_t call_id)
{
	struct pptp_seq *pptp_seq;

	hash_for_each_possible(pptp_seq_ht, pptp_seq, hlist, call_id) {
		if (pptp_seq->call_id == call_id)
			return pptp_seq;
	}

	return ERR_PTR(-ENODEV);
}

static int mtk_tops_pptp_seq_alloc_no_lock(uint16_t call_id, uint32_t seq_start,
					   int *seq_gen_idx)
{
	struct pptp_seq *pptp_seq;
	int ret;

	if (!IS_ERR(mtk_tops_pptp_seq_find_no_lock(call_id)))
		return -EBUSY;

	ret = mtk_tops_seq_gen_alloc(seq_gen_idx);
	if (ret)
		return ret;

	pptp_seq = kzalloc(sizeof(struct pptp_seq), GFP_KERNEL);
	if (!pptp_seq) {
		mtk_tops_seq_gen_free(*seq_gen_idx);
		return -ENOMEM;
	}

	pptp_seq->seq_gen_idx = *seq_gen_idx;
	pptp_seq->call_id = call_id;
	hash_add(pptp_seq_ht, &pptp_seq->hlist, pptp_seq->call_id);

	mtk_tops_seq_gen_set_32(*seq_gen_idx, seq_start);

	return 0;
}

int mtk_tops_pptp_seq_alloc(uint16_t call_id, uint32_t seq_start,
			    int *seq_gen_idx)
{
	unsigned long flag;
	int ret;

	spin_lock_irqsave(&pptp_seq_ht_lock, flag);

	ret = mtk_tops_pptp_seq_alloc_no_lock(call_id, seq_start, seq_gen_idx);

	spin_unlock_irqrestore(&pptp_seq_ht_lock, flag);

	return ret;
}

static void mtk_tops_pptp_seq_free_no_lock(uint16_t call_id)
{
	struct pptp_seq *pptp_seq;

	pptp_seq = mtk_tops_pptp_seq_find_no_lock(call_id);
	if (IS_ERR(pptp_seq))
		return;

	mtk_tops_seq_gen_free(pptp_seq->seq_gen_idx);
	hash_del(&pptp_seq->hlist);
	kfree(pptp_seq);
}

void mtk_tops_pptp_seq_free(uint16_t call_id)
{
	unsigned long flag;

	spin_lock_irqsave(&pptp_seq_ht_lock, flag);

	mtk_tops_pptp_seq_free_no_lock(call_id);

	spin_unlock_irqrestore(&pptp_seq_ht_lock, flag);
}

static int mtk_tops_pptp_seq_next_no_lock(uint16_t call_id, uint32_t *val)
{
	struct pptp_seq *pptp_seq;

	pptp_seq = mtk_tops_pptp_seq_find_no_lock(call_id);
	if (IS_ERR(pptp_seq))
		return -EINVAL;

	return mtk_tops_seq_gen_next_32(pptp_seq->seq_gen_idx, val);
}

static int mtk_tops_pptp_seq_next(uint16_t call_id, uint32_t *val)
{
	unsigned long flag;
	int ret;

	spin_lock_irqsave(&pptp_seq_ht_lock, flag);

	mtk_tops_hwspin_lock(HWSPINLOCK_GROUP_CLUST,
			     HWSPINLOCK_CLUST_SLOT_PPTP_SEQ);

	ret = mtk_tops_pptp_seq_next_no_lock(call_id, val);

	mtk_tops_hwspin_unlock(HWSPINLOCK_GROUP_CLUST,
			       HWSPINLOCK_CLUST_SLOT_PPTP_SEQ);

	spin_unlock_irqrestore(&pptp_seq_ht_lock, flag);

	return ret;
}

static int mtk_tops_pptp_seq_get_seq_gen_idx_no_lock(uint16_t call_id,
						     int *seq_gen_idx)
{
	struct pptp_seq *pptp_seq;

	pptp_seq = mtk_tops_pptp_seq_find_no_lock(call_id);
	if (IS_ERR(pptp_seq))
		return -EINVAL;

	*seq_gen_idx = pptp_seq->seq_gen_idx;

	return 0;
}

int mtk_tops_pptp_seq_get_seq_gen_idx(uint16_t call_id, int *seq_gen_idx)
{
	unsigned long flag;
	int ret;

	spin_lock_irqsave(&pptp_seq_ht_lock, flag);

	ret = mtk_tops_pptp_seq_get_seq_gen_idx_no_lock(call_id, seq_gen_idx);

	spin_unlock_irqrestore(&pptp_seq_ht_lock, flag);

	return ret;
}

void mtk_tops_pptp_seq_init(void)
{
	mtk_pptp_seq_next = mtk_tops_pptp_seq_next;
}

void mtk_tops_pptp_seq_deinit(void)
{
	mtk_pptp_seq_next = NULL;
}
