# PVEClient Linux 部署与自启动手册

## 1. 目标

本手册用于把 PVEClient 部署到 Linux 系统，并实现以下能力：

- 程序安装路径固定为 /usr/local/PVEClient/bin/PVEClient
- 守护脚本放在 /home/pveclient/bin/
- 开机自动登录普通用户 pveclient
- 自动进入 Openbox 图形环境
- 开机自动拉起 PVEClient
- PVEClient 退出后自动拉起
- 连续 5 次用户主动退出后停止拉起
- 通过右键菜单 Open PVEClient 恢复监听

本仓库已提供下列脚本模板：

- deploy/linux/pveclient-supervisor.sh
- deploy/linux/pveclient-open.sh
- deploy/linux/openbox-autostart-example.sh
- pveclient-install.sh


## 2. 目录约定

生产部署建议保持以下路径：

- 主程序：/usr/local/PVEClient/bin/PVEClient
- P2P 组件目录：/usr/local/PVEClient/bin/package/
- P2P 可执行文件：/usr/local/PVEClient/bin/package/easytier-core
- P2P 可执行文件：/usr/local/PVEClient/bin/package/easytier-cli
- 守护脚本：/home/pveclient/bin/pveclient-supervisor.sh
- 恢复脚本：/home/pveclient/bin/pveclient-open.sh
- Openbox 自启动：/home/pveclient/.config/openbox/autostart
- Openbox 菜单：/home/pveclient/.config/openbox/menu.xml
- 壁纸：/home/pveclient/Pictures/wallpaper.jpg
- 守护日志：/run/user/<uid>/pveclient-state/supervisor.log


## 3. 系统依赖

推荐基础镜像：

        debian-12.13.0-amd64-netinst.iso

建议先做 Debian 12 最小安装，再安装以下依赖。

### 3.1 基础桌面与工具

        sudo apt update
        sudo apt install -y \
            xorg openbox xinit x11-xserver-utils feh lxterminal \
            network-manager network-manager-gnome pavucontrol arandr system-config-printer \
            virt-viewer

### 3.2 FreeRDP（先安装 v2，可选升级到 v3）

        sudo apt update
        sudo apt install -y freerdp2-x11
        echo "deb http://mirrors.ustc.edu.cn/debian bookworm-backports main" | sudo tee /etc/apt/sources.list.d/backports.list
        sudo apt update
        sudo apt install -y -t bookworm-backports freerdp3-x11

### 3.3 Qt5 核心运行库

        sudo apt install -y \
            libqt5core5a \
            libqt5gui5 \
            libqt5widgets5 \
            libqt5network5 \
            libqt5svg5 \
            libqt5dbus5

### 3.4 X11 必要库

        sudo apt install -y \
            libx11-6 \
            libxext6 \
            libxrender1 \
            libxkbcommon0 \
            libxkbcommon-x11-0

### 3.5 SPICE 支持

        sudo apt install -y \
            libspice-client-gtk-3.0-5 \
            spice-html5

### 3.6 其他常见运行依赖

        sudo apt install -y \
            libssl3 \
            libcrypto++6 \
            libc6

### 3.7 验证 Qt5 库安装

        ldconfig -p | grep libQt5Widgets

### 3.8 已知包差异说明（不同源/版本）

以下包名在不同镜像源或版本中可能存在差异，建议现场按实际仓库查询后替换：

- libcrypto++6
    - 在部分环境中可能是 libcrypto++8 或其他版本号后缀。
    - 查询建议：apt search '^libcrypto\+\+[0-9]+'

- freerdp3-x11
    - 需要 bookworm-backports，若 backports 不可用，可暂用 freerdp2-x11。

- spice-html5
    - 某些最小源可能未提供，可按需保留 virt-viewer 即可完成 SPICE 客户端能力。

- pavucontrol、arandr、system-config-printer
    - 如果不需要对应菜单项，可不安装；安装失败不影响 PVEClient 核心运行。


## 4. 创建普通用户并授权

如果系统还没有 pveclient 用户：

    sudo useradd -m -s /bin/bash pveclient

给重启和关机菜单授权（无密码）：

    sudo visudo -f /etc/sudoers.d/pveclient-power

写入内容：

    pveclient ALL=(root) NOPASSWD: /sbin/reboot, /sbin/poweroff
    pveclient ALL=(root) NOPASSWD: /usr/sbin/reboot, /usr/sbin/poweroff

修正权限并校验：

    sudo chown root:root /etc/sudoers.d/pveclient-power
    sudo chmod 0440 /etc/sudoers.d/pveclient-power
    sudo visudo -c


## 5. 部署程序与脚本

### 5.1 部署主程序

    sudo mkdir -p /usr/local/PVEClient/bin
    sudo cp PVEClient /usr/local/PVEClient/bin/PVEClient
    sudo chmod +x /usr/local/PVEClient/bin/PVEClient

