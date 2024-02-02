/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * Copyright (c) 2023 MediaTek Inc. All Rights Reserved.
 *
 * Author: Ren-Ting Wang <ren-ting.wang@mediatek.com>
 */

#ifndef _TOPS_H_
#define _TOPS_H_

#define CORE_TOPS_MASK				(GENMASK(CORE_TOPS_NUM - 1, 0))

enum core_id {
	CORE_OFFLOAD_0,
	CORE_OFFLOAD_1,
	CORE_OFFLOAD_2,
	CORE_OFFLOAD_3,
	CORE_OFFLOAD_NUM,
	CORE_MGMT = CORE_OFFLOAD_NUM,
	CORE_TOPS_NUM,
	CORE_AP = CORE_TOPS_NUM,
	CORE_MAX,
};
#endif /* _TOPS_H_ */
