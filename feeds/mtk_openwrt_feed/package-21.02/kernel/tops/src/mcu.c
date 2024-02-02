// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (c) 2023 MediaTek Inc. All Rights Reserved.
 *
 * Author: Ren-Ting Wang <ren-ting.wang@mediatek.com>
 */

#include <linux/delay.h>
#include <linux/device.h>
#include <linux/dma-mapping.h>
#include <linux/err.h>
#include <linux/firmware.h>
#include <linux/io.h>
#include <linux/kthread.h>
#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/of_device.h>
#include <linux/of_platform.h>
#include <linux/platform_device.h>
#include <linux/pm_domain.h>
#include <linux/pm_runtime.h>

#include <pce/pce.h>

#include "tops/ctrl.h"
#include "tops/firmware.h"
#include "tops/hpdma.h"
#include "tops/internal.h"
#include "tops/mbox.h"
#include "tops/mcu.h"
#include "tops/netsys.h"
#include "tops/tdma.h"
#include "tops/trm.h"

#define TDMA_TIMEOUT_MAX_CNT			(3)
#define TDMA_TIMEOUT_DELAY			(100) /* 100ms */

#define MCU_STATE_TRANS_TIMEOUT			(5000) /* 5000ms */
#define MCU_CTRL_DONE_BIT			(31)
#define MCU_CTRL_DONE				(CORE_TOPS_MASK | \
						BIT(MCU_CTRL_DONE_BIT))

/* TRM dump length */
#define TOP_CORE_BASE_LEN			(0x80)
#define TOP_L2SRAM_LEN				(0x40000)
#define TOP_CORE_M_XTCM_LEN			(0x8000)

#define CLUST_CORE_BASE_LEN			(0x80)
#define CLUST_L2SRAM_LEN			(0x40000)
#define CLUST_CORE_X_XTCM_LEN			(0x8000)

/* MCU State */
#define MCU_STATE_FUNC_DECLARE(name)						\
static int mtk_tops_mcu_state_ ## name ## _enter(struct mcu_state *state);	\
static int mtk_tops_mcu_state_ ## name ## _leave(struct mcu_state *state);	\
static struct mcu_state *mtk_tops_mcu_state_ ## name ## _trans(			\
					u32 mcu_act,			\
					struct mcu_state *state)

#define MCU_STATE_DATA(name, id)						\
	[id] = {								\
		.state = id,							\
		.state_trans = mtk_tops_mcu_state_ ## name ## _trans,		\
		.enter = mtk_tops_mcu_state_ ## name ## _enter,			\
		.leave = mtk_tops_mcu_state_ ## name ## _leave,			\
	}

static inline void mcu_ctrl_issue_pending_act(u32 mcu_act);
static enum mbox_msg_cnt mtk_tops_ap_recv_mgmt_mbox_msg(struct mailbox_dev *mdev,
							struct mailbox_msg *msg);
static enum mbox_msg_cnt mtk_tops_ap_recv_offload_mbox_msg(struct mailbox_dev *mdev,
							   struct mailbox_msg *msg);
static int mcu_trm_hw_dump(void *dst, u32 ofs, u32 len);

MCU_STATE_FUNC_DECLARE(shutdown);
MCU_STATE_FUNC_DECLARE(init);
MCU_STATE_FUNC_DECLARE(freerun);
MCU_STATE_FUNC_DECLARE(stall);
MCU_STATE_FUNC_DECLARE(netstop);
MCU_STATE_FUNC_DECLARE(reset);
MCU_STATE_FUNC_DECLARE(abnormal);

struct npu {
	void __iomem *base;

	struct clk *bus_clk;
	struct clk *sram_clk;
	struct clk *xdma_clk;
	struct clk *offload_clk;
	struct clk *mgmt_clk;

	struct device **pd_devices;
	struct device_link **pd_links;
	int pd_num;

	struct task_struct *mcu_ctrl_thread;
	struct timer_list mcu_ctrl_timer;
	struct mcu_state *next_state;
	struct mcu_state *cur_state;
	/* ensure that only 1 user can trigger state transition at a time */
	struct mutex mcu_ctrl_lock;
	spinlock_t pending_act_lock;
	wait_queue_head_t mcu_ctrl_wait_act;
	wait_queue_head_t mcu_state_wait_done;
	bool mcu_bring_up_done;
	bool state_trans_fail;
	u32 pending_act;

	spinlock_t ctrl_done_lock;
	wait_queue_head_t mcu_ctrl_wait_done;
	enum mcu_cmd_type ctrl_done_cmd;
	/* MSB = 1 means that mcu control done. Otherwise it is still ongoing */
	u32 ctrl_done;

	struct work_struct recover_work;
	bool in_reset;
	bool in_recover;
	bool netsys_fe_ser;
	bool shuting_down;

	struct mailbox_msg ctrl_msg;
	struct mailbox_dev recv_mgmt_mbox_dev;
	struct mailbox_dev send_mgmt_mbox_dev;

	struct mailbox_dev recv_offload_mbox_dev[CORE_OFFLOAD_NUM];
	struct mailbox_dev send_offload_mbox_dev[CORE_OFFLOAD_NUM];
};

static struct mcu_state mcu_states[__MCU_STATE_TYPE_MAX] = {
	MCU_STATE_DATA(shutdown, MCU_STATE_TYPE_SHUTDOWN),
	MCU_STATE_DATA(init, MCU_STATE_TYPE_INIT),
	MCU_STATE_DATA(freerun, MCU_STATE_TYPE_FREERUN),
	MCU_STATE_DATA(stall, MCU_STATE_TYPE_STALL),
	MCU_STATE_DATA(netstop, MCU_STATE_TYPE_NETSTOP),
	MCU_STATE_DATA(reset, MCU_STATE_TYPE_RESET),
	MCU_STATE_DATA(abnormal, MCU_STATE_TYPE_ABNORMAL),
};

