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
    if (checkOptHeader(buf)) {
        if (attrHeader.tag == (int) Instrument::Tags::GPSC
            || genAttrAdvHeader.tag == (int) Instrument::Tags::GPSC)
            readGpscompassData(buf);
        else {
            fillFft(buf);
        }
    }
    //tcpBuffer.clear();
}

bool DataStreamBaseClass::readHeader(const QByteArray &buf)
{
    if (buf.size() > 20) {
        QDataStream ds(buf);
        ds >> header.magicNumber >> header.versionMinor >> header.versionMajor >> header.seqNumber
            >> header.reserved >> header.dataSize;
        return true;
    } else
        return false;
}

bool DataStreamBaseClass::
    checkHeader() // Reads the initial bytes of every packet and does a quick sanity check of the data
{
    if (header.magicNumber != 0x000eb200) {
        return false;
    }

    else if (header.dataSize > 100000) {
        qDebug() << "Enormous header size" << header.dataSize;
        return false;
    } else {
        return true;
    }
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

    if (devicePtr->mode == Instrument::Mode::PSCAN
        && (attrHeader.tag != (int) Instrument::Tags::GPSC
            && genAttrAdvHeader.tag != (int) Instrument::Tags::GPSC)) { // skip this for gpsc
        quint64 startFreq = 0, stopFreq = 0;
        quint32 stepFreq = 0;

        if (((devicePtr->optHeaderPr100 || devicePtr->optHeaderEb500) && attrHeader.optHeaderLength)
            || (devicePtr->advProtocol && genAttrAdvHeader.optHeaderLength)) {
            readPscanOptHeader(ds);
            startFreq = (quint64) optHeaderPscanEb500.startFreqHigh << 32
                        | optHeaderPscanEb500.startFreqLow;
            stopFreq = (quint64) optHeaderPscanEb500.stopFreqHigh << 32
                       | optHeaderPscanEb500.stopFreqLow;
            stepFreq = optHeaderPscanEb500.stepFreq;
        } else if (devicePtr->optHeaderDscan && attrHeader.optHeaderLength) {
            readDscanOptHeader(ds);
            startFreq = esmbOptHeader.startFreq;
            stopFreq = esmbOptHeader.stopFreq;
            stepFreq = esmbOptHeader.stepFreq;
        }

        if (startFreq != devicePtr->pscanStartFrequency || stopFreq != devicePtr->pscanStopFrequency
            || stepFreq != devicePtr->pscanResolution) {
            /*qDebug() << "Freq/resolution mismatch" << (long)startFreq - (long)devicePtr->pscanStartFrequency
                     << (long)stopFreq - (long)devicePtr->pscanStopFrequency
                     << stepFreq << devicePtr->pscanResolution << genAttrAdvHeader.tag;*/
            errorCtr++;

            if (!errorHandleSent && errorCtr > 100) {
                emit streamErrorResetFreq();
                errorHandleSent = true;
            }

            return false;
        } else {
            if (startFreq != startfreq || stopFreq != stopfreq) {
                startfreq = startFreq;
                stopfreq = stopFreq;
                emit freqChanged(startfreq, stopfreq);
            }
            if (resolution != stepFreq) {
                resolution = stepFreq;
                emit resChanged(resolution);
            }
            errorCtr = 0;
            errorHandleSent = false;
        }
    }

    else if (devicePtr->mode == Instrument::Mode::FFM
             && attrHeader.tag != (int) Instrument::Tags::GPSC
             && genAttrAdvHeader.tag != (int) Instrument::Tags::GPSC) {
        if (attrHeader.tag == (int) Instrument::Tags::IFPAN
            || genAttrAdvHeader.tag == (int) Instrument::Tags::ADVIFP)
            readIfpanOptHeader(ds); // Same for old and new protocol
        // TODO: Error checks
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
        >> optHeaderIfPanEb500.avgType >> optHeaderIfPanEb500.measureTime
        >> optHeaderIfPanEb500.freqHigh >> optHeaderIfPanEb500.demodFreqChannel
        >> optHeaderIfPanEb500.demodFreqLow >> optHeaderIfPanEb500.demodFreqHigh
        >> optHeaderIfPanEb500.outputTimestamp;
    unsigned long tmpStart = (quint64) optHeaderIfPanEb500.freqHigh << 32
                             | optHeaderIfPanEb500.freqLow - optHeaderIfPanEb500.freqSpan / 2;
    unsigned long tmpStop = tmpStart + optHeaderIfPanEb500.freqSpan;
    if (startfreq != tmpStart || stopfreq != tmpStop) {
        startfreq = tmpStart;
        stopfreq = tmpStop;
        emit freqChanged(startfreq, stopfreq);
    }

    if (attrHeader.tag == (int) Instrument::Tags::IFPAN) { // Old protocol
        if (resolution != (int) (optHeaderIfPanEb500.freqSpan / (attrHeader.numItems - 1))) {
            resolution = optHeaderIfPanEb500.freqSpan / (attrHeader.numItems - 1);
            emit resChanged(resolution);
        }
    } else if (genAttrAdvHeader.tag == (int) Instrument::Tags::ADVIFP) { // New protocol
        if (resolution != (int) (optHeaderIfPanEb500.freqSpan / (genAttrAdvHeader.numItems - 1))) {
            resolution = optHeaderIfPanEb500.freqSpan / (genAttrAdvHeader.numItems - 1);
            emit resChanged(resolution);
            emit freqChanged(startfreq, stopfreq);
        }
    }
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
        >> genAttrAdvHeader.res1 >> genAttrAdvHeader.res2 >> genAttrAdvHeader.res3
        >> genAttrAdvHeader.res4 >> genAttrAdvHeader.numItems >> genAttrAdvHeader.channelNumber
        >> genAttrAdvHeader.optHeaderLength >> genAttrAdvHeader.selectorFlagsLow
        >> genAttrAdvHeader.selectorFlagsHigh >> genAttrAdvHeader.res5 >> genAttrAdvHeader.res6
        >> genAttrAdvHeader.res7 >> genAttrAdvHeader.res8;
}

void DataStreamBaseClass::fillFft(const QByteArray &buf)
{
    if (timeoutTimer->isActive())
        timeoutTimer->start(timeoutInMs); // restart timer upon data arrival

    QDataStream ds(buf);
    int skipBytes = header.size;
    if (devicePtr->attrHeader) {
        skipBytes += attrHeader.size + attrHeader.optHeaderLength;
    } else if (devicePtr->advProtocol)
        skipBytes += genAttrAdvHeader.size + genAttrAdvHeader.optHeaderLength;

    ds.skipRawData(skipBytes);

    qint16 data;
    if (devicePtr->mode == Instrument::Mode::PSCAN
        && (attrHeader.tag == (int) Instrument::Tags::PSCAN
            || genAttrAdvHeader.tag == (int) Instrument::Tags::ADVPSC)) {
        /*while (!ds.atEnd())
        {*/
        QList<qint16> tmpBuffer;
        tmpBuffer.resize((buf.size() - skipBytes) / 2);
        int readBytes = ds.readRawData((char *) tmpBuffer.data(), tmpBuffer.size() * 2);
        if (readBytes == tmpBuffer.size() * 2) {
            for (auto &val : tmpBuffer) // readRaw makes a mess out of byte order. Reorder manually
                val = qToBigEndian(val);

            fft.append(tmpBuffer);
            /*ds >> data;
            if (data > 2000) { // shouldn't happen, unless hell breaks loose. discard all the data
                fft.clear();
                //qDebug() << "Dropped trace, values > 200 dBuV!";
                break;
            }
            else if (data != 2000) fft.append(data);
            else {*/
        }
        if (fft.last() == 2000) {
            fft.removeLast();

            if (fft.size() == calcPscanPointsPerTrace())
                emit newFftData(fft);

            else if (fft.size() > calcPscanPointsPerTrace()
                     && (devicePtr->id.contains("USRP") || devicePtr->id.contains("EM100")
                         || devicePtr->id.contains(
                             "PR100"))) { // usrp/em100/pr100 exception, keep those data even if it's too much

                while (fft.size() > calcPscanPointsPerTrace())
                    fft.removeLast();
                emit newFftData(fft);
            }

            fft.clear();
            traceCtr++;
            if (traceTimer->isValid() && traceCtr >= 10) {
                emit tracesPerSecond(1e10 / traceTimer->nsecsElapsed());
                traceCtr = 0;
                traceTimer->start();
            }
            if (!traceTimer->isValid())
                traceTimer->start();
        }
    }

    else if (devicePtr->mode == Instrument::Mode::FFM
             && (attrHeader.tag == (int) Instrument::Tags::IFPAN
                 || genAttrAdvHeader.tag == (int) Instrument::Tags::ADVIFP)) {
        QList<qint16> tmpBuffer;
        int packetCtr;
        if (devicePtr->advProtocol)
            packetCtr = genAttrAdvHeader.numItems;
        else
            packetCtr = attrHeader.numItems;
        tmpBuffer.resize(packetCtr);
        int readBytes = ds.readRawData((char *) tmpBuffer.data(), packetCtr * 2);
        if (readBytes != packetCtr * 2) {
            qWarning() << "Data failed to copy, aborting" << tmpBuffer.size() << buf.size()
                       << readBytes << attrHeader.numItems << genAttrAdvHeader.numItems;
        } else {
            for (auto &val : tmpBuffer) // readRaw makes a mess out of byte order. Reorder manually
                val = qToBigEndian(val);
            emit newFftData(tmpBuffer);

            traceCtr++;
            if (traceTimer->isValid() && traceCtr >= 100) {
                emit tracesPerSecond(1e11 / traceTimer->nsecsElapsed());
                traceCtr = 0;
                traceTimer->start();
            }
            if (!traceTimer->isValid())
                traceTimer->start();
        }
    }

    else if (devicePtr->mode == Instrument::Mode::PSCAN
             && attrHeader.tag == (int) Instrument::Tags::DSCAN) {
        while (!ds.atEnd()) {
            ds >> data;
            if (data != 2000)
                fft.append(data);
            else {
                if (fft.size() >= calcPscanPointsPerTrace()) {
                    //if (fft.size() > calcPscanPointsPerTrace()) qDebug() << "Stream debug" << fft.size() << calcPscanPointsPerTrace();

                    while (fft.size() > calcPscanPointsPerTrace())
                        fft.removeLast();
                    emit newFftData(fft);
                }
                fft.clear();
                if (traceTimer->isValid())
                    emit tracesPerSecond(1e9 / traceTimer->nsecsElapsed());
                traceTimer->start();
            }
        }
    }
}

void DataStreamBaseClass::calcBytesPerSecond()
{
    bytesPerSecTimer->start();
    emit bytesPerSecond(byteCtr);
    byteCtr = 0;
}

int DataStreamBaseClass::calcPscanPointsPerTrace()
{
    return 1
           + ((devicePtr->pscanStopFrequency - devicePtr->pscanStartFrequency)
              / devicePtr->pscanResolution);
}

void DataStreamBaseClass::timeoutCallback()
{
    closeListener();
    emit timeout();
}

void DataStreamBaseClass::readGpscompassData(const QByteArray &buf)
{
    QDataStream ds(buf);
    qint16 gpsValid = 0, heading = 0, sats = 0, latRef = 0, latDeg = 0, lonRef = 0, lonDeg = 0;
    float latMin = 0, lonMin = 0, dilution = 0;
    qint32 altitude = 0;
    qint16 sog = 0, cog = 0;
    quint64 gnssTimestamp = 0;

    ds.setFloatingPointPrecision(QDataStream::SinglePrecision);
    if (ds.skipRawData(36) == -1)
        qDebug() << "GNSS tag, error skipping 36 bytes";
    else
        ds >> heading;
    if (ds.skipRawData(2) == -1)
        qDebug() << "GNSS tag, error skipping 2 bytes";
    else
        ds >> gpsValid >> sats >> latRef >> latDeg >> latMin >> lonRef >> lonDeg >> lonMin
            >> dilution;
    if (ds.skipRawData(24) == -1)
        qDebug() << "GNSS tag, error skipping 24 bytes";
    else
        ds >> altitude >> sog >> cog;
    if (ds.skipRawData(8) == -1)
        qDebug() << "GNSS tag, error skipping 8 bytes";
    else
        ds >> gnssTimestamp;

    //qDebug() << (float)heading / 10.0 << gpsValid << sats << latRef << latDeg << latMin << lonRef << lonDeg << lonMin << dilution;

    if (ds.status() == QDataStream::Ok) {
        //gnss.altitude = static_cast<double>(altitude) / 100.0;
        //gnss.dop = dilution;
        double lat = latDeg + (latMin * 0.0166666667);
        if (latRef == 'S')
            lat *= -1;
        double lng = lonDeg + (lonMin * 0.0166666667);
        if (lonRef == 'W')
            lng *= -1;
        if (lat != 0 && lng != 0) {
            devicePtr->latitude = lat;
            devicePtr->longitude = lng;
            devicePtr->altitude = altitude;
            devicePtr->dop = dilution;
            devicePtr->sog = (float) ((sog) / 10.0) * 1.94384449;
            devicePtr->cog = (float) cog;
            devicePtr->gnssTimestamp = QDateTime::fromMSecsSinceEpoch(gnssTimestamp / 1e6);
            devicePtr->sats = sats;
        }
        if (gpsValid > 0)
            devicePtr->positionValid = true;
        else
            devicePtr->positionValid = false;
        //qDebug() << "GPS debug" << gpsValid << lat << lng;
    }
}
