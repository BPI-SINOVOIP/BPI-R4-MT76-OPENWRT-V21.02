/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * Copyright (c) 2023 MediaTek Inc. All Rights Reserved.
 *
 * Author: Ren-Ting Wang <ren-ting.wang@mediatek.com>
 */

#ifndef _TOPS_UDP_PARAMS_H_
#define _TOPS_UDP_PARAMS_H_

#include <linux/types.h>
#include <linux/udp.h>

struct tops_udp_params {
	u16 sport;
	u16 dport;
};
#endif /* _TOPS_UDP_PARAMS_H_ */
