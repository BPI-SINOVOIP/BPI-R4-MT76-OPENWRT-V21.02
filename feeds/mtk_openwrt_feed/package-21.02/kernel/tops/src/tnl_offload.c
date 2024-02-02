// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (c) 2023 MediaTek Inc. All Rights Reserved.
 *
 * Author: Ren-Ting Wang <ren-ting.wang@mediatek.com>
 */

#include <linux/completion.h>
#include <linux/device.h>
#include <linux/dmaengine.h>
#include <linux/dma-mapping.h>
#include <linux/err.h>
#include <linux/hashtable.h>
#include <linux/if_ether.h>
#include <linux/ip.h>
#include <linux/kthread.h>
#include <linux/list.h>
#include <linux/lockdep.h>
#include <linux/string.h>

#include <mtk_eth_soc.h>
#include <mtk_hnat/hnat.h>
#include <mtk_hnat/nf_hnat_mtk.h>

#include <pce/cdrt.h>
#include <pce/cls.h>
#include <pce/dipfilter.h>
#include <pce/netsys.h>
#include <pce/pce.h>

#include "tops/internal.h"
#include "tops/mbox.h"
#include "tops/mcu.h"
#include "tops/netsys.h"
#include "tops/protocol/tunnel/gre/gretap.h"
#include "tops/protocol/tunnel/l2tp/l2tpv2.h"
#include "tops/protocol/tunnel/pptp/pptp.h"
#include "tops/tunnel.h"

#define TOPS_PPE_ENTRY_BUCKETS		(64)
#define TOPS_PPE_ENTRY_BUCKETS_BIT	(6)

struct tops_tnl {
	/* tunnel types */
	struct tops_tnl_type *offload_tnl_types[__TOPS_TUNNEL_TYPE_MAX];
	u32 offload_tnl_type_num;
	u32 tnl_base_addr;

	/* tunnel table */
	DECLARE_HASHTABLE(ht, CONFIG_TOPS_TNL_MAP_BIT);
	DECLARE_BITMAP(tnl_used, CONFIG_TOPS_TNL_NUM);
	wait_queue_head_t tnl_sync_wait;
	spinlock_t tnl_sync_lock;
	spinlock_t tbl_lock;
	bool has_tnl_to_sync;
	struct task_struct *tnl_sync_thread;
	struct list_head *tnl_sync_pending;
	struct list_head *tnl_sync_submit;
	struct tops_tnl_info *tnl_infos;

	/* dma request */
	struct completion dma_done;
	struct dma_chan *dmachan;

	struct device *dev;
};

static enum mbox_msg_cnt tnl_offload_mbox_cmd_recv(struct mailbox_dev *mdev,
						   struct mailbox_msg *msg);

static struct tops_tnl tops_tnl;

static LIST_HEAD(tnl_sync_q1);
static LIST_HEAD(tnl_sync_q2);

struct mailbox_dev tnl_offload_mbox_recv =
	MBOX_RECV_MGMT_DEV(TNL_OFFLOAD, tnl_offload_mbox_cmd_recv);

/* tunnel mailbox communication */
static enum mbox_msg_cnt tnl_offload_mbox_cmd_recv(struct mailbox_dev *mdev,
						   struct mailbox_msg *msg)
{
	switch (msg->msg1) {
	case TOPS_TNL_START_ADDR_SYNC:
		tops_tnl.tnl_base_addr = msg->msg2;

		return MBOX_NO_RET_MSG;
	default:
		break;
	}

	return MBOX_NO_RET_MSG;
}

static inline void tnl_flush_ppe_entry(struct foe_entry *entry, u32 tnl_idx)
{
	u32 bind_tnl_idx;

	if (unlikely(!entry))
		return;

	switch (entry->bfib1.pkt_type) {
	case IPV4_HNAPT:
		if (entry->ipv4_hnapt.tport_id != NR_TDMA_TPORT
		    &&  entry->ipv4_hnapt.tport_id != NR_TDMA_QDMA_TPORT)
			return;

		bind_tnl_idx = entry->ipv4_hnapt.tops_entry - __TOPS_TUNNEL_TYPE_MAX;

		break;
	default:
		return;
	}

	/* unexpected tunnel index */
	if (bind_tnl_idx >= __TOPS_TUNNEL_TYPE_MAX)
		return;

	if (tnl_idx == __TOPS_TUNNEL_TYPE_MAX || tnl_idx == bind_tnl_idx)
		memset(entry, 0, sizeof(*entry));
}

static inline void skb_set_tops_tnl_idx(struct sk_buff *skb, u32 tnl_idx)
{
	skb_hnat_tops(skb) = tnl_idx + __TOPS_TUNNEL_TYPE_MAX;
}

static inline bool skb_tops_valid(struct sk_buff *skb)
{
	return (skb && skb_hnat_tops(skb) < __TOPS_TUNNEL_TYPE_MAX);
}

static inline struct tops_tnl_type *skb_to_tnl_type(struct sk_buff *skb)
{
	enum tops_tunnel_type tnl_proto_type = skb_hnat_tops(skb);
	struct tops_tnl_type *tnl_type;

	if (unlikely(!tnl_proto_type || tnl_proto_type >= __TOPS_TUNNEL_TYPE_MAX))
		return ERR_PTR(-EINVAL);

	tnl_type = tops_tnl.offload_tnl_types[tnl_proto_type];

	return tnl_type ? tnl_type : ERR_PTR(-ENODEV);
}

static inline struct tops_tnl_info *skb_to_tnl_info(struct sk_buff *skb)
{
	u32 tnl_idx = skb_hnat_tops(skb) - __TOPS_TUNNEL_TYPE_MAX;

	if (tnl_idx >= CONFIG_TOPS_TNL_NUM)
		return ERR_PTR(-EINVAL);

	if (!test_bit(tnl_idx, tops_tnl.tnl_used))
		return ERR_PTR(-EACCES);

	return &tops_tnl.tnl_infos[tnl_idx];
}

static inline void skb_mark_unbind(struct sk_buff *skb)
{
	skb_hnat_tops(skb) = 0;
	skb_hnat_is_decap(skb) = 0;
	skb_hnat_alg(skb) = 1;
}

static inline u32 tnl_params_hash(struct tops_tnl_params *tnl_params)
{
	if (!tnl_params)
		return 0;

	/* TODO: check collision possibility? */
	return (tnl_params->params.network.ip.sip ^ tnl_params->params.network.ip.dip);
}

static inline bool tnl_info_decap_is_enable(struct tops_tnl_info *tnl_info)
{
	return tnl_info->cache.flag & TNL_DECAP_ENABLE;
}

static inline void tnl_info_decap_enable(struct tops_tnl_info *tnl_info)
{
	tnl_info->cache.flag |= TNL_DECAP_ENABLE;
}

static inline void tnl_info_decap_disable(struct tops_tnl_info *tnl_info)
{
	tnl_info->cache.flag &= ~(TNL_DECAP_ENABLE);
}

static inline bool tnl_info_encap_is_enable(struct tops_tnl_info *tnl_info)
{
	return tnl_info->cache.flag & TNL_ENCAP_ENABLE;
}

static inline void tnl_info_encap_enable(struct tops_tnl_info *tnl_info)
{
	tnl_info->cache.flag |= TNL_ENCAP_ENABLE;
}

static inline void tnl_info_encap_disable(struct tops_tnl_info *tnl_info)
{
	tnl_info->cache.flag &= ~(TNL_ENCAP_ENABLE);
}

static inline void tnl_info_sta_updated_no_tnl_lock(struct tops_tnl_info *tnl_info)
{
	tnl_info->status &= (~TNL_STA_UPDATING);
	tnl_info->status &= (~TNL_STA_INIT);
	tnl_info->status |= TNL_STA_UPDATED;
}

