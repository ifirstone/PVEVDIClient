#!/usr/bin/env bash
# ==============================================================================
# PVEClient Image Installer
# 用途: 将 pveclient.img.xz (或 .img) 写入目标磁盘
# 语法: sudo ./pveclient-install.sh <镜像路径> [目标磁盘]
# 示例:
#   sudo ./pveclient-install.sh /mnt/payload/pveclient.img.xz /dev/sda
#   sudo ./pveclient-install.sh /mnt/payload/pveclient.img.xz
# ==============================================================================

set -euo pipefail

LOG_FILE="/tmp/pveclient-install.log"

log() {
  echo "[$(date '+%F %T')] $*" | tee -a "${LOG_FILE}"
}

die() {
  log "ERROR: $*"
  exit 1
}

require_cmd() {
  command -v "$1" >/dev/null 2>&1 || die "缺少命令: $1"
}

is_block_disk() {
  local dev="$1"
  [[ -b "${dev}" ]] || return 1
  [[ "$(lsblk -dn -o TYPE "${dev}" 2>/dev/null || true)" == "disk" ]]
}

get_live_boot_disk() {
  # 优先从 Debian Live 常见挂载点识别 live 介质
  local src
  for mp in /run/live/medium /usr/lib/live/mount/medium; do
    src="$(findmnt -n -o SOURCE "${mp}" 2>/dev/null || true)"
    [[ -n "${src}" ]] && break
  done
  if [[ -n "${src}" ]]; then
    # 光驱介质直接返回自身
    local t
    t="$(lsblk -dn -o TYPE "${src}" 2>/dev/null || true)"
    if [[ "${t}" == "rom" ]]; then
      echo "${src}"
      return 0
    fi

    # 分区 -> 父磁盘，如 /dev/sdb1 -> /dev/sdb, /dev/nvme0n1p1 -> /dev/nvme0n1
    local pk
    pk="$(lsblk -no PKNAME "${src}" 2>/dev/null || true)"
    if [[ -n "${pk}" ]]; then
      echo "/dev/${pk}"
      return 0
    fi
  fi

  # 兜底: root 文件系统所在父磁盘
  src="$(findmnt -n -o SOURCE / 2>/dev/null || true)"
  if [[ -n "${src}" ]]; then
    local pk
    pk="$(lsblk -no PKNAME "${src}" 2>/dev/null || true)"
    if [[ -n "${pk}" ]]; then
      echo "/dev/${pk}"
      return 0
    fi
  fi

  return 1
}

list_candidate_disks() {
  local live_disk="$1"
  while IFS= read -r dev; do
    [[ -n "${dev}" ]] || continue
    [[ "${dev}" == "${live_disk}" ]] && continue
    echo "${dev}"
  done < <(lsblk -dpno NAME,TYPE | awk '$2=="disk" {print $1}')
}

choose_target_disk() {
  local live_disk="$1"
  local disks=()
  while IFS= read -r d; do
    disks+=("${d}")
  done < <(list_candidate_disks "${live_disk}")

  [[ ${#disks[@]} -gt 0 ]] || die "没有可用目标磁盘（已自动排除启动盘 ${live_disk}）"

  echo ""
  echo "可选目标磁盘(已排除启动盘 ${live_disk}):"
  local i=1
  for d in "${disks[@]}"; do
    local info
    info="$(lsblk -dn -o SIZE,MODEL "${d}" | sed 's/^ *//')"
    echo "  ${i}) ${d}  ${info}"
    i=$((i + 1))
  done

  echo ""
  read -r -p "请输入序号选择目标磁盘: " idx
  [[ "${idx}" =~ ^[0-9]+$ ]] || die "输入无效"
  (( idx >= 1 && idx <= ${#disks[@]} )) || die "序号超出范围"

  echo "${disks[$((idx - 1))]}"
}

confirm_target() {
  local target="$1"
  echo ""
  log "目标磁盘: ${target}"
  lsblk -o NAME,SIZE,TYPE,MOUNTPOINT,MODEL "${target}" | tee -a "${LOG_FILE}"
  echo ""
  echo "!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!"
  echo "即将覆盖 ${target} 全盘数据，原有分区和数据将永久丢失。"
  echo "请输入完整设备名确认（例如 /dev/sda）"
  echo "!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!"
  read -r -p "确认输入: " typed
  [[ "${typed}" == "${target}" ]] || die "确认失败，已取消安装"
}

write_image() {
  local image_path="$1"
  local target="$2"

  case "${image_path}" in
    *.xz)
      log "写入 xz 镜像: ${image_path} -> ${target}"
      xz -dc "${image_path}" | dd of="${target}" bs=16M status=progress conv=fsync
      ;;
    *.img|*.raw)
      log "写入 raw 镜像: ${image_path} -> ${target}"
      dd if="${image_path}" of="${target}" bs=16M status=progress conv=fsync
      ;;
    *)
      die "不支持的镜像格式: ${image_path} (仅支持 .xz/.img/.raw)"
      ;;
  esac
}

main() {
  [[ "${EUID}" -eq 0 ]] || die "请使用 root 运行"

  require_cmd lsblk
  require_cmd findmnt
  require_cmd dd
  require_cmd xz
  require_cmd wipefs

  local image_path="${1:-}"
  local target_dev="${2:-}"

  [[ -n "${image_path}" ]] || die "用法: $0 <镜像路径> [目标磁盘]"
  [[ -f "${image_path}" ]] || die "镜像文件不存在: ${image_path}"

  local live_disk
  live_disk="$(get_live_boot_disk || true)"

  if [[ -n "${live_disk}" ]]; then
    log "Live 启动盘: ${live_disk}"
  else
    log "WARN: 未识别到 Live 启动盘，将跳过启动盘自动排除，仅使用安全规则"
  fi

  if [[ -z "${target_dev}" ]]; then
    [[ -n "${live_disk}" ]] || die "未识别到 Live 启动盘，且未手动指定目标盘"
    target_dev="$(choose_target_disk "${live_disk}")"
  fi

  is_block_disk "${target_dev}" || die "目标设备无效或不是磁盘: ${target_dev}"
  [[ "${target_dev}" != /dev/sr* ]] || die "目标盘不能是光驱设备: ${target_dev}"
  [[ "${target_dev}" != /dev/loop* ]] || die "目标盘不能是 loop 设备: ${target_dev}"
  if [[ -n "${live_disk}" ]]; then
    [[ "${target_dev}" != "${live_disk}" ]] || die "目标盘不能是当前启动盘: ${target_dev}"
  fi

  confirm_target "${target_dev}"

  log "卸载目标盘已挂载分区..."
  while IFS= read -r part; do
    [[ -n "${part}" ]] || continue
    umount "${part}" 2>/dev/null || true
  done < <(lsblk -ln -o NAME "${target_dev}" | tail -n +2 | sed "s#^#/dev/#")

  log "清理目标盘签名..."
  wipefs -a "${target_dev}" || true

  write_image "${image_path}" "${target_dev}"

  sync
  if command -v partprobe >/dev/null 2>&1; then
    partprobe "${target_dev}" || true
  elif command -v blockdev >/dev/null 2>&1; then
    blockdev --rereadpt "${target_dev}" || true
  fi

  log "安装完成: ${target_dev}"
  log "日志文件: ${LOG_FILE}"
  echo ""
  echo "请移除安装介质后重启系统。"
}

main "$@"
