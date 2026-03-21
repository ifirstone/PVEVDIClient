#ifndef RDPLAUNCHER_H
#define RDPLAUNCHER_H

#include "AbstractLauncher.h"
#include <QTimer>

// RDP 协议启动器：构建 xfreerdp 命令行并管理连接进程
class RdpLauncher : public AbstractLauncher
{
    Q_OBJECT

public:
    explicit RdpLauncher(QObject *parent = nullptr);

    bool launch(const ConnectionInfo &info) override;
    QString buildCommandLine(const ConnectionInfo &info) const override;
    QString protocolName() const override { return "RDP"; }

    // USB 安全沙箱目录路径常量
    static constexpr const char* USB_SANDBOX_DIR = "/tmp/pxvdi_usb_share";
    
    // 同步 USB 可移动设备到沙箱目录（仅创建软链接，不映射系统盘）
    static void syncUsbMounts();

private:
    // 构建 xfreerdp 参数列表
    QStringList buildArguments(const ConnectionInfo &info) const;

    // 获取 xfreerdp 可执行文件路径
    QString xfreerdpPath() const;
    
    // USB 热插拔监控定时器
    QTimer *m_usbWatchTimer = nullptr;
};

#endif // RDPLAUNCHER_H

