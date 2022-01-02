#include "datastreambaseclass.h"

DataStreamBaseClass::DataStreamBaseClass(QObject *parent)
    : QObject{parent}
{
    connect(udpSocket, &QUdpSocket::stateChanged, this, &DataStreamBaseClass::connectionStateChanged);
    connect(udpSocket, &QUdpSocket::readyRead, this, &DataStreamBaseClass::newData);

    connect(tcpSocket, &QTcpSocket::stateChanged, this, &DataStreamBaseClass::connectionStateChanged);
    connect(tcpSocket, &QTcpSocket::readyRead, this, &DataStreamBaseClass::newData);

    connect(bytesPerSecTimer, &QTimer::timeout, this, &DataStreamBaseClass::calcBytesPerSecond);
    bytesPerSecTimer->setInterval(1000);

    connect(timeoutTimer, &QTimer::timeout, this, &DataStreamBaseClass::timeoutCallback);
}

void DataStreamBaseClass::processData(const QByteArray &buf)
{
    if (checkHeader(buf) && checkOptHeader(buf)) // checks passed, let's fill up our trace vector
        fillFft(buf);
}

bool DataStreamBaseClass::checkHeader(const QByteArray &buf)    // Reads the initial bytes of every packet and does a quick sanity check of the data
{
    QDataStream ds(buf);
    ds >> header.magicNumber >> header.versionMinor >> header.versionMajor >> header.seqNumber
            >> header.reserved >> header.dataSize;
    if (header.magicNumber != 0x000eb200) {
        qDebug() << "Packet magic number != 0x000eb200, not Rohde & Schwarz?";
        return false;
    }

    if (header.dataSize > 100000) {
        qDebug() << "Enormous header size" << header.dataSize;
        return false;
    }
    return true;
}


bool DataStreamBaseClass::checkOptHeader(const QByteArray &buf)
{
    QDataStream ds(buf);
    int skipBytes = header.size;
    ds.skipRawData(skipBytes);

    if (devicePtr->attrHeader)
        readAttrHeader(ds);

    if (devicePtr->mode == Instrument::Mode::PSCAN) {
        if ((devicePtr->optHeaderPr100 || devicePtr->optHeaderEb500) && attrHeader.optHeaderLength)
            readPscanOptHeader(ds);

        quint64 startFreq = (quint64)optHeaderPscanEb500.startFreqHigh << 32 | optHeaderPscanEb500.startFreqLow;
        quint64 stopFreq  = (quint64)optHeaderPscanEb500.stopFreqHigh << 32 | optHeaderPscanEb500.stopFreqLow;

        if (startFreq != devicePtr->pscanStartFrequency ||
                stopFreq != devicePtr->pscanStopFrequency) {
            qDebug() << "Start/stop frequency mismatch, ignoring data" << (long)startFreq - (long)devicePtr->pscanStartFrequency
                     << (long)stopFreq - (long)devicePtr->pscanStopFrequency;
            return false;
        }
        if (optHeaderPscanEb500.stepFreq != devicePtr->pscanResolution) {
            qDebug() << "PScan step frequency mismatch, ignoring data" << optHeaderPscanEb500.stepFreq << devicePtr->pscanResolution;
            return false;
        }
    }
    return true;
}

void DataStreamBaseClass::readAttrHeader(QDataStream &ds)
{
    ds >> attrHeader.tag >> attrHeader.length >> attrHeader.numItems >> attrHeader.channelNumber
            >> attrHeader.optHeaderLength >> attrHeader.selectorFlags;
}

void DataStreamBaseClass::readPscanOptHeader(QDataStream &ds)
{
    ds >> optHeaderPscanEb500.startFreqLow >> optHeaderPscanEb500.stopFreqLow
            >> optHeaderPscanEb500.stepFreq >> optHeaderPscanEb500.startFreqHigh
            >> optHeaderPscanEb500.stopFreqHigh >> optHeaderPscanEb500.reserved
            >> optHeaderPscanEb500.outputTimestamp;
    // not reading last values of eb500 specific optHeader, because they are not relevant anyway
}

void DataStreamBaseClass::fillFft(const QByteArray &buf)
{
    if (timeoutTimer->isActive()) timeoutTimer->start(timeoutInMs); // restart timer upon data arrival

    QDataStream ds(buf);
    int skipBytes = header.size;
    if (devicePtr->attrHeader) {
        skipBytes += attrHeader.size + attrHeader.optHeaderLength;
    }
    ds.skipRawData(skipBytes);

    qint16 data;
    if (devicePtr->mode == Instrument::Mode::PSCAN && attrHeader.tag == (int)Instrument::Tags::PSCAN) {
        while (!ds.atEnd())
        {
            ds >> data;
            if (data != 2000) fft.append(data);
            else {
                if (fft.size() >= calcPscanPointsPerTrace()) {
                    while (fft.size() > calcPscanPointsPerTrace())
                        fft.removeLast();
                    emit newFftData(fft);
                }
                fft.clear();
                traceCtr++;
                if (!traceTimer->isValid())
                    traceTimer->start();
            }
        }
    }
    else qDebug() << "Asked for pscan and got... what?" << attrHeader.tag;
    if (tcpBuffer.size() > 0) tcpBuffer.clear();
}

void DataStreamBaseClass::calcBytesPerSecond()
{
    bytesPerSecTimer->start();
    emit bytesPerSecond(byteCtr);
    byteCtr = 0;
    if (traceTimer->isValid()) {
        emit tracesPerSecond((double)traceCtr * 1e9 / traceTimer->nsecsElapsed());
        traceCtr = 0;
        traceTimer->start();
    }
}

int DataStreamBaseClass::calcPscanPointsPerTrace()
{
    return 1 + ((devicePtr->pscanStopFrequency - devicePtr->pscanStartFrequency) / devicePtr->pscanResolution);
}

void DataStreamBaseClass::timeoutCallback()
{
    closeListener();
    emit timeout();
}