static inline void tnl_info_sta_updated(struct tops_tnl_info *tnl_info)
{
	unsigned long flag = 0;

	if (unlikely(!tnl_info))
		return;

	spin_lock_irqsave(&tnl_info->lock, flag);

	tnl_info_sta_updated_no_tnl_lock(tnl_info);

	spin_unlock_irqrestore(&tnl_info->lock, flag);
}

static inline bool tnl_info_sta_is_updated(struct tops_tnl_info *tnl_info)
{
	return tnl_info->status & TNL_STA_UPDATED;
}

static inline void tnl_info_sta_updating_no_tnl_lock(struct tops_tnl_info *tnl_info)
{
	tnl_info->status |= TNL_STA_UPDATING;
	tnl_info->status &= (~TNL_STA_QUEUED);
	tnl_info->status &= (~TNL_STA_UPDATED);
}

static inline void tnl_info_sta_updating(struct tops_tnl_info *tnl_info)
{
	unsigned long flag = 0;

	if (unlikely(!tnl_info))
		return;

	spin_lock_irqsave(&tnl_info->lock, flag);

	tnl_info_sta_updating_no_tnl_lock(tnl_info);

	spin_unlock_irqrestore(&tnl_info->lock, flag);
}

static inline bool tnl_info_sta_is_updating(struct tops_tnl_info *tnl_info)
{
	return tnl_info->status & TNL_STA_UPDATING;
}

static inline void tnl_info_sta_queued_no_tnl_lock(struct tops_tnl_info *tnl_info)
{
	tnl_info->status |= TNL_STA_QUEUED;
	tnl_info->status &= (~TNL_STA_UPDATED);
}

static inline void tnl_info_sta_queued(struct tops_tnl_info *tnl_info)
{
	unsigned long flag = 0;

	if (unlikely(!tnl_info))
		return;

	spin_lock_irqsave(&tnl_info->lock, flag);

	tnl_info_sta_queued_no_tnl_lock(tnl_info);

	spin_unlock_irqrestore(&tnl_info->lock, flag);
}

static inline bool tnl_info_sta_is_queued(struct tops_tnl_info *tnl_info)
{
	return tnl_info->status & TNL_STA_QUEUED;
}

static inline void tnl_info_sta_init_no_tnl_lock(struct tops_tnl_info *tnl_info)
{
	tnl_info->status = TNL_STA_INIT;
}

static inline void tnl_info_sta_init(struct tops_tnl_info *tnl_info)
{
	unsigned long flag = 0;

	if (unlikely(!tnl_info))
		return;

	spin_lock_irqsave(&tnl_info->lock, flag);

	tnl_info_sta_init_no_tnl_lock(tnl_info);

	spin_unlock_irqrestore(&tnl_info->lock, flag);
}

static inline bool tnl_info_sta_is_init(struct tops_tnl_info *tnl_info)
{
	return tnl_info->status & TNL_STA_INIT;
}

static inline void tnl_info_sta_uninit_no_tnl_lock(struct tops_tnl_info *tnl_info)
{
	tnl_info->status = TNL_STA_UNINIT;
}

static inline void tnl_info_sta_uninit(struct tops_tnl_info *tnl_info)
{
	unsigned long flag = 0;

	if (unlikely(!tnl_info))
		return;

	spin_lock_irqsave(&tnl_info->lock, flag);

	tnl_info_sta_uninit_no_tnl_lock(tnl_info);

	spin_unlock_irqrestore(&tnl_info->lock, flag);
}

static inline bool tnl_info_sta_is_uninit(struct tops_tnl_info *tnl_info)
{
	return tnl_info->status & TNL_STA_UNINIT;
}

static inline void tnl_info_submit_no_tnl_lock(struct tops_tnl_info *tnl_info)
{
	unsigned long flag = 0;

	spin_lock_irqsave(&tops_tnl.tnl_sync_lock, flag);

	list_add_tail(&tnl_info->sync_node, tops_tnl.tnl_sync_submit);

	tops_tnl.has_tnl_to_sync = true;

	spin_unlock_irqrestore(&tops_tnl.tnl_sync_lock, flag);

	if (mtk_tops_mcu_alive())
		wake_up_interruptible(&tops_tnl.tnl_sync_wait);
}

static void mtk_tops_tnl_info_cls_update_idx(struct tops_tnl_info *tnl_info)
{
	unsigned long flag;

	tnl_info->tnl_params.cls_entry = tnl_info->tcls->cls->idx;

	spin_lock_irqsave(&tnl_info->lock, flag);
	tnl_info->cache.cls_entry = tnl_info->tcls->cls->idx;
	spin_unlock_irqrestore(&tnl_info->lock, flag);
}

static void mtk_tops_tnl_info_cls_entry_unprepare(struct tops_tnl_info *tnl_info,
						  struct tops_tnl_params *tnl_params)
{
	struct tops_cls_entry *tcls = tnl_info->tcls;

	tnl_info->tcls = NULL;

	if (refcount_dec_and_test(&tcls->refcnt)) {
		list_del(&tcls->node);

		if (!tnl_params->cdrt)
			memset(&tcls->cls->cdesc, 0, sizeof(tcls->cls->cdesc));
		else
			/*
			 * recover tport_ix to let match packets to
			 * go through EIP197 only
			 */
			CLS_DESC_DATA(&tcls->cls->cdesc, tport_idx, 2);

		mtk_pce_cls_entry_write(tcls->cls);

		mtk_pce_cls_entry_free(tcls->cls);

		devm_kfree(tops_dev, tcls);
	}
}

static struct tops_cls_entry *
mtk_tops_tnl_info_cls_entry_prepare(struct tops_tnl_info *tnl_info,
				    struct tops_tnl_params *tnl_params)
{
	struct tops_cls_entry *tcls;
	int ret;

	tcls = devm_kzalloc(tops_dev, sizeof(struct tops_cls_entry), GFP_KERNEL);
	if (!tcls)
		return ERR_PTR(-ENOMEM);

	if (!tnl_params->cdrt) {
		tcls->cls = mtk_pce_cls_entry_alloc();
		if (IS_ERR(tcls->cls)) {
			ret = PTR_ERR(tcls->cls);
			goto free_tcls;
		}
	} else {
		struct cdrt_entry *cdrt = mtk_pce_cdrt_entry_find(tnl_params->cdrt);

		if (IS_ERR(cdrt)) {
			ret = PTR_ERR(cdrt);
			goto free_tcls;
		}
		if (unlikely(!cdrt->cls)) {
			ret = -ENODEV;
			goto free_tcls;
		}

		tcls->cls = cdrt->cls;
	}

	INIT_LIST_HEAD(&tcls->node);
	list_add_tail(&tnl_info->tnl_type->tcls_head, &tcls->node);

	tnl_info->tcls = tcls;
	refcount_set(&tcls->refcnt, 1);

	return tcls;

free_tcls:
	devm_kfree(tops_dev, tcls);

	return ERR_PTR(ret);
}

static int mtk_tops_tnl_info_cls_entry_write(struct tops_tnl_info *tnl_info)
{
	int ret;

	if (!tnl_info->tcls)
		return -EINVAL;

	ret = mtk_pce_cls_entry_write(tnl_info->tcls->cls);
	if (ret)
		return ret;

	tnl_info->tcls->updated = true;

	mtk_tops_tnl_info_cls_update_idx(tnl_info);

	return 0;
}

static int mtk_tops_tnl_info_cls_tear_down(struct tops_tnl_info *tnl_info,
					   struct tops_tnl_params *tnl_params)
{
	mtk_tops_tnl_info_cls_entry_unprepare(tnl_info, tnl_params);

