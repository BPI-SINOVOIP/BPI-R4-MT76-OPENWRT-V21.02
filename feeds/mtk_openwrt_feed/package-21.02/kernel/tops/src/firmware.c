// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (c) 2023 MediaTek Inc. All Rights Reserved.
 *
 * Author: Ren-Ting Wang <ren-ting.wang@mediatek.com>
 */

#include <linux/arm-smccc.h>
#include <linux/device.h>
#include <linux/dma-mapping.h>
#include <linux/err.h>
#include <linux/firmware.h>
#include <linux/io.h>
#include <linux/of.h>
#include <linux/of_device.h>
#include <linux/of_platform.h>
#include <linux/platform_device.h>

#include "tops/firmware.h"
#include "tops/internal.h"
#include "tops/mcu.h"

#define TOPS_MGMT_IMG				"mediatek/tops-mgmt.img"
#define TOPS_OFFLOAD_IMG			"mediatek/tops-offload.img"

#define MTK_SIP_TOPS_LOAD			0xC2000560

#define PAYLOAD_ALIGNMENT			(32)

#define TOPS_ITCM_BOOT_ADDR			(0x40020000)
#define TOPS_DTCM_BOOT_ADDR			(0x40000000)
#define TOPS_L2SRAM_BOOT_ADDR			(0x4E100000)
#define TOPS_DEFAULT_BOOT_ADDR			(TOPS_ITCM_BOOT_ADDR)

#define TOPS_FW_MAGIC				(0x53504f54)
#define TOPS_FW_HDR_VER				(1)
#define FW_HLEN					(sizeof(struct tops_fw_header))
#define FW_DATA(fw)				((fw)->data)
#define FW_ROLE(fw)				((fw)->hdr.role)
#define FW_PART_HLEN(fw)			((fw)->hdr.part_hdr_len)
#define FW_PART_HDR(fw, idx)			(FW_DATA(fw) + FW_PART_HLEN(fw) * (idx))
#define FW_NUM_PARTS(fw)			((fw)->hdr.num_parts)
#define FW_GIT_ID(fw)				((fw)->hdr.git_commit_id)
#define FW_BUILD_TS(fw)				((fw)->hdr.build_ts)

#define FW_PART_LOAD_ADDR_OVERRIDE		(BIT(0))
#define FW_PART_BOOT_OVERRIDE			(BIT(1))

enum tops_part_type {
	TOPS_PART_TYPE_IRAM0,
	TOPS_PART_TYPE_DRAM0,
	TOPS_PART_TYPE_L2SRAM,
	TOPS_PART_TYPE_METADATA,

	__TOPS_PART_TYPE_MAX,
};

enum tops_plat_id {
	TOPS_PLAT_MT7988,

	__TOPS_PLAT_MAX,
};

struct tops_boot_config {
	enum tops_part_type boot_type;
	u32 boot_addr;
};

struct tops_fw_plat {
	enum tops_plat_id plat;
	u16 id;
};

struct tops_fw_header {
	u32 magic;
	u8 hdr_ver;
	u8 api_ver;
	u16 hdr_len;
	u32 hdr_crc;
	u16 plat_id;
	u16 flags;
	u64 git_commit_id;
	u32 build_ts;
	u8 role;
	u8 signing_type;
	u8 num_parts;
	u8 part_hdr_len;
	u32 part_hdr_crc;
	u32 payload_len;
	u32 payload_crc;
	u32 sign_body_len;
} __aligned(4);

struct tops_fw_part_hdr {
	u8 part_type;
	u8 resv;
	u16 flags;
	u32 size;
	u32 value[2];
} __aligned(4);

struct tops_fw_part {
	const struct tops_fw_part_hdr *hdr[__TOPS_PART_TYPE_MAX];
	const void *payload[__TOPS_PART_TYPE_MAX];
};

struct tops_fw {
	struct tops_fw_header hdr;
	u8 data[0];
};