PVEClient 的 package 目录必须与主程序同级（用于 P2P 连接能力）：

    sudo mkdir -p /usr/local/PVEClient/bin/package
    sudo cp -r bin/package/* /usr/local/PVEClient/bin/package/
    sudo chmod +x /usr/local/PVEClient/bin/package/easytier-core
    sudo chmod +x /usr/local/PVEClient/bin/package/easytier-cli

说明：

- /usr/local/PVEClient/bin/PVEClient 为 Linux 客户端主程序。
- /usr/local/PVEClient/bin/package/ 下的 easytier-core 与 easytier-cli 为 P2P 运行必需文件。

### 5.2 部署守护脚本

    sudo mkdir -p /home/pveclient/bin
    sudo cp deploy/linux/pveclient-supervisor.sh /home/pveclient/bin/
    sudo cp deploy/linux/pveclient-open.sh /home/pveclient/bin/
    sudo chmod +x /home/pveclient/bin/pveclient-supervisor.sh
    sudo chmod +x /home/pveclient/bin/pveclient-open.sh
    sudo chown -R pveclient:pveclient /home/pveclient/bin


## 6. 配置自动登录与自动进图形

### 6.1 TTY1 自动登录

    sudo mkdir -p /etc/systemd/system/getty@tty1.service.d
    sudo tee /etc/systemd/system/getty@tty1.service.d/autologin.conf > /dev/null << 'EOF'
    [Service]
    ExecStart=
    ExecStart=-/sbin/agetty --autologin pveclient --noclear %I $TERM
    EOF
    sudo systemctl daemon-reload

### 6.2 pveclient 用户自动启动 X

以 pveclient 用户创建：

    mkdir -p /home/pveclient/.config/openbox

创建 /home/pveclient/.xinitrc：

    #!/bin/sh
    xset -dpms
    xset s off
    xset s noblank
    exec openbox-session

创建 /home/pveclient/.profile：

    if [ -z "$DISPLAY" ] && [ "$(tty)" = "/dev/tty1" ]; then
      startx
    fi

执行：

    chmod +x /home/pveclient/.xinitrc


## 7. Openbox 自启动与菜单

### 7.1 autostart

编辑 /home/pveclient/.config/openbox/autostart，示例：

    feh --bg-fill /home/pveclient/Pictures/wallpaper.jpg &
    xset -dpms
    xset s off
    xset s noblank
    APP_BIN=/usr/local/PVEClient/bin/PVEClient /home/pveclient/bin/pveclient-supervisor.sh &

### 7.2 菜单

建议菜单包含：

- Open PVEClient
- Terminal
- Network Setting
- Sound Setting
- Screen Setting
- Printer Setting
- Reboot
- Shutdown

Open PVEClient 菜单命令建议使用：

    /home/pveclient/bin/pveclient-open.sh

这样当守护已停止时，可自动恢复监听并拉起程序。


## 8. 启动行为说明

### 8.1 正常开机

- 自动登录 pveclient
- 自动进入 Openbox
- autostart 启动 pveclient-supervisor.sh
- supervisor 启动 PVEClient

### 8.2 退出与重启策略

- 程序异常退出：自动重启
- 用户主动退出：计数 +1
- 连续 5 次主动退出：停止守护
- 点击菜单 Open PVEClient：恢复守护


## 9. 验证清单

重启后执行检查：

    pgrep -a -f pveclient-supervisor.sh
    tail -n 30 /run/user/$(id -u)/pveclient-state/supervisor.log

功能验证：

- Alt+F4 连续 5 次后不再自动拉起
- 菜单 Open PVEClient 后恢复拉起
- Reboot 与 Shutdown 菜单可用


## 10. 使用 IMG 快速部署

如果采用镜像写盘部署：

- 提供 pveclient.img.xz
- 目标机 Live 环境执行安装脚本

示例：

    bash pveclient-install.sh /path/to/pveclient.img.xz /dev/sda

说明：

- 脚本会做目标盘校验
- 脚本会拒绝光驱与 loop 设备
- 写盘后建议移除安装介质并重启


## 11. 常见问题

### 11.1 openbox --reconfigure 报 display 错误

原因：在 SSH 下执行，没有图形会话。可忽略，重启后生效。

### 11.2 chmod ISO 内文件失败

原因：ISO 只读。请用 bash 直接执行脚本，或先复制到可写目录。

### 11.3 sudoers bad permissions

修复：

    sudo chown root:root /etc/sudoers.d/pveclient-power
    sudo chmod 0440 /etc/sudoers.d/pveclient-power

### 11.4 xorriso 读取 /dev/sr0 busy

先卸载光驱挂载点，或改用 ISO 文件路径作为输入。


## 12. 安全与运维建议

- 生产建议关闭不必要服务
- 仅开放必要远程管理入口
- 定期更新客户端与依赖
- 保留镜像 SHA256 校验文件
- 版本发布时记录变更与回滚方案
