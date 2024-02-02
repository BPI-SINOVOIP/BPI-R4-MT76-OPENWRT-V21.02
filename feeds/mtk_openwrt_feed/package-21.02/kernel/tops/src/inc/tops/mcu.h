/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * Copyright (c) 2023 MediaTek Inc. All Rights Reserved.
 *
 * Author: Ren-Ting Wang <ren-ting.wang@mediatek.com>
 */

#ifndef _TOPS_MCU_H_
#define _TOPS_MCU_H_

#include <linux/clk.h>
#include <linux/bits.h>
#include <linux/list.h>
#include <linux/wait.h>
#include <linux/timer.h>
#include <linux/device.h>
#include <linux/spinlock.h>
#include <linux/workqueue.h>
#include <linux/platform_device.h>

#include "tops/tops.h"

struct mcu_state;

#define TOP_CORE_BASE				(0x001000)
#define TOP_SEC_BASE				(0x00A000)
#define TOP_L2SRAM				(0x100000)
#define TOP_CORE_M_DTCM				(0x300000)
#define TOP_CORE_M_ITCM				(0x310000)
#define CLUST_CORE_BASE(x)			(0x501000 + 0x1000 * (x))
#define CLUST_SEC_BASE				(0x50A000)
#define CLUST_L2SRAM				(0x700000)
#define CLUST_CORE_X_DTCM(x)			(0x800000 + 0x20000 * (x))
#define CLUST_CORE_X_ITCM(x)			(0x810000 + 0x20000 * (x))

/* CORE */
#define TOP_CORE_NPU_SW_RST			(TOP_CORE_BASE + 0x00)
#define TOP_CORE_NPU_CTRL			(TOP_CORE_BASE + 0x04)
#define TOP_CORE_OCD_CTRL			(TOP_CORE_BASE + 0x18)

#define TOP_CORE_DBG_CTRL			(TOP_SEC_BASE + 0x64)
#define TOP_CORE_M_STAT_VECTOR_SEL		(TOP_SEC_BASE + 0x68)
#define TOP_CORE_M_RESET_VECTOR			(TOP_SEC_BASE + 0x6C)

#define CLUST_CORE_NPU_SW_RST(x)		(CLUST_CORE_BASE(x) + 0x00)
#define CLUST_CORE_NPU_CTRL(x)			(CLUST_CORE_BASE(x) + 0x04)
#define CLUST_CORE_OCD_CTRL(x)			(CLUST_CORE_BASE(x) + 0x18)

#define CLUST_CORE_DBG_CTRL			(CLUST_SEC_BASE + 0x64)
#define CLUST_CORE_X_STAT_VECTOR_SEL(x)		(CLUST_SEC_BASE + 0x68 + (0xC * (x)))
#define CLUST_CORE_X_RESET_VECTOR(x)		(CLUST_SEC_BASE + 0x6C + (0xC * (x)))

#define MCU_ACT_ABNORMAL			(BIT(MCU_ACT_ABNORMAL_BIT))
#define MCU_ACT_RESET				(BIT(MCU_ACT_RESET_BIT))
#define MCU_ACT_NETSTOP				(BIT(MCU_ACT_NETSTOP_BIT))
#define MCU_ACT_SHUTDOWN			(BIT(MCU_ACT_SHUTDOWN_BIT))
#define MCU_ACT_INIT				(BIT(MCU_ACT_INIT_BIT))
#define MCU_ACT_STALL				(BIT(MCU_ACT_STALL_BIT))
#define MCU_ACT_FREERUN				(BIT(MCU_ACT_FREERUN_BIT))

#define MCU_CTRL_ARG_NUM			2

enum mcu_act {
	MCU_ACT_ABNORMAL_BIT,
	MCU_ACT_RESET_BIT,
	MCU_ACT_NETSTOP_BIT,
	MCU_ACT_SHUTDOWN_BIT,
	MCU_ACT_INIT_BIT,
	MCU_ACT_STALL_BIT,
	MCU_ACT_FREERUN_BIT,

	__MCU_ACT_MAX,
};

enum mcu_state_type {
	MCU_STATE_TYPE_SHUTDOWN,
	MCU_STATE_TYPE_INIT,
	MCU_STATE_TYPE_FREERUN,
	MCU_STATE_TYPE_STALL,
	MCU_STATE_TYPE_NETSTOP,
	MCU_STATE_TYPE_RESET,
	MCU_STATE_TYPE_ABNORMAL,

	__MCU_STATE_TYPE_MAX,
};

enum mcu_cmd_type {
	MCU_CMD_TYPE_NULL,
	MCU_CMD_TYPE_INIT_DONE,
	MCU_CMD_TYPE_STALL,
	MCU_CMD_TYPE_STALL_DONE,
	MCU_CMD_TYPE_FREERUN,
	MCU_CMD_TYPE_FREERUN_DONE,
	MCU_CMD_TYPE_ASSERT_RESET,
	MCU_CMD_TYPE_ASSERT_RESET_DONE,
	MCU_CMD_TYPE_RELEASE_RESET,
	MCU_CMD_TYPE_RELEASE_RESET_DONE,

	__MCU_CMD_TYPE_MAX,
};

enum mcu_event_type {
	MCU_EVENT_TYPE_NULL,
	MCU_EVENT_TYPE_SYNC_TNL,
	MCU_EVENT_TYPE_WDT_TIMEOUT,
	MCU_EVENT_TYPE_FE_RESET,

	__MCU_EVENT_TYPE_MAX,
};

struct mcu_ctrl_cmd {
	enum mcu_event_type e;
	u32 arg[MCU_CTRL_ARG_NUM];
	/*
	 * if bit n (BIT(enum core_id)) == 1, send control message to that core.
	 * default send to all cores if core_mask == 0
	 */
	u32 core_mask;
};

struct mcu_state {
	enum mcu_state_type state;
	struct mcu_state *(*state_trans)(u32 mcu_act, struct mcu_state *state);
	int (*enter)(struct mcu_state *state);
	int (*leave)(struct mcu_state *state);
};

bool mtk_tops_mcu_alive(void);
bool mtk_tops_mcu_bring_up_done(void);
bool mtk_tops_mcu_netsys_fe_rst(void);
int mtk_tops_mcu_stall(struct mcu_ctrl_cmd *mcmd,
		       void (*callback)(void *param), void *param);
int mtk_tops_mcu_reset(struct mcu_ctrl_cmd *mcmd,
		       void (*callback)(void *param), void *param);

int mtk_tops_mcu_bring_up(struct platform_device *pdev);
void mtk_tops_mcu_tear_down(struct platform_device *pdev);
int mtk_tops_mcu_init(struct platform_device *pdev);
void mtk_tops_mcu_deinit(struct platform_device *pdev);
#endif /* _TOPS_MCU_H_ */
