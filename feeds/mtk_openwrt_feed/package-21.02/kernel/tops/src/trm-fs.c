// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (c) 2023 MediaTek Inc. All Rights Reserved.
 *
 * Author: Alvin Kuo <alvin.kuog@mediatek.com>
 *         Ren-Ting Wang <ren-ting.wang@mediatek.com>
 */

#include <linux/debugfs.h>
#include <linux/delay.h>
#include <linux/err.h>
#include <linux/relay.h>

#include "tops/trm-debugfs.h"
#include "tops/trm-fs.h"
#include "tops/trm-mcu.h"
#include "tops/trm.h"

#define RLY_RETRY_NUM				3

static struct rchan *relay;
static bool trm_fs_is_init;

bool mtk_trm_fs_is_init(void)
{
	return trm_fs_is_init;
}

void *mtk_trm_fs_relay_reserve(u32 size)
{
	u32 rty = 0;
	void *dst;

	while (rty < RLY_RETRY_NUM) {
		dst = relay_reserve(relay, size);
		if (likely(dst))
			return dst;

		rty++;
		msleep(100);
	}

	return ERR_PTR(-ENOMEM);
}

void mtk_trm_fs_relay_flush(void)
{
	relay_flush(relay);
}

static struct dentry *trm_fs_create_buf_file_cb(const char *filename,
						struct dentry *parent,
						umode_t mode,
						struct rchan_buf *buf,
						int *is_global)
{
	struct dentry *debugfs_file;

	debugfs_file = debugfs_create_file("dump-data", mode,
					   parent, buf,
					   &relay_file_operations);

	*is_global = 1;

	return debugfs_file;
}

static int trm_fs_remove_buf_file_cb(struct dentry *debugfs_file)
{
	debugfs_remove(debugfs_file);

	return 0;
}

int mtk_trm_fs_init(void)
{
	static struct rchan_callbacks relay_cb = {
		.create_buf_file = trm_fs_create_buf_file_cb,
		.remove_buf_file = trm_fs_remove_buf_file_cb,
	};
	int ret = 0;

	if (!trm_debugfs_root)
		return -ENOENT;

	if (!relay) {
		relay = relay_open("dump-data", trm_debugfs_root,
				   RLY_DUMP_SUBBUF_SZ,
				   RLY_DUMP_SUBBUF_NUM,
				   &relay_cb, NULL);
		if (!relay)
			return -EINVAL;
	}

	relay_reset(relay);

	trm_fs_is_init = true;

	return ret;
}

void mtk_trm_fs_deinit(void)
{
	trm_fs_is_init = false;

	relay_close(relay);
}
