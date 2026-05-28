#ifndef VIFSTREAMTCP_H
#define VIFSTREAMTCP_H

#include "datastreambaseclass.h"

/*
 * Class controlling its own tcp socket, to receive Vita 49 data.
 * Rebuilt to handle AMMOS formatted IF data.
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
    void setSamplesNeeded(int i) { samplesNeeded = i;}
    void restartTimeoutTimer() {}
    void setHeaderValidated(bool b) { headerValidated = b;}
    void invalidateHeader() { m_freq = m_bw = m_samplerate = 0;}
    bool isOpen() { return tcpSocket->isOpen();}

signals:
    void newFfmCenterFrequency(double);
    void iqHeaderData(qint64, qint64, qint64);
    void ifDataReady(const QVector<complexInt16>);
    void headerChanged(quint64, quint32, quint32, quint64);


private slots:
    quint32 calcStreamIdentifier();
    bool readHeader(const QByteArray &data);
    int parseDataPacket(const QByteArray &data);
    int parseContextPacket(const QByteArray &data);
    void readIfData();

private:
    int samplesNeeded;
    qint16 nrOfWords = 0, infClassCode = 0, packetClassCode = 0;
    quint32 readStreamId = 0;
    QVector<complexInt16> iqSamples;
    bool headerValidated = false;
    bool headerIsRead = false;
    quint64 m_sampleCtr = 0;
    quint64 m_freq = 0;
    quint32 m_bw = 0, m_samplerate = 0;

};

#endif // VIFSTREAMTCP_H
