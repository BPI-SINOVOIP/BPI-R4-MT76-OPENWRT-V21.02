// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (c) 2023 MediaTek Inc. All Rights Reserved.
 *
 * Author: Ren-Ting Wang <ren-ting.wang@mediatek.com>
 */

#include <linux/bitmap.h>
#include <linux/bitops.h>
#include <linux/delay.h>
#include <linux/device.h>
#include <linux/io.h>
#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/platform_device.h>

#include "tops/internal.h"
#include "tops/mbox.h"
#include "tops/mcu.h"
#include "tops/tdma.h"
#include "tops/tops.h"
#include "tops/trm.h"

/* TDMA dump length */
#define TDMA_BASE_LEN				(0x400)

static int tdma_trm_hw_dump(void *dst, u32 start_addr, u32 len);

struct tdma_hw {
	void __iomem *base;
	u32 start_ring;

	struct mailbox_dev mgmt_mdev;
	struct mailbox_dev offload_mdev[CORE_OFFLOAD_NUM];
};

struct tdma_hw tdma = {
	.mgmt_mdev = MBOX_SEND_MGMT_DEV(NET),
	.offload_mdev = {
		[CORE_OFFLOAD_0] = MBOX_SEND_OFFLOAD_DEV(0, NET),
		[CORE_OFFLOAD_1] = MBOX_SEND_OFFLOAD_DEV(1, NET),
		[CORE_OFFLOAD_2] = MBOX_SEND_OFFLOAD_DEV(2, NET),
		[CORE_OFFLOAD_3] = MBOX_SEND_OFFLOAD_DEV(3, NET),
	},
};

static inline void tdma_write(u32 reg, u32 val)
{
	writel(val, tdma.base + reg);
}

static inline void tdma_set(u32 reg, u32 mask)
{
	setbits(tdma.base + reg, mask);
}

static inline void tdma_clr(u32 reg, u32 mask)
{
	clrbits(tdma.base + reg, mask);
}

static inline void tdma_rmw(u32 reg, u32 mask, u32 val)
{
	clrsetbits(tdma.base + reg, mask, val);
}

static inline u32 tdma_read(u32 reg)
{
	return readl(tdma.base + reg);
}

static struct trm_config tdma_trm_configs[] = {
	{
		TRM_CFG_EN("netsys-tdma",
			   TDMA_BASE, TDMA_BASE_LEN,
			   0x0, TDMA_BASE_LEN,
			   0)
	},
};

static struct trm_hw_config tdma_trm_hw_cfg = {
	.trm_cfgs = tdma_trm_configs,
	.cfg_len = ARRAY_SIZE(tdma_trm_configs),
	.trm_hw_dump = tdma_trm_hw_dump,
};

static int tdma_trm_hw_dump(void *dst, u32 start_addr, u32 len)
{
	u32 ofs;

	if (unlikely(!dst))
		return -ENODEV;

	for (ofs = 0; len > 0; len -= 0x4, ofs += 0x4)
		writel(tdma_read(start_addr + ofs), dst + ofs);

	return 0;
}

static inline void tdma_prefetch_enable(bool en)
{
	if (en) {
		tdma_set(TDMA_PREF_TX_CFG, PREF_EN);
		tdma_set(TDMA_PREF_RX_CFG, PREF_EN);
	} else {
		/* wait for prefetch idle */
		while ((tdma_read(TDMA_PREF_TX_CFG) & PREF_BUSY)
		       || (tdma_read(TDMA_PREF_RX_CFG) & PREF_BUSY))
			;

		tdma_write(TDMA_PREF_TX_CFG,
			   tdma_read(TDMA_PREF_TX_CFG) & (~PREF_EN));
		tdma_write(TDMA_PREF_RX_CFG,
			   tdma_read(TDMA_PREF_RX_CFG) & (~PREF_EN));
	}
}

static inline void tdma_writeback_enable(bool en)
{
	if (en) {
		tdma_set(TDMA_WRBK_TX_CFG, WRBK_EN);
		tdma_set(TDMA_WRBK_RX_CFG, WRBK_EN);
	} else {
		/* wait for write back idle */
		while ((tdma_read(TDMA_WRBK_TX_CFG) & WRBK_BUSY)
		       || (tdma_read(TDMA_WRBK_RX_CFG) & WRBK_BUSY))
			;

		tdma_write(TDMA_WRBK_TX_CFG,
			   tdma_read(TDMA_WRBK_TX_CFG) & (~WRBK_EN));
		tdma_write(TDMA_WRBK_RX_CFG,
			   tdma_read(TDMA_WRBK_RX_CFG) & (~WRBK_EN));
	}
}

