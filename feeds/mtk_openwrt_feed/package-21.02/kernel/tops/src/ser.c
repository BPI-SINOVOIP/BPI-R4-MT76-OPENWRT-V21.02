// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (c) 2023 MediaTek Inc. All Rights Reserved.
 *
 * Author: Alvin Kuo <alvin.kuog@mediatek.com>
 */

#include <linux/err.h>
#include <linux/workqueue.h>
#include <linux/netdevice.h>
#include <linux/rtnetlink.h>

#include "tops/internal.h"
#include "tops/net-event.h"
#include "tops/ser.h"
#include "tops/trm.h"

struct tops_ser {
	struct work_struct work;
	struct tops_ser_params ser_params;
	spinlock_t params_lock;
};

struct tops_ser tops_ser;

static inline void __mtk_tops_ser_cmd_clear(void)
{
	memset(&tops_ser.ser_params, 0, sizeof(struct tops_ser_params));
	tops_ser.ser_params.type = __TOPS_SER_TYPE_MAX;
}

static inline void mtk_tops_ser_cmd_clear(void)
{
	unsigned long flag;

	spin_lock_irqsave(&tops_ser.params_lock, flag);

	__mtk_tops_ser_cmd_clear();

	spin_unlock_irqrestore(&tops_ser.params_lock, flag);
}

static void mtk_tops_ser_setup_mcmd(struct tops_ser_params *ser_params,
				    struct mcu_ctrl_cmd *mcmd)
{
	memset(mcmd, 0, sizeof(struct mcu_ctrl_cmd));

	switch (ser_params->type) {
	case TOPS_SER_NETSYS_FE_RST:
		mcmd->e = MCU_EVENT_TYPE_FE_RESET;
		break;
	case TOPS_SER_WDT_TO:
		mcmd->e = MCU_EVENT_TYPE_WDT_TIMEOUT;
		break;
	default:
		TOPS_ERR("unsupport TOPS SER type: %u\n", ser_params->type);
		return;
	}

	if (ser_params->ser_mcmd_setup)
		ser_params->ser_mcmd_setup(ser_params, mcmd);
}

static void mtk_tops_ser_reset_callback(void *params)
{
	struct tops_ser_params *ser_params = params;

	if (ser_params->ser_callback)
		ser_params->ser_callback(ser_params);
}

static void mtk_tops_ser_work(struct work_struct *work)
{
	struct tops_ser_params ser_params;
	struct mcu_ctrl_cmd mcmd;
	unsigned long flag = 0;

	spin_lock_irqsave(&tops_ser.params_lock, flag);

	while (tops_ser.ser_params.type != __TOPS_SER_TYPE_MAX) {
		memcpy(&ser_params,
		       &tops_ser.ser_params,
		       sizeof(struct tops_ser_params));

		spin_unlock_irqrestore(&tops_ser.params_lock, flag);

		mtk_tops_ser_setup_mcmd(&ser_params, &mcmd);

		if (mtk_tops_mcu_reset(&mcmd,
				       mtk_tops_ser_reset_callback,
				       &ser_params)) {
			TOPS_INFO("SER type: %u failed to recover\n",
				  ser_params.type);
			/*
			 * TODO: check is OK to directly return
			 * since mcu state machine should handle
			 * state transition failed?
			 */
			mtk_tops_ser_cmd_clear();
			return;
		}

		TOPS_INFO("SER type: %u successfully recovered\n", ser_params.type);

		spin_lock_irqsave(&tops_ser.params_lock, flag);
		/*
		 * If there isn't queued any other SER cmd that has higher priority
		 * than current SER command, clear SER command and exit.
		 * Otherwise let the work perform reset again for high priority SER.
		 */
		if (tops_ser.ser_params.type > ser_params.type
		    || !memcmp(&tops_ser.ser_params, &ser_params,
			       sizeof(struct tops_ser_params)))
			__mtk_tops_ser_cmd_clear();
	}

	spin_unlock_irqrestore(&tops_ser.params_lock, flag);
}

int mtk_tops_ser(struct tops_ser_params *ser_params)
{
	unsigned long flag;

	if (!ser_params)
		return -EINVAL;

	spin_lock_irqsave(&tops_ser.params_lock, flag);

	/* higher SER type should not override lower SER type */
	if (tops_ser.ser_params.type != __TOPS_SER_TYPE_MAX
	    && tops_ser.ser_params.type < ser_params->type)
		goto unlock;

	memcpy(&tops_ser.ser_params, ser_params, sizeof(*ser_params));

	schedule_work(&tops_ser.work);

unlock:
	spin_unlock_irqrestore(&tops_ser.params_lock, flag);

	return 0;
}

int mtk_tops_ser_init(struct platform_device *pdev)
{
	INIT_WORK(&tops_ser.work, mtk_tops_ser_work);

	spin_lock_init(&tops_ser.params_lock);

	tops_ser.ser_params.type = __TOPS_SER_TYPE_MAX;

	return 0;
}

int mtk_tops_ser_deinit(struct platform_device *pdev)
{
	return 0;
}
