// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (c) 2023 MediaTek Inc. All Rights Reserved.
 *
 * Author: Ren-Ting Wang <ren-ting.wang@mediatek.com>
 */

#include <linux/device.h>
#include <linux/dmaengine.h>
#include <linux/io.h>
#include <linux/lockdep.h>
#include <linux/of.h>
#include <linux/of_device.h>
#include <linux/of_dma.h>
#include <linux/wait.h>
#include <linux/workqueue.h>

#include <virt-dma.h>

#include "tops/hpdma.h"
#include "tops/hwspinlock.h"
#include "tops/internal.h"
#include "tops/mbox.h"
#include "tops/mcu.h"

#define HPDMA_CHAN_NUM			(4)

#define MTK_HPDMA_ALIGN_SIZE		(DMAENGINE_ALIGN_16_BYTES)
#define MTK_HPDMA_DMA_BUSWIDTHS		(BIT(DMA_SLAVE_BUSWIDTH_4_BYTES))

struct hpdma_dev;
struct hpdma_vchan;
struct hpdma_vdesc;
struct hpdma_init_data;

typedef struct hpdma_dev *(*hpdma_init_func_t)(struct platform_device *pdev,
					       const struct hpdma_init_data *data);
typedef void (*tx_pending_desc_t)(struct hpdma_dev *hpdma,
				  struct hpdma_vchan *hchan,
				  struct hpdma_vdesc *hdesc);
typedef struct dma_chan *(*of_dma_xlate_func_t)(struct of_phandle_args *dma_spec,
						struct of_dma *ofdma);

struct hpdma_vdesc {
	struct virt_dma_desc vdesc;
	dma_addr_t src;
	dma_addr_t dst;
	u32 total_num;
	u32 axsize;
	size_t len;
};

struct hpdma_vchan {
	struct virt_dma_chan vchan;
	struct work_struct tx_work;
	struct hpdma_vdesc *issued_desc;
	wait_queue_head_t stop_wait;
	bool busy;
	bool terminating;
	u8 pchan_id;
};

struct hpdma_ops {
	int (*vchan_init)(struct hpdma_dev *hpdma, struct dma_device *ddev);
	void (*vchan_deinit)(struct hpdma_dev *hpdma);
	int (*mbox_init)(struct platform_device *pdev, struct hpdma_dev *hpdma);
	void (*mbox_deinit)(struct platform_device *pdev, struct hpdma_dev *hpdma);
	tx_pending_desc_t tx_pending_desc;
	of_dma_xlate_func_t of_dma_xlate;
};

struct hpdma_init_data {
	struct hpdma_ops ops;
	hpdma_init_func_t init;
	mbox_handler_func_t mbox_handler;
	enum hwspinlock_group hwspinlock_grp;
	u32 trigger_start_slot; /* permission to start dma transfer */
	u32 ch_base_slot; /* permission to occupy a physical channel */
};

struct hpdma_dev {
	struct dma_device ddev;
	struct hpdma_ops ops;
	struct hpdma_vchan *hvchans;
	struct hpdma_vchan *issued_chan;
	spinlock_t lock; /* prevent inter-process racing hwspinlock */
	void __iomem *base;
	enum hwspinlock_group hwspinlock_grp;
	u32 trigger_start_slot; /* permission to start dma transfer */
	u32 ch_base_slot; /* permission to occupy a physical channel */
};

struct top_hpdma_dev {
	struct mailbox_dev mdev;
	struct hpdma_dev hpdma;
};

struct clust_hpdma_dev {
	struct mailbox_dev mdev[CORE_MAX];
	struct hpdma_dev hpdma;
};

static inline void hpdma_write(struct hpdma_dev *hpdma, u32 reg, u32 val)
{
	writel(val, hpdma->base + reg);
}

static inline void hpdma_set(struct hpdma_dev *hpdma, u32 reg, u32 mask)
{
	setbits(hpdma->base + reg, mask);
}

static inline void hpdma_clr(struct hpdma_dev *hpdma, u32 reg, u32 mask)
{
	clrbits(hpdma->base + reg, mask);
}

static inline void hpdma_rmw(struct hpdma_dev *hpdma, u32 reg, u32 mask, u32 val)
{
	clrsetbits(hpdma->base + reg, mask, val);
}

