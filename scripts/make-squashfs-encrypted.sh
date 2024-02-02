# !/bin/bash
ROOTFS=$1
ENCRYPTED_ROOTFS=$2
ROOTFS_KEY_DIR=$3
TARGET_DEVICE=$4

if [ -z "${BIN}" ]; then
	echo "bin directory not found"
	exit 1
fi

CRYPTSETUP=$BIN/cryptsetup
DEVICE_NAME=$(basename ${TARGET_DEVICE} | sed "s/\.[^.]*$//g")
ROOTFS_KEY=${ROOTFS_KEY_DIR}/${DEVICE_NAME}-rootfs-key.key
if [ ! -f "${ROOTFS}" ] || [ -z "${ENCRYPTED_ROOTFS}" ] ||
   [ ! -f "${ROOTFS_KEY}" ] || [ -z "${DEVICE_NAME}" ]; then
	exit 1
fi

FILE_SIZE=`stat -c "%s" ${ROOTFS}`
BLOCK_SIZE=4096
DATA_BLOCKS=$((${FILE_SIZE} / ${BLOCK_SIZE}))
[ $((${FILE_SIZE} % ${BLOCK_SIZE})) -ne 0 ] && DATA_BLOCKS=$((${DATA_BLOCKS} + 1))

# create container
dd if=/dev/zero of=$ENCRYPTED_ROOTFS bs=4096 count=$DATA_BLOCKS

# mapping encrypted device
sudo $CRYPTSETUP open --type=plain --cipher=aes-xts-plain64 --key-size=256 \
	--key-file=$ROOTFS_KEY $ENCRYPTED_ROOTFS ${DEVICE_NAME}

# encrypt squashfs
sudo dd if=$ROOTFS of=/dev/mapper/${DEVICE_NAME}

# close mapping device
sudo $CRYPTSETUP close /dev/mapper/${DEVICE_NAME}