	return 0;
}

/*
 * check cls entry is updated for tunnel protocols that only use 1 CLS HW entry
 *
 * since only tunnel sync task will operate on tcls linked list,
 * it is safe to access without lock
 *
 * return true on updated
 * return false on need update
 */
static bool mtk_tops_tnl_info_cls_single_is_updated(struct tops_tnl_info *tnl_info,
						    struct tops_tnl_type *tnl_type)
{
	/*
	 * check tnl_type has already allocate a tops_cls_entry
	 * if not, return false to prepare to allocate a new one
	 */
	if (list_empty(&tnl_type->tcls_head))
		return false;

	/*
	 * if tnl_info is not associate to tnl_type's cls entry,
	 * make a reference to tops_cls_entry
	 */
	if (!tnl_info->tcls) {
		tnl_info->tcls = list_first_entry(&tnl_type->tcls_head,
						  struct tops_cls_entry,
						  node);

		refcount_inc(&tnl_info->tcls->refcnt);
		mtk_tops_tnl_info_cls_update_idx(tnl_info);
	}

	return tnl_info->tcls->updated;
}

static int mtk_tops_tnl_info_cls_single_setup(struct tops_tnl_info *tnl_info,
					      struct tops_tnl_params *tnl_params,
					      struct tops_tnl_type *tnl_type)
{
	struct tops_cls_entry *tcls;
	int ret;

	if (mtk_tops_tnl_info_cls_single_is_updated(tnl_info, tnl_type))
		return 0;

	if (tnl_info->tcls)
		goto cls_entry_write;

	tcls = mtk_tops_tnl_info_cls_entry_prepare(tnl_info, tnl_params);
	if (IS_ERR(tcls))
		return PTR_ERR(tcls);

	if (!tnl_params->cdrt) {
		ret = tnl_type->cls_entry_setup(tnl_info, &tcls->cls->cdesc);
		if (ret) {
			TOPS_ERR("tops cls entry setup failed: %d\n", ret);
			goto cls_entry_unprepare;
		}
	} else {
		/*
		 * since CLS is already filled up with outer protocol rule
		 * we only update CLS tport here to let matched packet to go through
		 * QDMA and specify the destination port to TOPS
		 */
		CLS_DESC_DATA(&tcls->cls->cdesc, tport_idx, NR_EIP197_QDMA_TPORT);
		CLS_DESC_DATA(&tcls->cls->cdesc, fport, PSE_PORT_TDMA);
		CLS_DESC_DATA(&tcls->cls->cdesc, qid, 12);
	}

cls_entry_write:
	ret = mtk_tops_tnl_info_cls_entry_write(tnl_info);

cls_entry_unprepare:
	if (ret)
		mtk_tops_tnl_info_cls_entry_unprepare(tnl_info, tnl_params);

	return ret;
}

static struct tops_cls_entry *
mtk_tops_tnl_info_cls_entry_find(struct tops_tnl_type *tnl_type,
				 struct cls_desc *cdesc)
{
	struct tops_cls_entry *tcls;

	list_for_each_entry(tcls, &tnl_type->tcls_head, node)
		if (!memcmp(&tcls->cls->cdesc, cdesc, sizeof(struct cls_desc)))
			return tcls;

	return NULL;
}

static bool mtk_tops_tnl_info_cls_multi_is_updated(struct tops_tnl_info *tnl_info,
						   struct tops_tnl_type *tnl_type,
						   struct cls_desc *cdesc)
{
	struct tops_cls_entry *tcls;

	if (list_empty(&tnl_type->tcls_head))
		return false;

	if (tnl_info->tcls) {
		if (!memcmp(cdesc, &tnl_info->tcls->cls->cdesc, sizeof(*cdesc)))
			return tnl_info->tcls->updated;

		memcpy(&tnl_info->tcls->cls->cdesc, cdesc, sizeof(*cdesc));
		tnl_info->tcls->updated = false;
		return false;
	}

	tcls = mtk_tops_tnl_info_cls_entry_find(tnl_type, cdesc);
	if (!tcls)
		return false;

	tnl_info->tcls = tcls;
	refcount_inc(&tnl_info->tcls->refcnt);
	mtk_tops_tnl_info_cls_update_idx(tnl_info);

	return tcls->updated;
}

static int mtk_tops_tnl_info_cls_multi_setup(struct tops_tnl_info *tnl_info,
					     struct tops_tnl_params *tnl_params,
					     struct tops_tnl_type *tnl_type)
{
	struct tops_cls_entry *tcls;
	struct cls_desc cdesc;

	int ret;

	if (!tnl_params->cdrt) {
		memset(&cdesc, 0, sizeof(struct cls_desc));

		/* prepare cls_desc from tnl_type */
		ret = tnl_type->cls_entry_setup(tnl_info, &cdesc);
		if (ret) {
			TOPS_ERR("tops cls entry setup failed: %d\n", ret);
			return ret;
		}
	} else {
		struct cdrt_entry *cdrt = mtk_pce_cdrt_entry_find(tnl_params->cdrt);

		if (IS_ERR(cdrt)) {
			TOPS_ERR("no cdrt idx: %u related CDRT found\n",
				 tnl_params->cdrt);
			return PTR_ERR(cdrt);
		}

		memcpy(&cdesc, &cdrt->cls->cdesc, sizeof(struct cls_desc));

		CLS_DESC_DATA(&cdesc, tport_idx, 0x7);
	}

	/*
	 * check cdesc is already updated, if tnl_info is not associate with a
	 * tcls but we found a tcls has the same cls desc content as cdesc
	 * tnl_info will setup an association with that tcls
	 *
	 * we only go further to this if condition when
	 * a tcls is not yet updated or
	 * tnl_info is not yet associated to a tcls
	 */
	if (mtk_tops_tnl_info_cls_multi_is_updated(tnl_info, tnl_type, &cdesc))
		return 0;

	/* tcls is not yet updated, update this tcls */
	if (tnl_info->tcls)
		return mtk_tops_tnl_info_cls_entry_write(tnl_info);

	/* create a new tcls entry and associate with tnl_info */
	tcls = mtk_tops_tnl_info_cls_entry_prepare(tnl_info, tnl_params);
	if (IS_ERR(tcls))
		return PTR_ERR(tcls);

	memcpy(&tcls->cls->cdesc, &cdesc, sizeof(struct cls_desc));

	ret = mtk_tops_tnl_info_cls_entry_write(tnl_info);
	if (ret)
		mtk_tops_tnl_info_cls_entry_unprepare(tnl_info, tnl_params);

	return ret;
}

static int mtk_tops_tnl_info_cls_setup(struct tops_tnl_info *tnl_info,
				       struct tops_tnl_params *tnl_params)
{
	struct tops_tnl_type *tnl_type;

	if (tnl_info->tcls && tnl_info->tcls->updated)
		return 0;

	tnl_type = tnl_info->tnl_type;
	if (!tnl_type)
		return -EINVAL;

	if (!tnl_type->use_multi_cls)
		return mtk_tops_tnl_info_cls_single_setup(tnl_info,
							  tnl_params,
							  tnl_type);

	return mtk_tops_tnl_info_cls_multi_setup(tnl_info, tnl_params, tnl_type);
}

static int mtk_tops_tnl_info_dipfilter_tear_down(struct tops_tnl_info *tnl_info)
{
	struct dip_desc dipd;

	memset(&dipd, 0, sizeof(struct dip_desc));

	dipd.ipv4 = be32_to_cpu(tnl_info->tnl_params.params.network.ip.sip);
	dipd.tag = DIPFILTER_IPV4;

	return mtk_pce_dipfilter_entry_del(&dipd);
}