static inline u32 hpdma_read(struct hpdma_dev *hpdma, u32 reg)
{
	return readl(hpdma->base + reg);
}

struct hpdma_dev *chan_to_hpdma_dev(struct dma_chan *chan)
{
	return container_of(chan->device, struct hpdma_dev, ddev);
}

struct hpdma_vchan *chan_to_hpdma_vchan(struct dma_chan *chan)
{
	return container_of(chan, struct hpdma_vchan, vchan.chan);
}

struct hpdma_vdesc *vdesc_to_hpdma_vdesc(struct virt_dma_desc *vdesc)
{
	return container_of(vdesc, struct hpdma_vdesc, vdesc);
}

static inline void __mtk_hpdma_vchan_deinit(struct virt_dma_chan *vchan)
{
	list_del(&vchan->chan.device_node);
	tasklet_kill(&vchan->task);
}

static inline int mtk_hpdma_prepare_transfer(struct hpdma_dev *hpdma)
{
	/*
	 * release when hpdma done
	 * prevent other APMCU's process contend hw spinlock
	 * since this lock will not be contended in interrupt context,
	 * it's safe to hold it without disable irq
	 */
	spin_lock(&hpdma->lock);

	/* it is not expected any issued chan right here */
	if (!hpdma->issued_chan)
		return 0;

	dev_err(hpdma->ddev.dev,
		"hpdma issued_chan is not empty when transfer started");

	WARN_ON(1);

	spin_unlock(&hpdma->lock);

	return -1;
}

static inline void mtk_hpdma_unprepare_transfer(struct hpdma_dev *hpdma)
{
	spin_unlock(&hpdma->lock);
}

static inline int mtk_hpdma_start_transfer(struct hpdma_dev *hpdma,
					   struct hpdma_vchan *hvchan,
					   struct hpdma_vdesc *hvdesc)
{
	/* occupy hpdma start permission */
	mtk_tops_hwspin_lock(hpdma->hwspinlock_grp, hpdma->trigger_start_slot);

	/* acknowledge the terminate flow that HW is going to start */
	hvchan->busy = true;

	list_del(&hvdesc->vdesc.node);

	/* set vdesc to current channel's pending transfer */
	hvchan->issued_desc = hvdesc;
	hpdma->issued_chan = hvchan;

	/* last chance to abort the transfer if channel is terminating */
	if (unlikely(hvchan->terminating))
		goto terminate_transfer;

	/* trigger dma start */
	hpdma_set(hpdma, TOPS_HPDMA_X_START(hvchan->pchan_id), HPDMA_START);

	return 0;

terminate_transfer:
	hvchan->busy = false;

	hpdma->issued_chan = NULL;

	mtk_tops_hwspin_unlock(hpdma->hwspinlock_grp, hpdma->trigger_start_slot);

	return -1;
}

/* setup a channel's parameter before it acquires the permission to start transfer */
static inline void mtk_hpdma_config_pchan(struct hpdma_dev *hpdma,
					  struct hpdma_vchan *hvchan,
					  struct hpdma_vdesc *hvdesc)
{
	/* update axsize */
	hpdma_rmw(hpdma,
		  TOPS_HPDMA_X_CTRL(hvchan->pchan_id),
		  HPDMA_AXSIZE_MASK,
		  FIELD_PREP(HPDMA_AXSIZE_MASK, hvdesc->axsize));

	/* update total num */
	hpdma_rmw(hpdma,
		  TOPS_HPDMA_X_NUM(hvchan->pchan_id),
		  HPDMA_TOTALNUM_MASK,
		  FIELD_PREP(HPDMA_TOTALNUM_MASK, hvdesc->total_num));

	/* set src addr */
	hpdma_write(hpdma, TOPS_HPDMA_X_SRC(hvchan->pchan_id), hvdesc->src);

	/* set dst addr */
	hpdma_write(hpdma, TOPS_HPDMA_X_DST(hvchan->pchan_id), hvdesc->dst);
}

/*
 * TODO: in general, we should allocate some buffer for dma transmission
 * nothing to allocate for hpdma right now?
 * TODO: we may not need this right now
 */
