#ifndef DEVICEFINDER_H
#define DEVICEFINDER_H

#include <QTcpServer>
#include <QCoreApplication>
#include <QCommandLineParser>
#include <QNetworkInterface>
#include <QUdpSocket>
#include <QTcpServer>
#include <QTcpSocket>
#include <QThread>
#include <QTimer>
#include <QByteArray>
#include <QPair>

#include <qmdnsengine/server.h>
#include <qmdnsengine/provider.h>
#include <qmdnsengine/hostname.h>
#include <qmdnsengine/service.h>

constexpr quint16 UDP_TARGET_PORT = 9910;
constexpr quint16 UDP_LISTEN_PORT = 68;
constexpr quint16 TCP_LISTEN_PORT = 80;
const QByteArray MESSAGE = "Hello, Device! From Finder!";
const QByteArray SERVICE_TYPE = "_test._tcp.local.";
const QByteArray SERVICE_NAME = "JumpWDevice";
const QByteArray EXIT_MESSAGE = "EXIT";
const QByteArray HEARTBEAT = "heartbeat";

class NetworkUtils {
public:
    static QPair<QHostAddress,QHostAddress> getLocalIp() {
        foreach (const QNetworkInterface &interface, QNetworkInterface::allInterfaces()) {
            // 跳过未启用或未运行的接口
            if (!interface.flags().testFlag(QNetworkInterface::IsUp) ||
                !interface.flags().testFlag(QNetworkInterface::IsRunning)) {
                continue;
            }

            // 检查接口类型：有线或无线
            bool isWired = (interface.type() == QNetworkInterface::Ethernet);
            bool isWireless = (interface.type() == QNetworkInterface::Ieee80211);
            if (!isWired && !isWireless) {
                continue;
            }

            // 排除名称包含虚拟设备或蓝牙的接口
            QString ifaceName = interface.humanReadableName();
            if (ifaceName.contains("VMware", Qt::CaseInsensitive) ||
                ifaceName.contains("Bluetooth", Qt::CaseInsensitive)) {
                continue;
            }
            foreach (const QNetworkAddressEntry &entry, interface.addressEntries()) {
                if (!entry.ip().isLoopback() && entry.ip().protocol() == QAbstractSocket::IPv4Protocol) {
                    return QPair<QHostAddress, QHostAddress>{entry.ip(), entry.netmask()};
                }
            }
        }
        return QPair<QHostAddress, QHostAddress>{QHostAddress::LocalHost, QHostAddress::LocalHost};
    }

    static QList<QNetworkInterface> getValidInterfaces() {
        QList<QNetworkInterface> validInterfaces;
        foreach (const QNetworkInterface &interface, QNetworkInterface::allInterfaces()) {
            if (interface.flags() & QNetworkInterface::IsUp &&
                !(interface.flags() & QNetworkInterface::IsLoopBack)) {
                    validInterfaces.append(interface);
                }
        }
        return validInterfaces;
    }
};

class DeviceFinder : public QObject {
    Q_OBJECT

public :
    explicit DeviceFinder(const QVector<bool> m,
                          const QString ip,
                          const quint16 tcpPort,
                          const quint16 udpPort,
                          const quint16 targetUdp,
                          QObject* parent=nullptr);

    void startDiscovery();

public slots:
    void stopDiscovery();

    void startListening();

signals:
    void deviceFound(int method);

private:
    void startBroadcast();

    void  startMdns();

    void startUdpScan(const QString &targetIp );

    void scanTarget(const QString &ip);

    void startSubnetScan();

    // Listening
    void startTcpServer();

    void handleTcpConnection();

    void startUdpListener();


    QMdnsEngine::Server *m_server;
    QMdnsEngine::Hostname *m_hostname;
    QMdnsEngine::Provider *m_provider;
    QMdnsEngine::Service m_service;

    QTcpServer *tcpServer;


    QVector<bool> method;
    QString m_targetIp;
    // mdnsService
    QUdpSocket *udpSocket;
    QTimer *broadcastTimer;
    int broadcastCounter = 0;
    int currentMethod = 3;

    quint16 m_udp_target = UDP_TARGET_PORT;
    quint16 m_udp_listen = UDP_LISTEN_PORT;
    quint16 m_tcp_listen = TCP_LISTEN_PORT;

    bool isconnected = false;
};

class ConnectionHandler : public QObject {
    Q_OBJECT
public:
    explicit ConnectionHandler(QObject *parent = nullptr);

public slots:
    void startListening();

signals:
    // 成功连接信号
    void connectionSuccess();

private:
     void startTcpServer();

    void handleTcpConnection();

    void startUdpListener();


    QTcpServer *tcpServer;
    QUdpSocket *udpSocket;

    quint16 m_udp_target = UDP_TARGET_PORT;
    quint16 m_udp_listen = UDP_LISTEN_PORT;
    quint16 m_tcp_listen = TCP_LISTEN_PORT;


};


#endif // DEVICEFINDER_H