struct tops_fw_attr {
	char *property;
	char *value;
};

struct tops_fw_info {
	struct tops_fw_attr *attrs;
	u64 git_commit_id;
	u32 build_ts;
	u32 nattr;
};

struct npu {
	void __iomem *base;
	struct device *dev;
	struct tops_fw_info fw_info[__TOPS_ROLE_TYPE_MAX];
};

#if !defined(CONFIG_MTK_TOPS_SECURE_FW)
static struct tops_boot_config tops_boot_configs[] = {
	{ .boot_type = TOPS_PART_TYPE_IRAM0, .boot_addr = TOPS_ITCM_BOOT_ADDR },
	{ .boot_type = TOPS_PART_TYPE_DRAM0, .boot_addr = TOPS_DTCM_BOOT_ADDR },
	{ .boot_type = TOPS_PART_TYPE_L2SRAM, .boot_addr = TOPS_L2SRAM_BOOT_ADDR},
};

static struct tops_fw_plat tops_plats[] = {
	{ .plat = TOPS_PLAT_MT7988, .id = 0x7988 },
};
#endif /* !defined(CONFIG_MTK_TOPS_SECURE_FW) */

static struct npu npu;

static inline void npu_write(u32 reg, u32 val)
{
	writel(val, npu.base + reg);
}

static inline void npu_set(u32 reg, u32 mask)
{
	setbits(npu.base + reg, mask);
}

static inline void npu_clr(u32 reg, u32 mask)
{
	clrbits(npu.base + reg, mask);
}

static inline void npu_rmw(u32 reg, u32 mask, u32 val)
{
	clrsetbits(npu.base + reg, mask, val);
}

static inline u32 npu_read(u32 reg)
{
	return readl(npu.base + reg);
}

u64 mtk_tops_fw_get_git_commit_id(enum tops_role_type rtype)
{
	if (rtype >= __TOPS_ROLE_TYPE_MAX)
		return 0;

	return npu.fw_info[rtype].git_commit_id;
}

void mtk_tops_fw_get_built_date(enum tops_role_type rtype, struct tm *tm)
{
	if (rtype >= __TOPS_ROLE_TYPE_MAX)
		return;

	time64_to_tm(npu.fw_info[rtype].build_ts, 0, tm);
}

u32 mtk_tops_fw_attr_get_num(enum tops_role_type rtype)
{
	if (rtype >= __TOPS_ROLE_TYPE_MAX)
		return 0;

	return npu.fw_info[rtype].nattr;
}

const char *mtk_tops_fw_attr_get_property(enum tops_role_type rtype, u32 idx)
{
	if (rtype >= __TOPS_ROLE_TYPE_MAX || idx >= npu.fw_info[rtype].nattr)
		return NULL;

	return npu.fw_info[rtype].attrs[idx].property;
}

const char *mtk_tops_fw_attr_get_value(enum tops_role_type rtype,
				       const char *property)
{
	u32 plen = strlen(property);
	u32 nattr;
	u32 i;

	if (rtype >= __TOPS_ROLE_TYPE_MAX)
		return NULL;

	nattr = npu.fw_info[rtype].nattr;
	for (i = 0; i < nattr; i++) {
		if (!strncmp(property, npu.fw_info[rtype].attrs[i].property, plen))
			return npu.fw_info[rtype].attrs[i].value;
	}

	return NULL;
}

static bool mtk_tops_fw_support_plat(const struct tops_fw_header *fw_hdr)
{
	u32 i;

	for (i = 0; i < __TOPS_PLAT_MAX; i++)
		if (le16_to_cpu(fw_hdr->plat_id) == tops_plats[i].plat)
			return true;

	return false;
}

