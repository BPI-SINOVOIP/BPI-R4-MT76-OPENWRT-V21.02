#
# Copyright (C) 2006-2010 OpenWrt.org
#
# This is free software, licensed under the GNU General Public License v2.
# See /LICENSE for more information.
#

include $(TOPDIR)/rules.mk

PKG_NAME:=popt
PKG_VERSION:=1.16
PKG_RELEASE:=2

PKG_SOURCE:=$(PKG_NAME)-$(PKG_VERSION).tar.gz
PKG_SOURCE_URL:= \
	http://distfiles.gentoo.org/distfiles/ \
	http://distcache.freebsd.org/ports-distfiles/ \
	http://rpm5.org/files/popt/
PKG_HASH:=e728ed296fe9f069a0e005003c3d6b2dde3d9cad453422a10d6558616d304cc8
PKG_LICENSE:=MIT
PKG_CPE_ID:=cpe:/a:popt_project:popt

PKG_INSTALL:=1
PKG_BUILD_PARALLEL:=1

HOST_BUILD_PREFIX:=$(STAGING_DIR_HOST)

include $(INCLUDE_DIR)/package.mk
include $(INCLUDE_DIR)/host-build.mk

TARGET_CFLAGS += $(FPIC)
HOST_CONFIGURE_ARGS += --enable-libiconv-tiny
HOST_BUILD_DEPENDS := libiconv/host

define Package/libpopt
  SECTION:=libs
  CATEGORY:=Libraries
  TITLE:=A command line option parsing library
  URL:=http://rpm5.org/files/popt/
  ABI_VERSION:=0
endef

CONFIGURE_ARGS += --enable-shared --enable-static

define Build/InstallDev
	$(INSTALL_DIR) $(1)/usr/include
	$(CP) $(PKG_INSTALL_DIR)/usr/include/popt.h $(1)/usr/include/
	$(INSTALL_DIR) $(1)/usr/lib
	$(CP) $(PKG_INSTALL_DIR)/usr/lib/libpopt.{a,so*} $(1)/usr/lib/
	$(INSTALL_DIR) $(1)/usr/lib/pkgconfig
	$(CP) $(PKG_INSTALL_DIR)/usr/lib/pkgconfig/popt.pc $(1)/usr/lib/pkgconfig/
endef

define Package/libpopt/install
	$(INSTALL_DIR) $(1)/usr/lib
	$(CP) $(PKG_INSTALL_DIR)/usr/lib/libpopt.so.* $(1)/usr/lib/
endef

$(eval $(call BuildPackage,libpopt))
$(eval $(call HostBuild))
