/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * Copyright (c) 2023 MediaTek Inc. All Rights Reserved.
 *
 * Author: Alvin Kuo <alvin.kuog@mediatek.com>
 */

#ifndef _TOPS_WDT_H_
#define _TOPS_WDT_H_

#include <linux/platform_device.h>

#include "tops/tops.h"

enum wdt_cmd {
	WDT_CMD_TRIGGER_TIMEOUT,

	__WDT_CMD_MAX,
};

struct tops_wdt_ser_data {
	u32 timeout_cores;
};

int mtk_tops_wdt_trigger_timeout(enum core_id core);
int mtk_tops_wdt_init(struct platform_device *pdev);
int mtk_tops_wdt_deinit(struct platform_device *pdev);
#endif /* _TOPS_WDT_H_ */
