#ifndef CONNECTIONLISTWIDGET_H
#define CONNECTIONLISTWIDGET_H

#include <QWidget>
#include <QListWidget>
#include <QLabel>
#include <QPushButton>
#include <QJsonArray>
#include <QJsonObject>
#include <QMap>

// VM 列表侧边栏：显示从 PVE API 获取的虚拟机列表
class ConnectionListWidget : public QWidget
{
    Q_OBJECT

public:
    explicit ConnectionListWidget(QWidget *parent = nullptr);

    // 用 PVE API 返回的 VM 数据更新列表
    void updateVmList(const QString &node, const QJsonArray &vms);

    // 清空列表
    void clearList();

    // 获取当前选中的 VM 信息
    QJsonObject selectedVm() const;

signals:
    // 用户选中了一个 VM
    void vmSelected(const QJsonObject &vmInfo);
    // 用户双击了一个 VM（快速连接）
    void vmDoubleClicked(const QJsonObject &vmInfo);

private slots:
    void onItemClicked(QListWidgetItem *item);
    void onItemDoubleClicked(QListWidgetItem *item);

private:
    void setupUI();
    QJsonObject vmFromItem(QListWidgetItem *item) const;

    QListWidget *m_listWidget;
    QLabel *m_titleLabel;
    QLabel *m_emptyHint;

    // 存储 VM 数据：key = "node/vmid"
    QMap<QString, QJsonObject> m_vmData;
};

#endif // CONNECTIONLISTWIDGET_H
