// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (c) 2023 MediaTek Inc. All Rights Reserved.
 *
 * Author: Ren-Ting Wang <ren-ting.wang@mediatek.com>
 */

#include <linux/io.h>
#include <linux/of.h>
#include <linux/err.h>
#include <linux/ktime.h>
#include <linux/device.h>
#include <linux/spinlock.h>
#include <linux/interrupt.h>
#include <linux/of_address.h>
#include <linux/platform_device.h>

#include "tops/mcu.h"
#include "tops/mbox.h"
#include "tops/internal.h"

#define MBOX_SEND_TIMEOUT		(2000)

struct mailbox_reg {
	u32 cmd_set_reg;
	u32 cmd_clr_reg;
	u32 msg_reg;
};

struct mailbox_core {
	struct list_head mdev_list;
	u32 registered_cmd;
	spinlock_t lock;
};

struct mailbox_hw {
	struct mailbox_core core[MBOX_ACT_MAX][CORE_MAX];
	struct device *dev;
	void __iomem *base;
};

static struct mailbox_hw mbox;

static inline void mbox_write(u32 reg, u32 val)
{
	writel(val, mbox.base + reg);
}

static inline void mbox_set(u32 reg, u32 mask)
{
	setbits(mbox.base + reg, mask);
}

static inline void mbox_clr(u32 reg, u32 mask)
{
	clrbits(mbox.base + reg, mask);
}

static inline void mbox_rmw(u32 reg, u32 mask, u32 val)
{
	clrsetbits(mbox.base + reg, mask, val);
}

static inline u32 mbox_read(u32 reg)
{
	return readl(mbox.base + reg);
}

static inline void mbox_fill_msg(enum mbox_msg_cnt cnt, struct mailbox_msg *msg,
				 struct mailbox_reg *mbox_reg)
{
	if (cnt == MBOX_RET_MSG4)
		goto send_msg4;
	else if (cnt == MBOX_RET_MSG3)
		goto send_msg3;
	else if (cnt == MBOX_RET_MSG2)
		goto send_msg2;
	else if (cnt == MBOX_RET_MSG1)
		goto send_msg1;
	else
		return;

send_msg4:
	mbox_write(mbox_reg->msg_reg + 0x10, msg->msg4);
send_msg3:
	mbox_write(mbox_reg->msg_reg + 0xC, msg->msg3);
send_msg2:
	mbox_write(mbox_reg->msg_reg + 0x8, msg->msg3);
send_msg1:
	mbox_write(mbox_reg->msg_reg + 0x4, msg->msg1);
}

static inline void mbox_clear_msg(enum mbox_msg_cnt cnt,
				  struct mailbox_reg *mbox_reg)
{
	if (cnt == MBOX_NO_RET_MSG)
		goto clear_msg4;
	else if (cnt == MBOX_RET_MSG1)
		goto clear_msg3;
	else if (cnt == MBOX_RET_MSG2)
		goto clear_msg2;
	else if (cnt == MBOX_RET_MSG3)
		goto clear_msg1;
	else
		return;

clear_msg4:
	mbox_write(mbox_reg->msg_reg + 0x4, 0);
clear_msg3:
	mbox_write(mbox_reg->msg_reg + 0x8, 0);
clear_msg2:
	mbox_write(mbox_reg->msg_reg + 0xC, 0);
clear_msg1:
	mbox_write(mbox_reg->msg_reg + 0x10, 0);
}

static void exec_mbox_handler(enum core_id core, struct mailbox_reg *mbox_reg)
{
	struct mailbox_core *mcore = &mbox.core[MBOX_RECV][core];
	struct mailbox_dev *mdev = NULL;
	struct mailbox_msg msg = {0};
	enum mbox_msg_cnt ret = 0;
	u32 cmd_id = 0;

	cmd_id = mbox_read(mbox_reg->msg_reg);

	list_for_each_entry(mdev, &mcore->mdev_list, list) {
		if (mdev->cmd_id == cmd_id) {
			if (!mdev->mbox_handler)
				goto out;

			/* setup msg for handler */
			msg.msg1 = mbox_read(mbox_reg->msg_reg + 0x4);
			msg.msg2 = mbox_read(mbox_reg->msg_reg + 0x8);
			msg.msg3 = mbox_read(mbox_reg->msg_reg + 0xC);
			msg.msg4 = mbox_read(mbox_reg->msg_reg + 0x10);

			ret = mdev->mbox_handler(mdev, &msg);

			mbox_fill_msg(ret, &msg, mbox_reg);

			break;
		}
	}
out:
	mbox_write(mbox_reg->msg_reg, 0);
	mbox_clear_msg(ret, mbox_reg);

	/* clear cmd */
	mbox_write(mbox_reg->cmd_clr_reg, 0xFFFFFFFF);
}

