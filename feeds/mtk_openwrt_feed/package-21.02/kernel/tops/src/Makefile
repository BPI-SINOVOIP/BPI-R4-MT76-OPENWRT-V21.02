# SPDX-License-Identifier: GPL-2.0-or-later
#
# Copyright (c) 2023 MediaTek Inc. All Rights Reserved.
#
# Author: Ren-Ting Wang <ren-ting.wang@mediatek.com>
#

obj-$(CONFIG_MTK_TOPS_SUPPORT) += tops.o

ccflags-y += -I$(src)/inc
ccflags-y += -I$(src)/protocol/inc

tops-y += ctrl.o
tops-y += debugfs.o
tops-y += firmware.o
tops-y += init.o
tops-y += hpdma.o
tops-y += hwspinlock.o
tops-y += mbox.o
tops-y += mcu.o
tops-y += netsys.o
tops-y += net-event.o
tops-y += tops_params.o
tops-y += tnl_offload.o
tops-y += ser.o
tops-y += tdma.o
tops-y += trm-fs.o
tops-y += trm-mcu.o
tops-y += trm-debugfs.o
tops-y += trm.o
tops-y += wdt.o
tops-y += seq_gen.o

tops-y += protocol/mac/eth.o
tops-y += protocol/mac/ppp.o
tops-y += protocol/network/ip.o
tops-y += protocol/transport/udp.o
tops-$(CONFIG_MTK_TOPS_GRETAP) += protocol/tunnel/gre/gretap.o
tops-$(CONFIG_MTK_TOPS_L2TP_V2) += protocol/tunnel/l2tp/l2tpv2.o
tops-$(CONFIG_MTK_TOPS_PPTP) += protocol/tunnel/pptp/pptp.o
tops-$(CONFIG_MTK_TOPS_PPTP) += protocol/tunnel/pptp/pptp_seq.o

include $(wildcard $(src)/*.mk)
