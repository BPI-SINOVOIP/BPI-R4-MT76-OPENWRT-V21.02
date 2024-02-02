/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * Copyright (c) 2023 MediaTek Inc. All Rights Reserved.
 *
 * Author: Ren-Ting Wang <ren-ting.wang@mediatek.com>
 */

#ifndef _TOPS_INTERNAL_H_
#define _TOPS_INTERNAL_H_

#include <linux/bitfield.h>
#include <linux/device.h>
#include <linux/io.h>

extern struct device *tops_dev;

#define TOPS_DBG(fmt, ...)		dev_dbg(tops_dev, fmt, ##__VA_ARGS__)
#define TOPS_INFO(fmt, ...)		dev_info(tops_dev, fmt, ##__VA_ARGS__)
#define TOPS_NOTICE(fmt, ...)		dev_notice(tops_dev, fmt, ##__VA_ARGS__)
#define TOPS_WARN(fmt, ...)		dev_warn(tops_dev, fmt, ##__VA_ARGS__)
#define TOPS_ERR(fmt, ...)		dev_err(tops_dev, fmt, ##__VA_ARGS__)

/* tops 32 bits read/write */
#define setbits(addr, set)		writel(readl(addr) | (set), (addr))
#define clrbits(addr, clr)		writel(readl(addr) & ~(clr), (addr))
#define clrsetbits(addr, clr, set)	writel((readl(addr) & ~(clr)) | (set), (addr))
#endif /* _TOPS_INTERNAL_H_ */