static int mtk_hpdma_alloc_chan_resources(struct dma_chan *chan)
{
	return 0;
}

/* TODO: we may not need this right now */
static void mtk_hpdma_free_chan_resources(struct dma_chan *chan)
{
	/* stop all transmission, we have nothing to free for each channel */
	dmaengine_terminate_sync(chan);
}

static void mtk_hpdma_issue_vchan_pending(struct hpdma_dev *hpdma,
					  struct hpdma_vchan *hvchan)
{
	struct virt_dma_desc *vdesc;

	/* vchan's lock need to be held since its list will be modified */
	lockdep_assert_held(&hvchan->vchan.lock);

	/* if there is pending transfer on the fly, we should wait until it done */
	if (unlikely(hvchan->issued_desc))
		return;

	/* fetch next desc to process */
	vdesc = vchan_next_desc(&hvchan->vchan);
	if (unlikely(!vdesc))
		return;

	/* start to transfer a pending descriptor */
	hpdma->ops.tx_pending_desc(hpdma, hvchan, vdesc_to_hpdma_vdesc(vdesc));
}

static void mtk_hpdma_issue_pending(struct dma_chan *chan)
{
	struct hpdma_dev *hpdma = chan_to_hpdma_dev(chan);
	struct hpdma_vchan *hvchan = chan_to_hpdma_vchan(chan);
	unsigned long flag;

	spin_lock_irqsave(&hvchan->vchan.lock, flag);

	if (vchan_issue_pending(&hvchan->vchan))
		mtk_hpdma_issue_vchan_pending(hpdma, hvchan);

	spin_unlock_irqrestore(&hvchan->vchan.lock, flag);
}

/*
 * since hpdma is not support to report how many chunks left to transfer,
 * we can only report that current desc is completed or not
 */
static enum dma_status mtk_hpdma_tx_status(struct dma_chan *chan,
					   dma_cookie_t cookie,
					   struct dma_tx_state *tx_state)
{
	return dma_cookie_status(chan, cookie, tx_state);
}

/* optimize the hpdma parameters to get maximum throughput */
static int mtk_hpdma_config_desc(struct hpdma_vdesc *hvdesc)
{
	hvdesc->axsize = 4;

	/*
	 * the total transfer length = axsize * total_num
	 * axsize can be 1, 2, 4, 8, 16 bytes
	 * calculate axsize
	 */
	while (hvdesc->axsize > 0 && hvdesc->len % (0x1 << hvdesc->axsize))
		hvdesc->axsize--;

	hvdesc->total_num = hvdesc->len / (0x1 << hvdesc->axsize);

	return 0;
}

static struct dma_async_tx_descriptor *mtk_hpdma_prep_dma_memcpy(struct dma_chan *chan,
								 dma_addr_t dst,
								 dma_addr_t src,
								 size_t len,
								 unsigned long flags)
{
	struct hpdma_vdesc *hvdesc;
	int ret = 0;

	if (!len)
		return ERR_PTR(-EPERM);

	if (dst > 0xFFFFFFFF || src > 0xFFFFFFFF)
		return ERR_PTR(-EINVAL);

	hvdesc = kzalloc(sizeof(struct hpdma_vdesc), GFP_NOWAIT);
	if (!hvdesc)
		return ERR_PTR(-ENOMEM);

	hvdesc->src = src;
	hvdesc->dst = dst;
	hvdesc->len = len;

	ret = mtk_hpdma_config_desc(hvdesc);
	if (ret) {
		kfree(hvdesc);
		return ERR_PTR(ret);
	}

	return vchan_tx_prep(to_virt_chan(chan), &hvdesc->vdesc, flags);
}

static void mtk_hpdma_terminate_all_inactive_desc(struct dma_chan *chan)
{
	struct virt_dma_chan *vchan = to_virt_chan(chan);
	unsigned long flag;
	LIST_HEAD(head);

	spin_lock_irqsave(&vchan->lock, flag);

	list_splice_tail_init(&vchan->desc_allocated, &head);
	list_splice_tail_init(&vchan->desc_submitted, &head);
	list_splice_tail_init(&vchan->desc_issued, &head);

	spin_unlock_irqrestore(&vchan->lock, flag);

	vchan_dma_desc_free_list(vchan, &head);
}

