/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * Copyright (c) 2023 MediaTek Inc. All Rights Reserved.
 *
 * Author: Alvin Kuo <alvin.kuo@mediatek.com>
 */

#ifndef _TRM_DEBUGFS_H_
#define _TRM_DEBUGFS_H_

#include <linux/debugfs.h>

extern struct dentry *trm_debugfs_root;

int mtk_trm_debugfs_init(void);
void mtk_trm_debugfs_deinit(void);
#endif /* _TRM_DEBUGFS_H_ */
