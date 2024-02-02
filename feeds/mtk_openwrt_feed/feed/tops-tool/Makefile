# SPDX-License-Identifier: GPL-2.0-or-later
#
# Copyright (C) 2023 Mediatek Inc. All Rights Reserved.
# Author: Alvin Kuo <alvin.kuo@mediatek.com>
#

include $(TOPDIR)/rules.mk

PKG_NAME:=tops-tool
PKG_RELEASE:=1

include $(INCLUDE_DIR)/package.mk
include $(INCLUDE_DIR)/package-defaults.mk

define Package/tops-tool
  TITLE:=Mediatek Tunnel Offload Processor System User Tool
  SECTION:=MTK Properties
  CATEGORY:=MTK Properties
  DEFAULT:=y
  SUBMENU:=Applications
  DEPENDS:=kmod-tops
endef

define Package/tops-tool/description
  This package is support for MediaTek tunnel offload processor system user tool.
  The tool will monitor TOPS status, such as TOPS core utilization rate and core
  debug dump info. Some features are not monitored by default unless user trigger
  it to start. The tool will support logging functionality in the future.
endef

TARGET_CFLAGS += \
	-I$(PKG_BUILD_DIR)/inc

define Build/Compile
	$(MAKE) -C $(PKG_BUILD_DIR) \
		CC="$(TARGET_CC)" \
		CFLAGS="$(TARGET_CFLAGS) -Wall -Wextra" \
		LDFLAGS="$(TARGET_LDFLAGS)"
endef

define Package/tops-tool/install
	$(INSTALL_DIR) $(1)/usr/sbin
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/tops-tool $(1)/usr/sbin/
endef

define Package/tops-tool-autoload
  TITLE:=Mediatek Tunnel Offload Processor System User Tool Auto Load
  CATEGORY:=MTK Properties
  SUBMENU:=Applications
  DEPENDS:=tops-tool
  DEFAULT:=y if PACKAGE_kmod-tops-release-autoload || PACKAGE_kmod-tops-dev-autoload
endef

define Package/tops-tool-autoload/description
  This package is support for MediaTek tunnel offload processor system user tool
  autoload. The tool will automatically start monitor TOPS status when system
  boot up.
endef

define Package/tops-tool-autoload/install
	$(INSTALL_DIR) $(1)/etc/init.d
	$(INSTALL_BIN) ./files/tops-tool.init $(1)/etc/init.d/tops-tool
endef

$(eval $(call BuildPackage,tops-tool))
$(eval $(call BuildPackage,tops-tool-autoload))
