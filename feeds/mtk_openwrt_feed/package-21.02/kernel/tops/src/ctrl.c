// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (c) 2023 MediaTek Inc. All Rights Reserved.
 *
 * Author: Ren-Ting Wang <ren-ting.wang@mediatek.com>
 */

#include <linux/device.h>

#include "tops/firmware.h"
#include "tops/internal.h"
#include "tops/mcu.h"
#include "tops/trm.h"
#include "tops/tunnel.h"
#include "tops/wdt.h"

static int mtk_tops_trm_fetch_setting(const char *buf,
				      int *ofs,
				      char *name,
				      u32 *offset,
				      u32 *size,
				      u8 *enable)
{
	int nchar = 0;
	int ret = 0;

	ret = sscanf(buf + *ofs, "%31s %x %x %hhx %n",
		name, offset, size, enable, &nchar);
	if (ret != 4)
		return -EPERM;

	*ofs += nchar;

	return nchar;
}

static ssize_t mtk_tops_trm_store(struct device *dev,
				  struct device_attribute *attr,
				  const char *buf,
				  size_t count)
{
	char name[TRM_CONFIG_NAME_MAX_LEN] = { 0 };
	char cmd[21] = { 0 };
	int nchar = 0;
	int ret = 0;
	u32 offset;
	u8 enable;
	u32 size;

	ret = sscanf(buf, "%20s %n", cmd, &nchar);
	if (ret != 1)
		return -EPERM;

	if (!strcmp(cmd, "trm_dump")) {
		ret = mtk_trm_dump(TRM_RSN_NULL);
		if (ret)
			return ret;
	} else if (!strcmp(cmd, "trm_cfg_setup")) {
		ret = mtk_tops_trm_fetch_setting(buf, &nchar,
			name, &offset, &size, &enable);
		if (ret < 0)
			return ret;

		ret = mtk_trm_cfg_setup(name, offset, size, enable);
		if (ret)
			return ret;
	}

	return count;
}

static ssize_t mtk_tops_wdt_store(struct device *dev,
				  struct device_attribute *attr,
				  const char *buf,
				  size_t count)
{
	char cmd[21] = {0};
	u32 core = 0;
	u32 i;
	int ret;

	ret = sscanf(buf, "%20s %x", cmd, &core);
	if (ret != 2)
		return -EPERM;

	core &= CORE_TOPS_MASK;
	if (!strcmp(cmd, "WDT_TO")) {
		for (i = 0; i < CORE_TOPS_NUM; i++) {
			if (core & 0x1)
				mtk_tops_wdt_trigger_timeout(i);
			core >>= 1;
		}
	} else {
		return -EINVAL;
	}

	return count;
}

static DEVICE_ATTR_WO(mtk_tops_trm);
static DEVICE_ATTR_WO(mtk_tops_wdt);

static struct attribute *mtk_tops_attributes[] = {
	&dev_attr_mtk_tops_trm.attr,
	&dev_attr_mtk_tops_wdt.attr,
	NULL,
};

static const struct attribute_group mtk_tops_attr_group = {
	.name = "mtk_tops",
	.attrs = mtk_tops_attributes,
};

int mtk_tops_ctrl_init(struct platform_device *pdev)
{
	int ret = 0;

	ret = sysfs_create_group(&pdev->dev.kobj, &mtk_tops_attr_group);
	if (ret) {
		TOPS_ERR("create sysfs failed\n");
		return ret;
	}

	return ret;
}

void mtk_tops_ctrl_deinit(struct platform_device *pdev)
{
	sysfs_remove_group(&pdev->dev.kobj, &mtk_tops_attr_group);
}