static struct npu npu = {
	.send_mgmt_mbox_dev = MBOX_SEND_MGMT_DEV(CORE_CTRL),
	.send_offload_mbox_dev = {
		[CORE_OFFLOAD_0] = MBOX_SEND_OFFLOAD_DEV(0, CORE_CTRL),
		[CORE_OFFLOAD_1] = MBOX_SEND_OFFLOAD_DEV(1, CORE_CTRL),
		[CORE_OFFLOAD_2] = MBOX_SEND_OFFLOAD_DEV(2, CORE_CTRL),
		[CORE_OFFLOAD_3] = MBOX_SEND_OFFLOAD_DEV(3, CORE_CTRL),
	},
	.recv_mgmt_mbox_dev =
		MBOX_RECV_MGMT_DEV(CORE_CTRL, mtk_tops_ap_recv_mgmt_mbox_msg),
	.recv_offload_mbox_dev = {
		[CORE_OFFLOAD_0] =
			MBOX_RECV_OFFLOAD_DEV(0,
					      CORE_CTRL,
					      mtk_tops_ap_recv_offload_mbox_msg
					     ),
		[CORE_OFFLOAD_1] =
			MBOX_RECV_OFFLOAD_DEV(1,
					      CORE_CTRL,
					      mtk_tops_ap_recv_offload_mbox_msg
					     ),
		[CORE_OFFLOAD_2] =
			MBOX_RECV_OFFLOAD_DEV(2,
					      CORE_CTRL,
					      mtk_tops_ap_recv_offload_mbox_msg
					     ),
		[CORE_OFFLOAD_3] =
			MBOX_RECV_OFFLOAD_DEV(3,
					      CORE_CTRL,
					      mtk_tops_ap_recv_offload_mbox_msg
					     ),
	},
};

static struct trm_config mcu_trm_cfgs[] = {
	{
		TRM_CFG_EN("top-core-base",
			   TOP_CORE_BASE, TOP_CORE_BASE_LEN,
			   0x0, TOP_CORE_BASE_LEN,
			   0)
	},
	{
		TRM_CFG_EN("clust-core0-base",
			   CLUST_CORE_BASE(0), CLUST_CORE_BASE_LEN,
			   0x0, CLUST_CORE_BASE_LEN,
			   0)
	},
	{
		TRM_CFG_EN("clust-core1-base",
			   CLUST_CORE_BASE(1), CLUST_CORE_BASE_LEN,
			   0x0, CLUST_CORE_BASE_LEN,
			   0)
	},
	{
		TRM_CFG_EN("clust-core2-base",
			   CLUST_CORE_BASE(2), CLUST_CORE_BASE_LEN,
			   0x0, CLUST_CORE_BASE_LEN,
			   0)
	},
	{
		TRM_CFG_EN("clust-core3-base",
			   CLUST_CORE_BASE(3), CLUST_CORE_BASE_LEN,
			   0x0, CLUST_CORE_BASE_LEN,
			   0)
	},
	{
		TRM_CFG_CORE_DUMP_EN("top-core-m-dtcm",
				     TOP_CORE_M_DTCM, TOP_CORE_M_XTCM_LEN,
				     0x0, TOP_CORE_M_XTCM_LEN,
				     0, CORE_MGMT)
	},
	{
		TRM_CFG_CORE_DUMP_EN("clust-core-0-dtcm",
				     CLUST_CORE_X_DTCM(0), CLUST_CORE_X_XTCM_LEN,
				     0x0, CLUST_CORE_X_XTCM_LEN,
				     0, CORE_OFFLOAD_0)
	},
	{
		TRM_CFG_CORE_DUMP_EN("clust-core-1-dtcm",
				     CLUST_CORE_X_DTCM(1), CLUST_CORE_X_XTCM_LEN,
				     0x0, CLUST_CORE_X_XTCM_LEN,
				     0, CORE_OFFLOAD_1)
	},
	{
		TRM_CFG_CORE_DUMP_EN("clust-core-2-dtcm",
				     CLUST_CORE_X_DTCM(2), CLUST_CORE_X_XTCM_LEN,
				     0x0, CLUST_CORE_X_XTCM_LEN,
				     0, CORE_OFFLOAD_2)
	},
	{
		TRM_CFG_CORE_DUMP_EN("clust-core-3-dtcm",
				     CLUST_CORE_X_DTCM(3), CLUST_CORE_X_XTCM_LEN,
				     0x0, CLUST_CORE_X_XTCM_LEN,
				     0, CORE_OFFLOAD_3)
	},
	{
		TRM_CFG("top-core-m-itcm",
			TOP_CORE_M_ITCM, TOP_CORE_M_XTCM_LEN,
			0x0, TOP_CORE_M_XTCM_LEN,
			0)
	},
	{
		TRM_CFG("clust-core-0-itcm",
			CLUST_CORE_X_ITCM(0), CLUST_CORE_X_XTCM_LEN,
			0x0, CLUST_CORE_X_XTCM_LEN,
			0)
	},
	{
		TRM_CFG("clust-core-1-itcm",
			CLUST_CORE_X_ITCM(1), CLUST_CORE_X_XTCM_LEN,
			0x0, CLUST_CORE_X_XTCM_LEN,
			0)
	},
	{
		TRM_CFG("clust-core-2-itcm",
			CLUST_CORE_X_ITCM(2), CLUST_CORE_X_XTCM_LEN,
			0x0, CLUST_CORE_X_XTCM_LEN,
			0)
	},
	{
		TRM_CFG("clust-core-3-itcm",
			CLUST_CORE_X_ITCM(3), CLUST_CORE_X_XTCM_LEN,
			0x0, CLUST_CORE_X_XTCM_LEN,
			0)
	},
	{
		TRM_CFG("top-l2sram",
			TOP_L2SRAM, TOP_L2SRAM_LEN,
			0x0, TOP_L2SRAM_LEN,
			0)
	},
	{
		TRM_CFG_EN("clust-l2sram",
			   CLUST_L2SRAM, CLUST_L2SRAM_LEN,
			   0x38000, 0x8000,
			   0)
	},
};

static struct trm_hw_config mcu_trm_hw_cfg = {
	.trm_cfgs = mcu_trm_cfgs,
	.cfg_len = ARRAY_SIZE(mcu_trm_cfgs),
	.trm_hw_dump = mcu_trm_hw_dump,
};

static inline void npu_write(u32 reg, u32 val)
{
	writel(val, npu.base + reg);
}

static inline void npu_set(u32 reg, u32 mask)
{
	setbits(npu.base + reg, mask);
}

static inline void npu_clr(u32 reg, u32 mask)
{
	clrbits(npu.base + reg, mask);
}

static inline void npu_rmw(u32 reg, u32 mask, u32 val)
{
	clrsetbits(npu.base + reg, mask, val);
}

