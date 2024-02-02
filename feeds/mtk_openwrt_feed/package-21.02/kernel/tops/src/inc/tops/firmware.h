/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * Copyright (c) 2023 MediaTek Inc. All Rights Reserved.
 *
 * Author: Ren-Ting Wang <ren-ting.wang@mediatek.com>
 */

#ifndef _TOPS_FW_H_
#define _TOPS_FW_H_

#include <linux/platform_device.h>
#include <linux/time.h>

enum tops_role_type {
	TOPS_ROLE_TYPE_MGMT,
	TOPS_ROLE_TYPE_CLUSTER,

	__TOPS_ROLE_TYPE_MAX,
};

u64 mtk_tops_fw_get_git_commit_id(enum tops_role_type rtype);
void mtk_tops_fw_get_built_date(enum tops_role_type rtype, struct tm *tm);
u32 mtk_tops_fw_attr_get_num(enum tops_role_type rtype);
const char *mtk_tops_fw_attr_get_property(enum tops_role_type rtype, u32 idx);
const char *mtk_tops_fw_attr_get_value(enum tops_role_type rtype,
				       const char *property);

int mtk_tops_fw_bring_up_default_cores(void);
int mtk_tops_fw_bring_up_core(const char *fw_path);
void mtk_tops_fw_clean_up(void);
int mtk_tops_fw_init(struct platform_device *pdev);
#endif /* _TOPS_FW_H_ */
