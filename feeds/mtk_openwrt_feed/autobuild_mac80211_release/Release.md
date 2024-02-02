# Mediatek Upstream SoftMAC WiFi Driver - MT76 Release Note

## Filogic 880 WiFi7 Non-MLO SDK Release (20240112)
Please note that the upcoming MLO SDK will not be able to use patches to support WiFi 7 MLO based on the 20240112 non-MLO SDK revision.
It is essential to be aware that a complete upgrade of the SDK codebase is mandatory due to the Software MLO Architecture Change.

### External Release

```
#Get Openwrt 21.02 source code from Git server
git clone --branch openwrt-21.02 https://git.openwrt.org/openwrt/openwrt.git
cd openwrt; git checkout 4a1d8ef55cbf247f06dae8e958eb8eb42f1882a5; cd -;

#Get Openwrt master source code from Git Server
git clone --branch master https://git.openwrt.org/openwrt/openwrt.git mac80211_package
cd mac80211_package; git checkout 2fe8ecd880396b5ae25fe9583aaa1d71be0b8468; cd -;

#Get mtk-openwrt-feeds source code
git clone --branch master https://git01.mediatek.com/openwrt/feeds/mtk-openwrt-feeds
cd mtk-openwrt-feeds; git checkout cd559121bfb96b0a21209c315b3314b110aa7fe3; cd -;

#Change to openwrt folder
cp -rf mtk-openwrt-feeds/autobuild_mac80211_release openwrt
cd openwrt; mv autobuild_mac80211_release autobuild

#Add MTK feed
echo "src-git mtk_openwrt_feed https://git01.mediatek.com/openwrt/feeds/mtk-openwrt-feeds" >> feeds.conf.default

#!!! CAUTION!!! Modify feed's revision
vim autobuild/feeds.conf.default-21.02

#Run Filogic880 auto build script
./autobuild/mt7988_mt7996_mac80211/lede-branch-build-sanity.sh

#Further Build (After 1st full build)
./scripts/feeds update –a
make V=s
```
### Feeds Revision
```
#vim autobuild/feeds.conf.default-21.01
src-git packages https://git.openwrt.org/feed/packages.git^2219ac4
src-git luci https://git.openwrt.org/project/luci.git^e4c4633
src-git routing https://git.openwrt.org/feed/routing.git^a9e4310
src-git mtk_openwrt_feed https://git01.mediatek.com/openwrt/feeds/mtk-openwrt-feeds^cd55912
```
### WiFi Package Version

| Platform                 | OpenWrt/21.02                 | GIT01.mediatek.com                                                                |
|--------------------------|-------------------------------|-----------------------------------------------------------------------------------|
| Kernel                   | 5.4.260                       | autobuild_mac80211_release/target/linux/mediatek/patches-5.4 + autobuild_mac80211_release/**mt7988-mt7996-mac80211**/target/linux/mediatek/patches-5.4                     |
| WiFi Package             | OpenWrt/master                | MTK Internal Patches                                                              |
| Hostapd                  | PKG_SOURCE_DATE:=2023-09-08   | autobuild_mac80211_release/package/network/services/**hostapd_new**/patches       |
| libnl-tiny               | PKG_SOURCE_DATE:=2023-12-05   | N/A                                                                               |
| iw                       | PKG_VERSION:=5.19             | N/A                                                                               |
| iwinfo                   | PKG_SOURCE_DATE:=2023-07-01   | N/A                                                                               |
| wireless-regdb           | PKG_VERSION:=2023-09-01       | autobuild_mac80211_release/package/firmware/wireless-regdb/patches                |
| netifd                   | PKG_VERSION:=2024-01-04       | autobuild_mac80211_release/package/network/config/**netifd_new**/patches          |
| MAC80211                 | PKG_VERSION:=6.5              | autobuild_mac80211_release/package/kernel/**mac80211_dev**/patches                |
| MT76                     | PKG_SOURCE_DATE:=2023-12-18   | **Patches**: autobuild_mac80211_release/**mt7988-mt7996-mac80211**/package/kernel/mt76/patches **Firmware** autobuild_mac80211_release/package/kernel/mt76/src/firmware/mt7996 |
| Manufacture Tool (CMD)   | mt76/tools                    |                                                                                   |

## Filogic 830 MP2.2 Release (20231027)
### External Release

