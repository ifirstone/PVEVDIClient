#ifndef SPICELAUNCHER_H
#define SPICELAUNCHER_H

#include "AbstractLauncher.h"

// SPICE 协议启动器：通过 remote-viewer 或 PVE API 获取的 .vv 文件连接虚拟机
class SpiceLauncher : public AbstractLauncher
{
    Q_OBJECT

public:
    explicit SpiceLauncher(QObject *parent = nullptr);

    bool launch(const ConnectionInfo &info) override;
    QString buildCommandLine(const ConnectionInfo &info) const override;
    QString protocolName() const override { return "SPICE"; }

    // 通过 PVE API 返回的 SPICE 配置数据启动连接
    bool launchWithVvFile(const QString &vvContent, bool fullscreen = true);

private:
    // 构建 remote-viewer 参数列表
    QStringList buildArguments(const ConnectionInfo &info) const;

    // 获取 remote-viewer 可执行文件路径
    QString remoteViewerPath() const;

    // 将 PVE SPICE 配置写入临时 .vv 文件
    QString writeVvFile(const QString &content) const;
};

#endif // SPICELAUNCHER_H
