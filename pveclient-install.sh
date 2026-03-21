#!/bin/bash
# ==============================================================================
# PVEClient OS Installer - 商业级零客户机一键克隆脚本
# 用途：将当前运行的 PVEClient Live 镜像 (U盘) 完整克隆到目标硬盘
# 效果：重启后目标机器自动变身为深度定制的只读 PVEClient 瘦客户机
# 语法：sudo ./pveclient-install.sh /dev/sda
# ==============================================================================
set -e

if [ "$EUID" -ne 0 ]; then
  echo "请使用 root 权限运行此脚本 (sudo ./pveclient-install.sh)"
  exit 1
fi

TARGET_DEV=$1
if [ -z "$TARGET_DEV" ]; then
  echo "用法: $0 <目标磁盘>"
  echo "例如: $0 /dev/sda"
  echo -e "\033[31m警告: 目标磁盘上的所有数据将被彻底擦除！\033[0m"
  echo "可用磁盘列表："
  lsblk -d -n -o NAME,SIZE,MODEL | awk '{print "  /dev/"$0}'
  exit 1
fi

if [ ! -b "$TARGET_DEV" ]; then
  echo "错误: 找不到目标设备 $TARGET_DEV"
  exit 1
fi

echo "=========================================================="
echo -e "\033[31m 警告：即将把整个系统环境覆盖到 $TARGET_DEV ！！！\033[0m"
echo -e "\033[31m $TARGET_DEV 上的所有历史分区、系统和数据将永久丢失！\033[0m"
echo "=========================================================="
read -p "确认继续吗？(输入 YES 继续): " CONFIRM

if [ "$CONFIRM" != "YES" ]; then
  echo "安装已取消。"
  exit 0
fi

# 自动寻找当前系统所在的块设备 (基于 Debian Live-build 逻辑)
SOURCE_DEV=$(findmnt -n -o SOURCE /run/live/medium 2>/dev/null || echo "")

if [ -z "$SOURCE_DEV" ]; then
  # 回退策略：如果找不到标准的 live 介质，那么尝试寻找挂载了 iso9660 或者根挂载点底层物理盘
  SOURCE_DEV=$(lsblk -no PKNAME $(findmnt -n -o SOURCE / | sed 's/\[.*\]//') 2>/dev/null || echo "")
  if [ -n "$SOURCE_DEV" ]; then
      SOURCE_DEV="/dev/$SOURCE_DEV"
  fi
fi

if [ -z "$SOURCE_DEV" ]; then
  echo "无法识别当前的启动介质，请确保您正处于 Live OS (U盘启动) 模式下。"
  exit 1
fi

# 防止手抖把当前系统所在盘写坏
if [ "$SOURCE_DEV" == "$TARGET_DEV" ]; then
  echo "错误：目标磁盘和当前启动的源磁盘是同一个！"
  exit 1
fi

echo ""
echo "[1/3] 发现源启动介质: $SOURCE_DEV"
echo "[2/3] 开始进行底层硬件级数据克隆至: $TARGET_DEV"
echo "      这个过程可能需要几分钟，期间不会有画面跳动，请耐心等待..."

# 核心克隆操作：使用较大的缓存块提高克隆速度
dd if=$SOURCE_DEV of=$TARGET_DEV bs=8M status=progress

sync
echo ""
echo "[3/3] 克隆完成！正在刷新目标磁盘的分区表..."
partprobe $TARGET_DEV

# （注：瘦客户机/零客户机的一大优势就在于系统基于只读镜像运行不可变，不惧怕断电和中病毒。
# 此处我们不自动扩容分区，而是完整保留 Live OS 的结构。用户配置则通过本地额外的 Overlay 分区来固化。）

echo ""
echo "========================================================================="
echo -e "\033[32m 系统安装/流片成功！\033[0m"
echo " PVEClient 瘦客户机环境已完美复刻到您的内置硬盘上。"
echo " 请拔出安装 U 盘，然后重启电脑。"
echo "========================================================================="
