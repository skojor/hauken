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
    void newDataHandler();
    void setSamplesNeeded(int i) { samplesNeeded = i;}
    void restartTimeoutTimer() {}
    void setHeaderValidated(bool b) { headerValidated = b;}

signals:
    void newFfmCenterFrequency(double);
    void iqHeaderData(qint64, qint64, qint64);

private slots:
    quint32 calcStreamIdentifier();
    bool readHeader(const QByteArray &data);
    int parseDataPacket(const QByteArray &data);
    int parseContextPacket(const QByteArray &data);

private:
    int samplesNeeded;
    qint16 nrOfWords = 0, infClassCode = 0, packetClassCode = 0;
    quint32 readStreamId = 0;
    QList<complexInt16> iqSamples;
    bool headerValidated = false;

};

#endif // VIFSTREAMTCP_H
