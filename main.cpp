#include "mainwindow.h"

#include <QApplication>
#include <QThread>
#include "networksettingsDialog.h"
#include "devicefinder.h"

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    MainWindow w;
    w.show();

    QPair<QHostAddress, QHostAddress> ip_mask = NetworkUtils::getLocalIp();
    qDebug() << "local IP:" << ip_mask.first;
    qDebug() << "local IP netmask:" << ip_mask.second;


    NetworkSettingsDialog dlg;
    QVector<bool> method;
    QString ipaddress;
    if (dlg.exec() == QDialog::Accepted) {
        method = dlg.broadcastMethods();
        ipaddress = dlg.ipAddress();
        qDebug() << "IP Address:" << dlg.ipAddress();
        qDebug() << "Methods" << dlg.broadcastMethods();
        qDebug() << "TCP Port:" << dlg.tcpPort();
        qDebug() << "UDP Port:" << dlg.udpPort();
        qDebug() << "TARGET Port:" << dlg.targetUdpPort();
        qDebug() << "Frequency:" << dlg.frequency();
    }


    DeviceFinder finder;
    ConnectionHandler handler;
    QTimer::singleShot(0, &finder, [&](){
        finder.startDiscovery(method, ipaddress);
    });
    QTimer::singleShot(0, &finder, &DeviceFinder::startListening);

    return a.exec();
}