static inline void tdma_assert_prefetch_reset(bool en)
{
	if (en) {
		tdma_set(TDMA_PREF_TX_FIFO_CFG0, PREF_TX_RING0_CLEAR);
		tdma_set(TDMA_PREF_RX_FIFO_CFG0,
			 PREF_RX_RINGX_CLEAR(0) | PREF_RX_RINGX_CLEAR(1));
		tdma_set(TDMA_PREF_RX_FIFO_CFG1,
			 PREF_RX_RINGX_CLEAR(2) | PREF_RX_RINGX_CLEAR(3));
	} else {
		tdma_clr(TDMA_PREF_TX_FIFO_CFG0, PREF_TX_RING0_CLEAR);
		tdma_clr(TDMA_PREF_RX_FIFO_CFG0,
			 PREF_RX_RINGX_CLEAR(0) | PREF_RX_RINGX_CLEAR(1));
		tdma_clr(TDMA_PREF_RX_FIFO_CFG1,
			 PREF_RX_RINGX_CLEAR(2) | PREF_RX_RINGX_CLEAR(3));
	}
}

static inline void tdma_assert_fifo_reset(bool en)
{
	if (en) {
		tdma_set(TDMA_TX_XDMA_FIFO_CFG0,
			 (PAR_FIFO_CLEAR
			 | CMD_FIFO_CLEAR
			 | DMAD_FIFO_CLEAR
			 | ARR_FIFO_CLEAR));
		tdma_set(TDMA_RX_XDMA_FIFO_CFG0,
			 (PAR_FIFO_CLEAR
			 | CMD_FIFO_CLEAR
			 | DMAD_FIFO_CLEAR
			 | ARR_FIFO_CLEAR
			 | LEN_FIFO_CLEAR
			 | WID_FIFO_CLEAR
			 | BID_FIFO_CLEAR));
	} else {
		tdma_clr(TDMA_TX_XDMA_FIFO_CFG0,
			 (PAR_FIFO_CLEAR
			 | CMD_FIFO_CLEAR
			 | DMAD_FIFO_CLEAR
			 | ARR_FIFO_CLEAR));
		tdma_clr(TDMA_RX_XDMA_FIFO_CFG0,
			 (PAR_FIFO_CLEAR
			 | CMD_FIFO_CLEAR
			 | DMAD_FIFO_CLEAR
			 | ARR_FIFO_CLEAR
			 | LEN_FIFO_CLEAR
			 | WID_FIFO_CLEAR
			 | BID_FIFO_CLEAR));
	}
}

static inline void tdma_assert_writeback_reset(bool en)
{
	if (en) {
		tdma_set(TDMA_WRBK_TX_FIFO_CFG0, WRBK_RING_CLEAR);
		tdma_set(TDMA_WRBK_RX_FIFO_CFGX(0), WRBK_RING_CLEAR);
		tdma_set(TDMA_WRBK_RX_FIFO_CFGX(1), WRBK_RING_CLEAR);
		tdma_set(TDMA_WRBK_RX_FIFO_CFGX(2), WRBK_RING_CLEAR);
		tdma_set(TDMA_WRBK_RX_FIFO_CFGX(3), WRBK_RING_CLEAR);
	} else {
		tdma_clr(TDMA_WRBK_TX_FIFO_CFG0, WRBK_RING_CLEAR);
		tdma_clr(TDMA_WRBK_RX_FIFO_CFGX(0), WRBK_RING_CLEAR);
		tdma_clr(TDMA_WRBK_RX_FIFO_CFGX(1), WRBK_RING_CLEAR);
		tdma_clr(TDMA_WRBK_RX_FIFO_CFGX(2), WRBK_RING_CLEAR);
		tdma_clr(TDMA_WRBK_RX_FIFO_CFGX(3), WRBK_RING_CLEAR);
	}
}

static inline void tdma_assert_prefetch_ring_reset(bool en)
{
	if (en) {
		tdma_set(TDMA_PREF_SIDX_CFG,
			 (TX_RING0_SIDX_CLR
			 | RX_RINGX_SIDX_CLR(0)
			 | RX_RINGX_SIDX_CLR(1)
			 | RX_RINGX_SIDX_CLR(2)
			 | RX_RINGX_SIDX_CLR(3)));
	} else {
		tdma_clr(TDMA_PREF_SIDX_CFG,
			 (TX_RING0_SIDX_CLR
			 | RX_RINGX_SIDX_CLR(0)
			 | RX_RINGX_SIDX_CLR(1)
			 | RX_RINGX_SIDX_CLR(2)
			 | RX_RINGX_SIDX_CLR(3)));
	}
}

