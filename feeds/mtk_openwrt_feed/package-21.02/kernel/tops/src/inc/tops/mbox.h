/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * Copyright (c) 2023 MediaTek Inc. All Rights Reserved.
 *
 * Author: Ren-Ting Wang <ren-ting.wang@mediatek.com>
 */

#ifndef _TOPS_MBOX_H_
#define _TOPS_MBOX_H_

#include <linux/list.h>

#include "tops/mbox_id.h"
#include "tops/tops.h"

/* mbox device macros */
#define MBOX_DEV(core_id, cmd)				\
	.core = core_id,					\
	.cmd_id = cmd,

#define MBOX_SEND_DEV(core_id, cmd)			\
	{						\
		MBOX_DEV(core_id, cmd)			\
	}

#define MBOX_SEND_MGMT_DEV(cmd)				\
	MBOX_SEND_DEV(CORE_MGMT, MBOX_AP2CM_CMD_ ## cmd)

#define MBOX_SEND_OFFLOAD_DEV(core_id, cmd)		\
	MBOX_SEND_DEV(CORE_OFFLOAD_ ## core_id, MBOX_AP2CX_CMD_ ## cmd)

#define MBOX_RECV_DEV(core_id, cmd, handler)		\
	{						\
		MBOX_DEV(core_id, cmd)			\
		.mbox_handler = handler,		\
	}

#define MBOX_RECV_MGMT_DEV(cmd, handler)		\
	MBOX_RECV_DEV(CORE_MGMT, MBOX_CM2AP_CMD_ ## cmd, handler)

#define MBOX_RECV_OFFLOAD_DEV(core_id, cmd, handler)	\
	MBOX_RECV_DEV(CORE_OFFLOAD_ ## core_id, MBOX_CX2AP_CMD_ ## cmd, handler)

/* Base Address */
#define MBOX_TOP_BASE				(0x010000)
#define MBOX_CLUST0_BASE			(0x510000)

/* TOP Mailbox */
#define TOPS_TOP_CM_SLOT			(MBOX_TOP_BASE + 0x000)
#define TOPS_TOP_AP_SLOT			(MBOX_TOP_BASE + 0x004)

#define TOPS_TOP_AP_TO_CM_CMD_SET		(MBOX_TOP_BASE + 0x200)
#define TOPS_TOP_AP_TO_CM_CMD_CLR		(MBOX_TOP_BASE + 0x204)
#define TOPS_TOP_CM_TO_AP_CMD_SET		(MBOX_TOP_BASE + 0x21C)
#define TOPS_TOP_CM_TO_AP_CMD_CLR		(MBOX_TOP_BASE + 0x220)

#define TOPS_TOP_AP_TO_CM_MSG_N(n)		(MBOX_TOP_BASE + 0x208 + 0x4 * (n))
#define TOPS_TOP_CM_TO_AP_MSG_N(n)		(MBOX_TOP_BASE + 0x224 + 0x4 * (n))

/* CLUST Mailbox */
#define TOPS_CLUST0_CX_SLOT(x)			(MBOX_CLUST0_BASE + (0x4 * (x)))
#define TOPS_CLUST0_CM_SLOT			(MBOX_CLUST0_BASE + 0x10)
#define TOPS_CLUST0_AP_SLOT			(MBOX_CLUST0_BASE + 0x14)

#define TOPS_CLUST0_CX_TO_CY_CMD_SET(x, y)	\
		(MBOX_CLUST0_BASE + 0x100 + ((x) * 0x200) + ((y) * 0x40))
#define TOPS_CLUST0_CX_TO_CY_CMD_CLR(x, y)	\
		(MBOX_CLUST0_BASE + 0x104 + ((x) * 0x200) + ((y) * 0x40))
#define TOPS_CLUST0_CX_TO_CM_CMD_SET(x)		\
		(MBOX_CLUST0_BASE + 0x200 + ((x) * 0x200))
#define TOPS_CLUST0_CX_TO_CM_CMD_CLR(x)		\
		(MBOX_CLUST0_BASE + 0x204 + ((x) * 0x200))
#define TOPS_CLUST0_CX_TO_AP_CMD_SET(x)		\
		(MBOX_CLUST0_BASE + 0x240 + ((x) * 0x200))
#define TOPS_CLUST0_CX_TO_AP_CMD_CLR(x)		\
		(MBOX_CLUST0_BASE + 0x244 + ((x) * 0x200))
#define TOPS_CLUST0_CM_TO_CX_CMD_SET(x)		\
		(MBOX_CLUST0_BASE + 0x900 + ((x) * 0x40))
#define TOPS_CLUST0_CM_TO_CX_CMD_CLR(x)		\
		(MBOX_CLUST0_BASE + 0x904 + ((x) * 0x40))
#define TOPS_CLUST0_AP_TO_CX_CMD_SET(x)		\
		(MBOX_CLUST0_BASE + 0xB00 + ((x) * 0x40))
#define TOPS_CLUST0_AP_TO_CX_CMD_CLR(x)		\
		(MBOX_CLUST0_BASE + 0xB04 + ((x) * 0x40))

#define TOPS_CLUST0_CX_TO_CY_MSG_N(x, y, n)	\
		(MBOX_CLUST0_BASE + 0x108 + ((n) * 0x4) + ((x) * 0x200) + ((y) * 0x40))
#define TOPS_CLUST0_CX_TO_CM_MSG_N(x, n)	\
		(MBOX_CLUST0_BASE + 0x208 + ((n) * 0x4) + ((x) * 0x200))
#define TOPS_CLUST0_CX_TO_AP_MSG_N(x, n)	\
		(MBOX_CLUST0_BASE + 0x248 + ((n) * 0x4) + ((x) * 0x200))
#define TOPS_CLUST0_CM_TO_CX_MSG_N(x, n)	\
		(MBOX_CLUST0_BASE + 0x908 + ((n) * 0x4) + ((x) * 0x40))
#define TOPS_CLUST0_AP_TO_CX_MSG_N(x, n)	\
		(MBOX_CLUST0_BASE + 0xB08 + ((n) * 0x4) + ((x) * 0x40))

#define MBOX_TOP_MBOX_FROM_C0			(0x1)
#define MBOX_TOP_MBOX_FROM_C1			(0x2)
#define MBOX_TOP_MBOX_FROM_C2			(0x4)
#define MBOX_TOP_MBOX_FROM_C3			(0x8)
#define MBOX_TOP_MBOX_FROM_AP			(0x10)
#define MBOX_TOP_MBOX_FROM_CM			(0x20) /* TODO: need DE update */

#define MBOX_CLUST0_MBOX_FROM_C0		(0x1)
#define MBOX_CLUST0_MBOX_FROM_C1		(0x2)
#define MBOX_CLUST0_MBOX_FROM_C2		(0x4)
#define MBOX_CLUST0_MBOX_FROM_C3		(0x8)
#define MBOX_CLUST0_MBOX_FROM_CM		(0x10)
#define MBOX_CLUST0_MBOX_FROM_AP		(0x20)

struct mailbox_msg;
struct mailbox_dev;
enum mbox_msg_cnt;

typedef void (*mbox_ret_func_t)(void *priv, struct mailbox_msg *msg);
typedef enum mbox_msg_cnt (*mbox_handler_func_t)(struct mailbox_dev *mdev,
						 struct mailbox_msg *msg);

enum mbox_act {
	MBOX_SEND,
	MBOX_RECV,
	MBOX_ACT_MAX,
};

enum mbox_msg_cnt {
	MBOX_NO_RET_MSG,
	MBOX_RET_MSG1,
	MBOX_RET_MSG2,
	MBOX_RET_MSG3,
	MBOX_RET_MSG4,
};

struct mailbox_msg {
	u32 msg1;
	u32 msg2;
	u32 msg3;
	u32 msg4;
};

struct mailbox_dev {
	struct list_head list;
	enum core_id core;
	mbox_handler_func_t mbox_handler;
	void *priv;
	u8 cmd_id;
};

int mbox_send_msg_no_wait_irq(struct mailbox_dev *mdev, struct mailbox_msg *msg);
int mbox_send_msg_no_wait(struct mailbox_dev *mdev, struct mailbox_msg *msg);
int mbox_send_msg(struct mailbox_dev *mdev, struct mailbox_msg *msg, void *priv,
		  mbox_ret_func_t ret_handler);
int register_mbox_dev(enum mbox_act act, struct mailbox_dev *mdev);
int unregister_mbox_dev(enum mbox_act act, struct mailbox_dev *mdev);
void mtk_tops_mbox_clear_all_cmd(void);
int mtk_tops_mbox_init(void);
void mtk_tops_mbox_exit(void);
#endif /* _TOPS_MBOX_H_ */
