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
#include "typedefs.h"


class DataStreamBaseClass : public QObject
{
    Q_OBJECT
public:
    explicit DataStreamBaseClass(QObject *parent = nullptr);
    QVector<qint16>fft;
    QHostAddress ownAddress;
    quint16 port = 0, udpPort;
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
    int traceCtr = 0;
    int errorCtr = 0;
    bool errorHandleSent = false;
    const int timeoutInMs = 20000;

public slots:
    virtual void openListener() = 0;
    virtual void openListener(const QHostAddress addr, const int port) = 0;
    virtual void closeListener() = 0;
    virtual void connectionStateChanged(QAbstractSocket::SocketState) = 0;
    void setOwnAddress(QHostAddress addr) { ownAddress = addr; }
    void setOwnPort(quint16 p) { port = p; }
    void setDeviceType(QSharedPointer<Device> p) { devicePtr = p; }
    quint16 getUdpPort() { return udpSocket->localPort(); }
    quint16 getTcpPort() { return tcpSocket->localPort(); }
    virtual void newData() = 0;
    void processData(const QByteArray &);
    bool checkHeader(const QByteArray &);
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

signals:
    void connectedState(bool);
    void newFftData(QVector<qint16> &);
    void bytesPerSecond(int);
    void tracesPerSecond(double);
    void timeout();
    void streamErrorResetFreq();

private slots:

protected:

private:
};

#endif // DATASTREAMBASECLASS_H
