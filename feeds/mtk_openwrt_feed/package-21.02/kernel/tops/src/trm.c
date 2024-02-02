// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (c) 2023 MediaTek Inc. All Rights Reserved.
 *
 * Author: Alvin Kuo <alvin.kuog@mediatek.com>
 *         Ren-Ting Wang <ren-ting.wang@mediatek.com>
 */

#include <linux/debugfs.h>
#include <linux/err.h>
#include <linux/io.h>
#include <linux/mutex.h>
#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/platform_device.h>
#include <linux/printk.h>
#include <linux/relay.h>
#include <linux/types.h>

#include "tops/internal.h"
#include "tops/mbox.h"
#include "tops/mcu.h"
#include "tops/netsys.h"
#include "tops/trm-fs.h"
#include "tops/trm-mcu.h"
#include "tops/trm.h"

#define TRM_HDR_LEN				(sizeof(struct trm_header))

#define RLY_DUMP_SUBBUF_DATA_MAX		(RLY_DUMP_SUBBUF_SZ - TRM_HDR_LEN)

struct tops_runtime_monitor {
	struct mailbox_dev mgmt_send_mdev;
	struct mailbox_dev offload_send_mdev[CORE_OFFLOAD_NUM];
};

struct trm_info {
	char name[TRM_CONFIG_NAME_MAX_LEN];
	u64 dump_time;
	u32 start_addr;
	u32 size;
	u32 rsn; /* TRM_RSN_* */
};

struct trm_header {
	struct trm_info info;
	u32 data_offset;
	u32 data_len;
	u8 last_frag;
};

struct device *trm_dev;

static struct tops_runtime_monitor trm = {
	.mgmt_send_mdev = MBOX_SEND_MGMT_DEV(TRM),
	.offload_send_mdev = {
		[CORE_OFFLOAD_0] = MBOX_SEND_OFFLOAD_DEV(0, TRM),
		[CORE_OFFLOAD_1] = MBOX_SEND_OFFLOAD_DEV(1, TRM),
		[CORE_OFFLOAD_2] = MBOX_SEND_OFFLOAD_DEV(2, TRM),
		[CORE_OFFLOAD_3] = MBOX_SEND_OFFLOAD_DEV(3, TRM),
	},
};
static struct trm_hw_config *trm_hw_configs[__TRM_HARDWARE_MAX];
struct mutex trm_lock;

static inline void trm_hdr_init(struct trm_header *trm_hdr,
				struct trm_config *trm_cfg,
				u32 size,
				u64 dump_time,
				u32 dump_rsn)
{
	if (unlikely(!trm_hdr || !trm_cfg))
		return;

	memset(trm_hdr, 0, TRM_HDR_LEN);

	strncpy(trm_hdr->info.name, trm_cfg->name, TRM_CONFIG_NAME_MAX_LEN);
	trm_hdr->info.name[TRM_CONFIG_NAME_MAX_LEN - 1] = '\0';
	trm_hdr->info.start_addr = trm_cfg->addr + trm_cfg->offset;
	trm_hdr->info.size = size;
	trm_hdr->info.dump_time = dump_time;
	trm_hdr->info.rsn = dump_rsn;
}

static inline int trm_cfg_sanity_check(struct trm_config *trm_cfg)
{
	u32 start = trm_cfg->addr + trm_cfg->offset;
	u32 end = start + trm_cfg->size;

	if (start < trm_cfg->addr || end > trm_cfg->addr + trm_cfg->len)
		return -1;

	return 0;
}

static inline bool trm_cfg_is_core_dump_en(struct trm_config *trm_cfg)
{
	return trm_cfg->flag & TRM_CONFIG_F_CORE_DUMP;
}

static inline bool trm_cfg_is_en(struct trm_config *trm_cfg)
{
	return trm_cfg->flag & TRM_CONFIG_F_ENABLE;
}

static inline int __mtk_trm_cfg_setup(struct trm_config *trm_cfg,
				      u32 offset, u32 size, u8 enable)
{
	struct trm_config tmp = { 0 };

	if (!enable) {
		trm_cfg->flag &= ~TRM_CONFIG_F_ENABLE;
	} else {
		tmp.addr = trm_cfg->addr;
		tmp.len = trm_cfg->len;
		tmp.offset = offset;
		tmp.size = size;

		if (trm_cfg_sanity_check(&tmp))
			return -EINVAL;

		trm_cfg->offset = offset;
		trm_cfg->size = size;
		trm_cfg->flag |= TRM_CONFIG_F_ENABLE;
	}

	return 0;
}

