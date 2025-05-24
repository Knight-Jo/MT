#ifndef NETWORKSETTINGSDIALOGH_H
#define NETWORKSETTINGSDIALOGH_H

#include <QDialog>
#include <QDialogButtonBox>
#include <QLineEdit>
#include <QCheckBox>
#include <QSpinBox>
#include <QFormLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QPushButton>
#include <QSettings>
#include <QRegularExpressionValidator>
#include "devicefinder.h"

class NetworkSettingsDialog : public QDialog {
    Q_OBJECT

public:
    explicit NetworkSettingsDialog(QWidget *parent = nullptr) : QDialog(parent) {
        setupUI();
        setupValidations();
        loadSettings();
        connectSignals();
    }

    // 获取配置参数的访问方法
    QString ipAddress() const { return m_ipEdit->text().trimmed(); }
    QVector<bool> broadcastMethods() const {
        return {m_method1Check->isChecked(),
                m_method2Check->isChecked(),
                m_method3Check->isChecked()};
    }
    quint16 tcpPort() const { return static_cast<quint16>(m_tcpPortSpin->value()); }
    quint16 udpPort() const { return static_cast<quint16>(m_udpPortSpin->value()); }
    quint16 targetUdpPort() const { return static_cast<quint16>(m_targetUdpSpin->value()); }
    int frequency() const { return m_frequencySpin->value(); }

protected:
    void accept() override {
        if (validateInput()) {
            saveSettings();
            QDialog::accept();
        }
    }

private slots:
    void onApply() {
        if (validateInput()) {
            saveSettings();
            emit settingsApplied();
        }
    }

signals:
    void settingsApplied();

private:
    void setupUI() {
        QVBoxLayout *mainLayout = new QVBoxLayout(this);

        // 配置表单区域
        QFormLayout *formLayout = new QFormLayout;
        setupFormFields(formLayout);
        mainLayout->addLayout(formLayout);

        // 按钮区域
        setupButtonBox(mainLayout);

        setWindowTitle(tr("Network Configuration"));
        resize(480, 320);  // 合理的初始尺寸
    }

    void setupFormFields(QFormLayout *layout) {
        // IP地址
        m_ipEdit = new QLineEdit(this);
        layout->addRow(tr("IP Address:"), m_ipEdit);

        // 广播方法
        QGroupBox *methodGroup = new QGroupBox(tr("Broadcast Methods"), this);
        QHBoxLayout *methodLayout = new QHBoxLayout;
        m_method1Check = new QCheckBox(tr("Method 1"), methodGroup);
        m_method2Check = new QCheckBox(tr("Method 2"), methodGroup);
        m_method3Check = new QCheckBox(tr("Method 3"), methodGroup);
        methodLayout->addWidget(m_method1Check);
        methodLayout->addWidget(m_method2Check);
        methodLayout->addWidget(m_method3Check);
        methodGroup->setLayout(methodLayout);
        layout->addRow(methodGroup);

        // 端口设置
        m_tcpPortSpin = createPortSpinBox(TCP_LISTEN_PORT);
        m_udpPortSpin = createPortSpinBox(UDP_LISTEN_PORT);
        m_targetUdpSpin = createPortSpinBox(UDP_TARGET_PORT);
        qDebug() << "UDP_TARGET_PORT: " << UDP_TARGET_PORT;
        layout->addRow(tr("TCP Listen Port:"), m_tcpPortSpin);
        layout->addRow(tr("UDP Listen Port:"), m_udpPortSpin);
        layout->addRow(tr("Target UDP Port:"), m_targetUdpSpin);

        // 发送频率
        m_frequencySpin = new QSpinBox(this);
        m_frequencySpin->setRange(1, 1000);
        m_frequencySpin->setValue(100);
        m_frequencySpin->setSuffix(tr(" ms"));
        layout->addRow(tr("Send Frequency:"), m_frequencySpin);
    }

