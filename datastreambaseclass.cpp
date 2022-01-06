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

    timeoutTimer->setSingleShot(true);
    timeoutTimer->setInterval(timeoutInMs);
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
        qDebug() << "Packet magic number != 0x000eb200, not Rohde & Schwarz?" << header.magicNumber << header.seqNumber << header.dataSize;
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
    else if (devicePtr->advProtocol)
        readAdvHeader(ds);

    if (devicePtr->mode == Instrument::Mode::PSCAN) {
        quint64 startFreq = 0, stopFreq = 0;
        quint32 stepFreq = 0;

        if (((devicePtr->optHeaderPr100 || devicePtr->optHeaderEb500) && attrHeader.optHeaderLength) ||
                (devicePtr->advProtocol && genAttrAdvHeader.optHeaderLength)) {
            readPscanOptHeader(ds);
            startFreq = (quint64)optHeaderPscanEb500.startFreqHigh << 32 | optHeaderPscanEb500.startFreqLow;
            stopFreq  = (quint64)optHeaderPscanEb500.stopFreqHigh << 32 | optHeaderPscanEb500.stopFreqLow;
            stepFreq  = optHeaderPscanEb500.stepFreq;
        }
        else if (devicePtr->optHeaderDscan && attrHeader.optHeaderLength) {
            readDscanOptHeader(ds);
            startFreq = esmbOptHeader.startFreq;
            stopFreq  = esmbOptHeader.stopFreq;
            stepFreq  = esmbOptHeader.stepFreq;
        }

        if (startFreq != devicePtr->pscanStartFrequency ||
                stopFreq != devicePtr->pscanStopFrequency ||
                stepFreq != devicePtr->pscanResolution) {
            qDebug() << "Freq/resolution mismatch" << (long)startFreq - (long)devicePtr->pscanStartFrequency
                     << (long)stopFreq - (long)devicePtr->pscanStopFrequency
                     << stepFreq << devicePtr->pscanResolution;
            errorCtr++;

            if (!errorHandleSent && errorCtr > 20) {
                emit streamErrorResetFreq();
                errorHandleSent = true;
            }

            return false;
        }
        else {
            errorCtr = 0;
            errorHandleSent = false;
        }
    }

    else if (devicePtr->mode == Instrument::Mode::FFM) {
        readIfpanOptHeader(ds);
        quint64 ffmFreq = (quint64)optHeaderIfPanEb500.freqHigh << 32 | optHeaderIfPanEb500.freqLow;
        if (ffmFreq != devicePtr->ffmCenterFrequency ||
                optHeaderIfPanEb500.freqSpan != devicePtr->ffmFrequencySpan) {
            qDebug() << "chuj" << ffmFreq << devicePtr->ffmCenterFrequency << optHeaderIfPanEb500.freqSpan << devicePtr->ffmFrequencySpan;
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

void DataStreamBaseClass::readIfpanOptHeader(QDataStream &ds)
{
    ds >> optHeaderIfPanEb500.freqLow >> optHeaderIfPanEb500.freqSpan >> optHeaderIfPanEb500.avgTime
            >> optHeaderIfPanEb500.avgType >> optHeaderIfPanEb500.measureTime >> optHeaderIfPanEb500.freqHigh
            >> optHeaderIfPanEb500.demodFreqChannel >> optHeaderIfPanEb500.demodFreqLow
            >> optHeaderIfPanEb500.demodFreqHigh >> optHeaderIfPanEb500.outputTimestamp;
}

void DataStreamBaseClass::readPscanOptHeader(QDataStream &ds)
{
    ds >> optHeaderPscanEb500.startFreqLow >> optHeaderPscanEb500.stopFreqLow
            >> optHeaderPscanEb500.stepFreq >> optHeaderPscanEb500.startFreqHigh
            >> optHeaderPscanEb500.stopFreqHigh >> optHeaderPscanEb500.reserved
            >> optHeaderPscanEb500.outputTimestamp;
    // not reading last values of eb500 specific optHeader, because they are not relevant anyway
}

void DataStreamBaseClass::readDscanOptHeader(QDataStream &ds)
{
    ds >> esmbOptHeader.startFreq >> esmbOptHeader.stopFreq >> esmbOptHeader.stepFreq
            >> esmbOptHeader.markFreq >> esmbOptHeader.bwZoom >> esmbOptHeader.refLevel;
}

void DataStreamBaseClass::readAdvHeader(QDataStream &ds)
{
    ds >> genAttrAdvHeader.tag >> genAttrAdvHeader.reserved >> genAttrAdvHeader.length
            >> genAttrAdvHeader.res1 >> genAttrAdvHeader.res2 >> genAttrAdvHeader.res3 >> genAttrAdvHeader.res4
            >> genAttrAdvHeader.numItems >> genAttrAdvHeader.channelNumber >> genAttrAdvHeader.optHeaderLength
            >> genAttrAdvHeader.selectorFlagsLow >> genAttrAdvHeader.selectorFlagsHigh
            >> genAttrAdvHeader.res5 >> genAttrAdvHeader.res6 >> genAttrAdvHeader.res7 >> genAttrAdvHeader.res8;
}

void DataStreamBaseClass::fillFft(const QByteArray &buf)
{
    if (timeoutTimer->isActive()) timeoutTimer->start(timeoutInMs); // restart timer upon data arrival

    QDataStream ds(buf);
    int skipBytes = header.size;
    if (devicePtr->attrHeader) {
        skipBytes += attrHeader.size + attrHeader.optHeaderLength;
    }
    else if (devicePtr->advProtocol)
        skipBytes += genAttrAdvHeader.size + genAttrAdvHeader.optHeaderLength;

    ds.skipRawData(skipBytes);

    qint16 data;
    if (devicePtr->mode == Instrument::Mode::PSCAN && (attrHeader.tag == (int)Instrument::Tags::PSCAN || genAttrAdvHeader.tag == (int)Instrument::Tags::ADVPSC)) {
        while (!ds.atEnd())
        {
            ds >> data;
            if (data > 2000) { // shouldn't happen, unless hell breaks loose. discard all the data
                fft.clear();
                qDebug() << "Dropped trace, values > 200 dBuV!";
                break;
            }
            else if (data != 2000) fft.append(data);
            else {
                if (fft.size() == calcPscanPointsPerTrace())
                    emit newFftData(fft);

                else if (fft.size() > calcPscanPointsPerTrace() && devicePtr->id.contains("USRP")) { // usrp exception, keep those data even if it's too much
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

    else if (devicePtr->mode == Instrument::Mode::FFM && attrHeader.tag == (int)Instrument::Tags::IFPAN) {
        qDebug() << "FFM it is";
    }

    else if (devicePtr->mode == Instrument::Mode::PSCAN && attrHeader.tag == (int)Instrument::Tags::DSCAN) {
        while (!ds.atEnd())
        {
            ds >> data;
            if (data != 2000) fft.append(data);
            else {
                if (fft.size() >= calcPscanPointsPerTrace()) {
                    if (fft.size() > calcPscanPointsPerTrace()) qDebug() << "stor pakke" << fft.size() << calcPscanPointsPerTrace();

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