static int mtk_tops_fw_valid_hdr(const struct tops_fw *tfw, uint32_t fw_size)
{
	const struct tops_fw_header *fw_hdr = &tfw->hdr;
	u32 total_size;
	u32 ph_len;

	if (fw_size < FW_HLEN) {
		TOPS_ERR("requested fw hlen is less than minimal TOPS fw hlen\n");
		return -EINVAL;
	}

	if (le32_to_cpu(fw_hdr->magic) != TOPS_FW_MAGIC) {
		TOPS_ERR("not a tops fw!\n");
		return -EBADF;
	}

	if (le16_to_cpu(fw_hdr->hdr_ver) != TOPS_FW_HDR_VER) {
		TOPS_ERR("unsupport tops fw header: %u\n",
			      le16_to_cpu(fw_hdr->hdr_ver));
		return -EBADF;
	}

	if (le16_to_cpu(fw_hdr->hdr_len) != sizeof(struct tops_fw_header)) {
		TOPS_ERR("tops fw header length mismatch\n");
		return -EBADF;
	}

	if (fw_hdr->part_hdr_len != sizeof(struct tops_fw_part_hdr)) {
		TOPS_ERR("unsupport tops fw header len: %u\n",
			      fw_hdr->part_hdr_len);
		return -EBADF;
	}

	if (!mtk_tops_fw_support_plat(fw_hdr)) {
		TOPS_ERR("unsupport tops platform fw: %u\n",
			le16_to_cpu(fw_hdr->plat_id));
		return -EBADF;
	}

	if (fw_hdr->role >= __TOPS_ROLE_TYPE_MAX) {
		TOPS_ERR("unsupport tops role: %u\n", fw_hdr->role);
		return -EBADF;
	}

	if (fw_hdr->num_parts > __TOPS_PART_TYPE_MAX) {
		TOPS_ERR("number of parts exceeds tops' support: %u\n",
			fw_hdr->num_parts);
		return -EBADF;
	}

	ph_len = fw_hdr->part_hdr_len * fw_hdr->num_parts;
	total_size = fw_hdr->hdr_len + ph_len + fw_hdr->payload_len;

	if (total_size > fw_size) {
		TOPS_ERR("firmware incomplete\n");
		return -EBADF;
	}

	return 0;
}

static int mtk_tops_fw_init_part_data(const struct tops_fw *fw,
				      struct tops_fw_part *part)
{
	const struct tops_fw_part_hdr *phdr;
	uint32_t part_off = FW_PART_HLEN(fw) * FW_NUM_PARTS(fw);
	int ret = 0;
	u8 i;

	for (i = 0; i < FW_NUM_PARTS(fw); i++) {
		/* get part hdr */
		phdr = (struct tops_fw_part_hdr *)FW_PART_HDR(fw, i);
		if (phdr->part_type >= __TOPS_PART_TYPE_MAX) {
			TOPS_ERR("unknown part type: %u\n", phdr->part_type);
			return -EBADF;
		}

		part->hdr[phdr->part_type] = phdr;

		/* get part payload */
		part->payload[phdr->part_type] = FW_DATA(fw) + part_off;

		part_off += ALIGN(le32_to_cpu(phdr->size), PAYLOAD_ALIGNMENT);
	}

	return ret;
}

#if defined(CONFIG_MTK_TOPS_SECURE_FW)
static int mtk_tops_fw_smc(u32 smc_id,
			   u64 x1,
			   u64 x2,
			   u64 x3,
			   u64 x4,
			   struct arm_smccc_res *res)
{
	if (!res)
		return -EINVAL;

	arm_smccc_smc(smc_id, x1, x2, x3, x4, 0, 0, 0, res);

	return res->a0;
}

