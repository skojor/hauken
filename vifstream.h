#ifndef VIFSTREAM_H
#define VIFSTREAM_H

#include "datastreambaseclass.h"


class VifStream : public DataStreamBaseClass
{
    Q_OBJECT
public:
    VifStream();
    void openListener() {}
    void openListener(const QHostAddress host, const int port);
    void closeListener();
    void connectionStateChanged(QAbstractSocket::SocketState);
    void newData();
    bool checkVifHeader(QNetworkDatagram datagram);
    void readIqData(QNetworkDatagram datagram);
    void processVifData();

signals:

private slots:

private:
    bool headerIsRead = false;
    int bytectr = 0;
    QVector<qint16> i;
    QVector<qint16> q;
};

#endif // VIFSTREAM_H
