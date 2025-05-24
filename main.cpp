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

    QThread finderThread;
    QThread handlerThread;

    finder.moveToThread(&finderThread);
    handler.moveToThread(&handlerThread);

    QObject::connect(&finderThread, &QThread::started, [&]() {
        finder.startDiscovery(method, ipaddress);
    });

    QObject::connect(&handlerThread, &QThread::started, &handler, &ConnectionHandler::startListening);

    QObject::connect(&handler, &ConnectionHandler::connectionSuccess,
                     &finder, &DeviceFinder::stopDiscovery, Qt::QueuedConnection);



    finderThread.start();
    handlerThread.start();

    // 确保线程结束后自动清理
    QObject::connect(&finderThread, &QThread::finished,
                     &finderThread, &QObject::deleteLater);
    QObject::connect(&handlerThread, &QThread::finished,
                     &handlerThread, &QObject::deleteLater);

    return a.exec();
}
