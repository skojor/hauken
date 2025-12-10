#ifndef TCPDATASTREAM_H
#define TCPDATASTREAM_H

#include "datastreambaseclass.h"
#include <QMutex>

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
    void newDataHandler();
    void restartTimeoutTimer() { timeoutTimer->stop();}

signals:

private:
    bool headerIsRead = false;
    QMutex mutex;
};

#endif // TCPDATASTREAM_H
