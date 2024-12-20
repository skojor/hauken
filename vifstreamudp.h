#ifndef VIFSTREAMUDP_H
#define VIFSTREAMUDP_H

#include "datastreambaseclass.h"


class VifStreamUdp : public DataStreamBaseClass
{
    Q_OBJECT
public:
    VifStreamUdp();
    void openListener() {}
    void openListener(const QHostAddress host, const int port);
    void closeListener();
    void connectionStateChanged(QAbstractSocket::SocketState);
    void newDataHandler();
    bool checkVifHeader(QNetworkDatagram datagram);
    void readIqData(QNetworkDatagram datagram);
    void processVifData();
    void restartTimeoutTimer() {}

signals:

private slots:

private:
    bool headerIsRead = false;
    int bytectr = 0;
    QList<qint16> i;
    QList<qint16> q;
};

#endif // VIFSTREAMUDP_H