static int mtk_hpdma_terminate_all(struct dma_chan *chan)
{
	struct hpdma_vchan *hvchan = chan_to_hpdma_vchan(chan);

	hvchan->terminating = true;

	/* first terminate all inactive descriptors */
	mtk_hpdma_terminate_all_inactive_desc(chan);

	if (!hvchan->issued_desc)
		goto out;

	/* if there is a desc on the fly, we must wait until it done */
	wait_event_interruptible(hvchan->stop_wait, !hvchan->busy);

	vchan_terminate_vdesc(&hvchan->issued_desc->vdesc);

	hvchan->issued_desc = NULL;

	vchan_synchronize(&hvchan->vchan);

out:
	hvchan->terminating = false;

	return 0;
}

static void mtk_hpdma_vdesc_free(struct virt_dma_desc *vdesc)
{
	kfree(container_of(vdesc, struct hpdma_vdesc, vdesc));
}

static void mtk_hpdma_tx_work(struct work_struct *work)
{
	struct hpdma_vchan *hvchan = container_of(work, struct hpdma_vchan, tx_work);
	struct hpdma_dev *hpdma = chan_to_hpdma_dev(&hvchan->vchan.chan);
	unsigned long flag;

	if (unlikely(!vchan_next_desc(&hvchan->vchan)))
		return;

	spin_lock_irqsave(&hvchan->vchan.lock, flag);

	mtk_hpdma_issue_vchan_pending(hpdma, hvchan);

	spin_unlock_irqrestore(&hvchan->vchan.lock, flag);
}

static int mtk_hpdma_provider_init(struct platform_device *pdev,
				   struct hpdma_dev *hpdma)
{
	struct dma_device *ddev = &hpdma->ddev;
	int ret = 0;

	dma_cap_set(DMA_MEMCPY, ddev->cap_mask);
	dma_cap_set(DMA_PRIVATE, ddev->cap_mask);

	ddev->dev = &pdev->dev;
	ddev->directions = BIT(DMA_MEM_TO_MEM);
	ddev->copy_align = MTK_HPDMA_ALIGN_SIZE;
	ddev->src_addr_widths = MTK_HPDMA_DMA_BUSWIDTHS;
	ddev->dst_addr_widths = MTK_HPDMA_DMA_BUSWIDTHS;
	ddev->residue_granularity = DMA_RESIDUE_GRANULARITY_DESCRIPTOR;

	ddev->device_alloc_chan_resources = mtk_hpdma_alloc_chan_resources;
	ddev->device_free_chan_resources = mtk_hpdma_free_chan_resources;
	ddev->device_issue_pending = mtk_hpdma_issue_pending;
	ddev->device_tx_status = mtk_hpdma_tx_status;
	ddev->device_prep_dma_memcpy = mtk_hpdma_prep_dma_memcpy;
	ddev->device_terminate_all = mtk_hpdma_terminate_all;

	INIT_LIST_HEAD(&ddev->channels);

	ret = hpdma->ops.vchan_init(hpdma, ddev);
	if (ret)
		return ret;

	ret = dma_async_device_register(ddev);
	if (ret) {
		dev_err(&pdev->dev, "register async dma device failed: %d\n", ret);
		return ret;
	}

	ret = of_dma_controller_register(pdev->dev.of_node,
					 hpdma->ops.of_dma_xlate,
					 ddev);
	if (ret) {
		dev_err(&pdev->dev, "register dma controller failed: %d\n", ret);
		goto unregister_async_dev;
	}

	return ret;

unregister_async_dev:
	dma_async_device_unregister(ddev);

	return ret;
}

