#!/usr/bin/env bash
#
# Copyright (C) 2013 OpenWrt.org
#
# This is free software, licensed under the GNU General Public License v2.
# See /LICENSE for more information.
#

set -ex
[ $# -eq 6 ] || {
    echo "SYNTAX: $0 <file> <GPT image> <bl2 image> <fip image> <kernel image> <rootfs system>"
    exit 1
}

OUTPUT_FILE="$1"
GPT_FILE="$2"
BL2_FILE="$3"
FIP_FILE="$4"
KERNEL_FILE="$5"
ROOTFS_FILE="$6"

BS=512
GPT_OFFSET=0	       # 0x00000000
BL2_OFFSET=1024        # 0x00080000
FIP_OFFSET=17408       # 0x00880000
KERNEL_OFFSET=21504    # 0x00a80000
ROOTFS_OFFSET=87040    # 0x02a80000

dd bs="$BS" if="$GPT_FILE"            of="$OUTPUT_FILE"    seek="$GPT_OFFSET"
dd bs="$BS" if="$BL2_FILE"            of="$OUTPUT_FILE"    seek="$BL2_OFFSET"
dd bs="$BS" if="$FIP_FILE"            of="$OUTPUT_FILE"    seek="$FIP_OFFSET"
dd bs="$BS" if="$KERNEL_FILE"         of="$OUTPUT_FILE"    seek="$KERNEL_OFFSET" 
dd bs="$BS" if="$ROOTFS_FILE"         of="$OUTPUT_FILE"    seek="$ROOTFS_OFFSET" 
