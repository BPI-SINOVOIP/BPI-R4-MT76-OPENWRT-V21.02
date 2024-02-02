// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (c) 2023 MediaTek Inc. All Rights Reserved.
 *
 * Author: Ren-Ting Wang <ren-ting.wang@mediatek.com>
 */

#include <linux/device.h>
#include <linux/io.h>
#include <linux/mutex.h>
#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/platform_device.h>

#include <mtk_hnat/hnat.h>

#include <pce/netsys.h>

#include "tops/hpdma.h"
#include "tops/internal.h"
#include "tops/mcu.h"
#include "tops/netsys.h"
#include "tops/tdma.h"
#include "tops/trm.h"

/* Netsys dump length */
#define FE_BASE_LEN				(0x2900)

#define PPE_DEFAULT_ENTRY_SIZE			(0x400)

static int netsys_trm_hw_dump(void *dst, u32 ofs, u32 len);

struct netsys_hw {
	void __iomem *base;
};

static struct netsys_hw netsys;

static struct trm_config netsys_trm_configs[] = {
	{
		TRM_CFG_EN("netsys-fe",
			   FE_BASE, FE_BASE_LEN,
			   0x0, FE_BASE_LEN,
			   0)
	},
};

static struct trm_hw_config netsys_trm_hw_cfg = {
	.trm_cfgs = netsys_trm_configs,
	.cfg_len = ARRAY_SIZE(netsys_trm_configs),
	.trm_hw_dump = netsys_trm_hw_dump,
};

static inline void netsys_write(u32 reg, u32 val)
{
	writel(val, netsys.base + reg);
}

static inline void netsys_set(u32 reg, u32 mask)
{
	setbits(netsys.base + reg, mask);
}

static inline void netsys_clr(u32 reg, u32 mask)
{
	clrbits(netsys.base + reg, mask);
}

static inline void netsys_rmw(u32 reg, u32 mask, u32 val)
{
	clrsetbits(netsys.base + reg, mask, val);
}

static inline u32 netsys_read(u32 reg)
{
	return readl(netsys.base + reg);
}

static int netsys_trm_hw_dump(void *dst, u32 start_addr, u32 len)
{
	u32 ofs;

	if (unlikely(!dst))
		return -ENODEV;

	for (ofs = 0; len > 0; len -= 0x4, ofs += 0x4)
		writel(netsys_read(start_addr + ofs), dst + ofs);

	return 0;
}

static inline void ppe_rmw(enum pse_port ppe, u32 reg, u32 mask, u32 val)
{
	if (ppe == PSE_PORT_PPE0)
		netsys_rmw(PPE0_BASE + reg, mask, val);
	else if (ppe == PSE_PORT_PPE1)
		netsys_rmw(PPE1_BASE + reg, mask, val);
	else if (ppe == PSE_PORT_PPE2)
		netsys_rmw(PPE2_BASE + reg, mask, val);
}

static inline u32 ppe_read(enum pse_port ppe, u32 reg)
{
	if (ppe == PSE_PORT_PPE0)
		return netsys_read(PPE0_BASE + reg);
	else if (ppe == PSE_PORT_PPE1)
		return netsys_read(PPE1_BASE + reg);
	else if (ppe == PSE_PORT_PPE2)
		return netsys_read(PPE2_BASE + reg);

	return 0;
}

u32 mtk_tops_netsys_ppe_get_max_entry_num(u32 ppe_id)
{
	u32 tbl_entry_num;
	enum pse_port ppe;

	if (ppe_id == 0)
		ppe = PSE_PORT_PPE0;
	else if (ppe_id == 1)
		ppe = PSE_PORT_PPE1;
	else if (ppe_id == 2)
		ppe = PSE_PORT_PPE2;
	else
		return PPE_DEFAULT_ENTRY_SIZE << 5; /* max entry count */

	tbl_entry_num = ppe_read(ppe, PPE_TBL_CFG);
	if (tbl_entry_num > 5)
		return PPE_DEFAULT_ENTRY_SIZE << 5;

	return PPE_DEFAULT_ENTRY_SIZE << tbl_entry_num;
}

int mtk_tops_netsys_init(struct platform_device *pdev)
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

	netsys.base = devm_ioremap(&pdev->dev, res.start, resource_size(&res));
	if (!netsys.base)
		return -ENOMEM;

	ret = mtk_trm_hw_config_register(TRM_NETSYS, &netsys_trm_hw_cfg);
	if (ret)
		return ret;

	return ret;
}

void mtk_tops_netsys_deinit(struct platform_device *pdev)
{
	mtk_trm_hw_config_unregister(TRM_NETSYS, &netsys_trm_hw_cfg);
}
