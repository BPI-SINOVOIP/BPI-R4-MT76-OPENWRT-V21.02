/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * Copyright (c) 2023 MediaTek Inc. All Rights Reserved.
 *
 * Author: Ren-Ting Wang <ren-ting.wang@mediatek.com>
 */

#ifndef _TOPS_IP_PARAMS_H_
#define _TOPS_IP_PARAMS_H_

#include <linux/types.h>

struct tops_ip_params {
	__be32 sip;
	__be32 dip;
	u8 proto;
	u8 tos;
	u8 ttl;
};
#endif /* _TOPS_IP_PARAMS_H_ */