static irqreturn_t mtk_tops_mbox_handler(int irq, void *dev_id)
{
	struct mailbox_reg mreg = {0};
	u32 cluster_reg = 0;
	u32 top_reg = 0;

	top_reg = mbox_read(TOPS_TOP_AP_SLOT);
	cluster_reg = mbox_read(TOPS_CLUST0_AP_SLOT);

	if (top_reg & MBOX_TOP_MBOX_FROM_CM) {
		mreg.cmd_set_reg = TOPS_TOP_CM_TO_AP_CMD_SET;
		mreg.cmd_clr_reg = TOPS_TOP_CM_TO_AP_CMD_CLR;
		mreg.msg_reg = TOPS_TOP_CM_TO_AP_MSG_N(0);
		exec_mbox_handler(CORE_MGMT, &mreg);
	}
	if (cluster_reg & MBOX_CLUST0_MBOX_FROM_C0) {
		mreg.cmd_set_reg = TOPS_CLUST0_CX_TO_AP_CMD_SET(0);
		mreg.cmd_clr_reg = TOPS_CLUST0_CX_TO_AP_CMD_CLR(0);
		mreg.msg_reg = TOPS_CLUST0_CX_TO_AP_MSG_N(0, 0);
		exec_mbox_handler(CORE_OFFLOAD_0, &mreg);
	}
	if (cluster_reg & MBOX_CLUST0_MBOX_FROM_C1) {
		mreg.cmd_set_reg = TOPS_CLUST0_CX_TO_AP_CMD_SET(1);
		mreg.cmd_clr_reg = TOPS_CLUST0_CX_TO_AP_CMD_CLR(1);
		mreg.msg_reg = TOPS_CLUST0_CX_TO_AP_MSG_N(1, 0);
		exec_mbox_handler(CORE_OFFLOAD_1, &mreg);
	}
	if (cluster_reg & MBOX_CLUST0_MBOX_FROM_C2) {
		mreg.cmd_set_reg = TOPS_CLUST0_CX_TO_AP_CMD_SET(2);
		mreg.cmd_clr_reg = TOPS_CLUST0_CX_TO_AP_CMD_CLR(2);
		mreg.msg_reg = TOPS_CLUST0_CX_TO_AP_MSG_N(2, 0);
		exec_mbox_handler(CORE_OFFLOAD_2, &mreg);
	}
	if (cluster_reg & MBOX_CLUST0_MBOX_FROM_C3) {
		mreg.cmd_set_reg = TOPS_CLUST0_CX_TO_AP_CMD_SET(3);
		mreg.cmd_clr_reg = TOPS_CLUST0_CX_TO_AP_CMD_CLR(3);
		mreg.msg_reg = TOPS_CLUST0_CX_TO_AP_MSG_N(3, 0);
		exec_mbox_handler(CORE_OFFLOAD_3, &mreg);
	}

	return IRQ_HANDLED;
}

static int mbox_get_send_reg(struct mailbox_dev *mdev,
			     struct mailbox_reg *mbox_reg)
{
	if (!mdev) {
		dev_notice(mbox.dev, "no mdev specified!\n");
		return -EINVAL;
	}