static int __mtk_tops_fw_bring_up_core(const void *fw, u32 fw_size)
{
	struct arm_smccc_res res = {0};
	dma_addr_t fw_paddr;
	void *fw_vaddr;
	u32 order = 0;
	u32 psize;
	int ret;

	psize = (fw_size / PAGE_SIZE) + 1;
	while ((1 << order) < psize)
		order++;

	fw_vaddr = __get_free_pages(GFP_KERNEL, order);
	if (!fw_vaddr)
		return -ENOMEM;

	memcpy(fw_vaddr, fw, fw_size);

	fw_paddr = dma_map_single(tops_dev, fw_vaddr, PAGE_SIZE, DMA_TO_DEVICE);
	if (dma_mapping_error(tops_dev, fw_paddr)) {
		ret = -ENOMEM;
		goto dma_map_err;
	}
	/* make sure firmware data is written and mapped to buffer */
	wmb();

	ret = mtk_tops_fw_smc(MTK_SIP_TOPS_LOAD, 0, fw_paddr, fw_size, 0, &res);
	if (ret)
		TOPS_ERR("tops secure firmware load failed: %d\n", ret);

	dma_unmap_single(tops_dev, fw_paddr, fw_size, DMA_TO_DEVICE);

dma_map_err:
	free_pages(fw_vaddr, order);

	return ret;
}
#else /* !defined(CONFIG_MTK_TOPS_SECURE_FW) */
static u32 mtk_tops_fw_get_boot_addr(struct tops_fw_part *part)
{
	const struct tops_fw_part_hdr *hdr = NULL;
	u32 boot_addr = TOPS_DEFAULT_BOOT_ADDR;
	u32 i;

	for (i = TOPS_PART_TYPE_IRAM0; i < TOPS_PART_TYPE_METADATA; i++) {
		hdr = part->hdr[i];

		if (le16_to_cpu(hdr->flags) & FW_PART_BOOT_OVERRIDE) {
			boot_addr = tops_boot_configs[i].boot_addr;

			if (le16_to_cpu(hdr->flags) & FW_PART_LOAD_ADDR_OVERRIDE)
				boot_addr = le32_to_cpu(hdr->value[0]);
		}
	}

	return boot_addr;
}

static void __mtk_tops_fw_load_data(const struct tops_fw_part_hdr *phdr,
				     const void *payload,
				     u32 addr)
{
	int ofs;

	for (ofs = 0; ofs < le32_to_cpu(phdr->size); ofs += 0x4)
		npu_write(addr + ofs, *(u32 *)(payload + ofs));
}

static int mtk_tops_fw_load_core_mgmt(struct tops_fw_part *part)
{
	if (!part)
		return -ENODEV;

	__mtk_tops_fw_load_data(part->hdr[TOPS_PART_TYPE_IRAM0],
				part->payload[TOPS_PART_TYPE_IRAM0],
				TOP_CORE_M_ITCM);

	__mtk_tops_fw_load_data(part->hdr[TOPS_PART_TYPE_DRAM0],
				part->payload[TOPS_PART_TYPE_DRAM0],
				TOP_CORE_M_DTCM);

	__mtk_tops_fw_load_data(part->hdr[TOPS_PART_TYPE_L2SRAM],
				part->payload[TOPS_PART_TYPE_L2SRAM],
				TOP_L2SRAM);

	return 0;
}

static int mtk_tops_fw_bring_up_core_mgmt(struct tops_fw_part *part)
{
	int ret = 0;

	/* setup boot address */
	npu_write(TOP_CORE_M_RESET_VECTOR, mtk_tops_fw_get_boot_addr(part));

	/* de-assert core reset */
	npu_write(TOP_CORE_NPU_SW_RST, 0);

	/* enable run stall */
	npu_write(TOP_CORE_NPU_CTRL, 0x1);

	/* enable ext bootup sel */
	npu_write(TOP_CORE_M_STAT_VECTOR_SEL, 0x1);

	/* toggle reset */
	npu_write(TOP_CORE_NPU_SW_RST, 0x1);
	npu_write(TOP_CORE_NPU_SW_RST, 0x0);

	/* load firmware */
	ret = mtk_tops_fw_load_core_mgmt(part);
	if (ret) {
		TOPS_ERR("load core mgmt fw failed: %d\n", ret);
		return ret;
	}

	/* release run stall */
	npu_write(TOP_CORE_NPU_CTRL, 0);

	return ret;
}

