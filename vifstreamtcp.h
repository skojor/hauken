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
    void parseVifData(const QByteArray &data);
    void processVifData();
    void setSamplesNeeded(int i) { samplesNeeded = i;}
    void startIqDataTimeout() { stopIqStreamTimer->start(8000); }
    void restartTimeoutTimer() {}
    void setRecordingMultipleBands(bool b) { recordingMultipleBands = b;}
    void setMultipleFfmCenterFreqs(const QList<double> l) { multipleFfmCenterFreqs = l;}

signals:
    void newFfmCenterFrequency(double);

private slots:
    quint32 calcStreamIdentifier();

private:
    bool headerIsRead = false;
    int bytectr = 0;
    int samplesNeeded;
    QTimer *stopIqStreamTimer;
    bool recordingMultipleBands = false;
    QList<double> multipleFfmCenterFreqs;
};

#endif // VIFSTREAMTCP_H
