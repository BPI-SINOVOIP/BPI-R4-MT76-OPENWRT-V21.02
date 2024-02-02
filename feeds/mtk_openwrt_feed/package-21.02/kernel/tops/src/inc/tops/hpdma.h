/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * Copyright (c) 2023 MediaTek Inc. All Rights Reserved.
 *
 * Author: Ren-Ting Wang <ren-ting.wang@mediatek.com>
 */

#ifndef _TOPS_HPDMA_H_
#define _TOPS_HPDMA_H_

#include <linux/bitops.h>
#include <linux/bitfield.h>

/* AXI DMA */
#define TOPS_HPDMA_X_SRC(x)			(0x100 * (x) + 0x0000)
#define TOPS_HPDMA_X_DST(x)			(0x100 * (x) + 0x0004)
#define TOPS_HPDMA_X_NUM(x)			(0x100 * (x) + 0x0008)
#define TOPS_HPDMA_X_CTRL(x)			(0x100 * (x) + 0x000C)
#define TOPS_HPDMA_X_CLRIRQ(x)			(0x100 * (x) + 0x0010)
#define TOPS_HPDMA_X_START(x)			(0x100 * (x) + 0x0014)
#define TOPS_HPDMA_X_RRESP(x)			(0x100 * (x) + 0x0018)
#define TOPS_HPDMA_X_BRESP(x)			(0x100 * (x) + 0x001C)
#define TOPS_HPDMA_X_HW(x)			(0x100 * (x) + 0x0020)
#define TOPS_HPDMA_X_ERR(x)			(0x100 * (x) + 0x0024)


/* AXI DMA NUM */
#define HPDMA_TOTALNUM_SHIFT			(0)
#define HPDMA_TOTALNUM_MASK			GENMASK(15, 0)

/* AXI DMA CTRL */
#define HPDMA_AXLEN_SHIFT			(0)
#define HPDMA_AXLEN_MASK			GENMASK(3, 0)
#define HPDMA_AXSIZE_SHIFT			(8)
#define HPDMA_AXSIZE_MASK			GENMASK(10, 8)
#define HPDMA_IRQEN				BIT(16)
#define HPDMA_AWMODE_EN				BIT(24)
#define HPDMA_OUTSTD_SHIFT			(25)
#define HPDMA_OUTSTD_MASK			GENMASK(29, 25)

/* AXI DMA START */
#define HPDMA_STATUS_SHIFT			(0)
#define HPDMA_STATUS_MASK			GENMASK(0, 0)
#define HPDMA_SKIP_RACE_SHIFT			(7)
#define HPDMA_SKIP_RACE_MASK			GENMASK(7, 7)
#define HPDMA_START				BIT(15)

/* AXI DMA RRESP */
#define HPDMA_LOG_SHIFT				(0)
#define HPDMA_LOG_MASK				GENMASK(15, 0)
#define HPDMA_RESP_SHIFT			(16)
#define HPDMA_RESP_MASK				GENMASK(17, 16)

/* AXI DMA HW */
#define HPDMA_FIFO_DEPTH_SHIFT			(0)
#define HPDMA_FIFO_DEPTH_MASK			GENMASK(7, 0)
#define HPDMA_MAX_AXSIZE_SHIFT			(8)
#define HPDMA_MAX_AXSIZE_MASK			GENMASK(15, 8)

enum hpdma_err {
	AWMODE_ERR = 0x1 << 0,
	AXSIZE_ERR = 0x1 << 1,
	ARADDR_ERR = 0x1 << 2,
	AWADDR_ERR = 0x1 << 3,
	RACE_ERR = 0x1 << 4,
};

enum top_hpdma_req {
	TOP_HPDMA_TNL_SYNC_REQ,

	__TOP_HPDMA_REQ,
};

enum clust_hpdma_req {
	CLUST_HPDMA_DUMMY_REQ,

	__CLUST_HPDMA_REQ,
};

int mtk_tops_hpdma_init(void);
void mtk_tops_hpdma_exit(void);
#endif /* _TOPS_HPDMA_H_ */