static inline u32 npu_read(u32 reg)
{
	return readl(npu.base + reg);
}

static int mcu_trm_hw_dump(void *dst, u32 start_addr, u32 len)
{
	u32 ofs;

	if (unlikely(!dst))
		return -ENODEV;

	for (ofs = 0; len > 0; len -= 0x4, ofs += 0x4)
		writel(npu_read(start_addr + ofs), dst + ofs);

	return 0;
}

static int mcu_power_on(void)
{
	int ret = 0;

	ret = clk_prepare_enable(npu.bus_clk);
	if (ret) {
		TOPS_ERR("bus clk enable failed: %d\n", ret);
		return ret;
	}

	ret = clk_prepare_enable(npu.sram_clk);
	if (ret) {
		TOPS_ERR("sram clk enable failed: %d\n", ret);
		goto err_disable_bus_clk;
	}

	ret = clk_prepare_enable(npu.xdma_clk);
	if (ret) {
		TOPS_ERR("xdma clk enable failed: %d\n", ret);
		goto err_disable_sram_clk;
	}

	ret = clk_prepare_enable(npu.offload_clk);
	if (ret) {
		TOPS_ERR("offload clk enable failed: %d\n", ret);
		goto err_disable_xdma_clk;
	}

	ret = clk_prepare_enable(npu.mgmt_clk);
	if (ret) {
		TOPS_ERR("mgmt clk enable failed: %d\n", ret);
		goto err_disable_offload_clk;
	}

	ret = pm_runtime_get_sync(tops_dev);
	if (ret < 0) {
		TOPS_ERR("power on failed: %d\n", ret);
		goto err_disable_mgmt_clk;
	}

	return ret;

err_disable_mgmt_clk:
	clk_disable_unprepare(npu.mgmt_clk);

err_disable_offload_clk:
	clk_disable_unprepare(npu.offload_clk);

err_disable_xdma_clk:
	clk_disable_unprepare(npu.xdma_clk);

err_disable_sram_clk:
	clk_disable_unprepare(npu.sram_clk);

err_disable_bus_clk:
	clk_disable_unprepare(npu.bus_clk);

	return ret;
}

static void mcu_power_off(void)
{
	pm_runtime_put_sync(tops_dev);

	clk_disable_unprepare(npu.mgmt_clk);

	clk_disable_unprepare(npu.offload_clk);

	clk_disable_unprepare(npu.xdma_clk);

	clk_disable_unprepare(npu.sram_clk);

	clk_disable_unprepare(npu.bus_clk);
}

static inline int mcu_state_send_cmd(struct mcu_state *state)
{
	unsigned long flag;
	enum core_id core;
	u32 ctrl_cpu;
	int ret;

	spin_lock_irqsave(&npu.ctrl_done_lock, flag);
	ctrl_cpu = (~npu.ctrl_done) & CORE_TOPS_MASK;
	spin_unlock_irqrestore(&npu.ctrl_done_lock, flag);

	if (ctrl_cpu & BIT(CORE_MGMT)) {
		ret = mbox_send_msg_no_wait(&npu.send_mgmt_mbox_dev,
					    &npu.ctrl_msg);
		if (ret)
			goto out;
	}

	for (core = CORE_OFFLOAD_0; core < CORE_OFFLOAD_NUM; core++) {
		if (ctrl_cpu & BIT(core)) {
			ret = mbox_send_msg_no_wait(&npu.send_offload_mbox_dev[core],
						    &npu.ctrl_msg);
			if (ret)
				goto out;
		}
	}

out:
	return ret;
}

static inline void mcu_state_trans_start(void)
{
	mod_timer(&npu.mcu_ctrl_timer,
		  jiffies + msecs_to_jiffies(MCU_STATE_TRANS_TIMEOUT));
}

static inline void mcu_state_trans_end(void)
{
	del_timer_sync(&npu.mcu_ctrl_timer);
}

static inline void mcu_state_trans_err(void)
{
	wake_up_interruptible(&npu.mcu_ctrl_wait_done);
}

static inline int mcu_state_wait_complete(void (*state_complete_cb)(void))
{
	unsigned long flag;
	int ret = 0;

	wait_event_interruptible(npu.mcu_state_wait_done,
				 (npu.ctrl_done == CORE_TOPS_MASK) ||
				 (npu.state_trans_fail));

	if (npu.state_trans_fail)
		return -EINVAL;

	npu.ctrl_msg.msg1 = npu.ctrl_done_cmd;

	spin_lock_irqsave(&npu.ctrl_done_lock, flag);
	npu.ctrl_done |= BIT(MCU_CTRL_DONE_BIT);
	spin_unlock_irqrestore(&npu.ctrl_done_lock, flag);

	if (state_complete_cb)
		state_complete_cb();

	wake_up_interruptible(&npu.mcu_ctrl_wait_done);

	return ret;
}

static inline void mcu_state_prepare_wait(enum mcu_cmd_type done_cmd)
{
	unsigned long flag;

	/* if user does not specify CPU to control, default controll all CPU */
	spin_lock_irqsave(&npu.ctrl_done_lock, flag);
	if ((npu.ctrl_done & CORE_TOPS_MASK) == CORE_TOPS_MASK)
		npu.ctrl_done = 0;
	spin_unlock_irqrestore(&npu.ctrl_done_lock, flag);

	npu.ctrl_done_cmd = done_cmd;
}

static struct mcu_state *mtk_tops_mcu_state_shutdown_trans(u32 mcu_act,
							   struct mcu_state *state)
{
	if (mcu_act == MCU_ACT_INIT)
		return &mcu_states[MCU_STATE_TYPE_INIT];

	return ERR_PTR(-ENODEV);
}

static int mtk_tops_mcu_state_shutdown_enter(struct mcu_state *state)
{
	mcu_power_off();

	mtk_tops_tdma_record_last_state();

	mtk_tops_fw_clean_up();

	npu.mcu_bring_up_done = false;

	if (npu.shuting_down) {
		npu.shuting_down = false;
		wake_up_interruptible(&npu.mcu_ctrl_wait_done);

		return 0;
	}

	if (npu.in_recover || npu.in_reset)
		mcu_ctrl_issue_pending_act(MCU_ACT_INIT);

	return 0;
}

static int mtk_tops_mcu_state_shutdown_leave(struct mcu_state *state)
{
	return 0;
}