static inline void tdma_assert_writeback_ring_reset(bool en)
{
	if (en) {
		tdma_set(TDMA_WRBK_SIDX_CFG,
			 (TX_RING0_SIDX_CLR
			 | RX_RINGX_SIDX_CLR(0)
			 | RX_RINGX_SIDX_CLR(1)
			 | RX_RINGX_SIDX_CLR(2)
			 | RX_RINGX_SIDX_CLR(3)));
	} else {
		tdma_clr(TDMA_WRBK_SIDX_CFG,
			 (TX_RING0_SIDX_CLR
			 | RX_RINGX_SIDX_CLR(0)
			 | RX_RINGX_SIDX_CLR(1)
			 | RX_RINGX_SIDX_CLR(2)
			 | RX_RINGX_SIDX_CLR(3)));
	}
}

static void mtk_tops_tdma_retrieve_last_state(void)
{
	tdma.start_ring = tdma_read(TDMA_TX_CTX_IDX_0);
}

void mtk_tops_tdma_record_last_state(void)
{
	tdma_write(TDMA_TX_CTX_IDX_0, tdma.start_ring);
}

static void tdma_get_next_rx_ring(void)
{
	u32 pkt_num_per_core = tdma_read(TDMA_RX_MAX_CNT_X(0));
	u32 ring[TDMA_RING_NUM] = {0};
	u32 start = 0;
	u32 tmp_idx;
	u32 i;

	for (i = 0; i < TDMA_RING_NUM; i++) {
		tmp_idx = (tdma.start_ring + i) % TDMA_RING_NUM;
		ring[i] = tdma_read(TDMA_RX_DRX_IDX_X(tmp_idx));
	}

	for (i = 1; i < TDMA_RING_NUM; i++) {
		if (ring[i] >= (pkt_num_per_core - 1) && !ring[i - 1])
			ring[i - 1] += pkt_num_per_core;

		if (!ring[i] && ring[i - 1] >= (pkt_num_per_core - 1))
			ring[i] = pkt_num_per_core;

		if (ring[i] < ring[i - 1])
			start = i;
	}

	tdma.start_ring = (tdma.start_ring + start) & TDMA_RING_NUM_MOD;
}

void mtk_tops_tdma_reset(void)
{
	if (!mtk_tops_mcu_netsys_fe_rst())
		/* get next start Rx ring if TDMA reset without NETSYS FE reset */
		tdma_get_next_rx_ring();
	else
		/*
		 * NETSYS FE reset will restart CDM ring index
		 * so we don't need to calculate next ring index
		 */
		tdma.start_ring = 0;

	/* then start reset TDMA */
	tdma_assert_prefetch_reset(true);
	tdma_assert_prefetch_reset(false);

	tdma_assert_fifo_reset(true);
	tdma_assert_fifo_reset(false);

	tdma_assert_writeback_reset(true);
	tdma_assert_writeback_reset(false);

	/* reset tdma ring */
	tdma_set(TDMA_RST_IDX,
		 (RST_DTX_IDX_0
		 | RST_DRX_IDX_X(0)
		 | RST_DRX_IDX_X(1)
		 | RST_DRX_IDX_X(2)
		 | RST_DRX_IDX_X(3)));

	tdma_assert_prefetch_ring_reset(true);
	tdma_assert_prefetch_ring_reset(false);

	tdma_assert_writeback_ring_reset(true);
	tdma_assert_writeback_ring_reset(false);

	/* TODO: should we reset Tx/Rx CPU ring index? */
}

int mtk_tops_tdma_enable(void)
{
	struct mailbox_msg msg = {
		.msg1 = TOPS_NET_CMD_START,
		.msg2 = tdma.start_ring,
	};
	int ret;
	u32 i;

	tdma_prefetch_enable(true);

	tdma_set(TDMA_GLO_CFG0, RX_DMA_EN | TX_DMA_EN);

	tdma_writeback_enable(true);

	/* notify TOPS start network processing */
	ret = mbox_send_msg_no_wait(&tdma.mgmt_mdev, &msg);
	if (unlikely(ret))
		return ret;

	for (i = CORE_OFFLOAD_0; i < CORE_OFFLOAD_NUM; i++) {
		ret = mbox_send_msg_no_wait(&tdma.offload_mdev[i], &msg);
		if (unlikely(ret))
			return ret;
	}

	return ret;
}