static int mtk_tops_fw_load_core_offload(struct tops_fw_part *part,
					 enum core_id core)
{
	if (!part)
		return -ENODEV;

	if (core >= CORE_OFFLOAD_NUM)
		return -EPERM;

	__mtk_tops_fw_load_data(part->hdr[TOPS_PART_TYPE_IRAM0],
				part->payload[TOPS_PART_TYPE_IRAM0],
				CLUST_CORE_X_ITCM(core));

	__mtk_tops_fw_load_data(part->hdr[TOPS_PART_TYPE_DRAM0],
				part->payload[TOPS_PART_TYPE_DRAM0],
				CLUST_CORE_X_DTCM(core));

	return 0;
}

static int __mtk_tops_fw_bring_up_core_offload(struct tops_fw_part *part,
					       enum core_id core)
{
	int ret = 0;

	/* setup boot address */
	npu_write(CLUST_CORE_X_RESET_VECTOR(core),
		mtk_tops_fw_get_boot_addr(part));

	/* de-assert core reset */
	npu_write(CLUST_CORE_NPU_SW_RST(core), 0);

	/* enable run stall */
	npu_write(CLUST_CORE_NPU_CTRL(core), 0x1);

	/* enable ext bootup sel */
	npu_write(CLUST_CORE_X_STAT_VECTOR_SEL(core), 0x1);

	/* toggle reset */
	npu_write(CLUST_CORE_NPU_SW_RST(core), 0x1);
	npu_write(CLUST_CORE_NPU_SW_RST(core), 0x0);

	/* load firmware */
	ret = mtk_tops_fw_load_core_offload(part, core);
	if (ret) {
		TOPS_ERR("load core offload fw failed: %d\n", ret);
		return ret;
	}

	/* release run stall */
	npu_write(CLUST_CORE_NPU_CTRL(core), 0);

	return ret;
}

static int mtk_tops_fw_bring_up_core_offload(struct tops_fw_part *part)
{
	int ret = 0;
	u32 i = 0;

	__mtk_tops_fw_load_data(part->hdr[TOPS_PART_TYPE_L2SRAM],
				part->payload[TOPS_PART_TYPE_L2SRAM],
				CLUST_L2SRAM);

	for (i = 0; i < CORE_OFFLOAD_NUM; i++) {
		ret = __mtk_tops_fw_bring_up_core_offload(part, i);
		if (ret)
			return ret;
	}

	return ret;
}

static int __mtk_tops_fw_bring_up_core(const struct tops_fw *tfw,
				       struct tops_fw_part *part)
{
	int ret = 0;

	if (!tfw || !part)
		return -EINVAL;

	/* bring up core by role */
	switch (FW_ROLE(tfw)) {
	case TOPS_ROLE_TYPE_MGMT:
		ret = mtk_tops_fw_bring_up_core_mgmt(part);

		break;
	case TOPS_ROLE_TYPE_CLUSTER:
		ret = mtk_tops_fw_bring_up_core_offload(part);

		break;
	default:
		TOPS_ERR("unsupport tops fw role\n");

		return -EBADF;
	}

	return ret;
}
#endif /* defined(CONFIG_MTK_TOPS_SECURE_FW) */

