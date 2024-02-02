#
# Copyright (C) 2023 Weijie Gao <weijie.gao@mediatek.com>
#
# This is free software, licensed under the GNU General Public License v2.
# See /LICENSE for more information.
#
include $(TOPDIR)/rules.mk

PKG_NAME:=codechecker

PKG_SOURCE_URL=https://github.com/Ericsson/codechecker.git
PKG_SOURCE_PROTO:=git
PKG_SOURCE_VERSION:=v6.22.2
PKG_MIRROR_HASH:=6dddd1c7455e1c6d2115ccffcad831a51826f0f1dce18928d651d2665aa41f60

include $(INCLUDE_DIR)/host-build.mk

define Host/Compile
	$(MAKE) -C $(HOST_BUILD_DIR)/analyzer/tools/build-logger -f Makefile.native ldlogger.so
endef

define Host/Install
	$(INSTALL_DATA) $(HOST_BUILD_DIR)/analyzer/tools/build-logger/ldlogger.so $(STAGING_DIR_HOST)/lib/
endef

define Host/Clean
	rm -f $(STAGING_DIR_HOST)/lib/ldlogger.so
endef

$(eval $(call HostBuild))