int mtk_trm_cfg_setup(char *name, u32 offset, u32 size, u8 enable)
{
	struct trm_hw_config *trm_hw_cfg;
	struct trm_config *trm_cfg;
	int ret = 0;
	u32 i, j;

	for (i = 0; i < __TRM_HARDWARE_MAX; i++) {
		trm_hw_cfg = trm_hw_configs[i];
		if (unlikely(!trm_hw_cfg || !trm_hw_cfg->trm_cfgs))
			continue;

		for (j = 0; j < trm_hw_cfg->cfg_len; j++) {
			trm_cfg = &trm_hw_cfg->trm_cfgs[j];

			if (!strncmp(trm_cfg->name, name, strlen(name))) {
				mutex_lock(&trm_lock);

				ret = __mtk_trm_cfg_setup(trm_cfg,
							  offset,
							  size,
							  enable);

				mutex_unlock(&trm_lock);
			}
		}
	}

	return ret;
}

/* append core dump(via ocd) in bottom of core-x-dtcm file */
static inline void __mtk_trm_save_core_dump(struct trm_config *trm_cfg,
					    void *dst,
					    u32 *frag_len)
{
	*frag_len -= CORE_DUMP_FRAME_LEN;
	memcpy(dst + *frag_len, &cd_frams[trm_cfg->core], CORE_DUMP_FRAME_LEN);
}

static int __mtk_trm_dump(struct trm_hw_config *trm_hw_cfg,
			  struct trm_config *trm_cfg,
			  u64 dump_time,
			  u32 dump_rsn)
{
	struct trm_header trm_hdr;
	u32 total = trm_cfg->size;
	u32 i = 0;
	u32 frag_len;
	u32 ofs;
	void *dst;

	/* reserve core dump frame len if core dump enabled */
	if (trm_cfg_is_core_dump_en(trm_cfg))
		total += CORE_DUMP_FRAME_LEN;

	/* fill in trm inforamtion */
	trm_hdr_init(&trm_hdr, trm_cfg, total, dump_time, dump_rsn);

	while (total > 0) {
		if (total >= RLY_DUMP_SUBBUF_DATA_MAX) {
			frag_len = RLY_DUMP_SUBBUF_DATA_MAX;
			total -= RLY_DUMP_SUBBUF_DATA_MAX;
		} else {
			frag_len = total;
			total = 0;
			trm_hdr.last_frag = true;
		}

		trm_hdr.data_offset = i++ * RLY_DUMP_SUBBUF_DATA_MAX;
		trm_hdr.data_len = frag_len;

		dst = mtk_trm_fs_relay_reserve(frag_len + TRM_HDR_LEN);
		if (IS_ERR(dst))
			return PTR_ERR(dst);

		memcpy(dst, &trm_hdr, TRM_HDR_LEN);
		dst += TRM_HDR_LEN;

		/* TODO: what if core dump is being cut between 2 fragment? */
		if (trm_hdr.last_frag && trm_cfg_is_core_dump_en(trm_cfg))
			__mtk_trm_save_core_dump(trm_cfg, dst, &frag_len);

		ofs = trm_hdr.info.start_addr + trm_hdr.data_offset;

		/* let TRM HW write memory to destination */
		trm_hw_cfg->trm_hw_dump(dst, ofs, frag_len);

		mtk_trm_fs_relay_flush();
	}

	return 0;
}

static void trm_cpu_utilization_ret_handler(void *priv,
					    struct mailbox_msg *msg)
{
	u32 *cpu_utilization = priv;

	/*
	 * msg1: ticks of idle task
	 * msg2: ticks of this statistic period
	 */
	if (msg->msg2 != 0)
		*cpu_utilization = (msg->msg2 - msg->msg1) * 100U / msg->msg2;
}

int mtk_trm_cpu_utilization(enum core_id core, u32 *cpu_utilization)
{
	struct mailbox_dev *send_mdev;
	struct mailbox_msg msg;
	int ret;

	if (core > CORE_MGMT || !cpu_utilization)
		return -EINVAL;

	if (!mtk_tops_mcu_alive()) {
		TRM_ERR("mcu not alive\n");
		return -EAGAIN;
	}

	memset(&msg, 0, sizeof(struct mailbox_msg));
	msg.msg1 = TRM_CMD_TYPE_CPU_UTILIZATION;

	*cpu_utilization = 0;

	if (core == CORE_MGMT)
		send_mdev = &trm.mgmt_send_mdev;
	else
		send_mdev = &trm.offload_send_mdev[core];

	ret = mbox_send_msg(send_mdev,
			    &msg,
			    cpu_utilization,
			    trm_cpu_utilization_ret_handler);
	if (ret) {
		TRM_ERR("send CPU_UTILIZATION cmd failed(%d)\n", ret);
		return ret;
	}

	return 0;
}

