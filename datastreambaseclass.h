#ifndef DATASTREAMBASECLASS_H
#define DATASTREAMBASECLASS_H

/*
 * Base class for UDP and TCP datastream receivers. Collects trace data from receiver
 * and forwards to analyzis and display
 */

#include <QObject>
#include <QDataStream>
#include <QVector>
#include <QDebug>
#include <QAbstractSocket>
#include <QTcpSocket>
#include <QUdpSocket>
#include <QHostAddress>
#include <QByteArray>
#include <QSharedPointer>
#include <QTimer>
#include <QElapsedTimer>
#include <QNetworkDatagram>
#include <QFile>
#include <QApplication>
//#include <QtConcurrent/QtConcurrent>
#include <QFuture>
#include <QFutureWatcher>
#include <QPromise>
#include <QtEndian>
#include "typedefs.h"

class DataStreamBaseClass : public QObject
{
    Q_OBJECT
public:
    explicit DataStreamBaseClass(QObject *parent = nullptr);
    QList<qint16> fft;
    QHostAddress ownAddress;
    quint16 port = 0, udpPort = 5559;
    QTcpSocket *tcpSocket = new QTcpSocket;
    QUdpSocket *udpSocket = new QUdpSocket;
    int byteCtr = 0;
    QTimer *bytesPerSecTimer = new QTimer;
    QByteArray tcpBuffer, udpBuffer;
    QSharedPointer<Device> devicePtr;
    Eb200Header header;
    UdpDatagramAttribute attrHeader;
    EsmbOptHeaderDScan esmbOptHeader;
    OptHeaderPScan optHeaderPscan;
    OptHeaderPScanEB500 optHeaderPscanEb500;
    OptHeaderIfPanEB500 optHeaderIfPanEb500;
    GenAttrAdvanced genAttrAdvHeader;
    QTimer *timeoutTimer = new QTimer;
    QElapsedTimer *traceTimer = new QElapsedTimer;
    int errorCtr = 0;
    bool errorHandleSent = false;
    const int timeoutInMs = 10000;
    quint64 startfreq = 0, stopfreq = 0;
    unsigned int resolution = 0;
    int traceCtr = 0;
    QVector<QNetworkDatagram> ifBufferUdp;
    QByteArray ifBufferTcp;
    //QList<QByteArray> arrIfBufferTcp; // Multi recording buffer
    QVector<complexInt16> iq;
    quint16 sequenceNr = 0;
    bool waitingForPscanEndMarker = true;

public slots:
    virtual void openListener() = 0;
    virtual void openListener(const QHostAddress addr, const int port) = 0;
    virtual void closeListener() = 0;
    virtual void connectionStateChanged(QAbstractSocket::SocketState) = 0;
    void setOwnAddress(QHostAddress addr) { ownAddress = addr; }
    void setOwnPort(quint16 p) { port = p; }
    void setDeviceType(QSharedPointer<Device> p) { devicePtr = p; }
    quint16 getUdpPort() { return udpPort; }
    quint16 getTcpPort() { return tcpSocket->localPort(); }
    virtual void newDataHandler() = 0;
    void processData(const QByteArray &);
    bool readHeader(const QByteArray &);
    bool checkHeader();
    bool checkOptHeader(const QByteArray &);
    void readAttrHeader(QDataStream &ds);
    void readIfpanOptHeader(QDataStream &ds);
    void readPscanOptHeader(QDataStream &ds);
    void readDscanOptHeader(QDataStream &ds);
    void readAdvHeader(QDataStream &ds);
    void fillFft(const QByteArray &);
    void calcBytesPerSecond();
    int calcPscanPointsPerTrace();
    void timeoutCallback();
    void readGpscompassData(const QByteArray &buf);
    virtual void restartTimeoutTimer() = 0;

signals:
    void connectedState(bool);
    void newFftData(QVector<qint16> &);
    void bytesPerSecond(int);
    void tracesPerSecond(double);
    void timeout();
    void streamErrorResetFreq();
    void streamErrorResetConnection();
    void freqChanged(double, double);
    void resChanged(double);
    void newIqData(const QList<complexInt16>&);

private slots:

protected:

private:

};

#endif // DATASTREAMBASECLASS_H
