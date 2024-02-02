# SPDX-License-Identifier: GPL-2.0-or-later
#
# Copyright (C) 2023 Mediatek Inc. All Rights Reserved.
# Author: Ren-Ting Wang <ren-ting.wang@mediatek.com>
#

include $(TOPDIR)/rules.mk
include $(INCLUDE_DIR)/kernel.mk

PKG_NAME:=tops
PKG_RELEASE:=1

include $(INCLUDE_DIR)/package.mk
include $(INCLUDE_DIR)/package-defaults.mk

define Build/Prepare
	mkdir -p $(PKG_BUILD_DIR)
	$(CP) ./firmware $(PKG_BUILD_DIR)/firmware
	$(CP) ./src/* $(PKG_BUILD_DIR)/
endef

define Package/tops-rebb-fw-release
  TITLE:=Mediatek Tunnel Offload Processor System ReBB Release Firmware
  SECTION:=firmware
  CATEGORY:=Firmware
  DEPENDS:= \
	@MTK_TOPS_RELEASE_TARGET \
	kmod-tops-release
endef

define Package/tops-rebb-fw-release/description
  Support for Mediatek Tunnel Offload Processor System ReBB release firmware.
  The firmware offload and accerlerate APMCU's tunnel protocols traffic.
  Available offload tunnel include L2oGRE, L2TP.
endef

define Package/tops-rebb-fw-release/install
	$(INSTALL_DIR) $(1)/lib/firmware/mediatek
	$(CP) \
		$(PKG_BUILD_DIR)/firmware/rebb/mt7988_mgmt/tops-mgmt.img \
		$(PKG_BUILD_DIR)/firmware/rebb/mt7988_offload/tops-offload.img \
		$(1)/lib/firmware/mediatek
endef

define KernelPackage/tops-release
  CATEGORY:=MTK Properties
  SUBMENU:=Drivers
  TITLE:= MTK Tunnel Offload Processor System Kernel Driver
  FILES+=$(PKG_BUILD_DIR)/tops.ko
  KCONFIG:=
  DEPENDS:= \
	@MTK_TOPS_RELEASE_TARGET \
	kmod-mediatek_hnat \
	kmod-tops \
	+kmod-pce \
	+@KERNEL_RELAY
ifeq ($(CONFIG_MTK_TOPS_PPTP), y)
  DEPENDS+= +kmod-pptp
endif
endef

define KernelPackage/tops-release/description
  Support for MTK Tunnel Offload Processor System Released Driver. The driver
  controls the TOPS system to reduce the loading of tunnel protocol processing
  in APMCU.
endef

define KernelPackage/tops-release-autoload
  CATEGORY:=MTK Properties
  SUBMENU:=Drivers
  TITLE:= MTK Tunnel Offload Processor System Auto Load
  AUTOLOAD:=$(call AutoLoad,51,tops)
  KCONFIG:=
  DEPENDS:= \
	kmod-tops-release \
	+kmod-pce-autoload
endef

define KernelPackage/tops-release-autoload/description
  Support for MTK Tunnel Offload Processor System auto load on system
  boot process.
endef

define KernelPackage/tops
  CATEGORY:=MTK Properties
  TITLE:=Mediatek Tunnel Offload Processor System
  SUBMENU:=Drivers
  DEFAULT:=y
  DEPENDS:= \
	@TARGET_mediatek_mt7988
endef

define KernelPackage/tops/config
	source "$(SOURCE)/Config.in"
endef

define KernelPackage/tops/description
  Support for MTK Tunnel Offload Processor System. This system reduces the
  loading of APMCU's tunnel protocol overhead and improve tunnel protocol's
  throughput.
endef

define Build/Compile
	$(MAKE) -C "$(LINUX_DIR)" \
		$(KERNEL_MAKE_FLAGS) \
		M="$(PKG_BUILD_DIR)" \
		EXTRA_CFLAGS="$(EXTRA_CFLAGS)" \
		$(EXTRA_KCONFIG) \
		modules
endef

include tops.mk
include $(filter-out tops.mk,$(wildcard *.mk))

$(eval $(call BuildPackage,tops-rebb-fw-release))
$(eval $(call KernelPackage,tops))
$(eval $(call KernelPackage,tops-release))
$(eval $(call KernelPackage,tops-release-autoload))
