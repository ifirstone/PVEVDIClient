#ifndef RDPLAUNCHER_H
#define RDPLAUNCHER_H

#include "AbstractLauncher.h"

// RDP 协议启动器：构建 xfreerdp 命令行并管理连接进程
class RdpLauncher : public AbstractLauncher
{
    Q_OBJECT

public:
    explicit RdpLauncher(QObject *parent = nullptr);

    bool launch(const ConnectionInfo &info) override;
    QString buildCommandLine(const ConnectionInfo &info) const override;
    QString protocolName() const override { return "RDP"; }

private:
    // 构建 xfreerdp 参数列表
    QStringList buildArguments(const ConnectionInfo &info) const;

    // 获取 xfreerdp 可执行文件路径
    QString xfreerdpPath() const;
};

#endif // RDPLAUNCHER_H