static int mtk_hpdma_probe(struct platform_device *pdev)
{
	const struct hpdma_init_data *init_data;
	struct hpdma_dev *hpdma;
	struct resource *res;
	int ret = 0;

	init_data = of_device_get_match_data(&pdev->dev);
	if (!init_data) {
		dev_err(&pdev->dev, "hpdma init data not exist\n");
		return -ENODEV;
	}

	hpdma = init_data->init(pdev, init_data);
	if (IS_ERR(hpdma)) {
		dev_err(&pdev->dev, "hpdma init failed: %ld\n", PTR_ERR(hpdma));
		return PTR_ERR(hpdma);
	}

	memcpy(&hpdma->ops, &init_data->ops, sizeof(struct hpdma_ops));
	hpdma->hwspinlock_grp = init_data->hwspinlock_grp;
	hpdma->trigger_start_slot = init_data->trigger_start_slot;
	hpdma->ch_base_slot = init_data->ch_base_slot;

	res = platform_get_resource_byname(pdev, IORESOURCE_MEM, "base");
	if (!res)
		return -ENXIO;

	hpdma->base = devm_ioremap(&pdev->dev, res->start, resource_size(res));
	if (!hpdma->base)
		return -ENOMEM;

	/*
	 * since hpdma does not send signal to APMCU,
	 * we need TOPS mailbox to notify us when hpdma done
	 */
	ret = hpdma->ops.mbox_init(pdev, hpdma);
	if (ret)
		return ret;

	ret = mtk_hpdma_provider_init(pdev, hpdma);
	if (ret)
		goto unregister_mbox;

	spin_lock_init(&hpdma->lock);

	platform_set_drvdata(pdev, hpdma);

	dev_info(hpdma->ddev.dev, "hpdma init done\n");

	return ret;

unregister_mbox:
	hpdma->ops.mbox_deinit(pdev, hpdma);

	return ret;
}

static int mtk_hpdma_remove(struct platform_device *pdev)
{
	struct hpdma_dev *hpdma = platform_get_drvdata(pdev);

	if (!hpdma)
		return 0;

	hpdma->ops.vchan_deinit(hpdma);

	hpdma->ops.mbox_deinit(pdev, hpdma);

	dma_async_device_unregister(&hpdma->ddev);

	of_dma_controller_free(pdev->dev.of_node);

	return 0;
}

static struct dma_chan *mtk_clust_hpdma_of_xlate(struct of_phandle_args *dma_spec,
						 struct of_dma *ofdma)
{
	struct dma_device *ddev = ofdma->of_dma_data;
	struct hpdma_dev *hpdma;
	u32 id;

	if (!ddev || dma_spec->args_count != 2)
		return ERR_PTR(-EINVAL);

	hpdma = container_of(ddev, struct hpdma_dev, ddev);
	id = dma_spec->args[0] * CORE_OFFLOAD_NUM + dma_spec->args[1];

	return dma_get_slave_channel(&hpdma->hvchans[id].vchan.chan);
}

static struct hpdma_dev *mtk_top_hpdma_init(struct platform_device *pdev,
					    const struct hpdma_init_data *data)
{
	struct top_hpdma_dev *top_hpdma = NULL;

	if (!data)
		return ERR_PTR(-EINVAL);

	top_hpdma = devm_kzalloc(&pdev->dev, sizeof(*top_hpdma), GFP_KERNEL);
	if (!top_hpdma)
		return ERR_PTR(-ENOMEM);

	top_hpdma->mdev.core = CORE_MGMT;
	top_hpdma->mdev.cmd_id = MBOX_CM2AP_CMD_HPDMA;
	top_hpdma->mdev.mbox_handler = data->mbox_handler;
	top_hpdma->mdev.priv = &top_hpdma->hpdma;

	return &top_hpdma->hpdma;
}

static void mtk_top_hpdma_vchan_deinit(struct hpdma_dev *hpdma)
{
	struct hpdma_vchan *hvchan;
	u32 i;

	for (i = 0; i < __TOP_HPDMA_REQ; i++) {
		hvchan = &hpdma->hvchans[i];
		__mtk_hpdma_vchan_deinit(&hvchan->vchan);
	}
}

static int mtk_top_hpdma_vchan_init(struct hpdma_dev *hpdma, struct dma_device *ddev)
{
	struct hpdma_vchan *hvchan;
	u32 i;

	hpdma->hvchans = devm_kcalloc(ddev->dev, __TOP_HPDMA_REQ,
				      sizeof(struct hpdma_vchan),
				      GFP_KERNEL);
	if (!hpdma->hvchans)
		return -ENOMEM;

	for (i = 0; i < __TOP_HPDMA_REQ; i++) {
		hvchan = &hpdma->hvchans[i];

		init_waitqueue_head(&hvchan->stop_wait);
		INIT_WORK(&hvchan->tx_work, mtk_hpdma_tx_work);

		hvchan->vchan.desc_free = mtk_hpdma_vdesc_free;
		/*
		 * TODO: maybe init vchan by ourselves with
		 * customized tasklet?
		 * if we setup customized tasklet to transmit
		 * remaining chunks in a channel, we should be careful about
		 * hpdma->lock since it will be acquired in softirq context
		 */
		vchan_init(&hvchan->vchan, ddev);
	}

	return 0;
}