	if (mdev->core == CORE_MGMT) {
		mbox_reg->cmd_set_reg = TOPS_TOP_AP_TO_CM_CMD_SET;
		mbox_reg->cmd_clr_reg = TOPS_TOP_AP_TO_CM_CMD_CLR;
		mbox_reg->msg_reg = TOPS_TOP_AP_TO_CM_MSG_N(0);
	} else if (mdev->core == CORE_OFFLOAD_0) {
		mbox_reg->cmd_set_reg = TOPS_CLUST0_AP_TO_CX_CMD_SET(0);
		mbox_reg->cmd_clr_reg = TOPS_CLUST0_AP_TO_CX_CMD_CLR(0);
		mbox_reg->msg_reg = TOPS_CLUST0_AP_TO_CX_MSG_N(0, 0);
	} else if (mdev->core == CORE_OFFLOAD_1) {
		mbox_reg->cmd_set_reg = TOPS_CLUST0_AP_TO_CX_CMD_SET(1);
		mbox_reg->cmd_clr_reg = TOPS_CLUST0_AP_TO_CX_CMD_CLR(1);
		mbox_reg->msg_reg = TOPS_CLUST0_AP_TO_CX_MSG_N(1, 0);
	} else if (mdev->core == CORE_OFFLOAD_2) {
		mbox_reg->cmd_set_reg = TOPS_CLUST0_AP_TO_CX_CMD_SET(2);
		mbox_reg->cmd_clr_reg = TOPS_CLUST0_AP_TO_CX_CMD_CLR(2);
		mbox_reg->msg_reg = TOPS_CLUST0_AP_TO_CX_MSG_N(2, 0);
	} else if (mdev->core == CORE_OFFLOAD_3) {
		mbox_reg->cmd_set_reg = TOPS_CLUST0_AP_TO_CX_CMD_SET(3);
		mbox_reg->cmd_clr_reg = TOPS_CLUST0_AP_TO_CX_CMD_CLR(3);
		mbox_reg->msg_reg = TOPS_CLUST0_AP_TO_CX_MSG_N(3, 0);
	} else {
		dev_notice(mbox.dev, "invalid mdev->core: %u\n", mdev->core);
		return -EINVAL;
	}

	return 0;
}

static void mbox_post_send(u32 msg_reg, struct mailbox_msg *msg,
			   void *priv,
			   mbox_ret_func_t ret_handler)
{
	if (!ret_handler)
		goto out;

	msg->msg1 = mbox_read(msg_reg + 0x4);
	msg->msg2 = mbox_read(msg_reg + 0x8);
	msg->msg3 = mbox_read(msg_reg + 0xC);
	msg->msg4 = mbox_read(msg_reg + 0x10);

	ret_handler(priv, msg);

out:
	mbox_write(msg_reg, 0);
	mbox_write(msg_reg + 0x4, 0);
	mbox_write(msg_reg + 0x8, 0);
	mbox_write(msg_reg + 0xC, 0);
	mbox_write(msg_reg + 0x10, 0);
}

static inline bool mbox_send_msg_chk_timeout(ktime_t start)
{
	return ktime_to_us(ktime_sub(ktime_get(), start)) > MBOX_SEND_TIMEOUT;
}

static inline int __mbox_send_msg_no_wait_irq(struct mailbox_dev *mdev,
					      struct mailbox_msg *msg,
					      struct mailbox_reg *mbox_reg)
{
	ktime_t start;

	if (!mdev || !msg || !mbox_reg) {
		dev_notice(mbox.dev, "missing some necessary parameters!\n");
		return -EPERM;
	}

	start = ktime_get();

	/* wait for all cmd cleared */
	while (mbox_read(mbox_reg->cmd_set_reg)) {
		if (mbox_send_msg_chk_timeout(start)) {
			dev_notice(mbox.dev, "mbox occupied too long\n");
			dev_notice(mbox.dev, "cmd set reg (0x%x): 0x%x\n",
				mbox_reg->cmd_set_reg,
				mbox_read(mbox_reg->cmd_set_reg));
			dev_notice(mbox.dev, "msg1 reg (0x%x): 0x%x\n",
				mbox_reg->msg_reg,
				mbox_read(mbox_reg->msg_reg));
			dev_notice(mbox.dev, "msg2 reg (0x%x): 0x%x\n",
				mbox_reg->msg_reg,
				mbox_read(mbox_reg->msg_reg + 0x4));
			dev_notice(mbox.dev, "msg3 reg (0x%x): 0x%x\n",
				mbox_reg->msg_reg,
				mbox_read(mbox_reg->msg_reg + 0x8));
			dev_notice(mbox.dev, "msg4 reg (0x%x): 0x%x\n",
				mbox_reg->msg_reg,
				mbox_read(mbox_reg->msg_reg + 0xC));
			dev_notice(mbox.dev, "msg5 reg (0x%x): 0x%x\n",
				mbox_reg->msg_reg,
				mbox_read(mbox_reg->msg_reg + 0x10));
			WARN_ON(1);
		}
	}

	/* write msg */
	mbox_write(mbox_reg->msg_reg, mdev->cmd_id);
	mbox_write(mbox_reg->msg_reg + 0x4, msg->msg1);
	mbox_write(mbox_reg->msg_reg + 0x8, msg->msg2);
	mbox_write(mbox_reg->msg_reg + 0xC, msg->msg3);
	mbox_write(mbox_reg->msg_reg + 0x10, msg->msg4);

