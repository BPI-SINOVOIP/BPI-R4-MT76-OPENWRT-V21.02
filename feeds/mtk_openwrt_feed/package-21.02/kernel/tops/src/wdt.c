// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (c) 2023 MediaTek Inc. All Rights Reserved.
 *
 * Author: Alvin Kuo <alvin.kuog@mediatek.com>,
 */

#include <linux/interrupt.h>
#include <linux/io.h>

#include "tops/internal.h"
#include "tops/mbox.h"
#include "tops/ser.h"
#include "tops/trm.h"
#include "tops/wdt.h"

#define WDT_IRQ_STATUS			0x0140B0
#define TOP_WDT_MODE			0x012000
#define CLUST_WDT_MODE(x)		(0x512000 + 0x100 * (x))

#define WDT_MODE_KEY			0x22000000

struct watchdog_hw {
	void __iomem *base;
	struct mailbox_dev mgmt_mdev;
	struct mailbox_dev offload_mdev[CORE_OFFLOAD_NUM];
};

static struct watchdog_hw wdt = {
	.mgmt_mdev = MBOX_SEND_MGMT_DEV(WDT),
	.offload_mdev = {
		[CORE_OFFLOAD_0] = MBOX_SEND_OFFLOAD_DEV(0, WDT),
		[CORE_OFFLOAD_1] = MBOX_SEND_OFFLOAD_DEV(1, WDT),
		[CORE_OFFLOAD_2] = MBOX_SEND_OFFLOAD_DEV(2, WDT),
		[CORE_OFFLOAD_3] = MBOX_SEND_OFFLOAD_DEV(3, WDT),
	},
};

static inline void wdt_write(u32 reg, u32 val)
{
	writel(val, wdt.base + reg);
}

static inline void wdt_set(u32 reg, u32 mask)
{
	setbits(wdt.base + reg, mask);
}

static inline void wdt_clr(u32 reg, u32 mask)
{
	clrbits(wdt.base + reg, mask);
}

static inline void wdt_rmw(u32 reg, u32 mask, u32 val)
{
	clrsetbits(wdt.base + reg, mask, val);
}

static inline u32 wdt_read(u32 reg)
{
	return readl(wdt.base + reg);
}

static inline void wdt_irq_clr(u32 wdt_mode_reg)
{
	wdt_set(wdt_mode_reg, WDT_MODE_KEY);
}

static void wdt_ser_callback(struct tops_ser_params *ser_params)
{
	if (ser_params->type != TOPS_SER_WDT_TO)
		WARN_ON(1);

	mtk_trm_dump(ser_params->data.wdt.timeout_cores);
}

static void wdt_ser_mcmd_setup(struct tops_ser_params *ser_params,
			       struct mcu_ctrl_cmd *mcmd)
{
	mcmd->core_mask = (~ser_params->data.wdt.timeout_cores) & CORE_TOPS_MASK;
}

static irqreturn_t wdt_irq_handler(int irq, void *dev_id)
{
	struct tops_ser_params ser_params = {
		.type = TOPS_SER_WDT_TO,
		.ser_callback = wdt_ser_callback,
		.ser_mcmd_setup = wdt_ser_mcmd_setup,
	};
	u32 status;
	u32 i;

	status = wdt_read(WDT_IRQ_STATUS);
	if (status) {
		ser_params.data.wdt.timeout_cores = status;
		mtk_tops_ser(&ser_params);

		/* clear wdt irq */
		if (status & BIT(CORE_MGMT))
			wdt_irq_clr(TOP_WDT_MODE);

		for (i = 0; i < CORE_OFFLOAD_NUM; i++)
			if (status & BIT(i))
				wdt_irq_clr(CLUST_WDT_MODE(i));
	}
	TOPS_ERR("WDT Timeout: 0x%x\n", status);

	return IRQ_HANDLED;
}

int mtk_tops_wdt_trigger_timeout(enum core_id core)
{
	struct mailbox_msg msg = {
		.msg1 = WDT_CMD_TRIGGER_TIMEOUT,
	};

	if (core == CORE_MGMT)
		mbox_send_msg_no_wait(&wdt.mgmt_mdev, &msg);
	else
		mbox_send_msg_no_wait(&wdt.offload_mdev[core], &msg);

	return 0;
}

static int mtk_tops_wdt_register_mbox(void)
{
	int ret;
	int i;

	ret = register_mbox_dev(MBOX_SEND, &wdt.mgmt_mdev);
	if (ret) {
		TOPS_ERR("register wdt mgmt mbox send failed: %d\n", ret);
		return ret;
	}

	for (i = 0; i < CORE_OFFLOAD_NUM; i++) {
		ret = register_mbox_dev(MBOX_SEND, &wdt.offload_mdev[i]);
		if (ret) {
			TOPS_ERR("register wdt offload %d mbox send failed: %d\n",
				 i, ret);
			goto err_unregister_offload_mbox;
		}
	}

	return ret;

err_unregister_offload_mbox:
	for (i -= 1; i >= 0; i--)
		unregister_mbox_dev(MBOX_SEND, &wdt.offload_mdev[i]);

	unregister_mbox_dev(MBOX_SEND, &wdt.mgmt_mdev);

	return ret;
}

static void mtk_tops_wdt_unregister_mbox(void)
{
	int i;

	unregister_mbox_dev(MBOX_SEND, &wdt.mgmt_mdev);

	for (i = 0; i < CORE_OFFLOAD_NUM; i++)
		unregister_mbox_dev(MBOX_SEND, &wdt.offload_mdev[i]);
}

int mtk_tops_wdt_init(struct platform_device *pdev)
{
	struct resource *res = NULL;
	int ret;
	int irq;

	res = platform_get_resource_byname(pdev, IORESOURCE_MEM, "tops-base");
	if (!res)
		return -ENXIO;

	wdt.base = devm_ioremap(tops_dev, res->start, resource_size(res));
	if (!wdt.base)
		return -ENOMEM;

	irq = platform_get_irq_byname(pdev, "wdt");
	if (irq < 0) {
		TOPS_ERR("get wdt irq failed\n");
		return irq;
	}

	ret = devm_request_irq(tops_dev, irq,
			       wdt_irq_handler,
			       IRQF_ONESHOT,
			       pdev->name, NULL);
	if (ret) {
		TOPS_ERR("request wdt irq failed\n");
		return ret;
	}

	ret = mtk_tops_wdt_register_mbox();
	if (ret)
		return ret;

	return ret;
}

int mtk_tops_wdt_deinit(struct platform_device *pdev)
{
	mtk_tops_wdt_unregister_mbox();

	return 0;
}
