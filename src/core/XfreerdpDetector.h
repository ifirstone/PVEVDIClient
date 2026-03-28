#ifndef XFREERDPDETECTOR_H
#define XFREERDPDETECTOR_H

#include <QString>
#include <QStringList>

// XfreerdpDetector - 检测系统中可用的 FreeRDP 版本及其路径
class XfreerdpDetector
{
public:
    // 获取全局单例
    static XfreerdpDetector& instance();

    // 检测系统中是否存在 xfreerdp 版本 3
    bool hasXfreerdp3() const;

    // 检测系统中是否存在 xfreerdp 版本 2
    bool hasXfreerdp2() const;

    // 获取 xfreerdp 版本 3 的完整路径
    QString getXfreerdp3Path() const;

    // 获取 xfreerdp 版本 2 的完整路径
    QString getXfreerdp2Path() const;

    // 根据版本号获取对应的可执行文件路径
    // version: 2, 3, 或 0 (auto - 优先选 3，其次 2)
    QString getXfreerdpPath(int version = 0) const;

    // 获取系统中可用的 xfreerdp 版本列表 (例如 [2, 3])
    QStringList getAvailableVersions() const;

    // 获取系统中首选的 xfreerdp 版本 (优先级: 3 > 2 > 0)
    int getPreferredVersion() const;

    // 重新检测（用于运行时环境变化时调用）
    void refresh();

private:
    XfreerdpDetector();
    void detectVersions();
    QString findExecutable(const QString &name) const;

    bool m_hasV3 = false;
    bool m_hasV2 = false;
    QString m_pathV3;
    QString m_pathV2;
};

#endif // XFREERDPDETECTOR_H