static int mtk_tops_tnl_info_dipfilter_setup(struct tops_tnl_info *tnl_info)
{
	struct dip_desc dipd;

	/* setup dipfilter */
	memset(&dipd, 0, sizeof(struct dip_desc));

	dipd.ipv4 = be32_to_cpu(tnl_info->tnl_params.params.network.ip.sip);
	dipd.tag = DIPFILTER_IPV4;

	return mtk_pce_dipfilter_entry_add(&dipd);
}

void mtk_tops_tnl_info_submit_no_tnl_lock(struct tops_tnl_info *tnl_info)
{
	lockdep_assert_held(&tnl_info->lock);

	if (tnl_info_sta_is_queued(tnl_info))
		return;

	tnl_info_submit_no_tnl_lock(tnl_info);

	tnl_info_sta_queued_no_tnl_lock(tnl_info);
}

void mtk_tops_tnl_info_submit(struct tops_tnl_info *tnl_info)
{
	unsigned long flag = 0;

	if (unlikely(!tnl_info))
		return;

	spin_lock_irqsave(&tnl_info->lock, flag);

	mtk_tops_tnl_info_submit_no_tnl_lock(tnl_info);

	spin_unlock_irqrestore(&tnl_info->lock, flag);
}

static void mtk_tops_tnl_info_hash_no_lock(struct tops_tnl_info *tnl_info)
{
	lockdep_assert_held(&tops_tnl.tbl_lock);
	lockdep_assert_held(&tnl_info->lock);

	if (hash_hashed(&tnl_info->hlist))
		hash_del(&tnl_info->hlist);

	hash_add(tops_tnl.ht, &tnl_info->hlist, tnl_params_hash(&tnl_info->cache));
}

void mtk_tops_tnl_info_hash(struct tops_tnl_info *tnl_info)
{
	unsigned long flag = 0;

	if (unlikely(!tnl_info))
		return;

	spin_lock_irqsave(&tops_tnl.tbl_lock, flag);

	spin_lock(&tnl_info->lock);

	mtk_tops_tnl_info_hash_no_lock(tnl_info);

	spin_unlock(&tnl_info->lock);

	spin_unlock_irqrestore(&tops_tnl.tbl_lock, flag);
}

struct tops_tnl_info *mtk_tops_tnl_info_get_by_idx(u32 tnl_idx)
{
	if (tnl_idx >= CONFIG_TOPS_TNL_NUM)
		return ERR_PTR(-EINVAL);

	if (!test_bit(tnl_idx, tops_tnl.tnl_used))
		return ERR_PTR(-EACCES);

	return &tops_tnl.tnl_infos[tnl_idx];
}

static bool mtk_tops_tnl_info_match(struct tops_tnl_type *tnl_type,
				    struct tops_tnl_info *tnl_info,
				    struct tops_params *target)
{
	struct tops_params *p = &tnl_info->cache.params;
	unsigned long flag = 0;
	bool match;

	spin_lock_irqsave(&tnl_info->lock, flag);

	match = (p->tunnel.type == target->tunnel.type
		 && mtk_tops_params_match(p, target)
		 && tnl_type->tnl_param_match(p, target));

	spin_unlock_irqrestore(&tnl_info->lock, flag);

	return match;
}

struct tops_tnl_info *mtk_tops_tnl_info_find(struct tops_tnl_type *tnl_type,
					     struct tops_tnl_params *tnl_params)
{
	struct tops_tnl_info *tnl_info;

	lockdep_assert_held(&tops_tnl.tbl_lock);

	if (unlikely(!tnl_params->tops_entry_proto
		     || tnl_params->tops_entry_proto >= __TOPS_TUNNEL_TYPE_MAX))
		return ERR_PTR(-EINVAL);

	hash_for_each_possible(tops_tnl.ht,
			       tnl_info,
			       hlist,
			       tnl_params_hash(tnl_params))
		if (mtk_tops_tnl_info_match(tnl_type, tnl_info, &tnl_params->params))
			return tnl_info;

	return ERR_PTR(-ENODEV);
}

static inline void mtk_tops_tnl_info_preserve(struct tops_tnl_type *tnl_type,
					      struct tops_tnl_params *old,
					      struct tops_tnl_params *new)
{
	new->flag |= old->flag;
	new->cls_entry = old->cls_entry;
	if (old->cdrt)
		new->cdrt = old->cdrt;

	/* we can only get ttl from encapsulation */
	if (new->params.network.ip.ttl == 128 && old->params.network.ip.ttl != 0)
		new->params.network.ip.ttl = old->params.network.ip.ttl;

	if (tnl_type->tnl_param_restore)
		tnl_type->tnl_param_restore(&old->params, &new->params);
}

/* tnl_info->lock should be held before calling this function */
static int mtk_tops_tnl_info_setup(struct sk_buff *skb,
				   struct tops_tnl_type *tnl_type,
				   struct tops_tnl_info *tnl_info,
				   struct tops_tnl_params *tnl_params)
{
	bool has_diff = false;

	if (unlikely(!skb || !tnl_info || !tnl_params))
		return -EPERM;

	lockdep_assert_held(&tnl_info->lock);

	mtk_tops_tnl_info_preserve(tnl_type, &tnl_info->cache, tnl_params);

	has_diff = memcmp(&tnl_info->cache, tnl_params, sizeof(*tnl_params));
	if (has_diff) {
		memcpy(&tnl_info->cache, tnl_params, sizeof(*tnl_params));
		mtk_tops_tnl_info_hash_no_lock(tnl_info);
	}

	if (skb_hnat_is_decap(skb)) {
		/* the net_device is used to forward pkt to decap'ed inf when Rx */
		tnl_info->dev = skb->dev;
		if (!tnl_info_decap_is_enable(tnl_info)) {
			has_diff = true;
			tnl_info_decap_enable(tnl_info);
		}
	} else if (skb_hnat_is_encap(skb)) {
		/* set skb_hnat_tops(skb) to tunnel index for ppe binding */
		skb_set_tops_tnl_idx(skb, tnl_info->tnl_idx);
		if (!tnl_info_encap_is_enable(tnl_info)) {
			has_diff = true;
			tnl_info_encap_enable(tnl_info);
		}
	}

	if (has_diff)
		mtk_tops_tnl_info_submit_no_tnl_lock(tnl_info);

	return 0;
}

/* tops_tnl.tbl_lock should be acquired before calling this functions */
static struct tops_tnl_info *
mtk_tops_tnl_info_alloc_no_lock(struct tops_tnl_type *tnl_type)
{
	struct tops_tnl_info *tnl_info;
	unsigned long flag = 0;
	u32 tnl_idx;

	lockdep_assert_held(&tops_tnl.tbl_lock);

	tnl_idx = find_first_zero_bit(tops_tnl.tnl_used, CONFIG_TOPS_TNL_NUM);
	if (tnl_idx == CONFIG_TOPS_TNL_NUM) {
		TOPS_NOTICE("offload tunnel table full!\n");
		return ERR_PTR(-ENOMEM);
	}

	/* occupy used tunnel */
	tnl_info = &tops_tnl.tnl_infos[tnl_idx];
	memset(&tnl_info->tnl_params, 0, sizeof(struct tops_tnl_params));
	memset(&tnl_info->cache, 0, sizeof(struct tops_tnl_params));

	/* TODO: maybe spin_lock_bh() is enough? */
	spin_lock_irqsave(&tnl_info->lock, flag);

	if (tnl_info_sta_is_init(tnl_info)) {
		TOPS_ERR("error: fetched an initialized tunnel info\n");

		spin_unlock_irqrestore(&tnl_info->lock, flag);

		return ERR_PTR(-EBADF);
	}
	tnl_info_sta_init_no_tnl_lock(tnl_info);

	tnl_info->tnl_type = tnl_type;

	INIT_HLIST_NODE(&tnl_info->hlist);

	spin_unlock_irqrestore(&tnl_info->lock, flag);

	set_bit(tnl_idx, tops_tnl.tnl_used);

	return tnl_info;
}

