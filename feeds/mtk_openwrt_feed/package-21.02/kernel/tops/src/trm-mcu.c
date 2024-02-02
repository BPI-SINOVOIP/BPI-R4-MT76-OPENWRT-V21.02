// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (c) 2023 MediaTek Inc. All Rights Reserved.
 *
 * Author: Alvin Kuo <alvin.kuog@mediatek.com>
 *         Ren-Ting Wang <ren-ting.wang@mediatek.com>
 */

#include <linux/delay.h>
#include <linux/device.h>
#include <linux/dma-mapping.h>
#include <linux/io.h>
#include <linux/of.h>

#include "tops/internal.h"
#include "tops/mcu.h"
#include "tops/trm-debugfs.h"
#include "tops/trm-fs.h"
#include "tops/trm-mcu.h"
#include "tops/trm.h"

#define TOPS_OCD_RETRY_TIMES		(3)

#define TOPS_OCD_DCRSET			(0x200C)
#define ENABLE_OCD			(1 << 0)
#define DEBUG_INT			(1 << 1)

#define TOPS_OCD_DSR			(0x2010)
#define EXEC_DONE			(1 << 0)
#define EXEC_EXCE			(1 << 1)
#define EXEC_BUSY			(1 << 2)
#define STOPPED				(1 << 4)
#define DEBUG_PEND_HOST			(1 << 17)

#define TOPS_OCD_DDR			(0x2014)

#define TOPS_OCD_DIR0EXEC		(0x201C)

struct tops_ocd_dev {
	void __iomem *base;
	u32 base_offset;
	struct clk *debugsys_clk;
};

static struct tops_ocd_dev ocd;

struct core_dump_fram cd_frams[CORE_TOPS_NUM];

static inline void ocd_write(struct tops_ocd_dev *ocd, u32 reg, u32 val)
{
	writel(val, ocd->base + ocd->base_offset + reg);
}

static inline u32 ocd_read(struct tops_ocd_dev *ocd, u32 reg)
{
	return readl(ocd->base + ocd->base_offset + reg);
}

static inline void ocd_set(struct tops_ocd_dev *ocd, u32 reg, u32 mask)
{
	setbits(ocd->base + ocd->base_offset + reg, mask);
}

static inline void ocd_clr(struct tops_ocd_dev *ocd, u32 reg, u32 mask)
{
	clrbits(ocd->base + ocd->base_offset + reg, mask);
}

static int core_exec_instr(u32 instr)
{
	u32 rty = 0;
	int ret;

	ocd_set(&ocd, TOPS_OCD_DSR, EXEC_DONE);
	ocd_set(&ocd, TOPS_OCD_DSR, EXEC_EXCE);

	ocd_write(&ocd, TOPS_OCD_DIR0EXEC, instr);

	while ((ocd_read(&ocd, TOPS_OCD_DSR) & EXEC_BUSY)) {
		if (rty++ < TOPS_OCD_RETRY_TIMES) {
			usleep_range(1000, 1500);
		} else {
			TRM_ERR("run instruction(0x%x) timeout\n", instr);
			ret = -1;
			goto out;
		}
	}

	ret = ocd_read(&ocd, TOPS_OCD_DSR) & EXEC_EXCE ? -1 : 0;
	if (ret)
		TRM_ERR("run instruction(0x%x) fail\n", instr);

out:
	return ret;
}

