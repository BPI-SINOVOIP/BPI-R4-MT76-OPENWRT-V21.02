#!/usr/bin/env bash
#
# Copyright (C) 2013 OpenWrt.org
#
# This is free software, licensed under the GNU General Public License v2.
# See /LICENSE for more information.
#

set -ex
[ $# -eq 4 ] || {
    echo "SYNTAX: $0 <file> <bl2 image> <fip image> <factory image>"
    exit 1
}

OUTPUT_FILE="$1"
BL2_FILE="$2"
FIP_FILE="$3"
FACTORY_BIN_FILE="$4"

BS=512
BL2_OFFSET=0        		# 0x00000000
FIP_OFFSET=11264       		# 0x00580000
FACTORY_BIN_OFFSET=15360    	# 0x00780000

dd if=/dev/zero ibs=7864320 count=1 | tr "\000" "\377" > $OUTPUT_FILE

dd bs="$BS" if="$BL2_FILE"            	  of="$OUTPUT_FILE"    seek="$BL2_OFFSET" conv=notrunc
dd bs="$BS" if="$FIP_FILE"                of="$OUTPUT_FILE"    seek="$FIP_OFFSET" conv=notrunc
dd bs="$BS" if="$FACTORY_BIN_FILE"         of="$OUTPUT_FILE"    seek="$FACTORY_BIN_OFFSET" 
