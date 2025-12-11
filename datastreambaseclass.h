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
    QHostAddress ownAddress;
    quint16 port = 0, udpPort = 5559;
    QTcpSocket *tcpSocket = new QTcpSocket;
    QUdpSocket *udpSocket = new QUdpSocket;
    int byteCtr = 0;
    QTimer *bytesPerSecTimer = new QTimer;
    QByteArray tcpBuffer, udpBuffer;
    QSharedPointer<Device> devicePtr;
    Eb200Header header;
    AttrHeaderCombined attrHeader;
    EsmbOptHeaderDScan esmbOptHeader; // FIX
    QTimer *timeoutTimer = new QTimer;
    QElapsedTimer *traceTimer = new QElapsedTimer;
    const int timeoutInMs = 10000;
    QVector<QNetworkDatagram> ifBufferUdp;
    QByteArray ifBufferTcp;
    QVector<complexInt16> iq;
    quint16 sequenceNr = 0;

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
    bool readHeaders(const QByteArray &buf);
    bool readHeadersSimplified(const QByteArray &buf);
    void readDscanOptHeader(QDataStream &ds); //FIX
    void timeoutCallback();
    void readGpscompassData(const QByteArray &buf);
    virtual void restartTimeoutTimer() = 0;
    void calcBytesPerSecond();

signals:
    void connectedState(bool);
    void timeout();
    void newIqData(const QVector<complexInt16>&);
    void newAudioData(const QByteArray &);
    void newIfData(const QByteArray &);
    void newPscanData(const QByteArray &);
    void newIfPanData(const QByteArray &);
    void newGpsCompassData(const QByteArray &);
    void newCwData(const QByteArray &);
    void waitForPscanEndMarker(bool);
    void bytesPerSecond(int);

private:

};

#endif // DATASTREAMBASECLASS_H
