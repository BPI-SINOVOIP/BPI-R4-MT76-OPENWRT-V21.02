/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * Copyright (c) 2023 MediaTek Inc. All Rights Reserved.
 *
 * Author: Ren-Ting Wang <ren-ting.wang@mediatek.com>
 */

#ifndef _TOPS_CTRL_H_
#define _TOPS_CTRL_H_

#include <linux/platform_device.h>

int mtk_tops_ctrl_init(struct platform_device *pdev);
void mtk_tops_ctrl_deinit(struct platform_device *pdev);
#endif /* _TOPS_CTRL_H_ */
