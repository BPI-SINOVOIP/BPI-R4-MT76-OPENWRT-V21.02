#
# Copyright (C) 2022 MediaTek Inc. All rights reserved.
#
# This is free software, licensed under the GNU General Public License v2.
# See /LICENSE for more information.
#
include $(TOPDIR)/rules.mk

PKG_NAME:=aesgcm
PKG_VERSION:=1.0

include $(INCLUDE_DIR)/host-build.mk

define Host/Compile
	$(MAKE) -C $(HOST_BUILD_DIR) \
	OPENSSL_INCS_LOCATION=-I$(STAGING_DIR_HOST)/include/openssl-3 \
	OPENSSL_LIBS_LOCATION=-L$(STAGING_DIR_HOST)/lib/openssl-3
endef

define Host/Prepare
	mkdir -p $(HOST_BUILD_DIR)
	$(CP) -a ./src/* $(HOST_BUILD_DIR)/
endef

define Host/Install
	$(CP) $(HOST_BUILD_DIR)/aesgcm $(STAGING_DIR_HOST)/bin/
endef

$(eval $(call HostBuild))
