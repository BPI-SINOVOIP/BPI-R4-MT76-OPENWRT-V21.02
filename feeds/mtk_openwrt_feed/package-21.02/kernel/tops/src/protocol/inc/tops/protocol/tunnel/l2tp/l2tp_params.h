/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * Copyright (c) 2023 MediaTek Inc. All Rights Reserved.
 *
 * Author: Frank-zj Lin <rank-zj.lin@mediatek.com>
 */

#ifndef _TOPS_L2TP_H_
#define _TOPS_L2TP_H_

/* L2TP header constants */
#define L2TP_HDRFLAG_T		0x8000
#define L2TP_HDRFLAG_L		0x4000

#define L2TP_HDR_VER_MASK	0x000F
#define L2TP_HDR_VER_2		0x0002
#define L2TP_HDR_VER_3		0x0003

#define UDP_L2TP_PORT		1701

struct tops_l2tp_params {
	u16 dl_tid; /* l2tp tunnel id for DL */
	u16 dl_sid; /* l2tp session id for DL */
	u16 ul_tid; /* l2tp tunnel id for UL */
	u16 ul_sid; /* l2tp session id for UL */
};

/* Limited support: L2TPv2 only, no length field, no options */
struct udp_l2tp_data_hdr {
	u16 flag_ver;
	u16 tid;
	u16 sid;
};
#endif /* _TOPS_L2TP_H_ */
