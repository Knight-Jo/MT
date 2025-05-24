#include "mainwindow.h"
#include "./ui_mainwindow.h"
#include "networksettingsDialog.h"
#include "devicefinder.h"


MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    QPair<QHostAddress, QHostAddress> ip_mask = NetworkUtils::getLocalIp();
    qDebug() << "local IP:" << ip_mask.first;
    qDebug() << "local IP netmask:" << ip_mask.second;


    NetworkSettingsDialog dlg;
    QVector<bool> method;
    QString ipAddress;
    quint16 tcpPort;
    quint16 udpPort;
    quint16 targetUdpPort;

    if (dlg.exec() == QDialog::Accepted) {
        method = dlg.broadcastMethods();
        ipAddress = dlg.ipAddress();
        tcpPort = dlg.tcpPort();
        udpPort = dlg.udpPort();
        targetUdpPort = dlg.targetUdpPort();
        qDebug() << "IP Address:" << dlg.ipAddress();
        qDebug() << "Methods" << method;
        qDebug() << "TCP Port:" << dlg.tcpPort();
        qDebug() << "UDP Port:" << dlg.udpPort();
        qDebug() << "TARGET Port:" << dlg.targetUdpPort();
        qDebug() << "Frequency:" << dlg.frequency();
    }

    finder = new DeviceFinder(method, ipAddress, tcpPort, udpPort, targetUdpPort);
    QTimer::singleShot(0, finder, &DeviceFinder::startDiscovery);
    QTimer::singleShot(0, finder, &DeviceFinder::startListening);

    QTimer::connect(finder, &DeviceFinder::deviceFound, [&](QString ip){
        qDebug()<< "deviceFound main: " << ip ;
        finder->disconnect();
        finder->deleteLater();
    });
}
MainWindow::~MainWindow()
{
    delete ui;
}
