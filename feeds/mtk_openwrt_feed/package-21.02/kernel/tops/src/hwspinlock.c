// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (c) 2023 MediaTek Inc. All Rights Reserved.
 *
 * Author: Ren-Ting Wang <ren-ting.wang@mediatek.com>
 */

#include <linux/io.h>
#include <linux/err.h>
#include <linux/types.h>
#include <linux/device.h>
#include <linux/platform_device.h>

#include "tops/hwspinlock.h"
#include "tops/tops.h"

#define SEMA_ID				(BIT(CORE_AP))

static void __iomem *base;

static inline u32 hwspinlock_read(u32 reg)
{
	return readl(base + reg);
}

static inline void hwspinlock_write(u32 reg, u32 val)
{
	writel(val, base + reg);
}

static inline u32 __mtk_tops_hwspinlock_get_reg(enum hwspinlock_group grp, u32 slot)
{
	if (unlikely(slot >= HWSPINLOCK_SLOT_MAX || grp >= __HWSPINLOCK_GROUP_MAX))
		return 0;

	if (grp == HWSPINLOCK_GROUP_TOP)
		return HWSPINLOCK_TOP_BASE + slot * 4;
	else
		return HWSPINLOCK_CLUST_BASE + slot * 4;
}

/*
 * try take TOPS HW spinlock
 * return 1 on success
 * return 0 on failure
 */
int mtk_tops_hwspin_try_lock(enum hwspinlock_group grp, u32 slot)
{
	u32 reg = __mtk_tops_hwspinlock_get_reg(grp, slot);

	WARN_ON(!reg);

	hwspinlock_write(reg, SEMA_ID);

	return hwspinlock_read(reg) == SEMA_ID ? 1 : 0;
}

void mtk_tops_hwspin_lock(enum hwspinlock_group grp, u32 slot)
{
	u32 reg = __mtk_tops_hwspinlock_get_reg(grp, slot);

	WARN_ON(!reg);

	do {
		hwspinlock_write(reg, SEMA_ID);
	} while (hwspinlock_read(reg) != SEMA_ID);
}

void mtk_tops_hwspin_unlock(enum hwspinlock_group grp, u32 slot)
{
	u32 reg = __mtk_tops_hwspinlock_get_reg(grp, slot);

	WARN_ON(!reg);

	if (hwspinlock_read(reg) == SEMA_ID)
		hwspinlock_write(reg, SEMA_ID);
}

int mtk_tops_hwspinlock_init(struct platform_device *pdev)
{
	struct resource *res;

	res = platform_get_resource_byname(pdev, IORESOURCE_MEM, "tops-base");
	if (!res)
		return -ENXIO;

	base = devm_ioremap(&pdev->dev, res->start, resource_size(res));
	if (!base)
		return -ENOMEM;

	return 0;
}
