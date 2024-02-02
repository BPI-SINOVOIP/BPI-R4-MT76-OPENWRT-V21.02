# !/bin/bash
FDTGET=$BIN/fdtget
FDTPUT=$BIN/fdtput
PERL=$BIN/perl

FDT=$1
SUMMARY=$2
BOOTARGS=$(LD_LIBRARY_PATH=${LIBFDT_PATH} \
		${FDTGET} ${FDT} /chosen bootargs | sed "s/\"*$/;/g")
DEVICE_PATH=$(echo -n ${BOOTARGS} | grep -o "root=/dev/dm-[0-9]\+" | \
		grep -o "/dev/dm-[0-9]\+")
DEVICE_NUM=$(echo -n ${DEVICE_PATH} | grep -o "[0-9]\+")
BOOTARGS=$(echo -n ${BOOTARGS} | \
		sed "s/root=\/dev\/dm-${DEVICE_NUM}/root=\/dev\/dm-$((DEVICE_NUM+1))/g")
DATABLOCKS_NUM=$(cat ${SUMMARY} | grep "Data blocks:" | grep -o "[0-9]\+")
CIPHER="aes-xts-plain64"
KEY=$(${PERL} -E 'say "0" x 64')
IV_OFFSET=0
OFFSET=0

NEW_BOOTARGS=$( printf '%sdm-crypt,,,ro,0 %d crypt %s %s %d %s %d"' \
		"${BOOTARGS}" $((DATABLOCKS_NUM*8)) ${CIPHER} ${KEY} \
		${IV_OFFSET} ${DEVICE_PATH} ${OFFSET} )

LD_LIBRARY_PATH=${LIBFDT_PATH}
$FDTPUT $FDT /chosen bootargs -t s "${NEW_BOOTARGS}" || exit 1
