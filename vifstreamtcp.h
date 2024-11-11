#ifndef VIFSTREAMTCP_H
#define VIFSTREAMTCP_H

#include "datastreambaseclass.h"


class VifStreamTcp : public DataStreamBaseClass
{
    Q_OBJECT
public:
    VifStreamTcp();
    void openListener() {}
    void openListener(const QHostAddress host, const int port);
    void closeListener();
    void connectionStateChanged(QAbstractSocket::SocketState);
    void newData();
    bool checkVifHeader(QByteArray data);
    void readIqData(QByteArray data);
    void processVifData();

signals:

private slots:

private:
    bool headerIsRead = false;
    int bytectr = 0;
    QVector<qint16> i;
    QVector<qint16> q;

};

#endif // VIFSTREAMTCP_H