void mtk_tops_tdma_disable(void)
{
	struct mailbox_msg msg = {
		.msg1 = TOPS_NET_CMD_STOP,
	};
	u32 i;

	if (mtk_tops_mcu_bring_up_done()) {
		/* notify TOPS stop network processing */
		if (unlikely(mbox_send_msg_no_wait(&tdma.mgmt_mdev, &msg)))
			return;

		for (i = CORE_OFFLOAD_0; i < CORE_OFFLOAD_NUM; i++) {
			if (unlikely(mbox_send_msg_no_wait(&tdma.offload_mdev[i],
							   &msg)))
				return;
		}
	}

	tdma_prefetch_enable(false);

	/* There is no need to wait for Tx/Rx idle before we stop Tx/Rx */
	if (!mtk_tops_mcu_netsys_fe_rst())
		while (tdma_read(TDMA_GLO_CFG0) & RX_DMA_BUSY)
			;
	tdma_write(TDMA_GLO_CFG0, tdma_read(TDMA_GLO_CFG0) & (~RX_DMA_EN));

	if (!mtk_tops_mcu_netsys_fe_rst())
		while (tdma_read(TDMA_GLO_CFG0) & TX_DMA_BUSY)
			;
	tdma_write(TDMA_GLO_CFG0, tdma_read(TDMA_GLO_CFG0) & (~TX_DMA_EN));

	tdma_writeback_enable(false);
}

static int mtk_tops_tdma_register_mbox(void)
{
	int ret;
	int i;

	ret = register_mbox_dev(MBOX_SEND, &tdma.mgmt_mdev);
	if (ret) {
		TOPS_ERR("register tdma mgmt mbox send failed: %d\n", ret);
		return ret;
	}

	for (i = 0; i < CORE_OFFLOAD_NUM; i++) {
		ret = register_mbox_dev(MBOX_SEND, &tdma.offload_mdev[i]);
		if (ret) {
			TOPS_ERR("register tdma offload %d mbox send failed: %d\n",
				 i, ret);
			goto err_unregister_offload_mbox;
		}
	}

	return ret;

err_unregister_offload_mbox:
	for (i -= 1; i >= 0; i--)
		unregister_mbox_dev(MBOX_SEND, &tdma.offload_mdev[i]);

	unregister_mbox_dev(MBOX_SEND, &tdma.mgmt_mdev);

	return ret;
}

static void mtk_tops_tdma_unregister_mbox(void)
{
	int i;

	unregister_mbox_dev(MBOX_SEND, &tdma.mgmt_mdev);

	for (i = 0; i < CORE_OFFLOAD_NUM; i++)
		unregister_mbox_dev(MBOX_SEND, &tdma.offload_mdev[i]);
}

static int mtk_tops_tdma_dts_init(struct platform_device *pdev)
{
	struct device_node *fe_mem = NULL;
	struct resource res;
	int ret = 0;

	fe_mem = of_parse_phandle(pdev->dev.of_node, "fe_mem", 0);
	if (!fe_mem) {
		TOPS_ERR("can not find fe_mem node\n");
		return -ENODEV;
	}

	if (of_address_to_resource(fe_mem, 0, &res))
		return -ENXIO;

	/* map FE address */
	tdma.base = devm_ioremap(&pdev->dev, res.start, resource_size(&res));
	if (!tdma.base)
		return -ENOMEM;

	/* shift FE address to TDMA base */
	tdma.base += TDMA_BASE;

	of_node_put(fe_mem);

	return ret;
}

int mtk_tops_tdma_init(struct platform_device *pdev)
{
	int ret = 0;

	ret = mtk_tops_tdma_register_mbox();
	if (ret)
		return ret;

	ret = mtk_tops_tdma_dts_init(pdev);
	if (ret)
		return ret;

	ret = mtk_trm_hw_config_register(TRM_TDMA, &tdma_trm_hw_cfg);
	if (ret)
		return ret;

	mtk_tops_tdma_retrieve_last_state();

	return ret;
}

void mtk_tops_tdma_deinit(struct platform_device *pdev)
{
	mtk_trm_hw_config_unregister(TRM_TDMA, &tdma_trm_hw_cfg);

	mtk_tops_tdma_unregister_mbox();
}