static int core_dump(struct core_dump_fram *cd_fram)
{
	cd_fram->magic = CORE_DUMP_FRAM_MAGIC;
	cd_fram->num_areg = XCHAL_NUM_AREG;

	/*
	 * save
	 * PC, PS, WINDOWSTART, WINDOWBASE,
	 * EPC1, EXCCAUSE, EXCVADDR, EXCSAVE1
	 */
	core_exec_instr(0x13f500);

	core_exec_instr(0x03b500);
	core_exec_instr(0x136800);
	cd_fram->pc = ocd_read(&ocd, TOPS_OCD_DDR);

	core_exec_instr(0x03e600);
	core_exec_instr(0x136800);
	cd_fram->ps = ocd_read(&ocd, TOPS_OCD_DDR);

	core_exec_instr(0x034900);
	core_exec_instr(0x136800);
	cd_fram->windowstart = ocd_read(&ocd, TOPS_OCD_DDR);

	core_exec_instr(0x034800);
	core_exec_instr(0x136800);
	cd_fram->windowbase = ocd_read(&ocd, TOPS_OCD_DDR);

	core_exec_instr(0x03b100);
	core_exec_instr(0x136800);
	cd_fram->epc1 = ocd_read(&ocd, TOPS_OCD_DDR);

	core_exec_instr(0x03e800);
	core_exec_instr(0x136800);
	cd_fram->exccause = ocd_read(&ocd, TOPS_OCD_DDR);

	core_exec_instr(0x03ee00);
	core_exec_instr(0x136800);
	cd_fram->excvaddr = ocd_read(&ocd, TOPS_OCD_DDR);

	core_exec_instr(0x03d100);
	core_exec_instr(0x136800);
	cd_fram->excsave1 = ocd_read(&ocd, TOPS_OCD_DDR);

	core_exec_instr(0x03f500);

	/*
	 * save
	 * a0, a1, a2, a3, a4, a5, a6, a7
	 */
	core_exec_instr(0x136800);
	cd_fram->areg[0] = ocd_read(&ocd, TOPS_OCD_DDR);

	core_exec_instr(0x136810);
	cd_fram->areg[1] = ocd_read(&ocd, TOPS_OCD_DDR);

	core_exec_instr(0x136820);
	cd_fram->areg[2] = ocd_read(&ocd, TOPS_OCD_DDR);

	core_exec_instr(0x136830);
	cd_fram->areg[3] = ocd_read(&ocd, TOPS_OCD_DDR);

	core_exec_instr(0x136840);
	cd_fram->areg[4] = ocd_read(&ocd, TOPS_OCD_DDR);

	core_exec_instr(0x136850);
	cd_fram->areg[5] = ocd_read(&ocd, TOPS_OCD_DDR);

	core_exec_instr(0x136860);
	cd_fram->areg[6] = ocd_read(&ocd, TOPS_OCD_DDR);

	core_exec_instr(0x136870);
	cd_fram->areg[7] = ocd_read(&ocd, TOPS_OCD_DDR);

	/*
	 * save
	 * a8, a9, a10, a11, a12, a13, a14, a15
	 */
	core_exec_instr(0x136880);
	cd_fram->areg[8] = ocd_read(&ocd, TOPS_OCD_DDR);

	core_exec_instr(0x136890);
	cd_fram->areg[9] = ocd_read(&ocd, TOPS_OCD_DDR);

	core_exec_instr(0x1368a0);
	cd_fram->areg[10] = ocd_read(&ocd, TOPS_OCD_DDR);

	core_exec_instr(0x1368b0);
	cd_fram->areg[11] = ocd_read(&ocd, TOPS_OCD_DDR);

	core_exec_instr(0x1368c0);
	cd_fram->areg[12] = ocd_read(&ocd, TOPS_OCD_DDR);

	core_exec_instr(0x1368d0);
	cd_fram->areg[13] = ocd_read(&ocd, TOPS_OCD_DDR);

	core_exec_instr(0x1368e0);
	cd_fram->areg[14] = ocd_read(&ocd, TOPS_OCD_DDR);

	core_exec_instr(0x1368f0);
	cd_fram->areg[15] = ocd_read(&ocd, TOPS_OCD_DDR);

	core_exec_instr(0x408020);

	/*
	 * save
	 * a16, a17, a18, a19, a20, a21, a22, a23
	 */
	core_exec_instr(0x136880);
	cd_fram->areg[16] = ocd_read(&ocd, TOPS_OCD_DDR);

	core_exec_instr(0x136890);
	cd_fram->areg[17] = ocd_read(&ocd, TOPS_OCD_DDR);

	core_exec_instr(0x1368a0);
	cd_fram->areg[18] = ocd_read(&ocd, TOPS_OCD_DDR);

	core_exec_instr(0x1368b0);
	cd_fram->areg[19] = ocd_read(&ocd, TOPS_OCD_DDR);

	core_exec_instr(0x1368c0);
	cd_fram->areg[20] = ocd_read(&ocd, TOPS_OCD_DDR);

	core_exec_instr(0x1368d0);
	cd_fram->areg[21] = ocd_read(&ocd, TOPS_OCD_DDR);

	core_exec_instr(0x1368e0);
	cd_fram->areg[22] = ocd_read(&ocd, TOPS_OCD_DDR);

	core_exec_instr(0x1368f0);
	cd_fram->areg[23] = ocd_read(&ocd, TOPS_OCD_DDR);

	core_exec_instr(0x408020);

	/*
	 * save
	 * a24, a25, a26, a27, a28, a29, a30, a31
	 */
	core_exec_instr(0x136880);
	cd_fram->areg[24] = ocd_read(&ocd, TOPS_OCD_DDR);

	core_exec_instr(0x136890);
	cd_fram->areg[25] = ocd_read(&ocd, TOPS_OCD_DDR);

	core_exec_instr(0x1368a0);
	cd_fram->areg[26] = ocd_read(&ocd, TOPS_OCD_DDR);

	core_exec_instr(0x1368b0);
	cd_fram->areg[27] = ocd_read(&ocd, TOPS_OCD_DDR);

	core_exec_instr(0x1368c0);
	cd_fram->areg[28] = ocd_read(&ocd, TOPS_OCD_DDR);

	core_exec_instr(0x1368d0);
	cd_fram->areg[29] = ocd_read(&ocd, TOPS_OCD_DDR);

	core_exec_instr(0x1368e0);
	cd_fram->areg[30] = ocd_read(&ocd, TOPS_OCD_DDR);

	core_exec_instr(0x1368f0);
	cd_fram->areg[31] = ocd_read(&ocd, TOPS_OCD_DDR);

	core_exec_instr(0x408040);

	core_exec_instr(0xf1e000);

	return 0;
}

