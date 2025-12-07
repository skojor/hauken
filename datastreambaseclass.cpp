#include "datastreambaseclass.h"

DataStreamBaseClass::DataStreamBaseClass(QObject *parent)
    : QObject{parent}
{
    connect(udpSocket,
            &QUdpSocket::stateChanged,
            this,
            &DataStreamBaseClass::connectionStateChanged);
    connect(udpSocket, &QUdpSocket::readyRead, this, &DataStreamBaseClass::newDataHandler);

    connect(tcpSocket,
            &QTcpSocket::stateChanged,
            this,
            &DataStreamBaseClass::connectionStateChanged);
    connect(tcpSocket, &QTcpSocket::readyRead, this, &DataStreamBaseClass::newDataHandler);

    connect(bytesPerSecTimer, &QTimer::timeout, this, &DataStreamBaseClass::calcBytesPerSecond);
    bytesPerSecTimer->setInterval(1000);

    timeoutTimer->setSingleShot(true);
    timeoutTimer->setInterval(timeoutInMs);
    connect(timeoutTimer, &QTimer::timeout, this, &DataStreamBaseClass::timeoutCallback);
}

void DataStreamBaseClass::processData(const QByteArray &buf)
{
        if (attrHeader.tag == (int)Instrument::Tags::AUDIO or attrHeader.tag == (int)Instrument::Tags::AUDIO)
            emit newAudioData(buf);
        else if (attrHeader.tag == (int)Instrument::Tags::IF)
            emit newIfData(buf);
        else if (attrHeader.tag == (int)Instrument::Tags::PSCAN or attrHeader.tag == (int)Instrument::Tags::ADVPSC)
            emit newPscanData(buf);
        else if (attrHeader.tag == (int)Instrument::Tags::IFPAN or attrHeader.tag == (int)Instrument::Tags::ADVIFP)
            emit newIfPanData(buf);
        else if (attrHeader.tag == (int)Instrument::Tags::GPSC)
            emit newGpsCompassData(buf);
        else
            qDebug() << "Unknown tag:" << attrHeader.tag;
}

bool DataStreamBaseClass::readHeaders(const QByteArray &buf)
{
    QDataStream ds(buf);
    header.readData(ds);
    attrHeader.readData(ds);
    if (!ds.atEnd() and header.magicNumber == 0x000eb200)
        return true;
    return false;
}

void DataStreamBaseClass::readDscanOptHeader(QDataStream &ds)
{
    ds >> esmbOptHeader.startFreq >> esmbOptHeader.stopFreq >> esmbOptHeader.stepFreq
        >> esmbOptHeader.markFreq >> esmbOptHeader.bwZoom >> esmbOptHeader.refLevel;
}


void DataStreamBaseClass::timeoutCallback()
{
    closeListener();
    emit timeout();
}

void DataStreamBaseClass::calcBytesPerSecond()
{
    bytesPerSecTimer->start();
    emit bytesPerSecond(byteCtr);
    byteCtr = 0;
}