int mtk_trm_dump(u32 rsn)
{
	u64 time = ktime_to_ns(ktime_get_real()) / 1000000000;
	struct trm_hw_config *trm_hw_cfg;
	struct trm_config *trm_cfg;
	int ret = 0;
	u32 i, j;

	if (!mtk_trm_fs_is_init())
		return -EINVAL;

	mutex_lock(&trm_lock);

	mtk_trm_mcu_core_dump();

	for (i = 0; i < __TRM_HARDWARE_MAX; i++) {
		trm_hw_cfg = trm_hw_configs[i];
		if (unlikely(!trm_hw_cfg || !trm_hw_cfg->trm_hw_dump))
			continue;

		for (j = 0; j < trm_hw_cfg->cfg_len; j++) {
			trm_cfg = &trm_hw_cfg->trm_cfgs[j];
			if (unlikely(!trm_cfg || !trm_cfg_is_en(trm_cfg)))
				continue;

			if (unlikely(trm_cfg_sanity_check(trm_cfg))) {
				TRM_ERR("trm %s: sanity check fail\n", trm_cfg->name);
				ret = -EINVAL;
				goto out;
			}

			ret = __mtk_trm_dump(trm_hw_cfg, trm_cfg, time, rsn);
			if (ret) {
				TRM_ERR("trm %s: trm dump fail: %d\n",
					trm_cfg->name, ret);
				goto out;
			}
		}
	}

	TRM_NOTICE("TOPS runtime monitor dump\n");

out:
	mutex_unlock(&trm_lock);

	return ret;
}

static int mtk_tops_trm_register_mbox(void)
{
	int ret;
	int i;

	ret = register_mbox_dev(MBOX_SEND, &trm.mgmt_send_mdev);
	if (ret) {
		TRM_ERR("register trm mgmt mbox send failed: %d\n", ret);
		return ret;
	}

	for (i = 0; i < CORE_OFFLOAD_NUM; i++) {
		ret = register_mbox_dev(MBOX_SEND, &trm.offload_send_mdev[i]);
		if (ret) {
			TRM_ERR("register trm offload %d mbox send failed: %d\n",
				i, ret);
			goto err_unregister_offload_mbox;
		}
	}

	return ret;

err_unregister_offload_mbox:
	for (i -= 1; i >= 0; i--)
		unregister_mbox_dev(MBOX_SEND, &trm.offload_send_mdev[i]);

	unregister_mbox_dev(MBOX_SEND, &trm.mgmt_send_mdev);

	return ret;
}

static void mtk_tops_trm_unregister_mbox(void)
{
	int i;

	unregister_mbox_dev(MBOX_SEND, &trm.mgmt_send_mdev);

	for (i = 0; i < CORE_OFFLOAD_NUM; i++)
		unregister_mbox_dev(MBOX_SEND, &trm.offload_send_mdev[i]);
}

int __init mtk_tops_trm_init(void)
{
	int ret;

	mutex_init(&trm_lock);

	ret = mtk_tops_trm_register_mbox();
	if (ret)
		return ret;

	return mtk_tops_trm_mcu_init();
}

void __exit mtk_tops_trm_exit(void)
{
	mtk_tops_trm_unregister_mbox();

	mtk_tops_trm_mcu_exit();
}

int mtk_trm_hw_config_register(enum trm_hardware trm_hw,
			       struct trm_hw_config *trm_hw_cfg)
{
	if (unlikely(trm_hw >= __TRM_HARDWARE_MAX || !trm_hw_cfg))
		return -ENODEV;

	if (unlikely(!trm_hw_cfg->cfg_len || !trm_hw_cfg->trm_hw_dump))
		return -EINVAL;

	if (trm_hw_configs[trm_hw])
		return -EBUSY;

	trm_hw_configs[trm_hw] = trm_hw_cfg;

	return 0;
}

void mtk_trm_hw_config_unregister(enum trm_hardware trm_hw,
				  struct trm_hw_config *trm_hw_cfg)
{
	if (unlikely(trm_hw >= __TRM_HARDWARE_MAX || !trm_hw_cfg))
		return;

	if (trm_hw_configs[trm_hw] != trm_hw_cfg)
		return;

	trm_hw_configs[trm_hw] = NULL;
}
