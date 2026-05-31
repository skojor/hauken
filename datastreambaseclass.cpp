#include "datastreambaseclass.h"
#include <QtEndian>

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
    if (attrHeader.tag == (int)Instrument::Tags::AUDIO)
        emit newAudioData(buf);
    else if (attrHeader.tag == (int)Instrument::Tags::IF)
        emit newIfData(buf);
    else if (attrHeader.tag == (int)Instrument::Tags::PSCAN or attrHeader.tag == (int)Instrument::Tags::ADVPSC)
        emit newPscanData(buf);
    else if (attrHeader.tag == (int)Instrument::Tags::IFPAN or attrHeader.tag == (int)Instrument::Tags::ADVIFP)
        emit newIfPanData(buf);
    else if (attrHeader.tag == (int)Instrument::Tags::GPSC)
        emit newGpsCompassData(buf);
    else if (attrHeader.tag == (int)Instrument::Tags::CW or attrHeader.tag == (int)Instrument::Tags::ADVCW)
        emit newCwData(buf);
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

HeaderType DataStreamBaseClass::readHeadersSimplified(const QByteArray &buf)
{
    int loc = locateEb200Header(buf);
    if (loc > -1 and buf.size() > loc + 17) {
        memcpy(&header.magicNumber, buf.constData() + loc, sizeof(header.magicNumber));
        header.magicNumber = qToBigEndian(header.magicNumber);
        memcpy(&header.seqNumber, buf.constData() + loc + 8, sizeof(header.seqNumber));
        header.seqNumber = qToBigEndian(header.seqNumber);
        memcpy(&header.dataSize, buf.constData() + loc + 12, sizeof(header.dataSize));
        header.dataSize = qToBigEndian(header.dataSize);
        memcpy(&attrHeader.tag, buf.constData() + loc + 16, sizeof(attrHeader.tag));
        attrHeader.tag = qToBigEndian(attrHeader.tag);
        return HeaderType::EB200;
    }
    loc = locateAmmosHeader(buf);
    if (loc > -1 and buf.size() > loc + 26 * 4) {
        memcpy(&ammosHeader, buf.constData() + loc, sizeof(ammosHeader));
        return HeaderType::AMMOS;
    }
    loc = locateAmmosHeaderInv(buf);
    if (loc > -1 and buf.size() > loc + 26 * 4) {
        memcpy(&ammosHeader, buf.constData() + loc, sizeof(ammosHeader));
        swapAmmosHeader();
        return HeaderType::AMMOSINV;
    }
    return HeaderType::UNKNOWN;
}

bool DataStreamBaseClass::readHeaderVita(const QByteArray &buf)
{
    quint16 infClassCode, packetClassCode;
    memcpy(&vitaHeader, buf.constData(), sizeof(vitaHeader));

    if (qToBigEndian(vitaHeader.infClassCode) == 1
        && (qToBigEndian(vitaHeader.packetClassCode) == 1 || qToBigEndian(vitaHeader.packetClassCode) == 2)) {
        return true;
    }
    return false;
}

int DataStreamBaseClass::locateEb200Header(const QByteArray &buf)
{
    QByteArray be;
    be.append(char(0x00));
    be.append(char(0x0e));
    be.append(char(0xb2));
    be.append(char(0x00));
    return buf.indexOf(be);
}

int DataStreamBaseClass::locateVitaHeader(const QByteArray &buf)
{
    return -1;
}

int DataStreamBaseClass::locateAmmosHeader(const QByteArray &buf)
{
    QByteArray pattern;
    pattern.append(char(0x72));
    pattern.append(char(0x65));
    pattern.append(char(0x74));
    pattern.append(char(0xfb));
    //qDebug() << buf.first(4).toHex(' ' );
    return buf.indexOf(pattern);
}

int DataStreamBaseClass::locateAmmosHeaderInv(const QByteArray &buf)
{
    QByteArray pattern;
    pattern.append(char(0xfb));
    pattern.append(char(0x74));
    pattern.append(char(0x65));
    pattern.append(char(0x72));
    //qDebug() << buf.first(4).toHex(' ' );
    return buf.indexOf(pattern);
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

void DataStreamBaseClass::swapAmmosHeader()
{
    ammosHeader.magicWord = qbswap(ammosHeader.magicWord);
    ammosHeader.frameLength = qbswap(ammosHeader.frameLength);
    ammosHeader.frameCount = qbswap(ammosHeader.frameCount);
    ammosHeader.frameType = qbswap(ammosHeader.frameType);
    ammosHeader.dataHeaderLength = qbswap(ammosHeader.dataHeaderLength);
    ammosHeader.reserved = qbswap(ammosHeader.reserved);
    ammosHeader.datablockCount = qbswap(ammosHeader.datablockCount);
    ammosHeader.datablockLength = qbswap(ammosHeader.dataHeaderLength);
    ammosHeader.timetampLow = qbswap(ammosHeader.timetampLow);
    ammosHeader.timestampHigh = qbswap(ammosHeader.timestampHigh);
    ammosHeader.statusWord = qbswap(ammosHeader.statusWord);
    ammosHeader.signalSourceId = qbswap(ammosHeader.signalSourceId);
    ammosHeader.signalSourceState = qbswap(ammosHeader.signalSourceState);
    ammosHeader.freqLow = qbswap(ammosHeader.freqLow);
    ammosHeader.freqHigh = qbswap(ammosHeader.freqHigh);
    ammosHeader.bandwidth = qbswap(ammosHeader.bandwidth);
    ammosHeader.samplerate = qbswap(ammosHeader.samplerate);
    ammosHeader.interpolation = qbswap(ammosHeader.interpolation);
    ammosHeader.decimation = qbswap(ammosHeader.decimation);
    ammosHeader.intAntennaVoltageRef = qbswap(ammosHeader.intAntennaVoltageRef);
    ammosHeader.startTimestampLow = qbswap(ammosHeader.startTimestampHigh);
    ammosHeader.startTimestampHigh = qbswap(ammosHeader.startTimestampLow);
    ammosHeader.sampleCounterLow = qbswap(ammosHeader.sampleCounterLow);
    ammosHeader.sampleCounterHigh = qbswap(ammosHeader.sampleCounterHigh);
    ammosHeader.kFactor = qbswap(ammosHeader.kFactor);
    ammosHeader.datablockStatus = qbswap(ammosHeader.datablockStatus);

}
