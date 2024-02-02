/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * Copyright (c) 2023 MediaTek Inc. All Rights Reserved.
 *
 * Author: Alvin Kuo <alvin.kuog@mediatek.com>
 *         Ren-Ting Wang <ren-ting.wang@mediatek.com>
 */

#ifndef _TOPS_TRM_MCU_H_
#define _TOPS_TRM_MCU_H_

#include "tops/tops.h"

#define XCHAL_NUM_AREG                  (32)
#define CORE_DUMP_FRAM_MAGIC            (0x00BE00BE)

#define CORE_DUMP_FRAME_LEN		(sizeof(struct core_dump_fram))

/* need to sync with core_dump.S */
struct core_dump_fram {
	uint32_t magic;
	uint32_t num_areg;
	uint32_t pc;
	uint32_t ps;
	uint32_t windowstart;
	uint32_t windowbase;
	uint32_t epc1;
	uint32_t exccause;
	uint32_t excvaddr;
	uint32_t excsave1;
	uint32_t areg[XCHAL_NUM_AREG];
};

extern struct core_dump_fram cd_frams[CORE_TOPS_NUM];

int mtk_trm_mcu_core_dump(void);
int mtk_tops_trm_mcu_init(void);
void mtk_tops_trm_mcu_exit(void);
#endif /* _TOPS_TRM_MCU_H_ */