static int mtk_tops_fw_get_info(const struct tops_fw *tfw, struct tops_fw_part *part)
{
	const struct tops_fw_part_hdr *phdr;
	const u8 *payload;
	struct tops_fw_info *fw_info;
	struct tops_fw_attr *attrs;
	u32 kofs, klen, vofs, vlen;
	u32 meta_len;
	u32 ofs = 0;
	u32 nattr;
	int i;

	if (!tfw || !part)
		return -EINVAL;

	if (FW_ROLE(tfw) >= __TOPS_ROLE_TYPE_MAX)
		return -EINVAL;

	phdr = part->hdr[TOPS_PART_TYPE_METADATA];
	payload = part->payload[TOPS_PART_TYPE_METADATA];
	if (!phdr || !payload)
		return 0;

	meta_len = le32_to_cpu(phdr->size);
	if (!meta_len)
		return 0;

	fw_info = &npu.fw_info[FW_ROLE(tfw)];
	fw_info->nattr = nattr = le32_to_cpu(*((u32 *)payload));
	ofs += 0x4;

	fw_info->attrs = devm_kcalloc(tops_dev,
				      nattr * 2,
				      sizeof(char *),
				      GFP_KERNEL);
	if (!fw_info->attrs) {
		fw_info->nattr = 0;
		return -ENOMEM;
	}
	attrs = fw_info->attrs;

	for (i = 0; i < nattr; i++) {
		struct tops_fw_attr *attr = &attrs[i];

		/* get property offset */
		if (ofs + (i * 2) * 0x4 >= meta_len)
			break;
		kofs = le32_to_cpu(*((u32 *)(payload + ofs + (i * 2) * 0x4)));

		/* get value offset */
		if (ofs + (i * 2 + 1) * 0x4 >= meta_len)
			break;
		vofs = le32_to_cpu(*((u32 *)(payload + ofs + (i * 2 + 1) * 0x4)));

		klen = strlen(payload + kofs);
		vlen = strlen(payload + vofs);
		if (!kofs || !vofs || !klen || !vlen) {
			TOPS_ERR("invalid attribute property value pair, kofs: %u, klen: %u, vofs: %u, vlen: %u\n",
				 kofs, klen, vofs, vlen);
			break;
		}

		attr->property = devm_kzalloc(tops_dev,
					 sizeof(char) * klen + 1,
					 GFP_KERNEL);
		if (!attr->property)
			goto err_out;

		attr->value = devm_kzalloc(tops_dev,
					 sizeof(char) * vlen + 1,
					 GFP_KERNEL);
		if (!attr->value) {
			devm_kfree(tops_dev, attr->property);
			goto err_out;
		}

		strncpy(attr->property, payload + kofs, klen);
		strncpy(attr->value, payload + vofs, vlen);
	}

	fw_info->git_commit_id = le64_to_cpu(FW_GIT_ID(tfw));
	fw_info->build_ts = le32_to_cpu(FW_BUILD_TS(tfw));

	return 0;

err_out:
	fw_info->git_commit_id = 0;
	fw_info->build_ts = 0;

	for (i = i - 1; i >= 0; i--) {
		devm_kfree(tops_dev, attrs[i].property);
		devm_kfree(tops_dev, attrs[i].value);
	}

	devm_kfree(tops_dev, attrs);

	return -ENOMEM;
}

static void mtk_tops_fw_put_info(void)
{
	enum tops_role_type rtype;
	struct tops_fw_attr *attrs;
	u32 nattr;
	u32 i;

	for (rtype = TOPS_ROLE_TYPE_MGMT; rtype < __TOPS_ROLE_TYPE_MAX; rtype++) {
		nattr = npu.fw_info[rtype].nattr;
		attrs = npu.fw_info[rtype].attrs;

		npu.fw_info[rtype].git_commit_id = 0;
		npu.fw_info[rtype].build_ts = 0;

		if (!nattr)
			continue;

		for (i = 0; i < nattr; i++) {
			devm_kfree(tops_dev, attrs[i].property);
			devm_kfree(tops_dev, attrs[i].value);
		}

		devm_kfree(tops_dev, attrs);

		npu.fw_info[rtype].nattr = 0;
		npu.fw_info[rtype].attrs = NULL;
	}
}