	/* write cmd */
	mbox_write(mbox_reg->cmd_set_reg, BIT(mdev->cmd_id));

	return 0;
}

int mbox_send_msg_no_wait_irq(struct mailbox_dev *mdev, struct mailbox_msg *msg)
{
	struct mailbox_reg mbox_reg = {0};
	int ret = 0;

	ret = mbox_get_send_reg(mdev, &mbox_reg);
	if (ret)
		return ret;

	spin_lock(&mbox.core[MBOX_SEND][mdev->core].lock);

	/* send cmd + msg */
	ret = __mbox_send_msg_no_wait_irq(mdev, msg, &mbox_reg);

	spin_unlock(&mbox.core[MBOX_SEND][mdev->core].lock);

	return ret;
}
EXPORT_SYMBOL(mbox_send_msg_no_wait_irq);

int mbox_send_msg_no_wait(struct mailbox_dev *mdev, struct mailbox_msg *msg)
{
	struct mailbox_reg mbox_reg = {0};
	unsigned long flag = 0;
	int ret = 0;

	ret = mbox_get_send_reg(mdev, &mbox_reg);
	if (ret)
		return ret;

	spin_lock_irqsave(&mbox.core[MBOX_SEND][mdev->core].lock, flag);

	/* send cmd + msg */
	ret = __mbox_send_msg_no_wait_irq(mdev, msg, &mbox_reg);

	spin_unlock_irqrestore(&mbox.core[MBOX_SEND][mdev->core].lock, flag);

	return ret;
}
EXPORT_SYMBOL(mbox_send_msg_no_wait);

int mbox_send_msg(struct mailbox_dev *mdev, struct mailbox_msg *msg, void *priv,
		  mbox_ret_func_t ret_handler)
{
	struct mailbox_reg mbox_reg = {0};
	unsigned long flag = 0;
	ktime_t start;
	int ret = 0;

	ret = mbox_get_send_reg(mdev, &mbox_reg);
	if (ret)
		return ret;

	spin_lock_irqsave(&mbox.core[MBOX_SEND][mdev->core].lock, flag);

	/* send cmd + msg */
	ret = __mbox_send_msg_no_wait_irq(mdev, msg, &mbox_reg);

	start = ktime_get();

	/* wait for cmd clear */
	while (mbox_read(mbox_reg.cmd_set_reg) & BIT(mdev->cmd_id))
		mbox_send_msg_chk_timeout(start);

	/* execute return handler and clear message */
	mbox_post_send(mbox_reg.msg_reg, msg, priv, ret_handler);

	spin_unlock_irqrestore(&mbox.core[MBOX_SEND][mdev->core].lock, flag);

	return ret;
}
EXPORT_SYMBOL(mbox_send_msg);

static inline int mbox_ctrl_sanity_check(enum core_id core, enum mbox_act act)
{
	/* sanity check */
	if (core >= CORE_MAX || act >= MBOX_ACT_MAX)
		return -EINVAL;

	/* mbox handler should not be register to core itself */
	if (core == CORE_AP)
		return -EINVAL;

	return 0;
}

static void __register_mbox_dev(struct mailbox_core *mcore,
				struct mailbox_dev *mdev)
{
	struct mailbox_dev *cur = NULL;

	INIT_LIST_HEAD(&mdev->list);

	/* insert the mailbox_dev in order */
	list_for_each_entry(cur, &mcore->mdev_list, list)
		if (cur->cmd_id > mdev->cmd_id)
			break;

	list_add(&mdev->list, &cur->list);

	mcore->registered_cmd |= (0x1 << mdev->cmd_id);
}

static void __unregister_mbox_dev(struct mailbox_core *mcore,
				  struct mailbox_dev *mdev)
{
	struct mailbox_dev *cur = NULL;
	struct mailbox_dev *next = NULL;

	/* ensure the node being deleted is existed in the list */
	list_for_each_entry_safe(cur, next, &mcore->mdev_list, list) {
		if (cur->cmd_id == mdev->cmd_id && cur == mdev) {
			list_del(&mdev->list);
			break;
		}
	}

	mcore->registered_cmd &= (~(0x1 << mdev->cmd_id));
}

int register_mbox_dev(enum mbox_act act, struct mailbox_dev *mdev)
{
	struct mailbox_core *mcore;
	int ret = 0;

	/* sanity check */
	ret = mbox_ctrl_sanity_check(mdev->core, act);
	if (ret)
		return ret;

	mcore = &mbox.core[act][mdev->core];

	/* check cmd is occupied or not */
	if (mcore->registered_cmd & (0x1 << mdev->cmd_id))
		return -EBUSY;

	__register_mbox_dev(mcore, mdev);

	return 0;
}
EXPORT_SYMBOL(register_mbox_dev);

