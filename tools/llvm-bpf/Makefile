#
# Copyright (C) 2006-2016 OpenWrt.org
#
# This is free software, licensed under the GNU General Public License v2.
# See /LICENSE for more information.
#
include $(TOPDIR)/rules.mk

PKG_NAME:=llvm-project
PKG_VERSION:=15.0.7
PKG_RELEASE:=1

PKG_SOURCE:=$(PKG_NAME)-$(PKG_VERSION).src.tar.xz
PKG_SOURCE_URL:=https://github.com/llvm/llvm-project/releases/download/llvmorg-$(PKG_VERSION)
PKG_HASH:=8b5fcb24b4128cf04df1b0b9410ce8b1a729cb3c544e6da885d234280dedeac6
PKG_CPE_ID:=cpe:/a:llvm:llvm

HOST_BUILD_DIR:=$(BUILD_DIR_HOST)/$(PKG_NAME)-$(PKG_VERSION).src

CMAKE_BINARY_SUBDIR := build
CMAKE_SOURCE_SUBDIR := llvm

include $(INCLUDE_DIR)/host-build.mk
include $(INCLUDE_DIR)/cmake.mk

LLVM_BPF_PREFIX = llvm-bpf-$(PKG_VERSION).$(HOST_OS)-$(HOST_ARCH)

HOST_BUILD_PREFIX = $(STAGING_DIR_HOST)/$(LLVM_BPF_PREFIX)

CMAKE_HOST_OPTIONS += \
	-DLLVM_ENABLE_BINDINGS=OFF \
	-DLLVM_INCLUDE_DOCS=OFF \
	-DLLVM_INCLUDE_EXAMPLES=OFF \
	-DLLVM_INCLUDE_TESTS=OFF \
	-DLLVM_ENABLE_PROJECTS="clang;lld" \
	-DLLVM_TARGETS_TO_BUILD=BPF \
	-DCLANG_BUILD_EXAMPLES=OFF \
	-DLLVM_INSTALL_TOOLCHAIN_ONLY=ON \
	-DLLVM_LINK_LLVM_DYLIB=ON \
	-DLLVM_TOOLCHAIN_TOOLS="llvm-objcopy;llvm-objdump;llvm-readelf;llvm-strip;llvm-ar;llvm-as;llvm-dis;llvm-link;llvm-nm;llvm-ranlib;llc;opt" \
	-DCMAKE_SKIP_RPATH=OFF

define Host/Install
	rm -rf $(STAGING_DIR_HOST)/llvm-bpf*
	$(Host/Install/Default)
	ln -s $(LLVM_BPF_PREFIX) $(STAGING_DIR_HOST)/llvm-bpf
	STRIP_KMOD= PATCHELF= STRIP=strip $(SCRIPT_DIR)/rstrip.sh $(STAGING_DIR_HOST)/llvm-bpf
	echo "$(PKG_VERSION)" > $(HOST_BUILD_PREFIX)/.llvm-version
endef

define Host/Uninstall
	rm -rf $(STAGING_DIR_HOST)/llvm-bpf*
endef

$(eval $(call HostBuild))