static struct mcu_state *mtk_tops_mcu_state_init_trans(u32 mcu_act,
						       struct mcu_state *state)
{
	if (mcu_act == MCU_ACT_FREERUN)
		return &mcu_states[MCU_STATE_TYPE_FREERUN];
	else if (mcu_act == MCU_ACT_NETSTOP)
		return &mcu_states[MCU_STATE_TYPE_NETSTOP];

	return ERR_PTR(-ENODEV);
}

static void mtk_tops_mcu_state_init_enter_complete_cb(void)
{
	npu.mcu_bring_up_done = true;
	npu.in_reset = false;
	npu.in_recover = false;
	npu.netsys_fe_ser = false;

	mcu_ctrl_issue_pending_act(MCU_ACT_FREERUN);
}

static int mtk_tops_mcu_state_init_enter(struct mcu_state *state)
{
	int ret = 0;

	ret = mcu_power_on();
	if (ret)
		return ret;

	mtk_tops_mbox_clear_all_cmd();

	/* reset TDMA first */
	mtk_tops_tdma_reset();

	npu.ctrl_done = 0;
	mcu_state_prepare_wait(MCU_CMD_TYPE_INIT_DONE);

	ret = mtk_tops_fw_bring_up_default_cores();
	if (ret) {
		TOPS_ERR("bring up TOPS cores failed: %d\n", ret);
		goto out;
	}

	ret = mcu_state_wait_complete(mtk_tops_mcu_state_init_enter_complete_cb);
	if (unlikely(ret))
		TOPS_ERR("init leave failed\n");

out:
	return ret;
}

static int mtk_tops_mcu_state_init_leave(struct mcu_state *state)
{
	int ret;

	mtk_tops_tdma_enable();

	mtk_tops_tnl_offload_recover();

	/* enable cls, dipfilter */
	ret = mtk_pce_enable();
	if (ret) {
		TOPS_ERR("netsys enable failed: %d\n", ret);
		return ret;
	}

	return ret;
}

static struct mcu_state *mtk_tops_mcu_state_freerun_trans(u32 mcu_act,
							  struct mcu_state *state)
{
	if (mcu_act == MCU_ACT_RESET)
		return &mcu_states[MCU_STATE_TYPE_RESET];
	else if (mcu_act == MCU_ACT_STALL)
		return &mcu_states[MCU_STATE_TYPE_STALL];
	else if (mcu_act == MCU_ACT_NETSTOP)
		return &mcu_states[MCU_STATE_TYPE_NETSTOP];

	return ERR_PTR(-ENODEV);
}

static int mtk_tops_mcu_state_freerun_enter(struct mcu_state *state)
{
	/* TODO : switch to HW path */

	return 0;
}

static int mtk_tops_mcu_state_freerun_leave(struct mcu_state *state)
{
	/* TODO : switch to SW path */

	return 0;
}

static struct mcu_state *mtk_tops_mcu_state_stall_trans(u32 mcu_act,
							struct mcu_state *state)
{
	if (mcu_act == MCU_ACT_RESET)
		return &mcu_states[MCU_STATE_TYPE_RESET];
	else if (mcu_act == MCU_ACT_FREERUN)
		return &mcu_states[MCU_STATE_TYPE_FREERUN];
	else if (mcu_act == MCU_ACT_NETSTOP)
		return &mcu_states[MCU_STATE_TYPE_NETSTOP];

	return ERR_PTR(-ENODEV);
}

static int mtk_tops_mcu_state_stall_enter(struct mcu_state *state)
{
	int ret = 0;

	mcu_state_prepare_wait(MCU_CMD_TYPE_STALL_DONE);

	ret = mcu_state_send_cmd(state);
	if (ret)
		return ret;

	ret = mcu_state_wait_complete(NULL);
	if (ret)
		TOPS_ERR("stall enter failed\n");

	return ret;
}

static int mtk_tops_mcu_state_stall_leave(struct mcu_state *state)
{
	int ret = 0;

	/*
	 * if next state is going to stop network,
	 * we should not let mcu do freerun cmd since it is going to abort stall
	 */
	if (npu.next_state->state == MCU_STATE_TYPE_NETSTOP)
		return 0;

	mcu_state_prepare_wait(MCU_CMD_TYPE_FREERUN_DONE);

	ret = mcu_state_send_cmd(state);
	if (ret)
		return ret;

	ret = mcu_state_wait_complete(NULL);
	if (ret)
		TOPS_ERR("stall leave failed\n");

	return ret;
}

static struct mcu_state *mtk_tops_mcu_state_netstop_trans(u32 mcu_act,
							  struct mcu_state *state)
{
	if (mcu_act == MCU_ACT_ABNORMAL)
		return &mcu_states[MCU_STATE_TYPE_ABNORMAL];
	else if (mcu_act == MCU_ACT_RESET)
		return &mcu_states[MCU_STATE_TYPE_RESET];
	else if (mcu_act == MCU_ACT_SHUTDOWN)
		return &mcu_states[MCU_STATE_TYPE_SHUTDOWN];

	return ERR_PTR(-ENODEV);
}

static int mtk_tops_mcu_state_netstop_enter(struct mcu_state *state)
{
	mtk_tops_tnl_offload_flush();

	mtk_pce_disable();

	mtk_tops_tdma_disable();

	if (npu.in_recover)
		mcu_ctrl_issue_pending_act(MCU_ACT_ABNORMAL);
	else if (npu.in_reset)
		mcu_ctrl_issue_pending_act(MCU_ACT_RESET);
	else
		mcu_ctrl_issue_pending_act(MCU_ACT_SHUTDOWN);

	return 0;
}

static int mtk_tops_mcu_state_netstop_leave(struct mcu_state *state)
{
	return 0;
}

static struct mcu_state *mtk_tops_mcu_state_reset_trans(u32 mcu_act,
							struct mcu_state *state)
{
	if (mcu_act == MCU_ACT_FREERUN)
		return &mcu_states[MCU_STATE_TYPE_FREERUN];
	else if (mcu_act == MCU_ACT_SHUTDOWN)
		return &mcu_states[MCU_STATE_TYPE_SHUTDOWN];
	else if (mcu_act == MCU_ACT_NETSTOP)
		/*
		 * since netstop is already done before reset,
		 * there is no need to do it again. We just go to abnormal directly
		 */
		return &mcu_states[MCU_STATE_TYPE_ABNORMAL];

