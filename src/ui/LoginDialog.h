#ifndef LOGINDIALOG_H
#define LOGINDIALOG_H

#include <QDialog>
#include <QLineEdit>
#include <QSpinBox>
#include <QComboBox>

// PVE 登录对话框
class LoginDialog : public QDialog
{
    Q_OBJECT

public:
    explicit LoginDialog(QWidget *parent = nullptr);

    // 预填服务器信息
    void setServer(const QString &host, int port, const QString &username);

    // 获取输入值
    QString host() const;
    int port() const;
    QString username() const;
    QString password() const;
    QString realm() const;

private:
    void setupUI();

    QLineEdit *m_editHost;
    QSpinBox *m_spinPort;
    QLineEdit *m_editUsername;
    QLineEdit *m_editPassword;
    QComboBox *m_comboRealm;
};

#endif // LOGINDIALOG_H
