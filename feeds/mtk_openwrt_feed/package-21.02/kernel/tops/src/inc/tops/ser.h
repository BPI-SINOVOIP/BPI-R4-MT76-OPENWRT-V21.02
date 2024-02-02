/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * Copyright (c) 2023 MediaTek Inc. All Rights Reserved.
 *
 * Author: Alvin Kuo <alvin.kuog@mediatek.com>
 */

#ifndef _TOPS_SER_H_
#define _TOPS_SER_H_

#include <linux/list.h>
#include <linux/spinlock.h>
#include <linux/platform_device.h>

#include "tops/net-event.h"
#include "tops/mcu.h"
#include "tops/wdt.h"

enum tops_ser_type {
	TOPS_SER_NETSYS_FE_RST,
	TOPS_SER_WDT_TO,

	__TOPS_SER_TYPE_MAX,
};

struct tops_ser_params {
	enum tops_ser_type type;

	union {
		struct tops_net_ser_data net;
		struct tops_wdt_ser_data wdt;
	} data;

	void (*ser_callback)(struct tops_ser_params *ser_params);
	void (*ser_mcmd_setup)(struct tops_ser_params *ser_params,
			       struct mcu_ctrl_cmd *mcmd);
};

int mtk_tops_ser(struct tops_ser_params *ser_params);
int mtk_tops_ser_init(struct platform_device *pdev);
int mtk_tops_ser_deinit(struct platform_device *pdev);
#endif /* _TOPS_SER_H_ */