int unregister_mbox_dev(enum mbox_act act, struct mailbox_dev *mdev)
{
	struct mailbox_core *mcore;
	int ret = 0;

	/* sanity check */
	ret = mbox_ctrl_sanity_check(mdev->core, act);
	if (ret)
		return ret;

	mcore = &mbox.core[act][mdev->core];

	/* check cmd need to unregister or not */
	if (!(mcore->registered_cmd & (0x1 << mdev->cmd_id)))
		return 0;

	__unregister_mbox_dev(mcore, mdev);

	return 0;
}
EXPORT_SYMBOL(unregister_mbox_dev);

void mtk_tops_mbox_clear_all_cmd(void)
{
	u32 i, j;

	mbox_write(TOPS_TOP_AP_TO_CM_CMD_CLR, 0xFFFFFFFF);
	mbox_write(TOPS_TOP_CM_TO_AP_CMD_CLR, 0xFFFFFFFF);

	for (i = 0; i < CORE_OFFLOAD_NUM; i++) {
		mbox_write(TOPS_CLUST0_CX_TO_CM_CMD_CLR(i), 0xFFFFFFFF);
		mbox_write(TOPS_CLUST0_CM_TO_CX_CMD_CLR(i), 0xFFFFFFFF);
		mbox_write(TOPS_CLUST0_CX_TO_AP_CMD_CLR(i), 0xFFFFFFFF);
		mbox_write(TOPS_CLUST0_AP_TO_CX_CMD_CLR(i), 0xFFFFFFFF);

		for (j = 0; j < CORE_OFFLOAD_NUM; j++) {
			if (i == j)
				continue;

			mbox_write(TOPS_CLUST0_CX_TO_CY_CMD_CLR(i, j), 0xFFFFFFFF);
		}
	}
}

static int mtk_tops_mbox_probe(struct platform_device *pdev)
{
	struct device_node *tops = NULL;
	struct resource res;
	int irq = platform_get_irq_byname(pdev, "mbox");
	int ret = 0;
	u32 idx = 0;

	mbox.dev = &pdev->dev;

	tops = of_parse_phandle(pdev->dev.of_node, "tops", 0);
	if (!tops) {
		dev_err(mbox.dev, "can not find tops node\n");
		return -ENODEV;
	}

	if (of_address_to_resource(tops, 0, &res))
		return -ENXIO;

	mbox.base = devm_ioremap(mbox.dev, res.start, resource_size(&res));
	if (!mbox.base)
		return -ENOMEM;

	if (irq < 0) {
		dev_err(mbox.dev, "get mbox irq failed\n");
		return irq;
	}

	ret = devm_request_irq(&pdev->dev, irq,
			       mtk_tops_mbox_handler,
			       IRQF_ONESHOT,
			       pdev->name, NULL);
	if (ret) {
		dev_err(mbox.dev, "request mbox irq failed\n");
		return ret;
	}

	for (idx = 0; idx < CORE_MAX; idx++) {
		INIT_LIST_HEAD(&mbox.core[MBOX_SEND][idx].mdev_list);
		INIT_LIST_HEAD(&mbox.core[MBOX_RECV][idx].mdev_list);
		spin_lock_init(&mbox.core[MBOX_SEND][idx].lock);
		spin_lock_init(&mbox.core[MBOX_RECV][idx].lock);
	}

	mtk_tops_mbox_clear_all_cmd();

	return ret;
}

static int mtk_tops_mbox_remove(struct platform_device *pdev)
{
	return 0;
}

static struct of_device_id mtk_mbox_match[] = {
	{ .compatible = "mediatek,tops-mbox", },
	{ },
};

static struct platform_driver mtk_tops_mbox_driver = {
	.probe = mtk_tops_mbox_probe,
	.remove = mtk_tops_mbox_remove,
	.driver = {
		.name = "mediatek,tops-mbox",
		.owner = THIS_MODULE,
		.of_match_table = mtk_mbox_match,
	},
};

int __init mtk_tops_mbox_init(void)
{
	return platform_driver_register(&mtk_tops_mbox_driver);
}

void __exit mtk_tops_mbox_exit(void)
{
	platform_driver_unregister(&mtk_tops_mbox_driver);
}
