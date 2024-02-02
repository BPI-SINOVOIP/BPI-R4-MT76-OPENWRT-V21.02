/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * Copyright (c) 2023 MediaTek Inc. All Rights Reserved.
 *
 * Author: Ren-Ting Wang <ren-ting.wang@mediatek.com>
 */

#ifndef _TOPS_MBOX_ID_H_
#define _TOPS_MBOX_ID_H_

enum mbox_cm2ap_cmd_id {
	MBOX_CM2AP_CMD_CORE_CTRL = 0,
	MBOX_CM2AP_CMD_HPDMA = 10,
	MBOX_CM2AP_CMD_TNL_OFFLOAD = 11,
	MBOX_CM2AP_CMD_TEST = 31,
	__MBOX_CM2AP_CMD_MAX = 32,
};

enum mbox_ap2cm_cmd_id {
	MBOX_AP2CM_CMD_CORE_CTRL = 0,
	MBOX_AP2CM_CMD_NET = 1,
	MBOX_AP2CM_CMD_WDT = 2,
	MBOX_AP2CM_CMD_TRM = 3,
	MBOX_AP2CM_CMD_TNL_OFFLOAD = 11,
	MBOX_AP2CM_CMD_TEST = 31,
	__MBOX_AP2CM_CMD_MAX = 32,
};

enum mbox_cx2ap_cmd_id {
	MBOX_CX2AP_CMD_CORE_CTRL = 0,
	MBOX_CX2AP_CMD_HPDMA = 10,
	__MBOX_CX2AP_CMD_MAX = 32,
};

enum mbox_ap2cx_cmd_id {
	MBOX_AP2CX_CMD_CORE_CTRL = 0,
	MBOX_AP2CX_CMD_NET = 1,
	MBOX_AP2CX_CMD_WDT = 2,
	MBOX_AP2CX_CMD_TRM = 3,
	__MBOX_AP2CX_CMD_MAX = 32,
};

enum mbox_cm2cx_cmd_id {
	MBOX_CM2CX_CMD_CORE_CTRL = 0,
	__MBOX_CM2CX_CMD_MAX = 32,
};

enum mbox_cx2cm_cmd_id {
	MBOX_CX2CM_CMD_CORE_CTRL = 0,
	__MBOX_CX2CM_CMD_MAX = 32,
};

#endif /* _TOPS_MBOX_ID_H_ */
