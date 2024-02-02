// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (c) 2023 MediaTek Inc. All Rights Reserved.
 *
 * Author: Ren-Ting Wang <ren-ting.wang@mediatek.com>
 */

#include <linux/device.h>
#include <linux/hashtable.h>
#include <linux/netdevice.h>
#include <linux/netfilter.h>
#include <linux/netfilter_ipv4.h>
#include <linux/notifier.h>
#include <net/arp.h>
#include <net/flow.h>
#include <net/ip.h>
#include <net/ip_tunnels.h>
#include <net/netevent.h>
#include <net/net_namespace.h>
#include <net/neighbour.h>
#include <net/route.h>

#include "tops/internal.h"
#include "tops/netsys.h"
#include "tops/net-event.h"
#include "tops/mcu.h"
#include "tops/ser.h"
#include "tops/trm.h"
#include "tops/tunnel.h"

static struct completion wait_fe_reset_done;

static void mtk_tops_netdev_ser_callback(struct tops_ser_params *ser_param)
{
	struct net_device *netdev = ser_param->data.net.ndev;

	WARN_ON(ser_param->type != TOPS_SER_NETSYS_FE_RST);

	mtk_trm_dump(TRM_RSN_FE_RESET);

	/* send tops dump done notification to mtk eth */
	rtnl_lock();
	call_netdevice_notifiers(MTK_TOPS_DUMP_DONE, netdev);
	rtnl_unlock();

	/* wait for FE reset done notification */
	/* TODO : if not received FE reset done notification */
	wait_for_completion(&wait_fe_reset_done);
}

static inline void mtk_tops_netdev_ser(struct net_device *dev)
{
	struct tops_ser_params ser_params = {
		.type = TOPS_SER_NETSYS_FE_RST,
		.data.net.ndev = dev,
		.ser_callback = mtk_tops_netdev_ser_callback,
	};

	mtk_tops_ser(&ser_params);
}

/* TODO: update tunnel status when user delete or change tunnel parameters */
/*
 * eth will send out MTK_FE_START_RESET event if detected wdma abnormal, or
 * send out MTK_FE_STOP_TRAFFIC event if detected qdma or adma or tdma abnormal,
 * then do FE reset, so we use the same mcu event to represent it.
 *
 * after FE reset done, eth will send out MTK_FE_START_TRAFFIC event if this is
 * wdma abnormal induced FE reset, or send out MTK_FE_RESET_DONE event for qdma
 * or adma or tdma abnormal induced FE reset.
 */
static int mtk_tops_netdev_callback(struct notifier_block *nb,
				   unsigned long event,
				   void *data)
{
	struct net_device *dev = netdev_notifier_info_to_dev(data);
	int ret = 0;

	switch (event) {
	case NETDEV_UP:
		break;
	case NETDEV_DOWN:
		mtk_tops_tnl_offload_netdev_down(dev);
		break;
	case MTK_FE_START_RESET:
	case MTK_FE_STOP_TRAFFIC:
		mtk_tops_netdev_ser(dev);
		break;
	case MTK_FE_RESET_DONE:
	case MTK_FE_START_TRAFFIC:
		complete(&wait_fe_reset_done);
		break;
	default:
		break;
	}

	return ret;
}

static struct notifier_block mtk_tops_netdev_notifier = {
	.notifier_call = mtk_tops_netdev_callback,
};

static int mtk_tops_netevent_callback(struct notifier_block *nb,
				      unsigned long event,
				      void *data)
{
	int ret = 0;

	switch (event) {
	case NETEVENT_NEIGH_UPDATE:
		break;
	default:
		break;
	}

	return ret;
}

static struct notifier_block mtk_tops_netevent_notifier = {
	.notifier_call = mtk_tops_netevent_callback,
};

int mtk_tops_netevent_register(struct platform_device *pdev)
{
	int ret = 0;

	ret = register_netdevice_notifier(&mtk_tops_netdev_notifier);
	if (ret) {
		TOPS_ERR("TOPS register netdev notifier failed: %d\n", ret);
		return ret;
	}

	ret = register_netevent_notifier(&mtk_tops_netevent_notifier);
	if (ret) {
		unregister_netdevice_notifier(&mtk_tops_netdev_notifier);
		TOPS_ERR("TOPS register net event notifier failed: %d\n", ret);
		return ret;
	}

	init_completion(&wait_fe_reset_done);

	return ret;
}

void mtk_tops_netevent_unregister(struct platform_device *pdev)
{
	unregister_netevent_notifier(&mtk_tops_netevent_notifier);

	unregister_netdevice_notifier(&mtk_tops_netdev_notifier);
}