```
#Get Openwrt 21.02 source code from Git server
git clone --branch openwrt-21.02 https://git.openwrt.org/openwrt/openwrt.git
cd openwrt; git checkout 18f12e6f69a9597c13d2d18f5eb661f4549331e4; cd -;

#Get Openwrt master source code from Git Server
git clone --branch master https://git.openwrt.org/openwrt/openwrt.git mac80211_package
cd mac80211_package; git checkout e4ebc7b5662d6436fcc84b8e1583204b96fb0503; cd -;

#Get mtk-openwrt-feeds source code
git clone --branch master https://git01.mediatek.com/openwrt/feeds/mtk-openwrt-feeds
cd mtk-openwrt-feeds; git checkout c1d06e11a5c38f2ca84d5f9f3a1157dc6adbffa6; cd -;

#Change to openwrt folder
cp -rf mtk-openwrt-feeds/autobuild_mac80211_release openwrt
cd openwrt; mv autobuild_mac80211_release autobuild

#Add MTK feed
echo "src-git mtk_openwrt_feed https://git01.mediatek.com/openwrt/feeds/mtk-openwrt-feeds" >> feeds.conf.default

#!!! CAUTION!!! Modify feed's revision

#Run Filogic830 auto build script (APSoC: MT7986A/B , PCIE: MT7915A/D, MT7916) 
./autobuild/mt7986_mac80211/lede-branch-build-sanity.sh

#Further Build (After 1st full build)
./scripts/feeds update –a
make V=s
```
### Feeds Revision
```
#vim autobuild/feeds.conf.default-21.02
src-git packages https://git.openwrt.org/feed/packages.git^8df2214
src-git luci https://git.openwrt.org/project/luci.git^e98243e
src-git routing https://git.openwrt.org/feed/routing.git^d79f2b5
src-git mtk_openwrt_feed https://git01.mediatek.com/openwrt/feeds/mtk-openwrt-feeds^c1d06e1
```
### WiFi Package Version

| Platform                 | OpenWrt/21.02                 | GIT01.mediatek.com                                                                |
|--------------------------|-------------------------------|-----------------------------------------------------------------------------------|
| Kernel                   | 5.4.246                       | autobuild_mac80211_release /target/linux/mediatek/patches-5.4                     |
| WiFi Package             | OpenWrt/master                | MTK Internal Patches                                                              |
| Hostapd                  | PKG_SOURCE_DATE:=2022-07-29   | autobuild_mac80211_release/package/network/services/hostapd/patches               |
| libnl-tiny               | PKG_SOURCE_DATE:=2023-07-27   | N/A                                                                               |
| iw                       | PKG_VERSION:=5.19             | N/A                                                                               |
| iwinfo                   | PKG_SOURCE_DATE:=2023-07-01   | N/A                                                                               |
| wireless-regdb           | PKG_VERSION:=2023.09.01       | autobuild_mac80211_release/package/firmware/wireless-regdb/patches                |
| netifd                   | PKG_VERSION:=2023-07-17       | autobuild_mac80211_release/package/network/config/netifd/patches                  |
| MAC80211                 | PKG_VERSION:=5.15.81-1        | autobuild_mac80211_release/package/kernel/mac80211/patches/subsys                 |
| MT76                     | PKG_SOURCE_DATE:=2023-09-18   | **Patches**: autobuild_mac80211_release/package/kernel/mt76/patches **Firmware** autobuild_mac80211_release/package/kernel/mt76/src/firmware|
| Utility                  | Formal                        |                                                                                   |
| Manufacture Tool (ATENL) | feed/atenl                    |                                                                                   |
| Manufacture Tool (CMD)   | mt76/tools                    |                                                                                   |
| Vendor Tool              | feed/mt76-vendor              |                                                                                   |

## Filogic 630/830 MP2.1 Release (20230508)
### External Release

```
#Get Openwrt 21.02 source code from Git server
git clone --branch openwrt-21.02 https://git.openwrt.org/openwrt/openwrt.git
cd openwrt; git checkout 6a12ecbd6dd61bb9da35d75735e1280313659a20; cd -;

#Get Openwrt master source code from Git Server
git clone --branch master https://git.openwrt.org/openwrt/openwrt.git mac80211_package
cd mac80211_package; git checkout cf8d861978dbfdb572a25db460db464b50d9e809; cd -;

#Get mtk-openwrt-feeds source code
git clone --branch master https://git01.mediatek.com/openwrt/feeds/mtk-openwrt-feeds
cd mtk-openwrt-feeds; git checkout 00c6c7a71e0ef444d926cb19b6716250699e4f5c; cd -;

#Change to openwrt folder
cp -rf mtk-openwrt-feeds/autobuild_mac80211_release openwrt
cd openwrt; mv autobuild_mac80211_release autobuild

#Add MTK feed
echo "src-git mtk_openwrt_feed https://git01.mediatek.com/openwrt/feeds/mtk-openwrt-feeds" >> feeds.conf.default

#!!! CAUTION!!! Modify feed's revision

#Run AX6000/AX8400/AX3000 auto build script (APSoC: MT7986A/B , PCIE: MT7915A/D, MT7916)
./autobuild/mt7986_mac80211/lede-branch-build-sanity.sh

#Further Build (After 1st full build)
./scripts/feeds update –a
make V=s
```
### Feeds Revision
```
#vim autobuild/feeds.conf.default-21.01
src-git packages https://git.openwrt.org/feed/packages.git^c6fc6dd
src-git luci https://git.openwrt.org/project/luci.git^e98243e
src-git routing https://git.openwrt.org/feed/routing.git^8071852
src-git mtk_openwrt_feed https://git01.mediatek.com/openwrt/feeds/mtk-openwrt-feeds^00c6c7a
```
### WiFi Package Version