struct tops_tnl_info *mtk_tops_tnl_info_alloc(struct tops_tnl_type *tnl_type)
{
	struct tops_tnl_info *tnl_info;
	unsigned long flag = 0;

	spin_lock_irqsave(&tops_tnl.tbl_lock, flag);

	tnl_info = mtk_tops_tnl_info_alloc_no_lock(tnl_type);

	spin_unlock_irqrestore(&tops_tnl.tbl_lock, flag);

	return tnl_info;
}

static void mtk_tops_tnl_info_free_no_lock(struct tops_tnl_info *tnl_info)
{
	if (unlikely(!tnl_info))
		return;

	lockdep_assert_held(&tops_tnl.tbl_lock);
	lockdep_assert_held(&tnl_info->lock);

	hash_del(&tnl_info->hlist);

	tnl_info_sta_uninit_no_tnl_lock(tnl_info);

	clear_bit(tnl_info->tnl_idx, tops_tnl.tnl_used);
}

static void mtk_tops_tnl_info_free(struct tops_tnl_info *tnl_info)
{
	unsigned long flag = 0;

	spin_lock_irqsave(&tops_tnl.tbl_lock, flag);

	spin_lock(&tnl_info->lock);

	mtk_tops_tnl_info_free_no_lock(tnl_info);

	spin_unlock(&tnl_info->lock);

	spin_unlock_irqrestore(&tops_tnl.tbl_lock, flag);
}

static void __mtk_tops_tnl_offload_disable(struct tops_tnl_info *tnl_info)
{
	tnl_info->status |= TNL_STA_DELETING;
	mtk_tops_tnl_info_submit_no_tnl_lock(tnl_info);
}

static int mtk_tops_tnl_offload(struct sk_buff *skb,
				struct tops_tnl_type *tnl_type,
				struct tops_tnl_params *tnl_params)
{
	struct tops_tnl_info *tnl_info;
	unsigned long flag;
	int ret = 0;

	if (unlikely(!tnl_params))
		return -EPERM;

	/* prepare tnl_info */
	spin_lock_irqsave(&tops_tnl.tbl_lock, flag);

	tnl_info = mtk_tops_tnl_info_find(tnl_type, tnl_params);
	if (IS_ERR(tnl_info) && PTR_ERR(tnl_info) != -ENODEV) {
		/* error */
		ret = PTR_ERR(tnl_info);
		goto err_out;
	} else if (IS_ERR(tnl_info) && PTR_ERR(tnl_info) == -ENODEV) {
		/* not allocate yet */
		tnl_info = mtk_tops_tnl_info_alloc_no_lock(tnl_type);
	}

	if (IS_ERR(tnl_info)) {
		ret = PTR_ERR(tnl_info);
		TOPS_DBG("tnl offload alloc tnl_info failed: %d\n", ret);
		goto err_out;
	}

	spin_lock(&tnl_info->lock);
	ret = mtk_tops_tnl_info_setup(skb, tnl_type, tnl_info, tnl_params);
	spin_unlock(&tnl_info->lock);

err_out:
	spin_unlock_irqrestore(&tops_tnl.tbl_lock, flag);

	return ret;
}

static int mtk_tops_tnl_l2_update(struct sk_buff *skb)
{
	struct tops_tnl_info *tnl_info = skb_to_tnl_info(skb);
	struct tops_tnl_type *tnl_type;
	unsigned long flag;
	int ret;

	if (IS_ERR(tnl_info))
		return PTR_ERR(tnl_info);

	tnl_type = tnl_info->tnl_type;
	if (!tnl_type->tnl_l2_param_update)
		return -ENODEV;

	spin_lock_irqsave(&tnl_info->lock, flag);

	ret = tnl_type->tnl_l2_param_update(skb, &tnl_info->cache.params);
	/* tnl params need to be updated */
	if (ret == 1) {
		mtk_tops_tnl_info_submit_no_tnl_lock(tnl_info);
		ret = 0;
	}

	spin_unlock_irqrestore(&tnl_info->lock, flag);

	return ret;
}

static bool mtk_tops_tnl_decap_offloadable(struct sk_buff *skb)
{
	struct tops_tnl_type *tnl_type;
	struct ethhdr *eth;
	u32 cnt;
	u32 i;

	if (unlikely(!mtk_tops_mcu_alive())) {
		skb_mark_unbind(skb);
		return -EAGAIN;
	}

	/* skb should not carry tops here */
	if (skb_hnat_tops(skb))
		return false;

	eth = eth_hdr(skb);

	/* TODO: currently decap only support ethernet IPv4 */
	if (ntohs(eth->h_proto) != ETH_P_IP)
		return false;

	/* TODO: may can be optimized */
	for (i = TOPS_TUNNEL_GRETAP, cnt = 0;
	     i < __TOPS_TUNNEL_TYPE_MAX && cnt < tops_tnl.offload_tnl_type_num;
	     i++) {
		tnl_type = tops_tnl.offload_tnl_types[i];
		if (unlikely(!tnl_type))
			continue;

		cnt++;
		if (tnl_type->tnl_decap_offloadable
		    && tnl_type->tnl_decap_offloadable(skb)) {
			skb_hnat_tops(skb) = tnl_type->tnl_proto_type;
			return true;
		}
	}

	return false;
}

static int mtk_tops_tnl_decap_offload(struct sk_buff *skb)
{
	struct tops_tnl_params tnl_params;
	struct tops_tnl_type *tnl_type;
	int ret;

	if (unlikely(!mtk_tops_mcu_alive())) {
		skb_mark_unbind(skb);
		return -EAGAIN;
	}

	if (unlikely(!skb_tops_valid(skb) || !skb_hnat_is_decap(skb))) {
		skb_mark_unbind(skb);
		return -EINVAL;
	}

	tnl_type = skb_to_tnl_type(skb);
	if (IS_ERR(tnl_type)) {
		skb_mark_unbind(skb);
		return PTR_ERR(tnl_type);
	}

	if (unlikely(!tnl_type->tnl_decap_param_setup || !tnl_type->tnl_param_match)) {
		skb_mark_unbind(skb);
		return -ENODEV;
	}

	memset(&tnl_params, 0, sizeof(struct tops_tnl_params));

	/* push removed ethernet header back first */
	if (tnl_type->has_inner_eth)
		skb_push(skb, sizeof(struct ethhdr));

	ret = mtk_tops_decap_param_setup(skb,
					 &tnl_params.params,
					 tnl_type->tnl_decap_param_setup);

	/* pull ethernet header to restore skb->data to ip start */
	if (tnl_type->has_inner_eth)
		skb_pull(skb, sizeof(struct ethhdr));

	if (unlikely(ret)) {
		skb_mark_unbind(skb);
		return ret;
	}

	tnl_params.tops_entry_proto = tnl_type->tnl_proto_type;
	tnl_params.cdrt = skb_hnat_cdrt(skb);

	ret = mtk_tops_tnl_offload(skb, tnl_type, &tnl_params);

	/*
	 * whether success or fail to offload a decapsulation tunnel
	 * skb_hnat_tops(skb) must be cleared to avoid mtk_tnl_decap_offload() get
	 * called again
	 */
	skb_hnat_tops(skb) = 0;
	skb_hnat_is_decap(skb) = 0;

	return ret;
}