int mtk_tops_fw_bring_up_core(const char *fw_path)
{
	const struct firmware *fw;
	const struct tops_fw *tfw;
	struct tops_fw_part part;
	struct tm tm = {0};
	int ret;

	ret = request_firmware(&fw, fw_path, tops_dev);
	if (ret) {
		TOPS_ERR("request %s firmware failed\n", fw_path);
		return ret;
	}

	tfw = (const void *)fw->data;

	ret = mtk_tops_fw_valid_hdr(tfw, fw->size);
	if (ret) {
		TOPS_ERR("valid fw: %s image failed: %d\n", fw_path, ret);
		goto err_out;
	}

	ret = mtk_tops_fw_init_part_data(tfw, &part);
	if (ret) {
		TOPS_ERR("init fw part data failed: %d\n", ret);
		goto err_out;
	}

	ret = mtk_tops_fw_get_info(tfw, &part);
	if (ret) {
		TOPS_ERR("meta data initialize failed: %d\n", ret);
		goto err_out;
	}

	ret = __mtk_tops_fw_bring_up_core(tfw, &part);
	if (ret) {
		TOPS_ERR("bring up core %s failed\n", fw_path);
		mtk_tops_fw_put_info();
		goto err_out;
	}

	mtk_tops_fw_get_built_date(FW_ROLE(tfw), &tm);

	TOPS_NOTICE("TOPS Load Firmware: %s\n", fw_path);
	TOPS_NOTICE("\tFirmware version:\t%s\n",
		    mtk_tops_fw_attr_get_value(FW_ROLE(tfw), "version"));
	TOPS_NOTICE("\tGit revision:\t\t%llx\n",
		    mtk_tops_fw_get_git_commit_id(FW_ROLE(tfw)));
	TOPS_NOTICE("\tBuilt date:\t\t%04ld/%02d/%02d %02d:%02d:%02d\n",
		    tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday,
		    tm.tm_hour, tm.tm_min, tm.tm_sec);

err_out:
	release_firmware(fw);

	return ret;
}
#if defined(CONFIG_MTK_TOPS_EVALUATION)
EXPORT_SYMBOL(mtk_tops_fw_bring_up_core);
#endif /* defined(CONFIG_MTK_TOPS_EVALUATION) */

int mtk_tops_fw_bring_up_default_cores(void)
{
	int ret;

	ret = mtk_tops_fw_bring_up_core(TOPS_MGMT_IMG);
	if (ret)
		return ret;

	ret = mtk_tops_fw_bring_up_core(TOPS_OFFLOAD_IMG);

	return ret;
}

#if defined(CONFIG_MTK_TOPS_CORE_DEBUG)
static void mtk_tops_fw_enable_core_debug(void)
{
	u32 i;

	npu_write(TOP_CORE_DBG_CTRL, 0x3F);
	npu_write(CLUST_CORE_DBG_CTRL, 0x1F);

	npu_write(TOP_CORE_OCD_CTRL, 0x1);

	for (i = 0; i < CORE_OFFLOAD_NUM; i++)
		npu_write(CLUST_CORE_OCD_CTRL(i), 0x1);
}
#endif /* defined(CONFIG_MTK_TOPS_CORE_DEBUG) */

void mtk_tops_fw_clean_up(void)
{
	mtk_tops_fw_put_info();
}

int mtk_tops_fw_init(struct platform_device *pdev)
{
	struct resource *res;

	res = platform_get_resource_byname(pdev, IORESOURCE_MEM, "tops-base");
	if (!res)
		return -ENXIO;

	npu.base = devm_ioremap(&pdev->dev, res->start, resource_size(res));
	if (!npu.base)
		return -ENOMEM;

/* TODO: move to somewhere else */
#if defined(CONFIG_MTK_TOPS_CORE_DEBUG)
	mtk_tops_enable_core_debug();
#endif /* defined(CONFIG_MTK_TOPS_CORE_DEBUG) */

	return 0;
}
