/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * Copyright (c) 2023 MediaTek Inc. All Rights Reserved.
 *
 * Author: Ren-Ting Wang <ren-ting.wang@mediatek.com>
 */

#ifndef _TOPS_NET_EVENT_H_
#define _TOPS_NET_EVENT_H_

#include <linux/platform_device.h>

#include <mtk_eth_soc.h>
#include <mtk_eth_reset.h>

struct tops_net_ser_data {
	struct net_device *ndev;
};

int mtk_tops_netevent_register(struct platform_device *pdev);
void mtk_tops_netevent_unregister(struct platform_device *pdev);
#endif /* _TOPS_NET_EVENT_H_ */
