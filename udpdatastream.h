#ifndef UDPDATASTREAM_H
#define UDPDATASTREAM_H

#include "datastreambaseclass.h"

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
    void restartTimeoutTimer() {  { if (udpSocket->isOpen()) timeoutTimer->start(timeoutInMs);}}

signals:

private:
    bool headerIsRead = false;

};

#endif // UDPDATASTREAM_H