    void setupButtonBox(QLayout *layout) {
        QDialogButtonBox *buttonBox = new QDialogButtonBox(this);
        QPushButton *applyBtn = buttonBox->addButton(tr("Apply"), QDialogButtonBox::ApplyRole);
        buttonBox->addButton(QDialogButtonBox::Ok);
        buttonBox->addButton(QDialogButtonBox::Cancel);

        connect(buttonBox, &QDialogButtonBox::accepted, this, &QDialog::accept);
        connect(buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
        connect(applyBtn, &QPushButton::clicked, this, &NetworkSettingsDialog::onApply);

        layout->addWidget(buttonBox);
    }

    void connectSignals() {
        // 输入变化时自动验证
        connect(m_ipEdit, &QLineEdit::textChanged, this, [this]{ validateInput(); });
        connect(m_tcpPortSpin, QOverload<int>::of(&QSpinBox::valueChanged), this, [this]{ validateInput(); });
    }

    bool validateInput() {
        bool isValid = true;
        const QString ip = m_ipEdit->text().trimmed();

        // IP地址验证（允许空值）
        if (!ip.isEmpty() && !m_ipEdit->hasAcceptableInput()) {
            m_ipEdit->setToolTip(tr("Invalid IPv4 address format"));
            isValid = false;
        } else {
            m_ipEdit->setToolTip("");
        }

        // 至少选择一个广播方法
        if (!m_method1Check->isChecked() &&
            !m_method2Check->isChecked() &&
            !m_method3Check->isChecked()) {
            m_method1Check->setToolTip(tr("At least one method must be selected"));
            isValid = false;
        } else {
            m_method1Check->setToolTip("");
        }

        return isValid;
    }

    QSpinBox* createPortSpinBox(int defaultValue) {
        QSpinBox *spin = new QSpinBox(this);
        spin->setRange(1, 65535);
        spin->setValue(defaultValue);
        return spin;
    }

    void setupValidations() {
        // IPv4验证（允许空值）
        QRegularExpression ipRegex("^$|^((25[0-5]|2[0-4][0-9]|[01]?[0-9][0-9]?)\\.){3}"
                                   "(25[0-5]|2[0-4][0-9]|[01]?[0-9][0-9]?)$");
        QRegularExpressionValidator *ipValidator = new QRegularExpressionValidator(ipRegex, this);
        m_ipEdit->setValidator(ipValidator);
    }

    void loadSettings() {
        QSettings settings;
        m_ipEdit->setText(settings.value("Network/IP").toString());
        m_method1Check->setChecked(settings.value("Network/Method1", false).toBool());
        m_method2Check->setChecked(settings.value("Network/Method2", false).toBool());
        m_method3Check->setChecked(settings.value("Network/Method3", false).toBool());
        m_tcpPortSpin->setValue(settings.value("Network/TCPPort", TCP_LISTEN_PORT).toInt());
        m_udpPortSpin->setValue(settings.value("Network/UDPPort", UDP_LISTEN_PORT).toInt());
        m_targetUdpSpin->setValue(settings.value("Network/TargetUDP", UDP_TARGET_PORT).toInt());
        m_frequencySpin->setValue(settings.value("Network/Frequency", 100).toInt());
    }

    void saveSettings() {
        QSettings settings;
        settings.setValue("Network/IP", m_ipEdit->text());
        settings.setValue("Network/Method1", m_method1Check->isChecked());
        settings.setValue("Network/Method2", m_method2Check->isChecked());
        settings.setValue("Network/Method3", m_method3Check->isChecked());
        settings.setValue("Network/TCPPort", m_tcpPortSpin->value());
        settings.setValue("Network/UDPPort", m_udpPortSpin->value());
        settings.setValue("Network/TargetUDP", m_targetUdpSpin->value());
        settings.setValue("Network/Frequency", m_frequencySpin->value());
    }

    // 成员变量命名添加m_前缀
    QLineEdit *m_ipEdit;
    QCheckBox *m_method1Check;
    QCheckBox *m_method2Check;
    QCheckBox *m_method3Check;
    QSpinBox *m_tcpPortSpin;
    QSpinBox *m_udpPortSpin;
    QSpinBox *m_targetUdpSpin;
    QSpinBox *m_frequencySpin;
};
#endif // NETWORKSETTINGSDIALOGH_H
