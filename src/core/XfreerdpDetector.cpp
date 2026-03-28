#include "XfreerdpDetector.h"
#include <QStandardPaths>
#include <QFile>
#include <QDebug>

XfreerdpDetector::XfreerdpDetector()
{
    detectVersions();
}

XfreerdpDetector& XfreerdpDetector::instance()
{
    static XfreerdpDetector detector;
    return detector;
}

void XfreerdpDetector::detectVersions()
{
    // 重置状态
    m_hasV3 = false;
    m_hasV2 = false;
    m_pathV3.clear();
    m_pathV2.clear();

    // 检测 xfreerdp3
    QString v3Path = findExecutable("xfreerdp3");
    if (!v3Path.isEmpty()) {
        m_hasV3 = true;
        m_pathV3 = v3Path;
        qDebug() << "检测到 FreeRDP 3.x:" << v3Path;
    }

    // 检测 xfreerdp (版本 2)
    QString v2Path = findExecutable("xfreerdp");
    if (!v2Path.isEmpty()) {
        m_hasV2 = true;
        m_pathV2 = v2Path;
        qDebug() << "检测到 FreeRDP 2.x:" << v2Path;
    }

    // 如果两个都没找到，尝试常见路径
    if (!m_hasV3) {
        QStringList candidates3 = {
            "/usr/bin/xfreerdp3",
            "/usr/local/bin/xfreerdp3",
            "/bin/xfreerdp3",
        };
        for (const QString &path : candidates3) {
            if (QFile::exists(path)) {
                m_hasV3 = true;
                m_pathV3 = path;
                qDebug() << "检测到 FreeRDP 3.x (常见路径):" << path;
                break;
            }
        }
    }

    if (!m_hasV2) {
        QStringList candidates2 = {
            "/usr/bin/xfreerdp",
            "/usr/local/bin/xfreerdp",
            "/bin/xfreerdp",
        };
        for (const QString &path : candidates2) {
            if (QFile::exists(path)) {
                m_hasV2 = true;
                m_pathV2 = path;
                qDebug() << "检测到 FreeRDP 2.x (常见路径):" << path;
                break;
            }
        }
    }

    // 输出检测结果
    qDebug() << QString("FreeRDP 检测结果: v3=%1 v2=%2")
                .arg(m_hasV3 ? "✓" : "✗")
                .arg(m_hasV2 ? "✓" : "✗");
}

QString XfreerdpDetector::findExecutable(const QString &name) const
{
    // 首先在 PATH 中搜索
    QString path = QStandardPaths::findExecutable(name);
    if (!path.isEmpty()) {
        return path;
    }
    return QString();
}

bool XfreerdpDetector::hasXfreerdp3() const
{
    return m_hasV3;
}

bool XfreerdpDetector::hasXfreerdp2() const
{
    return m_hasV2;
}

QString XfreerdpDetector::getXfreerdp3Path() const
{
    return m_pathV3;
}

QString XfreerdpDetector::getXfreerdp2Path() const
{
    return m_pathV2;
}

QString XfreerdpDetector::getXfreerdpPath(int version) const
{
    if (version == 3) {
        if (m_hasV3) return m_pathV3;
        if (m_hasV2) return m_pathV2;  // 降级到 v2
    } else if (version == 2) {
        if (m_hasV2) return m_pathV2;
        if (m_hasV3) return m_pathV3;  // 降级到 v3
    } else {
        // auto: 优先选 v3，其次 v2
        if (m_hasV3) return m_pathV3;
        if (m_hasV2) return m_pathV2;
    }
    return QString();  // 两个都没有
}

QStringList XfreerdpDetector::getAvailableVersions() const
{
    QStringList versions;
    if (m_hasV2) versions << "2";
    if (m_hasV3) versions << "3";
    return versions;
}

int XfreerdpDetector::getPreferredVersion() const
{
    if (m_hasV3) return 3;
    if (m_hasV2) return 2;
    return 0;  // 都没有
}

void XfreerdpDetector::refresh()
{
    detectVersions();
}