static void mtk_top_hpdma_unregister_mbox(struct platform_device *pdev,
					  struct hpdma_dev *hpdma)
{
	struct top_hpdma_dev *top_hpdma;

	top_hpdma = container_of(hpdma, struct top_hpdma_dev, hpdma);

	unregister_mbox_dev(MBOX_RECV, &top_hpdma->mdev);
}

static int mtk_top_hpdma_register_mbox(struct platform_device *pdev,
				       struct hpdma_dev *hpdma)
{
	struct top_hpdma_dev *top_hpdma;
	int ret = 0;

	top_hpdma = container_of(hpdma, struct top_hpdma_dev, hpdma);

	ret = register_mbox_dev(MBOX_RECV, &top_hpdma->mdev);
	if (ret) {
		dev_err(&pdev->dev, "register mailbox device failed: %d\n", ret);
		return ret;
	}

	return ret;
}

static void mtk_top_hpdma_tx_pending_desc(struct hpdma_dev *hpdma,
					  struct hpdma_vchan *hvchan,
					  struct hpdma_vdesc *hvdesc)
{
	u32 slot = hpdma->ch_base_slot;
	enum hwspinlock_group grp = hpdma->hwspinlock_grp;

	hvchan->pchan_id = 0;

	mtk_hpdma_prepare_transfer(hpdma);

	/* occupy hpdma physical channel */
	while (!mtk_tops_hwspin_try_lock(grp, slot)) {

		if (unlikely(hvchan->terminating)) {
			spin_unlock(&hpdma->lock);
			return;
		}

		hvchan->pchan_id = (hvchan->pchan_id + 1) % HPDMA_CHAN_NUM;
		if (++slot - hpdma->ch_base_slot == HPDMA_CHAN_NUM)
			slot = hpdma->ch_base_slot;
	}

	mtk_hpdma_config_pchan(hpdma, hvchan, hvdesc);

	if (!mtk_hpdma_start_transfer(hpdma, hvchan, hvdesc))
		return;

	/* start transfer failed */
	mtk_tops_hwspin_unlock(grp, slot);

	mtk_hpdma_unprepare_transfer(hpdma);

	wake_up_interruptible(&hvchan->stop_wait);
}

static struct hpdma_dev *mtk_clust_hpdma_init(struct platform_device *pdev,
					      const struct hpdma_init_data *data)
{
	struct clust_hpdma_dev *clust_hpdma = NULL;
	u32 i;

	if (!data)
		return ERR_PTR(-EINVAL);

	clust_hpdma = devm_kzalloc(&pdev->dev, sizeof(*clust_hpdma), GFP_KERNEL);
	if (!clust_hpdma)
		return ERR_PTR(-ENOMEM);

	for (i = 0; i < CORE_OFFLOAD_NUM; i++) {
		clust_hpdma->mdev[i].core = CORE_OFFLOAD_0 + i;
		clust_hpdma->mdev[i].cmd_id = MBOX_CX2AP_CMD_HPDMA;
		clust_hpdma->mdev[i].mbox_handler = data->mbox_handler;
		clust_hpdma->mdev[i].priv = &clust_hpdma->hpdma;
	}

	return &clust_hpdma->hpdma;
}

static void mtk_clust_hpdma_vchan_deinit(struct hpdma_dev *hpdma)
{
	struct hpdma_vchan *hvchan;
	u32 i, j;

	for (i = 0; i < CORE_OFFLOAD_NUM; i++) {
		for (j = 0; j < __CLUST_HPDMA_REQ; j++) {
			hvchan = &hpdma->hvchans[i];
			__mtk_hpdma_vchan_deinit(&hvchan->vchan);
		}
	}
}