static int __mtk_tops_tnl_encap_offload(struct sk_buff *skb)
{
	struct tops_tnl_params tnl_params;
	struct tops_tnl_type *tnl_type;
	int ret;

	tnl_type = skb_to_tnl_type(skb);
	if (IS_ERR(tnl_type))
		return PTR_ERR(tnl_type);

	if (unlikely(!tnl_type->tnl_encap_param_setup || !tnl_type->tnl_param_match))
		return -ENODEV;

	memset(&tnl_params, 0, sizeof(struct tops_tnl_params));

	ret = mtk_tops_encap_param_setup(skb,
					 &tnl_params.params,
					 tnl_type->tnl_encap_param_setup);
	if (unlikely(ret))
		return ret;

	tnl_params.tops_entry_proto = tnl_type->tnl_proto_type;
	tnl_params.cdrt = skb_hnat_cdrt(skb);

	return mtk_tops_tnl_offload(skb, tnl_type, &tnl_params);
}

static int mtk_tops_tnl_encap_offload(struct sk_buff *skb)
{
	if (unlikely(!mtk_tops_mcu_alive())) {
		skb_mark_unbind(skb);
		return -EAGAIN;
	}

	if (!skb_hnat_is_encap(skb))
		return -EPERM;

	if (unlikely(skb_hnat_cdrt(skb)))
		return mtk_tops_tnl_l2_update(skb);

	return __mtk_tops_tnl_encap_offload(skb);
}

static struct net_device *mtk_tops_get_tnl_dev(int tnl_idx)
{
	if (tnl_idx < TOPS_CRSN_TNL_ID_START || tnl_idx > TOPS_CRSN_TNL_ID_END)
		return ERR_PTR(-EINVAL);

	tnl_idx = tnl_idx - TOPS_CRSN_TNL_ID_START;

	return tops_tnl.tnl_infos[tnl_idx].dev;
}

static void mtk_tops_tnl_sync_dma_done(void *param)
{
	/* TODO: check tx status with dmaengine_tx_status()? */
	complete(&tops_tnl.dma_done);
}

static void mtk_tops_tnl_sync_dma_start(void *param)
{
	dma_async_issue_pending(tops_tnl.dmachan);

	wait_for_completion(&tops_tnl.dma_done);
}

static void mtk_tops_tnl_sync_dma_unprepare(struct tops_tnl_info *tnl_info,
					    dma_addr_t *addr)
{
	dma_unmap_single(tops_dev, *addr, sizeof(struct tops_tnl_params),
			 DMA_TO_DEVICE);

	dma_release_channel(tops_tnl.dmachan);
}

static int mtk_tops_tnl_sync_dma_prepare(struct tops_tnl_info *tnl_info,
					 dma_addr_t *addr)
{
	u32 tnl_addr = tops_tnl.tnl_base_addr;
	struct dma_async_tx_descriptor *desc;
	dma_cookie_t cookie;
	int ret;

	if (!tnl_info)
		return -EPERM;

	tnl_addr += tnl_info->tnl_idx * sizeof(struct tops_tnl_params);

	tops_tnl.dmachan = dma_request_slave_channel(tops_dev, "tnl-sync");
	if (!tops_tnl.dmachan) {
		TOPS_ERR("request dma channel failed\n");
		return -ENODEV;
	}

	*addr = dma_map_single(tops_dev,
			       &tnl_info->tnl_params,
			       sizeof(struct tops_tnl_params),
			       DMA_TO_DEVICE);
	if (dma_mapping_error(tops_dev, *addr)) {
		ret = -ENOMEM;
		goto dma_release;
	}

	desc = dmaengine_prep_dma_memcpy(tops_tnl.dmachan,
					 (dma_addr_t)tnl_addr, *addr,
					 sizeof(struct tops_tnl_params),
					 0);
	if (!desc) {
		ret = -EBUSY;
		goto dma_unmap;
	}

	desc->callback = mtk_tops_tnl_sync_dma_done;

	cookie = dmaengine_submit(desc);
	ret = dma_submit_error(cookie);
	if (ret)
		goto dma_terminate;

	reinit_completion(&tops_tnl.dma_done);

	return ret;

dma_terminate:
	dmaengine_terminate_all(tops_tnl.dmachan);

dma_unmap:
	dma_unmap_single(tops_dev, *addr, sizeof(struct tops_tnl_params),
			 DMA_TO_DEVICE);

dma_release:
	dma_release_channel(tops_tnl.dmachan);

	return ret;
}

static int __mtk_tops_tnl_sync_param_delete(struct tops_tnl_info *tnl_info)
{
	struct mcu_ctrl_cmd mcmd;
	dma_addr_t addr;
	int ret;

	mcmd.e = MCU_EVENT_TYPE_SYNC_TNL;
	mcmd.arg[0] = TUNNEL_CTRL_EVENT_DEL;
	mcmd.arg[1] = tnl_info->tnl_idx;
	mcmd.core_mask = CORE_TOPS_MASK;

	ret = mtk_tops_mcu_stall(&mcmd, NULL, NULL);
	if (ret) {
		TOPS_ERR("tnl sync deletion notify mcu failed: %d\n", ret);
		return ret;
	}

	/* there shouldn't be any other reference to tnl_info right now */
	memset(&tnl_info->cache, 0, sizeof(struct tops_tnl_params));
	memset(&tnl_info->tnl_params, 0, sizeof(struct tops_tnl_params));

	ret = mtk_tops_tnl_sync_dma_prepare(tnl_info, &addr);
	if (ret) {
		TOPS_ERR("tnl sync deletion prepare dma request failed: %d\n", ret);
		return ret;
	}

	mtk_tops_tnl_sync_dma_start(NULL);

	mtk_tops_tnl_sync_dma_unprepare(tnl_info, &addr);

	return ret;
}

static int mtk_tops_tnl_sync_param_delete(struct tops_tnl_info *tnl_info)
{
	struct tops_tnl_params tnl_params;
	int ret;

	ret = mtk_tops_tnl_info_dipfilter_tear_down(tnl_info);
	if (ret) {
		TOPS_ERR("tnl sync dipfitler tear down failed: %d\n",
			 ret);
		return ret;
	}

	memcpy(&tnl_params, &tnl_info->tnl_params, sizeof(struct tops_tnl_params));
	ret = __mtk_tops_tnl_sync_param_delete(tnl_info);
	if (ret) {
		TOPS_ERR("tnl sync deletion failed: %d\n", ret);
		return ret;
	}

	ret = mtk_tops_tnl_info_cls_tear_down(tnl_info, &tnl_params);
	if (ret) {
		TOPS_ERR("tnl sync cls tear down faild: %d\n",
			 ret);
		return ret;
	}

	mtk_tops_tnl_info_free(tnl_info);

	return ret;
}

static int __mtk_tops_tnl_sync_param_update(struct tops_tnl_info *tnl_info,
					    bool is_new_tnl)
{
	struct mcu_ctrl_cmd mcmd;
	dma_addr_t addr;
	int ret;

	mcmd.e = MCU_EVENT_TYPE_SYNC_TNL;
	mcmd.arg[1] = tnl_info->tnl_idx;
	mcmd.core_mask = CORE_TOPS_MASK;

	if (is_new_tnl)
		mcmd.arg[0] = TUNNEL_CTRL_EVENT_NEW;
	else
		mcmd.arg[0] = TUNNEL_CTRL_EVENT_DIP_UPDATE;

	ret = mtk_tops_tnl_sync_dma_prepare(tnl_info, &addr);
	if (ret) {
		TOPS_ERR("tnl sync update prepare dma request failed: %d\n", ret);
		return ret;
	}

	ret = mtk_tops_mcu_stall(&mcmd, mtk_tops_tnl_sync_dma_start, NULL);
	if (ret)
		TOPS_ERR("tnl sync update notify mcu failed: %d\n", ret);

	mtk_tops_tnl_sync_dma_unprepare(tnl_info, &addr);

	return ret;
}

