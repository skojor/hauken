#ifndef VIFSTREAMTCP_H
#define VIFSTREAMTCP_H

#include "datastreambaseclass.h"

/*
 * TCP socket receiver for AMMOS formatted IF data.
 * Protocol parsing is handled by DatastreamAmmos.
 */

class VifStreamTcp : public DataStreamBaseClass
{
    Q_OBJECT
public:
    VifStreamTcp();
    void openListener() {}
    void openListener(const QHostAddress host, const int port);
    void closeListener();
    void connectionStateChanged(QAbstractSocket::SocketState);
    void newDataHandler();
    void restartTimeoutTimer() {}
    void invalidateHeader() { emit headerInvalidated(); }
    bool isOpen() { return tcpSocket->isOpen(); }

signals:
    void headerInvalidated();
};

#endif // VIFSTREAMTCP_H
