#ifndef UDPDATASTREAM_H
#define UDPDATASTREAM_H

#include "datastreambaseclass.h"

class UdpPacket {
public:
    UdpPacket(int i, QByteArray d)
    {
        seqNumber = i;
        data = d;
    }
    int seqNumber;
    QByteArray data;
};

class UdpDataStream : public DataStreamBaseClass
{
    Q_OBJECT
public:
    UdpDataStream();

public slots:
    void openListener();
    void openListener(const QHostAddress, const int) {}
    void closeListener();
    void connectionStateChanged(QAbstractSocket::SocketState state);
    void newDataHandler();
    void restartTimeoutTimer() { timeoutTimer->stop();}

signals:

private:
    bool headerIsRead = false;
    QList<UdpPacket> udpPacketList;
};

#endif // UDPDATASTREAM_H
