#ifndef TCPDATASTREAM_H
#define TCPDATASTREAM_H

#include "datastreambaseclass.h"

class TcpDataStream : public DataStreamBaseClass
{
    Q_OBJECT
public:
    TcpDataStream();

public slots:
    void openListener() {}
    void openListener(const QHostAddress host, const int port);
    void closeListener();
    void connectionStateChanged(QAbstractSocket::SocketState);
    void newData();
    void restartTimeoutTimer() {  { if (tcpSocket->isOpen()) timeoutTimer->start(timeoutInMs);}}

signals:

private:
    bool headerIsRead = false;

};

#endif // TCPDATASTREAM_H