	return ERR_PTR(-ENODEV);
}

static int mtk_tops_mcu_state_reset_enter(struct mcu_state *state)
{
	int ret = 0;

	mcu_state_prepare_wait(MCU_CMD_TYPE_ASSERT_RESET_DONE);

	if (!npu.netsys_fe_ser) {
		ret = mcu_state_send_cmd(state);
		if (ret)
			return ret;
	} else {
		/* skip to assert reset mcu if NETSYS SER */
		npu.ctrl_done = CORE_TOPS_MASK;
	}

	ret = mcu_state_wait_complete(NULL);
	if (ret)
		TOPS_ERR("assert reset failed\n");

	return ret;
}

static int mtk_tops_mcu_state_reset_leave(struct mcu_state *state)
{
	int ret = 0;

	/*
	 * if next state is going to shutdown,
	 * no need to let mcu do release reset cmd
	 */
	if (npu.next_state->state == MCU_STATE_TYPE_ABNORMAL
	    || npu.next_state->state == MCU_STATE_TYPE_SHUTDOWN)
		return 0;

	mcu_state_prepare_wait(MCU_CMD_TYPE_RELEASE_RESET_DONE);

	ret = mcu_state_send_cmd(state);
	if (ret)
		return ret;

	ret = mcu_state_wait_complete(NULL);
	if (ret)
		TOPS_ERR("release reset failed\n");

	return ret;
}

static struct mcu_state *mtk_tops_mcu_state_abnormal_trans(u32 mcu_act,
							   struct mcu_state *state)
{
	if (mcu_act == MCU_ACT_SHUTDOWN)
		return &mcu_states[MCU_STATE_TYPE_SHUTDOWN];

	return ERR_PTR(-ENODEV);
}

static int mtk_tops_mcu_state_abnormal_enter(struct mcu_state *state)
{
	mcu_ctrl_issue_pending_act(MCU_ACT_SHUTDOWN);

	return 0;
}

static int mtk_tops_mcu_state_abnormal_leave(struct mcu_state *state)
{
	if (npu.mcu_bring_up_done)
		mtk_trm_dump(TRM_RSN_MCU_STATE_ACT_FAIL);

	return 0;
}

static int mtk_tops_mcu_state_transition(u32 mcu_act)
{
	int ret = 0;

	npu.next_state = npu.cur_state->state_trans(mcu_act, npu.cur_state);
	if (IS_ERR(npu.next_state))
		return PTR_ERR(npu.next_state);

	/* skip mcu_state leave if current MCU_ACT has failure */
	if (unlikely(mcu_act == MCU_ACT_ABNORMAL))
		goto skip_state_leave;

	mcu_state_trans_start();
	if (npu.cur_state->leave) {
		ret = npu.cur_state->leave(npu.cur_state);
		if (ret) {
			TOPS_ERR("state%d transition leave failed: %d\n",
				npu.cur_state->state, ret);
			goto state_trans_end;
		}
	}
	mcu_state_trans_end();

skip_state_leave:
	npu.cur_state = npu.next_state;

	mcu_state_trans_start();
	if (npu.cur_state->enter) {
		ret = npu.cur_state->enter(npu.cur_state);
		if (ret) {
			TOPS_ERR("state%d transition enter failed: %d\n",
				npu.cur_state->state, ret);
			goto state_trans_end;
		}
	}

state_trans_end:
	mcu_state_trans_end();

	return ret;
}

static void mtk_tops_mcu_state_trans_timeout(struct timer_list *timer)
{
	TOPS_ERR("state%d transition timeout!\n", npu.cur_state->state);
	TOPS_ERR("ctrl_done=0x%x ctrl_msg.msg1: 0x%x\n",
		 npu.ctrl_done, npu.ctrl_msg.msg1);

	npu.state_trans_fail = true;

	wake_up_interruptible(&npu.mcu_state_wait_done);
}

static inline int mcu_ctrl_cmd_prepare(enum mcu_cmd_type cmd,
				       struct mcu_ctrl_cmd *mcmd)
{
	if (!mcmd || cmd == MCU_CMD_TYPE_NULL || cmd >= __MCU_CMD_TYPE_MAX)
		return -EINVAL;

	lockdep_assert_held(&npu.mcu_ctrl_lock);

	npu.ctrl_msg.msg1 = cmd;
	npu.ctrl_msg.msg2 = mcmd->e;
	npu.ctrl_msg.msg3 = mcmd->arg[0];
	npu.ctrl_msg.msg4 = mcmd->arg[1];

	if (mcmd->core_mask) {
		unsigned long flag;

		spin_lock_irqsave(&npu.ctrl_done_lock, flag);
		npu.ctrl_done = ~(CORE_TOPS_MASK & mcmd->core_mask);
		npu.ctrl_done &= CORE_TOPS_MASK;
		spin_unlock_irqrestore(&npu.ctrl_done_lock, flag);
	}

	return 0;
}

static inline void mcu_ctrl_callback(void (*callback)(void *param), void *param)
{
	if (callback)
		callback(param);
}

static inline void mcu_ctrl_issue_pending_act(u32 mcu_act)
{
	unsigned long flag;

	spin_lock_irqsave(&npu.pending_act_lock, flag);

	npu.pending_act |= mcu_act;

	spin_unlock_irqrestore(&npu.pending_act_lock, flag);

	wake_up_interruptible(&npu.mcu_ctrl_wait_act);
}

static inline enum mcu_act mcu_ctrl_pop_pending_act(void)
{
	unsigned long flag;
	enum mcu_act act;

	spin_lock_irqsave(&npu.pending_act_lock, flag);

	act = ffs(npu.pending_act) - 1;
	npu.pending_act &= ~BIT(act);

	spin_unlock_irqrestore(&npu.pending_act_lock, flag);

	return act;
}

static inline bool mcu_ctrl_is_complete(enum mcu_cmd_type done_cmd)
{
	unsigned long flag;
	bool ctrl_done;

	spin_lock_irqsave(&npu.ctrl_done_lock, flag);
	ctrl_done = npu.ctrl_done == MCU_CTRL_DONE && npu.ctrl_msg.msg1 == done_cmd;
	spin_unlock_irqrestore(&npu.ctrl_done_lock, flag);

	return ctrl_done;
}