static int mtk_clust_hpdma_vchan_init(struct hpdma_dev *hpdma, struct dma_device *ddev)
{
	struct hpdma_vchan *hvchan;
	u32 i, j;

	hpdma->hvchans = devm_kcalloc(ddev->dev, __CLUST_HPDMA_REQ * CORE_OFFLOAD_NUM,
				      sizeof(struct hpdma_vchan),
				      GFP_KERNEL);
	if (!hpdma->hvchans)
		return -ENOMEM;

	for (i = 0; i < CORE_OFFLOAD_NUM; i++) {
		for (j = 0; j < __CLUST_HPDMA_REQ; j++) {
			hvchan = &hpdma->hvchans[i * __CLUST_HPDMA_REQ + j];

			hvchan->pchan_id = i;
			init_waitqueue_head(&hvchan->stop_wait);
			INIT_WORK(&hvchan->tx_work, mtk_hpdma_tx_work);

			hvchan->vchan.desc_free = mtk_hpdma_vdesc_free;
			/*
			 * TODO: maybe init vchan by ourselves with
			 * customized tasklet?
			 * if we setup customized tasklet to transmit
			 * remaining chunks in a channel, we should be careful about
			 * hpdma->lock since it will be acquired in softirq context
			 */
			vchan_init(&hvchan->vchan, ddev);
		}
	}

	return 0;
}

static void mtk_clust_hpdma_unregister_mbox(struct platform_device *pdev,
					    struct hpdma_dev *hpdma)
{
	struct clust_hpdma_dev *clust_hpdma;
	u32 i;

	clust_hpdma = container_of(hpdma, struct clust_hpdma_dev, hpdma);

	for (i = 0; i < CORE_OFFLOAD_NUM; i++)
		unregister_mbox_dev(MBOX_RECV, &clust_hpdma->mdev[i]);
}

static int mtk_clust_hpdma_register_mbox(struct platform_device *pdev,
					 struct hpdma_dev *hpdma)
{
	struct clust_hpdma_dev *clust_hpdma;
	int ret = 0;
	int i;

	clust_hpdma = container_of(hpdma, struct clust_hpdma_dev, hpdma);

	for (i = 0; i < CORE_OFFLOAD_NUM; i++) {
		ret = register_mbox_dev(MBOX_RECV, &clust_hpdma->mdev[i]);
		if (ret) {
			dev_err(&pdev->dev, "register mbox%d failed: %d\n", i, ret);
			goto unregister_mbox;
		}
	}

	return ret;

unregister_mbox:
	for (--i; i >= 0; i--)
		unregister_mbox_dev(MBOX_RECV, &clust_hpdma->mdev[i]);

	return ret;
}

static void mtk_clust_hpdma_tx_pending_desc(struct hpdma_dev *hpdma,
					    struct hpdma_vchan *hvchan,
					    struct hpdma_vdesc *hvdesc)
{
	u32 slot = hpdma->ch_base_slot + hvchan->pchan_id;
	enum hwspinlock_group grp = hpdma->hwspinlock_grp;

	mtk_hpdma_prepare_transfer(hpdma);

	/* occupy hpdma physical channel */
	mtk_tops_hwspin_lock(grp, slot);

	mtk_hpdma_config_pchan(hpdma, hvchan, hvdesc);

	if (!mtk_hpdma_start_transfer(hpdma, hvchan, hvdesc))
		return;

	/* start transfer failed */
	mtk_tops_hwspin_unlock(grp, slot);

	mtk_hpdma_unprepare_transfer(hpdma);

	wake_up_interruptible(&hvchan->stop_wait);
}

static enum mbox_msg_cnt mtk_hpdma_ap_recv_mbox_msg(struct mailbox_dev *mdev,
						    struct mailbox_msg *msg)
{
	struct hpdma_dev *hpdma = mdev->priv;
	struct hpdma_vchan *hvchan;
	struct hpdma_vdesc *hvdesc;
	enum hwspinlock_group grp;
	unsigned long flag;
	u32 slot;

	if (!hpdma)
		return MBOX_NO_RET_MSG;

