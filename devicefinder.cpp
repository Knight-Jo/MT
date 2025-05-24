#include "devicefinder.h"

DeviceFinder::DeviceFinder(const QVector<bool> m,
                           const QString ip,
                           const quint16 tcpPort,
                           const quint16 udpPort,
                           const quint16 targetUdp,
                           QObject *parent )
    :
    method(m)
    ,m_tcp_listen(tcpPort)
    ,m_udp_listen(udpPort)
    ,m_targetIp(ip)
    ,QObject(parent)
{
    qDebug() <<"-----DEviceFinder initial-----";

    tcpServer = new QTcpServer(this);
    udpSocket = new QUdpSocket(this);
    udpSocket->setSocketOption(QAbstractSocket::MulticastLoopbackOption, 1);

}

void DeviceFinder::stopDiscovery()
{
    qDebug()<<"----Stop Discovery----" ;
    isconnected = true;
    // tcpServer->deleteLater();
    // udpSocket->deleteLater();

}



void DeviceFinder::startDiscovery()
{
    qDebug()<< "startDiscovery";
    if (!m_targetIp.isEmpty()) {
        qDebug() << "startUdpScan :" << m_targetIp;
        startUdpScan(m_targetIp);
    } else {
        if (method[0]) {
            startBroadcast();
        }
        if (method[1]){
            startMdns();
        }
        if (method[2]) {
            startUdpScan("");
        }
    }
}

void DeviceFinder::startBroadcast()
{
    QTimer::singleShot(0, this, [this]() {
        broadcastCounter = 0;
        broadcastTimer = new QTimer(this);

        connect(broadcastTimer, &QTimer::timeout, this, [this]() {
            if (broadcastCounter++ >= 30) {
                broadcastTimer->stop();
                return;
            }
            udpSocket->writeDatagram(MESSAGE, QHostAddress::Broadcast, m_udp_target);
        });

        broadcastTimer->start(1000);
    });
}

void  DeviceFinder::startMdns()
{
    qDebug()<< "Method 2 startMdns";
    m_server = new QMdnsEngine::Server(this);
    m_hostname = new QMdnsEngine::Hostname(m_server, this);
    m_provider = new QMdnsEngine::Provider(m_server, m_hostname, this);

    const QByteArray type = SERVICE_TYPE;
    const QByteArray name = SERVICE_NAME;

    m_service.setType(type);
    m_service.setName(name);
    m_service.setPort(8080);

    m_provider->update(m_service);
}

void DeviceFinder::startUdpScan(const QString &targetIp="")
{
    qDebug() << "Method 3 startUdpScan";
    QTimer *scanTimer = new QTimer(this);
    connect(scanTimer, &QTimer::timeout, this, [this, targetIp, scanTimer]() {
        if (isconnected) {
            scanTimer->stop();
            scanTimer->deleteLater();
            return;
        }
        if (!targetIp.isEmpty()) {
            scanTarget(targetIp);
        }else {
            startSubnetScan();
        }
    });
    scanTimer->start(1000);
}
void DeviceFinder::scanTarget(const QString &ip)
{

    QHostAddress address(ip);
    udpSocket->writeDatagram(MESSAGE, address, m_udp_target);
    qDebug()<< "Send to " << address << "Port: " <<m_udp_target;
}

void DeviceFinder::startSubnetScan()
{
    qDebug()<<"startSubnetScan-----------";
    QPair<QHostAddress, QHostAddress> ip_mask = NetworkUtils::getLocalIp();
    QHostAddress ipAddr = ip_mask.first;
    QHostAddress maskAddr = ip_mask.second;

    if (ipAddr.isNull() || maskAddr.isNull()) {
        qWarning() << "Failed to obtain valid network information";
        return;
    }

    quint32 ip = ipAddr.toIPv4Address();
    quint32 mask = maskAddr.toIPv4Address();

    quint32 network = ip & mask;
    quint32 broadcast = network | (~mask);
    for (quint32 addr = network + 1; addr < broadcast; ++addr) {
        QHostAddress targetAddr(addr);
        // 排除本机地址
        if (targetAddr != ipAddr) {
            scanTarget(targetAddr.toString());
        }
    }
}

void DeviceFinder::startListening()
{
    qDebug()<< "startListening";
    startTcpServer();
    startUdpListener();
}

