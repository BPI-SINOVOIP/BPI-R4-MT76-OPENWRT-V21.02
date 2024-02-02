#!/bin/bash
source ./autobuild/lede-build-sanity.sh

#get the brach_name
temp=${0%/*}
branch_name=${temp##*/}
swpath=0
kasan=0
backport_new=0
args=

for arg in $*; do
	case "$arg" in
	"swpath")
		swpath=1
		;;
	"kasan")
		kasan=1
		;;
	"dev")
		backport_new=1
		;;
	*)
		args="$args $arg"
		;;
	esac
done
set -- $args

change_dot_config() {
	[ "$swpath" = "1" ] && {
		sed -i 's/CONFIG_BRIDGE_NETFILTER=y/# CONFIG_BRIDGE_NETFILTER is not set/g' ${BUILD_DIR}/target/linux/mediatek/mt7986/config-5.4
		sed -i 's/CONFIG_NETFILTER_FAMILY_BRIDGE=y/# CONFIG_NETFILTER_FAMILY_BRIDGE is not set/g' ${BUILD_DIR}/target/linux/mediatek/mt7986/config-5.4
		sed -i 's/CONFIG_SKB_EXTENSIONS=y/# CONFIG_SKB_EXTENSIONS is not set/g' ${BUILD_DIR}/target/linux/mediatek/mt7986/config-5.4
		sed -i 's/CONFIG_BRIDGE_NETFILTER=y/# CONFIG_BRIDGE_NETFILTER is not set/g' ${BUILD_DIR}/target/linux/mediatek/mt7622/config-5.4
		sed -i 's/CONFIG_NETFILTER_FAMILY_BRIDGE=y/# CONFIG_NETFILTER_FAMILY_BRIDGE is not set/g' ${BUILD_DIR}/target/linux/mediatek/mt7622/config-5.4
		sed -i 's/CONFIG_SKB_EXTENSIONS=y/# CONFIG_SKB_EXTENSIONS is not set/g' ${BUILD_DIR}/target/linux/mediatek/mt7622/config-5.4
		sed -i '/AUTOLOAD:=$(call AutoProbe,mt7915e)/a\  MODPARAMS.mt7915e:=wed_enable=0' ${BUILD_DIR}/package/kernel/mt76/Makefile
	}

	[ "$kasan" = "1" ] && {
		sed -i 's/# CONFIG_KERNEL_KASAN is not set/CONFIG_KERNEL_KASAN=y/g' ${BUILD_DIR}/.config
		sed -i 's/# CONFIG_KERNEL_KALLSYMS is not set/CONFIG_KERNEL_KALLSYMS=y/g' ${BUILD_DIR}/.config
		echo "CONFIG_KERNEL_KASAN_OUTLINE=y" >> ${BUILD_DIR}/.config
		echo "CONFIG_DEBUG_KMEMLEAK=y" >> ${BUILD_DIR}/target/linux/mediatek/mt7986/config-5.4
		echo "CONFIG_DEBUG_KMEMLEAK_AUTO_SCAN=y" >> ${BUILD_DIR}/target/linux/mediatek/mt7986/config-5.4
		echo "# CONFIG_DEBUG_KMEMLEAK_DEFAULT_OFF is not set" >> ${BUILD_DIR}/target/linux/mediatek/mt7986/config-5.4
		echo "CONFIG_DEBUG_KMEMLEAK_MEM_POOL_SIZE=16000" >> ${BUILD_DIR}/target/linux/mediatek/mt7986/config-5.4
		echo "CONFIG_DEBUG_KMEMLEAK_TEST=m" >> ${BUILD_DIR}/target/linux/mediatek/mt7986/config-5.4
		echo "CONFIG_KALLSYMS=y" >> ${BUILD_DIR}/target/linux/mediatek/mt7986/config-5.4
		echo "CONFIG_KASAN=y" >> ${BUILD_DIR}/target/linux/mediatek/mt7986/config-5.4
		echo "CONFIG_KASAN_GENERIC=y" >> ${BUILD_DIR}/target/linux/mediatek/mt7986/config-5.4
		echo "# CONFIG_KASAN_INLINE is not set" >> ${BUILD_DIR}/target/linux/mediatek/mt7986/config-5.4
		echo "CONFIG_KASAN_OUTLINE=y" >> ${BUILD_DIR}/target/linux/mediatek/mt7986/config-5.4
		echo "CONFIG_KASAN_SHADOW_OFFSET=0xdfffffd000000000" >> ${BUILD_DIR}/target/linux/mediatek/mt7986/config-5.4
		echo "# CONFIG_TEST_KASAN is not set" >> ${BUILD_DIR}/target/linux/mediatek/mt7986/config-5.4
		echo "CONFIG_SLUB_DEBUG=y" >> ${BUILD_DIR}/target/linux/mediatek/mt7986/config-5.4
		echo "CONFIG_FRAME_WARN=4096" >> ${BUILD_DIR}/target/linux/mediatek/mt7986/config-5.4
	}

	[ "$backport_new" = "1" ] && {
		rm -rf ${BUILD_DIR}/package/kernel/mt76/patches/*revert-for-backports*.patch
	}

}

#step1 clean
#clean

#do prepare stuff
prepare

prepare_flowoffload

prepare_mac80211 ${backport_new}

prepare_final ${branch_name}

change_dot_config

#step2 build
if [ -z ${1} ]; then
	build_log ${branch_name} -j1 || [ "$LOCAL" != "1" ]
fi
