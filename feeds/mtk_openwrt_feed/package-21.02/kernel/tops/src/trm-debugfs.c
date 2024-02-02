// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (c) 2023 MediaTek Inc. All Rights Reserved.
 *
 * Author: Alvin Kuo <alvin.kuog@mediatek.com>
 */

#include <linux/debugfs.h>
#include <linux/uaccess.h>

#include "tops/debugfs.h"
#include "tops/internal.h"
#include "tops/tops.h"
#include "tops/trm-debugfs.h"
#include "tops/trm.h"

struct dentry *trm_debugfs_root;

static int cpu_utilization_debug_read(struct seq_file *s, void *private)
{
	u32 cpu_utilization;
	enum core_id core;
	int ret;

	seq_puts(s, "CPU Utilization:\n");
	for (core = CORE_OFFLOAD_0; core <= CORE_MGMT; core++) {
		ret = mtk_trm_cpu_utilization(core, &cpu_utilization);
		if (ret) {
			if (core <= CORE_OFFLOAD_3)
				TOPS_ERR("fetch Core%d cpu utilization failed(%d)\n", core, ret);
			else
				TOPS_ERR("fetch CoreM cpu utilization failed(%d)\n", ret);

			return ret;
		}

		if (core <= CORE_OFFLOAD_3)
			seq_printf(s, "Core%d\t\t%u%%\n", core, cpu_utilization);
		else
			seq_printf(s, "CoreM\t\t%u%%\n", cpu_utilization);
	}

	return 0;
}

static int cpu_utilization_debug_open(struct inode *inode, struct file *file)
{
	return single_open(file, cpu_utilization_debug_read, file->private_data);
}

static ssize_t cpu_utilization_debug_write(struct file *file,
					   const char __user *buffer,
					   size_t count, loff_t *data)
{
	return count;
}

static const struct file_operations cpu_utilization_debug_ops = {
	.open = cpu_utilization_debug_open,
	.read = seq_read,
	.llseek = seq_lseek,
	.write = cpu_utilization_debug_write,
	.release = single_release,
};

int mtk_trm_debugfs_init(void)
{
	if (!tops_debugfs_root)
		return -ENOENT;

	if (!trm_debugfs_root) {
		trm_debugfs_root = debugfs_create_dir("trm", tops_debugfs_root);
		if (IS_ERR(trm_debugfs_root)) {
			TOPS_ERR("create trm debugfs root directory failed\n");
			return PTR_ERR(trm_debugfs_root);
		}
	}

	debugfs_create_file("cpu-utilization", 0644, trm_debugfs_root, NULL,
			    &cpu_utilization_debug_ops);

	return 0;
}

void mtk_trm_debugfs_deinit(void)
{
	debugfs_remove_recursive(trm_debugfs_root);
}
