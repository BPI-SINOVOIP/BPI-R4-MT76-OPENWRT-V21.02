/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * Copyright (c) 2023 MediaTek Inc. All Rights Reserved.
 *
 * Author: Alvin Kuo <alvin.kuo@mediatek.com>
 */

#ifndef _TOPS_DEBUGFS_H_
#define _TOPS_DEBUGFS_H_

#include <linux/debugfs.h>
#include <linux/platform_device.h>

extern struct dentry *tops_debugfs_root;

int mtk_tops_debugfs_init(struct platform_device *pdev);
void mtk_tops_debugfs_deinit(struct platform_device *pdev);
#endif /* _TOPS_DEBUGFS_H_ */
