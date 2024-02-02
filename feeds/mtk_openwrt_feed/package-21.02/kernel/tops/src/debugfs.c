// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (c) 2023 Mediatek Inc. All Rights Reserved.
 *
 * Author: Ren-Ting Wang <ren-ting.wang@mediatek.com>
 */

#include <linux/debugfs.h>
#include <linux/inet.h>
#include <linux/uaccess.h>

#include "tops/debugfs.h"
#include "tops/firmware.h"
#include "tops/internal.h"
#include "tops/mcu.h"
#include "tops/trm.h"
#include "tops/tunnel.h"
#include "tops/wdt.h"

static const char *tops_role_name[__TOPS_ROLE_TYPE_MAX] = {
	[TOPS_ROLE_TYPE_MGMT] = "tops-mgmt",
	[TOPS_ROLE_TYPE_CLUSTER] = "tops-offload",
};

static struct dentry *tops_fw_debugfs_root;

static int tops_fw_info_show(struct seq_file *s, void *private)
{
	enum tops_role_type rtype;
	struct tm tm = {0};
	const char *value;
	const char *prop;
	u32 nattr;
	u32 i;

	for (rtype = TOPS_ROLE_TYPE_MGMT; rtype < __TOPS_ROLE_TYPE_MAX; rtype++) {
		mtk_tops_fw_get_built_date(rtype, &tm);

		seq_printf(s, "%s FW information:\n", tops_role_name[rtype]);
		seq_printf(s, "Git revision:\t%llx\n",
			   mtk_tops_fw_get_git_commit_id(rtype));
		seq_printf(s, "Build date:\t%04ld/%02d/%02d %02d:%02d:%02d\n",
			   tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday,
			   tm.tm_hour, tm.tm_min, tm.tm_sec);

		nattr = mtk_tops_fw_attr_get_num(rtype);

		for (i = 0; i < nattr; i++) {
			prop = mtk_tops_fw_attr_get_property(rtype, i);
			if (!prop)
				continue;

			value = mtk_tops_fw_attr_get_value(rtype, prop);

			seq_printf(s, "%s:\t%s\n", prop, value);
		}
		seq_puts(s, "\n");
	}

	return 0;
}

static int tops_tnl_show(struct seq_file *s, void *private)
{
	struct tops_tnl_info *tnl_info;
	struct tops_tnl_params *tnl_params;
	u32 i;

	for (i = 0; i < CONFIG_TOPS_TNL_NUM; i++) {
		tnl_info = mtk_tops_tnl_info_get_by_idx(i);
		if (IS_ERR(tnl_info))
			/* tunnel not enabled */
			continue;

		tnl_params = &tnl_info->tnl_params;
		if (!tnl_info->tnl_type || !tnl_info->tnl_type->tnl_param_dump)
			continue;

		seq_printf(s, "Tunnel Index: %02u\n", i);

		mtk_tops_mac_param_dump(s, &tnl_params->params);

		mtk_tops_network_param_dump(s, &tnl_params->params);

		mtk_tops_transport_param_dump(s, &tnl_params->params);

		tnl_info->tnl_type->tnl_param_dump(s, &tnl_params->params);

		seq_printf(s, "\tTOPS Entry: %02u CLS Entry: %02u CDRT: %02u Flag: 0x%x\n",
			   tnl_params->tops_entry_proto,
			   tnl_params->cls_entry,
			   tnl_params->cdrt,
			   tnl_params->flag);
	}

	return 0;
}

static int tops_tnl_open(struct inode *inode, struct file *file)
{
	return single_open(file, tops_tnl_show, file->private_data);
}

static int tops_tnl_add_new_tnl(const char *buf)
{
	struct tops_tnl_params tnl_params;
	struct tops_params *params;
	struct tops_tnl_info *tnl_info;
	struct tops_tnl_type *tnl_type;
	char proto[DEBUG_PROTO_LEN];
	int ofs = 0;
	int ret = 0;

	memset(&tnl_params, 0, sizeof(struct tops_tnl_params));
	memset(proto, 0, sizeof(proto));

	params = &tnl_params.params;

	ret = mtk_tops_debug_param_setup(buf, &ofs, params);
	if (ret)
		return ret;

	ret = mtk_tops_debug_param_proto_peek(buf, ofs, proto);
	if (ret < 0)
		return ret;

	ofs += ret;

	tnl_type = mtk_tops_tnl_type_get_by_name(proto);
	if (!tnl_type || !tnl_type->tnl_debug_param_setup)
		return -ENODEV;

	ret = tnl_type->tnl_debug_param_setup(buf, &ofs, params);
	if (ret < 0)
		return ret;

	tnl_params.flag |= TNL_DECAP_ENABLE;
	tnl_params.flag |= TNL_ENCAP_ENABLE;
	tnl_params.tops_entry_proto = tnl_type->tnl_proto_type;

	tnl_info = mtk_tops_tnl_info_alloc(tnl_type);
	if (IS_ERR(tnl_info))
		return -ENOMEM;

	tnl_info->flag |= TNL_INFO_DEBUG;
	memcpy(&tnl_info->cache, &tnl_params, sizeof(struct tops_tnl_params));

	mtk_tops_tnl_info_hash(tnl_info);

	mtk_tops_tnl_info_submit(tnl_info);

	return 0;
}

static ssize_t tops_tnl_write(struct file *file, const char __user *buffer,
			      size_t count, loff_t *data)
{
	char cmd[21] = {0};
	char buf[512];
	int nchar = 0;
	int ret = 0;

	if (count > sizeof(buf))
		return -ENOMEM;

	if (copy_from_user(buf, buffer, count))
		return -EFAULT;

	buf[count] = '\0';

	ret = sscanf(buf, "%20s %n", cmd, &nchar);

	if (ret != 1)
		return -EPERM;

	if (!strcmp(cmd, "NEW_TNL")) {
		ret = tops_tnl_add_new_tnl(buf + nchar);
		if (ret)
			return ret;
	} else {
		return -EINVAL;
	}

	return count;
}

DEFINE_SHOW_ATTRIBUTE(tops_fw_info);

static const struct file_operations tops_tnl_fops = {
	.open = tops_tnl_open,
	.read = seq_read,
	.write = tops_tnl_write,
	.llseek = seq_lseek,
	.release = single_release,
};

int mtk_tops_debugfs_init(struct platform_device *pdev)
{
	tops_fw_debugfs_root = debugfs_create_dir("fw", tops_debugfs_root);

	debugfs_create_file("firmware_info", 0400, tops_fw_debugfs_root, NULL,
			    &tops_fw_info_fops);

	debugfs_create_file("tunnel", 0444, tops_fw_debugfs_root, NULL,
			    &tops_tnl_fops);

	return 0;
}

void mtk_tops_debugfs_deinit(struct platform_device *pdev)
{
	debugfs_remove_recursive(tops_fw_debugfs_root);
}