void DeviceFinder::startTcpServer()
{
    qDebug()<< "startTcpServer";
    connect(tcpServer, &QTcpServer::newConnection, this, &DeviceFinder::handleTcpConnection);

    tcpServer->listen(QHostAddress::Any, m_tcp_listen);
}

void DeviceFinder::startUdpListener()
{
    qDebug() << "startUdpListener";
    udpSocket->bind(m_udp_listen);

    connect(udpSocket, &QUdpSocket::readyRead, this, [this]() {
        while(udpSocket->hasPendingDatagrams()) {
            QByteArray datagram;
            datagram.resize(udpSocket->pendingDatagramSize());

            QHostAddress sender;
            quint16 senderPort;
            udpSocket->readDatagram(datagram.data(), datagram.size(), &sender, &senderPort);
            qDebug() << "Udp client ip: " << sender << " port: " <<senderPort;
            qDebug() << "Udp received :" << datagram.data();
            if (datagram == HEARTBEAT) {
                udpSocket->writeDatagram(EXIT_MESSAGE, sender, senderPort);
                emit DeviceFinder::deviceFound(sender.toString());
            }
        }
    });
}

void DeviceFinder::handleTcpConnection()
{
    QTcpSocket *client = tcpServer->nextPendingConnection();
    qDebug() << "Tcp connection";
    isconnected=true;
    QTimer *heartbeatTimer = new QTimer(client);
    connect(client, &QTcpSocket::readyRead, [&, client, heartbeatTimer]() {
        heartbeatTimer->start(60000);
        QHostAddress clientAddr = QHostAddress(client->peerAddress().toIPv4Address());
        qDebug()<<"client ip: " <<clientAddr << " port: " << client->peerPort();
        QByteArray data = client->readAll();
        qDebug() << "TCP recevied message: " << data;
        if (data == HEARTBEAT) {
            client->write(EXIT_MESSAGE);
            emit DeviceFinder::deviceFound(client->peerAddress().toString());
        }

    });

    connect(heartbeatTimer, &QTimer::timeout, [client]() {
        client->close();
    });
}


// ****-------------------------------------------------****
ConnectionHandler::ConnectionHandler(QObject *parent)
    :QObject(parent)
{
    tcpServer = new QTcpServer(this);
    udpSocket = new QUdpSocket(this);
}

void ConnectionHandler::startListening()
{
    qDebug()<< "startListening";
    startTcpServer();
    startUdpListener();
}

void ConnectionHandler::startTcpServer()
{
    qDebug()<< "startTcpServer";
    connect(tcpServer, &QTcpServer::newConnection, this, &ConnectionHandler::handleTcpConnection);

    tcpServer->listen(QHostAddress::Any, m_tcp_listen);
}

void ConnectionHandler::startUdpListener()
{
    qDebug() << "startUdpListener";
    udpSocket->bind(m_udp_listen);

    connect(udpSocket, &QUdpSocket::readyRead, this, [this]() {
        while(udpSocket->hasPendingDatagrams()) {
            QByteArray datagram;
            datagram.resize(udpSocket->pendingDatagramSize());

            QHostAddress sender;
            quint16 senderPort;
            udpSocket->readDatagram(datagram.data(), datagram.size(), &sender, &senderPort);
            qDebug() << "Udp client ip: " << sender << " port: " <<senderPort;
            qDebug() << "Udp received :" << datagram.data();
            emit connectionSuccess();
            if (datagram == "heartbeat") {
                udpSocket->writeDatagram("heartbeat", sender, senderPort);
            }
        }
    });
}

void ConnectionHandler::handleTcpConnection()
{
    QTcpSocket *client = tcpServer->nextPendingConnection();
    qDebug() << "Tcp connection";
    QTimer *heartbeatTimer = new QTimer(client);
    emit connectionSuccess();
    connect(client, &QTcpSocket::readyRead, [client, heartbeatTimer]() {
        heartbeatTimer->start(60000);
        qDebug()<<"client ip: " <<client->peerAddress() << " port: " << client->peerPort();
        QByteArray data = client->readAll();
        qDebug() << "TCP recevied message: " << data;
        client->write("heartbeat");

    });

    connect(heartbeatTimer, &QTimer::timeout, [client]() {
        client->close();
    });
}



