static inline void mcu_ctrl_done(enum core_id core)
{
	unsigned long flag;

	if (core > CORE_MGMT)
		return;

	spin_lock_irqsave(&npu.ctrl_done_lock, flag);
	npu.ctrl_done |= BIT(core);
	spin_unlock_irqrestore(&npu.ctrl_done_lock, flag);
}

static int mcu_ctrl_task(void *data)
{
	enum mcu_act act;
	int ret;

	while (1) {
		wait_event_interruptible(npu.mcu_ctrl_wait_act,
					 npu.pending_act || kthread_should_stop());

		if (kthread_should_stop()) {
			TOPS_INFO("tops mcu ctrl task stop\n");
			break;
		}

		act = mcu_ctrl_pop_pending_act();
		if (unlikely(act >= __MCU_ACT_MAX)) {
			TOPS_ERR("invalid MCU act: %u\n", act);
			continue;
		}

		/*
		 * ensure that the act is submitted by either
		 * mtk_tops_mcu_stall, mtk_tops_mcu_reset or mtk_tops_mcu_cold_boot
		 * if mcu_act is ABNORMAL, it must be caused by the state transition
		 * triggerred by above APIs
		 * as a result, mcu_ctrl_lock must be held before mcu_ctrl_task start
		 */
		lockdep_assert_held(&npu.mcu_ctrl_lock);

		if (unlikely(!npu.cur_state->state_trans)) {
			TOPS_ERR("cur state has no state_trans()\n");
			WARN_ON(1);
		}

		ret = mtk_tops_mcu_state_transition(BIT(act));
		if (ret) {
			npu.state_trans_fail = true;

			mcu_state_trans_err();
		}
	}
	return 0;
}

bool mtk_tops_mcu_alive(void)
{
	return npu.mcu_bring_up_done && !npu.in_reset && !npu.state_trans_fail;
}

bool mtk_tops_mcu_bring_up_done(void)
{
	return npu.mcu_bring_up_done;
}

bool mtk_tops_mcu_netsys_fe_rst(void)
{
	return npu.netsys_fe_ser;
}

static int mtk_tops_mcu_wait_done(enum mcu_cmd_type done_cmd)
{
	int ret = 0;

	wait_event_interruptible(npu.mcu_ctrl_wait_done,
				 mcu_ctrl_is_complete(done_cmd)
				 || npu.state_trans_fail);

	if (npu.state_trans_fail)
		return -EINVAL;

	return ret;
}

int mtk_tops_mcu_stall(struct mcu_ctrl_cmd *mcmd,
		       void (*callback)(void *param), void *param)
{
	int ret = 0;

	if (unlikely(!npu.mcu_bring_up_done || npu.state_trans_fail))
		return -EBUSY;

	if (unlikely(!mcmd || mcmd->e >= __MCU_EVENT_TYPE_MAX))
		return -EINVAL;

	mutex_lock(&npu.mcu_ctrl_lock);

	/* go to stall state */
	ret = mcu_ctrl_cmd_prepare(MCU_CMD_TYPE_STALL, mcmd);
	if (ret)
		goto unlock;

	mcu_ctrl_issue_pending_act(MCU_ACT_STALL);

	ret = mtk_tops_mcu_wait_done(MCU_CMD_TYPE_STALL_DONE);
	if (ret) {
		TOPS_ERR("tops stall failed: %d\n", ret);
		goto recover_mcu;
	}

	mcu_ctrl_callback(callback, param);

	/* go to freerun state */
	ret = mcu_ctrl_cmd_prepare(MCU_CMD_TYPE_FREERUN, mcmd);
	if (ret)
		goto recover_mcu;

	mcu_ctrl_issue_pending_act(MCU_ACT_FREERUN);

	ret = mtk_tops_mcu_wait_done(MCU_CMD_TYPE_FREERUN_DONE);
	if (ret) {
		TOPS_ERR("tops freerun failed: %d\n", ret);
		goto recover_mcu;
	}

	/* stall freerun successfully done */
	goto unlock;

recover_mcu:
	schedule_work(&npu.recover_work);

unlock:
	mutex_unlock(&npu.mcu_ctrl_lock);

	return ret;
}

int mtk_tops_mcu_reset(struct mcu_ctrl_cmd *mcmd,
		       void (*callback)(void *param), void *param)
{
	int ret = 0;

	if (unlikely(!npu.mcu_bring_up_done || npu.state_trans_fail))
		return -EBUSY;

	if (unlikely(!mcmd || mcmd->e >= __MCU_EVENT_TYPE_MAX))
		return -EINVAL;

	mutex_lock(&npu.mcu_ctrl_lock);

	npu.in_reset = true;
	if (mcmd->e == MCU_EVENT_TYPE_FE_RESET)
		npu.netsys_fe_ser = true;

	ret = mcu_ctrl_cmd_prepare(MCU_CMD_TYPE_ASSERT_RESET, mcmd);
	if (ret)
		goto unlock;

	mcu_ctrl_issue_pending_act(MCU_ACT_NETSTOP);

	ret = mtk_tops_mcu_wait_done(MCU_CMD_TYPE_ASSERT_RESET_DONE);
	if (ret) {
		TOPS_ERR("tops assert reset failed: %d\n", ret);
		goto recover_mcu;
	}

	mcu_ctrl_callback(callback, param);

	switch (mcmd->e) {
	case MCU_EVENT_TYPE_WDT_TIMEOUT:
	case MCU_EVENT_TYPE_FE_RESET:
		mcu_ctrl_issue_pending_act(MCU_ACT_SHUTDOWN);

		ret = mtk_tops_mcu_wait_done(MCU_CMD_TYPE_INIT_DONE);
		if (ret)
			goto recover_mcu;

		break;
	default:
		ret = mcu_ctrl_cmd_prepare(MCU_CMD_TYPE_RELEASE_RESET, mcmd);
		if (ret)
			goto recover_mcu;

		mcu_ctrl_issue_pending_act(MCU_ACT_FREERUN);

		ret = mtk_tops_mcu_wait_done(MCU_CMD_TYPE_RELEASE_RESET_DONE);
		if (ret)
			goto recover_mcu;

		break;
	}

	goto unlock;

recover_mcu:
	schedule_work(&npu.recover_work);

unlock:
	mutex_unlock(&npu.mcu_ctrl_lock);

	return ret;
}