static int mtk_tops_tnl_sync_param_update(struct tops_tnl_info *tnl_info,
					  bool setup_pce, bool is_new_tnl)
{
	int ret;

	if (setup_pce) {
		ret = mtk_tops_tnl_info_cls_setup(tnl_info, &tnl_info->tnl_params);
		if (ret) {
			TOPS_ERR("tnl cls setup failed: %d\n", ret);
			return ret;
		}
	}

	ret = __mtk_tops_tnl_sync_param_update(tnl_info, is_new_tnl);
	if (ret) {
		TOPS_ERR("tnl sync failed: %d\n", ret);
		goto cls_tear_down;
	}

	tnl_info_sta_updated(tnl_info);

	if (setup_pce) {
		ret = mtk_tops_tnl_info_dipfilter_setup(tnl_info);
		if (ret) {
			TOPS_ERR("tnl dipfilter setup failed: %d\n", ret);
			/* TODO: should undo parameter sync */
			return ret;
		}
	}

	return ret;

cls_tear_down:
	mtk_tops_tnl_info_cls_tear_down(tnl_info, &tnl_info->tnl_params);

	return ret;
}

static inline int mtk_tops_tnl_sync_param_new(struct tops_tnl_info *tnl_info,
					      bool setup_pce)
{
	return mtk_tops_tnl_sync_param_update(tnl_info, setup_pce, true);
}

static void mtk_tops_tnl_sync_get_pending_queue(void)
{
	struct list_head *tmp = tops_tnl.tnl_sync_submit;
	unsigned long flag = 0;

	spin_lock_irqsave(&tops_tnl.tnl_sync_lock, flag);

	tops_tnl.tnl_sync_submit = tops_tnl.tnl_sync_pending;
	tops_tnl.tnl_sync_pending = tmp;

	tops_tnl.has_tnl_to_sync = false;

	spin_unlock_irqrestore(&tops_tnl.tnl_sync_lock, flag);
}

static void mtk_tops_tnl_sync_queue_proc(void)
{
	struct tops_tnl_info *tnl_info;
	struct tops_tnl_info *tmp;
	unsigned long flag = 0;
	bool is_decap = false;
	u32 tnl_status = 0;
	int ret;

	list_for_each_entry_safe(tnl_info,
				 tmp,
				 tops_tnl.tnl_sync_pending,
				 sync_node) {
		spin_lock_irqsave(&tnl_info->lock, flag);

		/* tnl update is on the fly, queue tnl to next round */
		if (tnl_info_sta_is_updating(tnl_info)) {
			list_del_init(&tnl_info->sync_node);

			tnl_info_submit_no_tnl_lock(tnl_info);

			goto next;
		}

		/*
		 * if tnl_info is not queued, something wrong
		 * just remove that tnl_info from the queue
		 * maybe trigger BUG_ON()?
		 */
		if (!tnl_info_sta_is_queued(tnl_info)) {
			list_del_init(&tnl_info->sync_node);
			goto next;
		}

		is_decap = (!(tnl_info->tnl_params.flag & TNL_DECAP_ENABLE)
			    && tnl_info_decap_is_enable(tnl_info));

		tnl_status = tnl_info->status;
		memcpy(&tnl_info->tnl_params, &tnl_info->cache,
		       sizeof(struct tops_tnl_params));

		list_del_init(&tnl_info->sync_node);

		/*
		 * mark tnl info to updating and release tnl info's spin lock
		 * since it is going to use dma to transfer data
		 * and might going to sleep
		 */
		tnl_info_sta_updating_no_tnl_lock(tnl_info);

		spin_unlock_irqrestore(&tnl_info->lock, flag);

		if (tnl_status & TNL_STA_INIT)
			ret = mtk_tops_tnl_sync_param_new(tnl_info, is_decap);
		else if (tnl_status & TNL_STA_DELETING)
			ret = mtk_tops_tnl_sync_param_delete(tnl_info);
		else
			ret = mtk_tops_tnl_sync_param_update(tnl_info,
							     is_decap,
							     false);

		if (ret)
			TOPS_ERR("sync tunnel parameter failed: %d\n", ret);

		continue;

next:
		spin_unlock_irqrestore(&tnl_info->lock, flag);
	}
}

static int tnl_sync_task(void *data)
{
	while (1) {
		wait_event_interruptible(tops_tnl.tnl_sync_wait,
				(tops_tnl.has_tnl_to_sync && mtk_tops_mcu_alive())
				|| kthread_should_stop());

		if (kthread_should_stop())
			break;

		mtk_tops_tnl_sync_get_pending_queue();

		mtk_tops_tnl_sync_queue_proc();
	}

	return 0;
}

static void mtk_tops_tnl_info_flush_ppe(struct tops_tnl_info *tnl_info)
{
	struct foe_entry *entry;
	u32 max_entry;
	u32 ppe_id;
	u32 eidx;

	/* tnl info's lock should be held */
	lockdep_assert_held(&tnl_info->lock);

	/* clear all TOPS related PPE entries */
	for (ppe_id = 0; ppe_id < MAX_PPE_NUM; ppe_id++) {
		max_entry = mtk_tops_netsys_ppe_get_max_entry_num(ppe_id);
		for (eidx = 0; eidx < max_entry; eidx++) {
			entry = hnat_get_foe_entry(ppe_id, eidx);
			if (IS_ERR(entry))
				break;

			if (!entry_hnat_is_bound(entry))
				continue;

			tnl_flush_ppe_entry(entry, tnl_info->tnl_idx);
		}
	}
	hnat_cache_ebl(1);
	/* make sure all data is written to dram PPE table */
	wmb();
}

void mtk_tops_tnl_offload_netdev_down(struct net_device *ndev)
{
	struct tops_tnl_info *tnl_info;
	unsigned long flag;
	u32 bkt;

	spin_lock_irqsave(&tops_tnl.tbl_lock, flag);

	hash_for_each(tops_tnl.ht, bkt, tnl_info, hlist) {
		spin_lock(&tnl_info->lock);

		if (tnl_info->dev == ndev) {
			mtk_tops_tnl_info_flush_ppe(tnl_info);

			__mtk_tops_tnl_offload_disable(tnl_info);

			spin_unlock(&tnl_info->lock);

			break;
		}

		spin_unlock(&tnl_info->lock);
	}

	spin_unlock_irqrestore(&tops_tnl.tbl_lock, flag);
}

void mtk_tops_tnl_offload_flush(void)
{
	struct tops_tnl_info *tnl_info;
	struct foe_entry *entry;
	unsigned long flag;
	u32 max_entry;
	u32 ppe_id;
	u32 eidx;
	u32 bkt;

	/* clear all TOPS related PPE entries */
	for (ppe_id = 0; ppe_id < MAX_PPE_NUM; ppe_id++) {
		max_entry = mtk_tops_netsys_ppe_get_max_entry_num(ppe_id);
		for (eidx = 0; eidx < max_entry; eidx++) {
			entry = hnat_get_foe_entry(ppe_id, eidx);
			if (IS_ERR(entry))
				break;

			if (!entry_hnat_is_bound(entry))
				continue;

			tnl_flush_ppe_entry(entry, __TOPS_TUNNEL_TYPE_MAX);
		}
	}
	hnat_cache_ebl(1);
	/* make sure all data is written to dram PPE table */
	wmb();

	spin_lock_irqsave(&tops_tnl.tbl_lock, flag);

	hash_for_each(tops_tnl.ht, bkt, tnl_info, hlist) {
		/* clear all tunnel's synced parameters, but preserve cache */
		memset(&tnl_info->tnl_params, 0, sizeof(struct tops_tnl_params));
		/*
		 * make tnl_info status to TNL_INIT state
		 * so that it can be added to TOPS again
		 */
		spin_lock(&tnl_info->lock);

		tnl_info_sta_init_no_tnl_lock(tnl_info);
		list_del_init(&tnl_info->sync_node);

		spin_unlock(&tnl_info->lock);
	}

	spin_unlock_irqrestore(&tops_tnl.tbl_lock, flag);
}