	hvchan = hpdma->issued_chan;
	if (!hvchan) {
		dev_err(hpdma->ddev.dev, "unexpected hpdma mailbox recv\n");
		return MBOX_NO_RET_MSG;
	}

	grp = hpdma->hwspinlock_grp;

	hvdesc = hvchan->issued_desc;

	/* clear issued channel before releasing hwspinlock */
	hpdma->issued_chan = NULL;

	hvchan->busy = false;
	hvchan->issued_desc = NULL;

	/* release hwspinlock */
	slot = hvchan->pchan_id + hpdma->ch_base_slot;

	mtk_tops_hwspin_unlock(grp, hpdma->trigger_start_slot);

	mtk_tops_hwspin_unlock(grp, slot);

	/* release to let other APMCU process to contend hw spinlock */
	spin_unlock(&hpdma->lock);

	if (unlikely(hvchan->terminating)) {
		wake_up_interruptible(&hvchan->stop_wait);
		return MBOX_NO_RET_MSG;
	}

	/*
	 * complete vdesc and schedule tx work again
	 * if there is more vdesc left in the channel
	 */
	spin_lock_irqsave(&hvchan->vchan.lock, flag);

	vchan_cookie_complete(&hvdesc->vdesc);

	if (vchan_next_desc(&hvchan->vchan))
		schedule_work(&hvchan->tx_work);

	spin_unlock_irqrestore(&hvchan->vchan.lock, flag);

	return MBOX_NO_RET_MSG;
}

struct hpdma_init_data top_hpdma_init_data = {
	.ops = {
		.vchan_init = mtk_top_hpdma_vchan_init,
		.vchan_deinit = mtk_top_hpdma_vchan_deinit,
		.mbox_init = mtk_top_hpdma_register_mbox,
		.mbox_deinit = mtk_top_hpdma_unregister_mbox,
		.tx_pending_desc = mtk_top_hpdma_tx_pending_desc,
		.of_dma_xlate = of_dma_xlate_by_chan_id,
	},
	.init = mtk_top_hpdma_init,
	.mbox_handler = mtk_hpdma_ap_recv_mbox_msg,
	.hwspinlock_grp = HWSPINLOCK_GROUP_TOP,
	.trigger_start_slot = HWSPINLOCK_TOP_SLOT_HPDMA_LOCK,
	.ch_base_slot = HWSPINLOCK_TOP_SLOT_HPDMA_PCH0,
};

static struct hpdma_init_data clust_hpdma_init_data = {
	.ops = {
		.vchan_init = mtk_clust_hpdma_vchan_init,
		.vchan_deinit = mtk_clust_hpdma_vchan_deinit,
		.mbox_init = mtk_clust_hpdma_register_mbox,
		.mbox_deinit = mtk_clust_hpdma_unregister_mbox,
		.tx_pending_desc = mtk_clust_hpdma_tx_pending_desc,
		.of_dma_xlate = mtk_clust_hpdma_of_xlate,
	},
	.init = mtk_clust_hpdma_init,
	.mbox_handler = mtk_hpdma_ap_recv_mbox_msg,
	.hwspinlock_grp = HWSPINLOCK_GROUP_CLUST,
	.trigger_start_slot = HWSPINLOCK_CLUST_SLOT_HPDMA_LOCK,
	.ch_base_slot = HWSPINLOCK_CLUST_SLOT_HPDMA_PCH0,
};

static struct of_device_id mtk_hpdma_match[] = {
	{ .compatible = "mediatek,hpdma-top", .data = &top_hpdma_init_data, },
	{ .compatible = "mediatek,hpdma-sub", .data = &clust_hpdma_init_data, },
	{ },
};

static struct platform_driver mtk_hpdma_driver = {
	.probe = mtk_hpdma_probe,
	.remove = mtk_hpdma_remove,
	.driver = {
		.name = "mediatek,hpdma",
		.owner = THIS_MODULE,
		.of_match_table = mtk_hpdma_match,
	},
};

int __init mtk_tops_hpdma_init(void)
{
	int ret = 0;

	ret = platform_driver_register(&mtk_hpdma_driver);
	if (ret)
		return ret;

	return ret;
}

void __exit mtk_tops_hpdma_exit(void)
{
	platform_driver_unregister(&mtk_hpdma_driver);
}