static int __mtk_trm_mcu_core_dump(enum core_id core)
{
	u32 rty = 0;
	int ret;

	ocd.base_offset = (core == CORE_MGMT) ? (0x0) : (0x5000 + (core * 0x4000));

	/* enable OCD */
	ocd_set(&ocd, TOPS_OCD_DCRSET, ENABLE_OCD);

	/* assert debug interrupt to core */
	ocd_set(&ocd, TOPS_OCD_DCRSET, DEBUG_INT);

	/* wait core into stopped state */
	while (!(ocd_read(&ocd, TOPS_OCD_DSR) & STOPPED)) {
		if (rty++ < TOPS_OCD_RETRY_TIMES) {
			usleep_range(10000, 15000);
		} else {
			TRM_ERR("wait core(%d) into stopped state timeout\n", core);
			ret = -1;
			goto out;
		}
	}

	/* deassert debug interrupt to core */
	ocd_set(&ocd, TOPS_OCD_DSR, DEBUG_PEND_HOST);

	/* dump core's registers and let core into running state */
	ret = core_dump(&cd_frams[core]);

out:
	return ret;
}

int mtk_trm_mcu_core_dump(void)
{
	enum core_id core;
	int ret;

	ret = clk_prepare_enable(ocd.debugsys_clk);
	if (ret) {
		TRM_ERR("debugsys clk enable failed: %d\n", ret);
		goto out;
	}

	memset(cd_frams, 0, sizeof(cd_frams));

	for (core = CORE_OFFLOAD_0; core <= CORE_MGMT; core++) {
		ret = __mtk_trm_mcu_core_dump(core);
		if (ret)
			break;
	}

	clk_disable_unprepare(ocd.debugsys_clk);

out:
	return ret;
}

static int mtk_tops_ocd_probe(struct platform_device *pdev)
{
	struct resource *res = NULL;
	int ret;

	trm_dev = &pdev->dev;

	ocd.debugsys_clk = devm_clk_get(trm_dev, "debugsys");
	if (IS_ERR(ocd.debugsys_clk)) {
		TRM_ERR("get debugsys clk failed: %ld\n", PTR_ERR(ocd.debugsys_clk));
		return PTR_ERR(ocd.debugsys_clk);
	}

	res = platform_get_resource_byname(pdev, IORESOURCE_MEM, "tops-ocd-base");
	if (!res)
		return -ENXIO;

	ocd.base = devm_ioremap(trm_dev, res->start, resource_size(res));
	if (!ocd.base)
		return -ENOMEM;

	ret = mtk_trm_debugfs_init();
	if (ret)
		return ret;

	ret = mtk_trm_fs_init();
	if (ret)
		return ret;

	TRM_INFO("tops-ocd init done\n");

	return 0;
}

static int mtk_tops_ocd_remove(struct platform_device *pdev)
{
	mtk_trm_fs_deinit();

	mtk_trm_debugfs_deinit();

	return 0;
}

static struct of_device_id mtk_tops_ocd_match[] = {
	{ .compatible = "mediatek,tops-ocd", },
	{ },
};

static struct platform_driver mtk_tops_ocd_driver = {
	.probe = mtk_tops_ocd_probe,
	.remove = mtk_tops_ocd_remove,
	.driver = {
		.name = "mediatek,tops-ocd",
		.owner = THIS_MODULE,
		.of_match_table = mtk_tops_ocd_match,
	},
};

int __init mtk_tops_trm_mcu_init(void)
{
	return platform_driver_register(&mtk_tops_ocd_driver);
}

void __exit mtk_tops_trm_mcu_exit(void)
{
	platform_driver_unregister(&mtk_tops_ocd_driver);
}
