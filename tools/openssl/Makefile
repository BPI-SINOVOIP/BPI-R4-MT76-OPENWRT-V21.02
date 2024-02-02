#
# Copyright (C) 2006-2016 OpenWrt.org
#
# This is free software, licensed under the GNU General Public License v2.
# See /LICENSE for more information.
#

include $(TOPDIR)/rules.mk

PKG_NAME:=openssl
PKG_VERSION:=3.0.7
PKG_RELEASE:=3

PKG_BUILD_PARALLEL:=1

PKG_SOURCE:=$(PKG_NAME)-$(PKG_VERSION).tar.gz
PKG_SOURCE_URL:= \
	http://www.openssl.org/source/ \
	http://ftp.fi.muni.cz/pub/openssl/source/ \
	ftp://ftp.pca.dfn.de/pub/tools/net/openssl/source/ \

PKG_HASH:=83049d042a260e696f62406ac5c08bf706fd84383f945cf21bd61e9ed95c396e

PKG_LICENSE:=OpenSSL
PKG_LICENSE_FILES:=LICENSE
PKG_MAINTAINER:=Eneas U de Queiroz <cotequeiroz@gmail.com>
PKG_CPE_ID:=cpe:/a:openssl:openssl

include $(INCLUDE_DIR)/host-build.mk

define Host/Configure
	(cd $(HOST_BUILD_DIR); \
	./Configure -no-shared)
endef

define Host/Install
	$(CP) $(HOST_BUILD_DIR)/apps/openssl $(STAGING_DIR_HOST)/bin/openssl-$(PKG_RELEASE)
	mkdir -p $(STAGING_DIR_HOST)/lib/openssl-$(PKG_RELEASE)
	$(CP) $(HOST_BUILD_DIR)/libssl.a $(STAGING_DIR_HOST)/lib/openssl-$(PKG_RELEASE)
	$(CP) $(HOST_BUILD_DIR)/libcrypto.a $(STAGING_DIR_HOST)/lib/openssl-$(PKG_RELEASE)
	ln -sf $(HOST_BUILD_DIR)/include $(STAGING_DIR_HOST)/include/openssl-$(PKG_RELEASE)
endef

$(eval $(call HostBuild))