void mtk_tops_tnl_offload_recover(void)
{
	struct tops_tnl_info *tnl_info;
	unsigned long flag;
	u32 bkt;

	spin_lock_irqsave(&tops_tnl.tbl_lock, flag);

	hash_for_each(tops_tnl.ht, bkt, tnl_info, hlist)
		mtk_tops_tnl_info_submit(tnl_info);

	spin_unlock_irqrestore(&tops_tnl.tbl_lock, flag);
}

int mtk_tops_tnl_offload_init(struct platform_device *pdev)
{
	struct tops_tnl_info *tnl_info;
	int ret = 0;
	int i = 0;

	hash_init(tops_tnl.ht);

	tops_tnl.tnl_infos = devm_kzalloc(&pdev->dev,
				sizeof(struct tops_tnl_info) * CONFIG_TOPS_TNL_NUM,
				GFP_KERNEL);
	if (!tops_tnl.tnl_infos)
		return -ENOMEM;

	for (i = 0; i < CONFIG_TOPS_TNL_NUM; i++) {
		tnl_info = &tops_tnl.tnl_infos[i];
		tnl_info->tnl_idx = i;
		tnl_info->status = TNL_STA_UNINIT;
		INIT_HLIST_NODE(&tnl_info->hlist);
		INIT_LIST_HEAD(&tnl_info->sync_node);
		spin_lock_init(&tnl_info->lock);
	}

	ret = register_mbox_dev(MBOX_RECV, &tnl_offload_mbox_recv);
	if (ret) {
		TOPS_ERR("tnl offload recv dev register failed: %d\n",
			ret);
		return ret;
	}

	init_completion(&tops_tnl.dma_done);
	init_waitqueue_head(&tops_tnl.tnl_sync_wait);

	tops_tnl.tnl_sync_thread = kthread_run(tnl_sync_task, NULL,
					       "tnl sync param task");
	if (IS_ERR(tops_tnl.tnl_sync_thread)) {
		TOPS_ERR("tnl sync thread create failed\n");
		ret = -ENOMEM;
		goto unregister_mbox;
	}

	mtk_tnl_encap_offload = mtk_tops_tnl_encap_offload;
	mtk_tnl_decap_offload = mtk_tops_tnl_decap_offload;
	mtk_tnl_decap_offloadable = mtk_tops_tnl_decap_offloadable;
	mtk_get_tnl_dev = mtk_tops_get_tnl_dev;

	tops_tnl.tnl_sync_submit = &tnl_sync_q1;
	tops_tnl.tnl_sync_pending = &tnl_sync_q2;
	spin_lock_init(&tops_tnl.tnl_sync_lock);
	spin_lock_init(&tops_tnl.tbl_lock);

	return 0;

unregister_mbox:
	unregister_mbox_dev(MBOX_RECV, &tnl_offload_mbox_recv);

	return ret;
}

void mtk_tops_tnl_offload_pce_clean_up(void)
{
	struct tops_tnl_info *tnl_info;
	unsigned long flag;
	u32 bkt;

	spin_lock_irqsave(&tops_tnl.tbl_lock, flag);

	hash_for_each(tops_tnl.ht, bkt, tnl_info, hlist) {
		mtk_tops_tnl_info_flush_ppe(tnl_info);

		mtk_tops_tnl_info_dipfilter_tear_down(tnl_info);

		mtk_tops_tnl_info_cls_tear_down(tnl_info, &tnl_info->tnl_params);
	}

	spin_unlock_irqrestore(&tops_tnl.tbl_lock, flag);
}

void mtk_tops_tnl_offload_deinit(struct platform_device *pdev)
{
	mtk_tnl_encap_offload = NULL;
	mtk_tnl_decap_offload = NULL;
	mtk_tnl_decap_offloadable = NULL;
	mtk_get_tnl_dev = NULL;

	kthread_stop(tops_tnl.tnl_sync_thread);

	mtk_tops_tnl_offload_pce_clean_up();

	unregister_mbox_dev(MBOX_RECV, &tnl_offload_mbox_recv);
}

int mtk_tops_tnl_offload_proto_setup(struct platform_device *pdev)
{
	mtk_tops_gretap_init();

	mtk_tops_l2tpv2_init();

	mtk_tops_pptp_init();

	return 0;
}

void mtk_tops_tnl_offload_proto_teardown(struct platform_device *pdev)
{
	mtk_tops_pptp_deinit();

	mtk_tops_l2tpv2_deinit();

	mtk_tops_gretap_deinit();
}

struct tops_tnl_type *mtk_tops_tnl_type_get_by_name(const char *name)
{
	enum tops_tunnel_type tnl_proto_type = TOPS_TUNNEL_NONE + 1;
	struct tops_tnl_type *tnl_type;

	if (unlikely(!name))
		return ERR_PTR(-EPERM);

	for (; tnl_proto_type < __TOPS_TUNNEL_TYPE_MAX; tnl_proto_type++) {
		tnl_type = tops_tnl.offload_tnl_types[tnl_proto_type];
		if (tnl_type && !strcmp(name, tnl_type->type_name))
			break;
	}

	return tnl_type;
}

int mtk_tops_tnl_type_register(struct tops_tnl_type *tnl_type)
{
	enum tops_tunnel_type tnl_proto_type = tnl_type->tnl_proto_type;

	if (unlikely(tnl_proto_type == TOPS_TUNNEL_NONE
		     || tnl_proto_type >= __TOPS_TUNNEL_TYPE_MAX)) {
		TOPS_ERR("invalid tnl_proto_type: %u\n", tnl_proto_type);
		return -EINVAL;
	}

	if (unlikely(!tnl_type))
		return -EINVAL;

	if (tops_tnl.offload_tnl_types[tnl_proto_type]) {
		TOPS_ERR("offload tnl type is already registered: %u\n",
			 tnl_proto_type);
		return -EBUSY;
	}

	INIT_LIST_HEAD(&tnl_type->tcls_head);
	tops_tnl.offload_tnl_types[tnl_proto_type] = tnl_type;
	tops_tnl.offload_tnl_type_num++;

	return 0;
}

void mtk_tops_tnl_type_unregister(struct tops_tnl_type *tnl_type)
{
	enum tops_tunnel_type tnl_proto_type = tnl_type->tnl_proto_type;

	if (unlikely(tnl_proto_type == TOPS_TUNNEL_NONE
		     || tnl_proto_type >= __TOPS_TUNNEL_TYPE_MAX)) {
		TOPS_ERR("invalid tnl_proto_type: %u\n", tnl_proto_type);
		return;
	}

	if (unlikely(!tnl_type))
		return;

	if (tops_tnl.offload_tnl_types[tnl_proto_type] != tnl_type) {
		TOPS_ERR("offload tnl type is registered by others\n");
		return;
	}

	tops_tnl.offload_tnl_types[tnl_proto_type] = NULL;
	tops_tnl.offload_tnl_type_num--;
}
