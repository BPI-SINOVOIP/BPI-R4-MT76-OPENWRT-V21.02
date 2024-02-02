KERNEL_LOADADDR := 0x48080000

define Device/BPI-R4-SD
  DEVICE_VENDOR := Banana Pi
  DEVICE_MODEL := Banana Pi R4
  DEVICE_TITLE := MTK7988a BPI R4 SD
  DEVICE_DTS := mt7988a-bananapi-bpi-r4-sd
  DEVICE_DTS_DIR := $(DTS_DIR)/mediatek
  SUPPORTED_DEVICES := bananapi,bpi-r4
  DEVICE_PACKAGES := mkf2fs e2fsprogs blkid blockdev losetup kmod-fs-ext4 \
		     kmod-mmc kmod-fs-f2fs kmod-fs-vfat kmod-nls-cp437 \
		     kmod-nls-iso8859-1
  IMAGE/sysupgrade.bin := sysupgrade-tar | append-metadata
endef
TARGET_DEVICES += BPI-R4-SD

define Device/BPI-R4-EMMC
  DEVICE_VENDOR := Banana Pi
  DEVICE_MODEL := Banana Pi R4
  DEVICE_TITLE := MTK7988a BPI R4 EMMC
  DEVICE_DTS := mt7988a-bananapi-bpi-r4-emmc
  DEVICE_DTS_DIR := $(DTS_DIR)/mediatek
  SUPPORTED_DEVICES := bananapi,bpi-r4
  DEVICE_PACKAGES := mkf2fs e2fsprogs blkid blockdev losetup kmod-fs-ext4 \
		     kmod-mmc kmod-fs-f2fs kmod-fs-vfat kmod-nls-cp437 \
		     kmod-nls-iso8859-1
  IMAGE/sysupgrade.bin := sysupgrade-tar | append-metadata
endef
TARGET_DEVICES += BPI-R4-EMMC

define Device/BPI-R4-NAND
  DEVICE_VENDOR := Banana Pi
  DEVICE_MODEL := Banana Pi R4
  DEVICE_TITLE := MTK7988a BPI R4 NAND
  DEVICE_DTS := mt7988a-bananapi-bpi-r4-nand
  DEVICE_DTS_DIR := $(DTS_DIR)/mediatek
  SUPPORTED_DEVICES := bananapi,bpi-r4
  UBINIZE_OPTS := -E 5
  BLOCKSIZE := 128k
  PAGESIZE := 2048
  IMAGE_SIZE := 131072k
  KERNEL_IN_UBI := 1
  IMAGES += factory.bin
  IMAGE/factory.bin := append-ubi | check-size $$$$(IMAGE_SIZE)
  IMAGE/sysupgrade.bin := sysupgrade-tar | append-metadata
endef
TARGET_DEVICES += BPI-R4-NAND