static void mtk_tops_mcu_recover_work(struct work_struct *work)
{
	int ret;

	mutex_lock(&npu.mcu_ctrl_lock);

	if (!npu.mcu_bring_up_done && !npu.in_reset && !npu.state_trans_fail)
		mcu_ctrl_issue_pending_act(MCU_ACT_INIT);
	else if (npu.in_reset || npu.state_trans_fail)
		mcu_ctrl_issue_pending_act(MCU_ACT_NETSTOP);

	npu.state_trans_fail = false;
	npu.in_recover = true;

	while ((ret = mtk_tops_mcu_wait_done(MCU_CMD_TYPE_INIT_DONE))) {
		if (npu.shuting_down)
			goto unlock;

		npu.mcu_bring_up_done = false;
		npu.state_trans_fail = false;
		TOPS_ERR("bring up failed: %d\n", ret);

		msleep(1000);

		mcu_ctrl_issue_pending_act(MCU_ACT_NETSTOP);
	}

unlock:
	mutex_unlock(&npu.mcu_ctrl_lock);
}

static int mtk_tops_mcu_register_mbox(void)
{
	int ret;
	int i;

	ret = register_mbox_dev(MBOX_SEND, &npu.send_mgmt_mbox_dev);
	if (ret) {
		TOPS_ERR("register mcu_ctrl mgmt mbox send failed: %d\n", ret);
		return ret;
	}

	ret = register_mbox_dev(MBOX_RECV, &npu.recv_mgmt_mbox_dev);
	if (ret) {
		TOPS_ERR("register mcu_ctrl mgmt mbox recv failed: %d\n", ret);
		goto err_unregister_mgmt_mbox_send;
	}

	for (i = 0; i < CORE_OFFLOAD_NUM; i++) {
		ret = register_mbox_dev(MBOX_SEND, &npu.send_offload_mbox_dev[i]);
		if (ret) {
			TOPS_ERR("register mcu_ctrl offload %d mbox send failed: %d\n",
				 i, ret);
			goto err_unregister_offload_mbox;
		}

		ret = register_mbox_dev(MBOX_RECV, &npu.recv_offload_mbox_dev[i]);
		if (ret) {
			TOPS_ERR("register mcu_ctrl offload %d mbox recv failed: %d\n",
				 i, ret);
			unregister_mbox_dev(MBOX_SEND, &npu.send_offload_mbox_dev[i]);
			goto err_unregister_offload_mbox;
		}
	}

	return ret;

err_unregister_offload_mbox:
	for (i -= 1; i >= 0; i--) {
		unregister_mbox_dev(MBOX_RECV, &npu.recv_offload_mbox_dev[i]);
		unregister_mbox_dev(MBOX_SEND, &npu.send_offload_mbox_dev[i]);
	}

	unregister_mbox_dev(MBOX_RECV, &npu.recv_mgmt_mbox_dev);

err_unregister_mgmt_mbox_send:
	unregister_mbox_dev(MBOX_SEND, &npu.send_mgmt_mbox_dev);

	return ret;
}

static void mtk_tops_mcu_unregister_mbox(void)
{
	int i;

	unregister_mbox_dev(MBOX_SEND, &npu.send_mgmt_mbox_dev);
	unregister_mbox_dev(MBOX_RECV, &npu.recv_mgmt_mbox_dev);

	for (i = 0; i < CORE_OFFLOAD_NUM; i++) {
		unregister_mbox_dev(MBOX_SEND, &npu.send_offload_mbox_dev[i]);
		unregister_mbox_dev(MBOX_RECV, &npu.recv_offload_mbox_dev[i]);
	}
}

static void mtk_tops_mcu_shutdown(void)
{
	npu.shuting_down = true;

	mutex_lock(&npu.mcu_ctrl_lock);

	mcu_ctrl_issue_pending_act(MCU_ACT_NETSTOP);

	wait_event_interruptible(npu.mcu_ctrl_wait_done,
				 !npu.mcu_bring_up_done && !npu.shuting_down);

	mutex_unlock(&npu.mcu_ctrl_lock);
}

/* TODO: should be implemented to not block other module's init tasks */
static int mtk_tops_mcu_cold_boot(void)
{
	int ret = 0;

	npu.cur_state = &mcu_states[MCU_STATE_TYPE_SHUTDOWN];

	mutex_lock(&npu.mcu_ctrl_lock);

	mcu_ctrl_issue_pending_act(MCU_ACT_INIT);
	ret = mtk_tops_mcu_wait_done(MCU_CMD_TYPE_INIT_DONE);

	mutex_unlock(&npu.mcu_ctrl_lock);
	if (!ret)
		return ret;

	TOPS_ERR("cold boot failed: %d\n", ret);

	schedule_work(&npu.recover_work);

	return 0;
}

int mtk_tops_mcu_bring_up(struct platform_device *pdev)
{
	int ret = 0;

	pm_runtime_enable(&pdev->dev);

	ret = mtk_tops_mcu_register_mbox();
	if (ret) {
		TOPS_ERR("register mcu ctrl mbox failed: %d\n", ret);
		goto runtime_disable;
	}

	npu.mcu_ctrl_thread = kthread_run(mcu_ctrl_task, NULL, "tops mcu ctrl task");
	if (IS_ERR(npu.mcu_ctrl_thread)) {
		ret = PTR_ERR(npu.mcu_ctrl_thread);
		TOPS_ERR("mcu ctrl thread create failed: %d\n", ret);
		goto err_unregister_mbox;
	}

	ret = mtk_tops_mcu_cold_boot();
	if (ret) {
		TOPS_ERR("cold boot failed: %d\n", ret);
		goto err_stop_mcu_ctrl_thread;
	}

	return ret;

err_stop_mcu_ctrl_thread:
	kthread_stop(npu.mcu_ctrl_thread);

err_unregister_mbox:
	mtk_tops_mcu_unregister_mbox();

runtime_disable:
	pm_runtime_disable(&pdev->dev);

	return ret;
}

void mtk_tops_mcu_tear_down(struct platform_device *pdev)
{
	mtk_tops_mcu_shutdown();

	kthread_stop(npu.mcu_ctrl_thread);

	/* TODO: stop mcu? */

	mtk_tops_mcu_unregister_mbox();

	pm_runtime_disable(&pdev->dev);
}