| Platform                 | OpenWrt/21.02                 | GIT01.mediatek.com                                                                |
|--------------------------|-------------------------------|-----------------------------------------------------------------------------------|
| Kernel                   | 5.4.238                       | autobuild_mac80211_release /target/linux/mediatek/patches-5.4                     |
| WiFi Package             | OpenWrt/master                | MTK Internal Patches                                                              |
| Hostapd                  | PKG_SOURCE_DATE:=2022-07-29   | autobuild_mac80211_release/package/network/services/hostapd/patches               |
| libnl-tiny               | PKG_SOURCE_DATE:=2023-04-02   | N/A                                                                               |
| iw                       | PKG_VERSION:=5.19             | N/A                                                                               |
| iwinfo                   | PKG_SOURCE_DATE:=2022-11-01   | N/A                                                                               |
| wireless-regdb           | PKG_VERSION:=2023.02.13       | autobuild_mac80211_release/package/firmware/wireless-regdb/patches                |
| MAC80211                 | PKG_VERSION:=5.15.81-1        | autobuild_mac80211_release/package/kernel/mac80211/patches/subsys                 |
| MT76                     | PKG_SOURCE_DATE:=2023-03-01   | **Patches**: autobuild_mac80211_release/package/kernel/mt76/patches **Firmware** autobuild_mac80211_release/package/kernel/mt76/src/firmware|
| Utility                  | Formal                        |                                                                                   |
| Manufacture Tool (ATENL) | feed/atenl                    |                                                                                   |
| Manufacture Tool (CMD)   | feed/mt76-test                |                                                                                   |
| Vendor Tool              | feed/mt76-vendor              |                                                                                   |

## Filogic 880 Beta Release (20230630)
### External Release

```
#Get Openwrt 21.02 source code from Git server
git clone --branch openwrt-21.02 https://git.openwrt.org/openwrt/openwrt.git
cd openwrt; git checkout eb8cae5391ceee679140a3d8d9abbdc47d0d6461; cd -;

#Get Openwrt master source code from Git Server
git clone --branch master https://git.openwrt.org/openwrt/openwrt.git mac80211_package
cd mac80211_package; git checkout 01885bc6a33dbfa6f3c9e97778fd8f4f60e2514f; cd -;

#Get mtk-openwrt-feeds source code
git clone --branch master https://git01.mediatek.com/openwrt/feeds/mtk-openwrt-feeds
cd mtk-openwrt-feeds; git checkout e600e91a5833365e02885f4f40f810f12a7f5a95; cd -;

#Change to openwrt folder
cp -rf mtk-openwrt-feeds/autobuild_mac80211_release openwrt
cd openwrt; mv autobuild_mac80211_release autobuild

#Add MTK feed
echo "src-git mtk_openwrt_feed https://git01.mediatek.com/openwrt/feeds/mtk-openwrt-feeds" >> feeds.conf.default

#!!! CAUTION!!! Modify feed's revision
vim autobuild/feeds.conf.default-21.02

#Run Filogic880 auto build script
./autobuild/mt7988_mt7996_mac80211/lede-branch-build-sanity.sh 

#Further Build (After 1st full build)
./scripts/feeds update –a
make V=s
```
### Feeds Revision
```
#vim autobuild/feeds.conf.default-21.01
src-git packages https://git.openwrt.org/feed/packages.git^8df2214
src-git luci https://git.openwrt.org/project/luci.giti^e98243e
src-git routing https://git.openwrt.org/feed/routing.git^d79f2b5
src-git mtk_openwrt_feed https://git01.mediatek.com/openwrt/feeds/mtk-openwrt-feeds^e600e91
```
### WiFi Package Version

| Platform                 | OpenWrt/21.02                 | GIT01.mediatek.com                                                                |
|--------------------------|-------------------------------|-----------------------------------------------------------------------------------|
| Kernel                   | 5.4.238                       | autobuild_mac80211_release /target/linux/mediatek/patches-5.4                     |
| WiFi Package             | OpenWrt/master                | MTK Internal Patches                                                              |
| Hostapd                  | PKG_SOURCE_DATE:=2023-03-29   | autobuild_mac80211_release/package/network/services/**hostapd_new**/patches + autobuild_mac80211_release/**mt7988-mt7996-mac80211**/package/network/services/hostapd/patches           |
| libnl-tiny               | PKG_SOURCE_DATE:=2023-04-02   | N/A                                                                               |
| iw                       | PKG_VERSION:=5.19             | N/A                                                                               |
| iwinfo                   | PKG_SOURCE_DATE:=2022-11-01   | N/A                                                                               |
| wireless-regdb           | PKG_VERSION:=2023.05.03       | autobuild_mac80211_release/package/firmware/wireless-regdb/patches                |
| MAC80211                 | PKG_VERSION:=6.1.24           | autobuild_mac80211_release/package/kernel/**mac80211_dev**/patches + autobuild_mac80211_release/**mt7988-mt7996-mac80211**/package/kernel/mac80211/patches |
| MT76                     | PKG_SOURCE_DATE:=2023-05-13   | **Patches**: autobuild_mac80211_release/package/kernel/mt76/patches **Firmware** autobuild_mac80211_release/package/kernel/mt76/src/firmware/mt7996 |
| Manufacture Tool (CMD)   | mt76/tools                    |                                                                                   |
