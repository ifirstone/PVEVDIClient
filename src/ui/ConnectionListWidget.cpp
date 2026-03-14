#include "ConnectionListWidget.h"
#include <QVBoxLayout>
#include <QLabel>
#include <QMap>

ConnectionListWidget::ConnectionListWidget(QWidget *parent)
    : QWidget(parent)
{
    setupUI();
}

void ConnectionListWidget::setupUI()
{
    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);

    // 标题
    m_titleLabel = new QLabel("虚拟机列表");
    m_titleLabel->setObjectName("sectionTitle");
    m_titleLabel->setAlignment(Qt::AlignCenter);
    QFont titleFont = m_titleLabel->font();
    titleFont.setPointSize(11);
    titleFont.setBold(true);
    m_titleLabel->setFont(titleFont);
    m_titleLabel->setContentsMargins(0, 10, 0, 10);
    layout->addWidget(m_titleLabel);

    // VM 列表
    m_listWidget = new QListWidget();
    m_listWidget->setObjectName("connectionList");
    m_listWidget->setSpacing(2);
    layout->addWidget(m_listWidget);

    // 空列表提示
    m_emptyHint = new QLabel("请先登录 PVE 服务器\n以获取虚拟机列表");
    m_emptyHint->setObjectName("emptyHint");
    m_emptyHint->setAlignment(Qt::AlignCenter);
    m_emptyHint->setWordWrap(true);
    m_emptyHint->setStyleSheet("color: #6c7086; font-size: 12px; padding: 20px;");
    layout->addWidget(m_emptyHint);

    // 信号连接
    connect(m_listWidget, &QListWidget::itemClicked,
            this, &ConnectionListWidget::onItemClicked);
    connect(m_listWidget, &QListWidget::itemDoubleClicked,
            this, &ConnectionListWidget::onItemDoubleClicked);
}

void ConnectionListWidget::updateVmList(const QString &node, const QJsonArray &vms)
{
    // 先移除该节点的旧数据
    QMutableMapIterator<QString, QJsonObject> it(m_vmData);
    while (it.hasNext()) {
        it.next();
        if (it.key().startsWith(node + "/")) {
            it.remove();
        }
    }

    // 添加新数据
    for (const QJsonValue &val : vms) {
        QJsonObject vm = val.toObject();
        vm["node"] = node; // 附加节点信息
        int vmId = vm["vmid"].toInt();
        QString key = QString("%1/%2").arg(node).arg(vmId);
        m_vmData[key] = vm;
    }

    // 重建列表 UI
    m_listWidget->clear();

    // 按 VMID 排序显示
    QList<QString> keys = m_vmData.keys();
    std::sort(keys.begin(), keys.end());

    for (const QString &key : keys) {
        QJsonObject vm = m_vmData[key];
        int vmId = vm["vmid"].toInt();
        QString name = vm["name"].toString();
        QString status = vm["status"].toString();
        QString vmNode = vm["node"].toString();

        // 状态图标
        QString statusIcon;
        if (status == "running") {
            statusIcon = "🟢";
        } else if (status == "stopped") {
            statusIcon = "🔴";
        } else {
            statusIcon = "🟡";
        }

        QString displayText = QString("%1 %2 (VM %3)").arg(statusIcon, name).arg(vmId);

        QListWidgetItem *item = new QListWidgetItem(displayText);
        // 存储 key 用于查找对应 VM 数据
        item->setData(Qt::UserRole, key);
        item->setSizeHint(QSize(0, 44));
        item->setToolTip(QString("VM %1: %2\n节点: %3\n状态: %4")
                             .arg(vmId).arg(name, vmNode, status));

        m_listWidget->addItem(item);
    }

    // 切换空列表提示的显示
    m_emptyHint->setVisible(m_vmData.isEmpty());
    m_listWidget->setVisible(!m_vmData.isEmpty());
}

void ConnectionListWidget::clearList()
{
    m_listWidget->clear();
    m_vmData.clear();
    m_emptyHint->setVisible(true);
    m_listWidget->setVisible(false);
}

QJsonObject ConnectionListWidget::selectedVm() const
{
    QListWidgetItem *item = m_listWidget->currentItem();
    if (!item) return QJsonObject();
    return vmFromItem(item);
}

QJsonObject ConnectionListWidget::vmFromItem(QListWidgetItem *item) const
{
    QString key = item->data(Qt::UserRole).toString();
    return m_vmData.value(key, QJsonObject());
}

void ConnectionListWidget::onItemClicked(QListWidgetItem *item)
{
    QJsonObject vm = vmFromItem(item);
    if (!vm.isEmpty()) {
        emit vmSelected(vm);
    }
}

void ConnectionListWidget::onItemDoubleClicked(QListWidgetItem *item)
{
    QJsonObject vm = vmFromItem(item);
    if (!vm.isEmpty()) {
        emit vmDoubleClicked(vm);
    }
}