static int mtk_tops_mcu_dts_init(struct platform_device *pdev)
{
	struct device_node *node = pdev->dev.of_node;
	struct resource *res = NULL;
	int ret = 0;

	if (!node)
		return -EINVAL;

	res = platform_get_resource_byname(pdev, IORESOURCE_MEM, "tops-base");
	if (!res) {
		TOPS_ERR("can not find tops base\n");
		return -ENXIO;
	}

	npu.base = devm_ioremap(&pdev->dev, res->start, resource_size(res));
	if (!npu.base) {
		TOPS_ERR("map tops base failed\n");
		return -ENOMEM;
	}

	npu.bus_clk = devm_clk_get(tops_dev, "bus");
	if (IS_ERR(npu.bus_clk)) {
		TOPS_ERR("get bus clk failed: %ld\n", PTR_ERR(npu.bus_clk));
		return PTR_ERR(npu.bus_clk);
	}

	npu.sram_clk = devm_clk_get(tops_dev, "sram");
	if (IS_ERR(npu.sram_clk)) {
		TOPS_ERR("get sram clk failed: %ld\n", PTR_ERR(npu.sram_clk));
		return PTR_ERR(npu.sram_clk);
	}

	npu.xdma_clk = devm_clk_get(tops_dev, "xdma");
	if (IS_ERR(npu.xdma_clk)) {
		TOPS_ERR("get xdma clk failed: %ld\n", PTR_ERR(npu.xdma_clk));
		return PTR_ERR(npu.xdma_clk);
	}

	npu.offload_clk = devm_clk_get(tops_dev, "offload");
	if (IS_ERR(npu.offload_clk)) {
		TOPS_ERR("get offload clk failed: %ld\n", PTR_ERR(npu.offload_clk));
		return PTR_ERR(npu.offload_clk);
	}

	npu.mgmt_clk = devm_clk_get(tops_dev, "mgmt");
	if (IS_ERR(npu.mgmt_clk)) {
		TOPS_ERR("get mgmt clk failed: %ld\n", PTR_ERR(npu.mgmt_clk));
		return PTR_ERR(npu.mgmt_clk);
	}

	return ret;
}

static void mtk_tops_mcu_pm_domain_detach(void)
{
	int i = npu.pd_num;

	while (--i >= 0) {
		device_link_del(npu.pd_links[i]);
		dev_pm_domain_detach(npu.pd_devices[i], true);
	}
}

static int mtk_tops_mcu_pm_domain_attach(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	int ret = 0;
	int i;

	npu.pd_num = of_count_phandle_with_args(dev->of_node,
						"power-domains",
						"#power-domain-cells");

	/* only 1 power domain exist, no need to link devices */
	if (npu.pd_num <= 1)
		return 0;

	npu.pd_devices = devm_kmalloc_array(dev, npu.pd_num,
					    sizeof(struct device),
					    GFP_KERNEL);
	if (!npu.pd_devices)
		return -ENOMEM;

	npu.pd_links = devm_kmalloc_array(dev, npu.pd_num,
					  sizeof(*npu.pd_links),
					  GFP_KERNEL);
	if (!npu.pd_links)
		return -ENOMEM;

	for (i = 0; i < npu.pd_num; i++) {
		npu.pd_devices[i] = dev_pm_domain_attach_by_id(dev, i);
		if (IS_ERR(npu.pd_devices[i])) {
			ret = PTR_ERR(npu.pd_devices[i]);
			goto pm_attach_fail;
		}

		npu.pd_links[i] = device_link_add(dev, npu.pd_devices[i],
						  DL_FLAG_STATELESS |
						  DL_FLAG_PM_RUNTIME);
		if (!npu.pd_links[i]) {
			ret = -EINVAL;
			dev_pm_domain_detach(npu.pd_devices[i], false);
			goto pm_attach_fail;
		}
	}

	return 0;

pm_attach_fail:
	TOPS_ERR("attach power domain failed: %d\n", ret);

	while (--i >= 0) {
		device_link_del(npu.pd_links[i]);
		dev_pm_domain_detach(npu.pd_devices[i], false);
	}

	return ret;
}

int mtk_tops_mcu_init(struct platform_device *pdev)
{
	int ret = 0;

	dma_set_mask(tops_dev, DMA_BIT_MASK(32));

	ret = mtk_tops_mcu_dts_init(pdev);
	if (ret)
		return ret;

	ret = mtk_tops_mcu_pm_domain_attach(pdev);
	if (ret)
		return ret;

	INIT_WORK(&npu.recover_work, mtk_tops_mcu_recover_work);
	init_waitqueue_head(&npu.mcu_ctrl_wait_act);
	init_waitqueue_head(&npu.mcu_ctrl_wait_done);
	init_waitqueue_head(&npu.mcu_state_wait_done);
	spin_lock_init(&npu.pending_act_lock);
	spin_lock_init(&npu.ctrl_done_lock);
	mutex_init(&npu.mcu_ctrl_lock);
	timer_setup(&npu.mcu_ctrl_timer, mtk_tops_mcu_state_trans_timeout, 0);

	ret = mtk_trm_hw_config_register(TRM_TOPS, &mcu_trm_hw_cfg);
	if (ret) {
		TOPS_ERR("TRM register failed: %d\n", ret);
		return ret;
	}

	return ret;
}

void mtk_tops_mcu_deinit(struct platform_device *pdev)
{
	mtk_trm_hw_config_unregister(TRM_TOPS, &mcu_trm_hw_cfg);

	mtk_tops_mcu_pm_domain_detach();
}

static enum mbox_msg_cnt mtk_tops_ap_recv_mgmt_mbox_msg(struct mailbox_dev *mdev,
							struct mailbox_msg *msg)
{
	if (msg->msg1 == npu.ctrl_done_cmd)
		/* mcu side state transition success */
		mcu_ctrl_done(mdev->core);
	else
		/* mcu side state transition failed */
		npu.state_trans_fail = true;

	wake_up_interruptible(&npu.mcu_state_wait_done);

	return MBOX_NO_RET_MSG;
}

static enum mbox_msg_cnt mtk_tops_ap_recv_offload_mbox_msg(struct mailbox_dev *mdev,
							   struct mailbox_msg *msg)
{
	if (msg->msg1 == npu.ctrl_done_cmd)
		/* mcu side state transition success */
		mcu_ctrl_done(mdev->core);
	else
		/* mcu side state transition failed */
		npu.state_trans_fail = true;

	wake_up_interruptible(&npu.mcu_state_wait_done);

	return MBOX_NO_RET_MSG;
}
