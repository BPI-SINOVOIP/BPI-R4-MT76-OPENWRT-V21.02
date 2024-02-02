/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * Copyright (c) 2023 MediaTek Inc. All Rights Reserved.
 *
 * Author: Ren-Ting Wang <ren-ting.wang@mediatek.com>
 */

#ifndef _TOPS_TDMA_H_
#define _TOPS_TDMA_H_

#include <linux/bitops.h>
#include <linux/bitfield.h>

/* TDMA */
#define TDMA_BASE				(0x6000)

#define TDMA_TX_CTX_IDX_0			(0x008)
#define TDMA_RX_MAX_CNT_X(idx)			(0x104 + ((idx) * 0x10))
#define TDMA_RX_CRX_IDX_X(idx)			(0x108 + ((idx) * 0x10))
#define TDMA_RX_DRX_IDX_X(idx)			(0x10C + ((idx) * 0x10))
#define TDMA_GLO_CFG0				(0x204)
#define TDMA_RST_IDX				(0x208)
#define TDMA_TX_XDMA_FIFO_CFG0			(0x238)
#define TDMA_RX_XDMA_FIFO_CFG0			(0x23C)
#define TDMA_PREF_TX_CFG			(0x2D0)
#define TDMA_PREF_TX_FIFO_CFG0			(0x2D4)
#define TDMA_PREF_RX_CFG			(0x2DC)
#define TDMA_PREF_RX_FIFO_CFG0			(0x2E0)
#define TDMA_PREF_SIDX_CFG			(0x2E4)
#define TDMA_WRBK_TX_CFG			(0x300)
#define TDMA_WRBK_TX_FIFO_CFG0			(0x304)
#define TDMA_WRBK_RX_CFG			(0x344)
#define TDMA_WRBK_RX_FIFO_CFGX(x)		(0x348 + 0x4 * (x))
#define TDMA_WRBK_SIDX_CFG			(0x388)
#define TDMA_PREF_RX_FIFO_CFG1			(0x3EC)

/* TDMA_GLO_CFG0 */
#define TX_DMA_EN				(BIT(0))
#define TX_DMA_BUSY				(BIT(1))
#define RX_DMA_EN				(BIT(2))
#define RX_DMA_BUSY				(BIT(3))
#define DMA_BT_SIZE_MASK			(0x7)
#define DMA_BT_SIZE_SHIFT			(11)
#define OTSD_THRES_MASK				(0xF)
#define OTSD_THRES_SHIFT			(14)
#define CDM_FCNT_THRES_MASK			(0xF)
#define CDM_FCNT_THRES_SHIFT			(18)
#define LB_MODE					(BIT(24))
#define PKT_WCOMP				(BIT(27))
#define DEC_WCOMP				(BIT(28))

/* TDMA_RST_IDX */
#define RST_DTX_IDX_0				(BIT(0))
#define RST_DRX_IDX_X(idx)			(BIT(16 + (idx)))

/* TDMA_TX_XDMA_FIFO_CFG0 TDMA_RX_XDMA_FIFO_CFG0 */
#define PAR_FIFO_CLEAR				(BIT(0))
#define CMD_FIFO_CLEAR				(BIT(4))
#define DMAD_FIFO_CLEAR				(BIT(8))
#define ARR_FIFO_CLEAR				(BIT(12))
#define LEN_FIFO_CLEAR				(BIT(15))
#define WID_FIFO_CLEAR				(BIT(18))
#define BID_FIFO_CLEAR				(BIT(21))

/* TDMA_SDL_CFG */
#define SDL_EN					(BIT(16))
#define SDL_MASK				(0xFFFF)
#define SDL_SHIFT				(0)

/* TDMA_PREF_TX_CFG TDMA_PREF_RX_CFG */
#define PREF_BUSY				BIT(1)
#define PREF_EN					BIT(0)

/* TDMA_PREF_TX_FIFO_CFG0 TDMA_PREF_RX_FIFO_CFG0 TDMA_PREF_RX_FIFO_CFG1 */
#define PREF_TX_RING0_CLEAR			(BIT(0))
#define PREF_RX_RINGX_CLEAR(x)			(BIT((((x) % 2) * 16)))
#define PREF_RX_RING1_CLEAR			(BIT(0))
#define PREF_RX_RING2_CLEAR			(BIT(16))
#define PREF_RX_RING3_CLEAR			(BIT(0))
#define PREF_RX_RING4_CLEAR			(BIT(16))

/* TDMA_PREF_SIDX_CFG TDMA_WRBK_SIDX_CFG */
#define TX_RING0_SIDX_CLR			(BIT(0))
#define RX_RINGX_SIDX_CLR(x)			(BIT(4 + (x)))

/* TDMA_WRBK_TX_FIFO_CFG0 TDMA_WRBK_RX_FIFO_CFGX */
#define WRBK_RING_CLEAR				(BIT(0))

/* TDMA_WRBK_TX_CFG TDMA_WRBK_RX_CFG */
#define WRBK_BUSY				(BIT(0))
#define BURST_SIZE_SHIFT			(6)
#define BURST_SIZE_MASK				(0x1F)
#define WRBK_THRES_SHIFT			(14)
#define WRBK_THRES_MASK				(0x3F)
#define FLUSH_TIMER_EN				(BIT(21))
#define MAX_PENDING_TIME_SHIFT			(22)
#define MAX_PENDING_TIME_MASK			(0xFF)
#define WRBK_EN					(BIT(30))

#define TDMA_RING_NUM				(4)
#define TDMA_RING_NUM_MOD			(TDMA_RING_NUM - 1)

enum tops_net_cmd {
	TOPS_NET_CMD_NULL,
	TOPS_NET_CMD_STOP,
	TOPS_NET_CMD_START,

	__TOPS_NET_CMD_MAX,
};

void mtk_tops_tdma_record_last_state(void);
void mtk_tops_tdma_reset(void);
int mtk_tops_tdma_enable(void);
void mtk_tops_tdma_disable(void);
int mtk_tops_tdma_init(struct platform_device *pdev);
void mtk_tops_tdma_deinit(struct platform_device *pdev);
#endif /* _TOPS_TDMA_H_ */
