#include "gnssdevice.h"

GnssDevice::GnssDevice(QSharedPointer<Config> c, int id)
{
    config = c;
    gnssData.id = id;

    connect(gnss, &QSerialPort::readyRead, this, [this] {
        QByteArray buffer = gnss->readAll();
        gnssBuffer.append(buffer);
        if (logToFile) appendToLogfile(buffer);
        handleBuffer();
    });

    connect(sendToAnalyzerTimer, &QTimer::timeout, this, [this]
            {
                if (this->gnss->isOpen() || tcpSocket->isOpen()) emit analyzeThisData(gnssData);
                checkPosValid(); // run once per sec
            });

    connect(tcpSocket, &QTcpSocket::readyRead, this, [this] {
        QByteArray buffer = tcpSocket->readAll();
        gnssBuffer.append(buffer);
        if (logToFile) appendToLogfile(buffer);
        handleBuffer();
    });

    connect(tcpSocket, &QTcpSocket::stateChanged, this, &GnssDevice::handleTcpStateChange);
    connect(delayedReportTimer, &QTimer::timeout, this, &GnssDevice::delayedReportHandler);
}

void GnssDevice::start()
{
    if (((gnssData.id == 1 && config->getGnssSerialPort1Activate()) || (gnssData.id == 2 && config->getGnssSerialPort2Activate())) &&
        (!gnss->isOpen() || tcpSocket->state() == QAbstractSocket::UnconnectedState)) connectToPort();
    sendToAnalyzerTimer->stop();

    if (gnss->isOpen() || tcpSocket->state() != QAbstractSocket::UnconnectedState) {
        sendToAnalyzerTimer->start(1000);
    }
}

void GnssDevice::connectToPort()
{
    //qDebug() << "connectToPort called";
    QSerialPortInfo portInfo;

    if (activate && !portName.isEmpty() && !baudrate.isEmpty()) {
        for (auto &val : QSerialPortInfo::availablePorts()) {
            if (!val.isNull() && val.portName().contains(portName, Qt::CaseInsensitive))
                portInfo = val;
        }
        if (!portInfo.isNull())
            gnss->setPort(portInfo);
        else { // probably hostname/ip address, try it. Baud rate should then contain a port number
            connectTcpSocket();
        }
        if (tcpSocket->state() == QAbstractSocket::UnconnectedState) {
            gnss->setBaudRate(baudrate.toUInt());
            if (gnss->open(QIODevice::ReadWrite))
                qDebug() << "Connected to" << gnss->portName();
            else
                qDebug() << "Cannot open" << gnss->portName();
            if (gnss->isOpen()) gnssData.inUse = true;
            else gnssData.inUse = false;
            gnss->setDataTerminalReady(true);
            if (uBloxDevice) askUbloxVersion();
        }
    }
}

void GnssDevice::handleBuffer()
{
    QByteArray binaryHeader = QByteArray::fromHex("b562");
    QByteArray nmeaSentence, binarySentence;
    int binaryIndex, nmeaIndex;
    int binarySize, nmeaSize;
    int failsafe = 0;
    do {
        failsafe++;
        nmeaSentence.clear();
        binarySentence.clear();
        binarySize = 0;

        binaryIndex = gnssBuffer.indexOf(binaryHeader);
        if (binaryIndex != -1 && gnssBuffer.size() > binaryIndex + 6)
            binarySize = 8 + (quint8)gnssBuffer.at(binaryIndex + 4) + (gnssBuffer.at(binaryIndex + 5) << 8);

        nmeaIndex = gnssBuffer.indexOf("$");
        if (nmeaIndex != -1 && gnssBuffer.contains('\n')) {
            int i = 0;
            do {
                nmeaSentence.append(gnssBuffer[nmeaIndex + i++]);
            } while (nmeaIndex + i < gnssBuffer.size() && gnssBuffer[nmeaIndex + i] != '\n');

            nmeaSize = nmeaSentence.size();
            nmeaSentence = nmeaSentence.simplified();
            if (nmeaSize > 512) { // failsafe, seems stray data mixed with nmea from faulty serial lines can make hell break loose
                nmeaSentence.clear();
                nmeaSize = 0;
                gnssBuffer.clear();
            }
            if (nmeaIndex > 0 && binaryIndex == -1) {   // somehow leftover data has ended up before the nmea sentence, looks like nothing. deleting
                gnssBuffer.remove(0, nmeaIndex);
                //qDebug() << "GNSS: Leftover data, cleaning";
            }

            //qDebug() << "NMEA:" << nmeaSentence << nmeaSize << nmeaIndex;
            if (nmeaSize  > 20 && checkChecksum(nmeaSentence)) {
                if (nmeaSentence.contains("GGA"))
                    decodeGga(nmeaSentence);
                else if (nmeaSentence.contains("GSA"))
                    decodeGsa(nmeaSentence);
                else if (nmeaSentence.contains("RMC"))
                    decodeRmc(nmeaSentence);
                else if (nmeaSentence.contains("GSV"))
                    decodeGsv(nmeaSentence);
                else if (nmeaSentence.contains("GNS"))
                    decodeGns(nmeaSentence);
            }
            gnssBuffer.remove(nmeaIndex, nmeaSize+1);
        }
        if (binaryIndex != -1) {
            if (gnssBuffer.size() >= binaryIndex + binarySize) binarySentence = gnssBuffer.mid(binaryIndex, binarySize);

            if (binarySentence.size() == binarySize
                && checkBinaryChecksum(binarySentence)
                && (quint8)binarySentence[2] == 0x0a && (quint8)binarySentence[3] == 0x04) {
                decodeBinary0a04(binarySentence);
                gnssBuffer.remove(binaryIndex, binarySize);
            }
            else if (binarySentence.size() == binarySize) {
                decodeBinary(binarySentence);
                gnssBuffer.remove(binaryIndex, binarySize);
            }
        }

    } while ((binaryIndex != -1 || nmeaIndex != -1) && (binarySentence.size() > 0 || nmeaSentence.size() > 0) && failsafe < 50);
    if (failsafe >= 50) {
        qDebug() << "Panic!" << gnssBuffer.size();
        gnssBuffer.clear();
    }
    if (gnssBuffer.size() > 15000) gnssBuffer.clear();
}

bool GnssDevice::decodeGsa(const QByteArray &val)
{
    QList<QByteArray> split = val.split(',');
    if (split.count() > 16) {
        gnssData.gsaValid = true;
        gnssData.hdop = split.at(16).toDouble();

        /*if (val.contains("$GN")) gnssData.gnssType = "GPS+GLONASS";
        else if (val.contains("$GP")) gnssData.gnssType = "GPS";
        else if (val.contains("$GL")) gnssData.gnssType = "GLONASS";
        else gnssData.gnssType = "Other";*/

        if (split.at(2).toInt() == 3) gnssData.fixType = "3D";
        else if (split.at(2).toInt() == 2) gnssData.fixType = "2D";
        else gnssData.fixType = "No fix";
    }
    else
        gnssData.gsaValid = false;

    return gnssData.gsaValid;
}

bool GnssDevice::decodeGga(const QByteArray &val)
{
    QList<QByteArray> split = val.split(',');
    if (split.size() > 9) {
        gnssData.ggaValid = true;

        if (!gnssData.gnsValid) gnssData.satsTracked = split.at(7).toInt();
        gnssData.fixQuality = split.at(6).toInt();
        gnssData.altitude = split.at(9).toDouble();
        //gnssData.hdop = split.at(8).toDouble();
        gnssData.latitude =
            int(split.at(2).toDouble() / 100) +
            ((split.at(2).toDouble() / 100) - int(split.at(2).toDouble() / 100)) * 1.6667;
        gnssData.longitude =
            int(split.at(4).toDouble() / 100) +
            ((split.at(4).toDouble() / 100) - int(split.at(4).toDouble() / 100)) * 1.6667;

        if (!split[3].isEmpty() && split[3][0] == 'S') gnssData.latitude *= -1;
        if (!split[5].isEmpty() && split[5][0] == 'W') gnssData.longitude *= -1;
    }
    else gnssData.ggaValid = false;
    return gnssData.ggaValid;
}

bool GnssDevice::decodeRmc(const QByteArray &val)
{
    QList<QByteArray> split = val.split(',');
    if (split.size() > 11) {
        gnssData.timestamp = convFromGnssTimeToQDateTime(split.at(9), split.at(1));
        if (split.at(2).contains('A'))
            gnssData.posValid = true;
        else
            gnssData.posValid = false;
        gnssData.sog = split.at(7).toDouble();
        gnssData.cog = split.at(8).toDouble();
    }
    return true;
}

bool GnssDevice::decodeGsv(const QByteArray &val)
{
    if (!checkedAllSatSystems) {
        if (val.contains("$GP") && !gnssData.gnssType.contains("GPS")) gnssData.gnssType += "GPS ";
        else if (val.contains("$GL") && !gnssData.gnssType.contains("GLONASS")) gnssData.gnssType += "GLONASS ";
        else if (val.contains("$GA") && !gnssData.gnssType.contains("Galileo")) gnssData.gnssType += "Galileo ";
        else if (val.contains("$PQ") && !gnssData.gnssType.contains("BeiDou")) gnssData.gnssType += "BeiDou ";
        QTimer::singleShot(20000, this, [this] { // Assume after 20 secs all valid systems are active
            checkedAllSatSystems = true;
        });
    }
    QList<QByteArray> split = val.split(',');
    if (split.size() > 4) {
        if (split.at(2).toInt() == gsvSentences.size() + 1)
            gsvSentences.append(val);
    }
    if (split.at(1).toInt() == gsvSentences.size()) { // received all x sentences, let's analyze!
        QList<int> cnoList;
        for (auto &gsvSentence : gsvSentences) {
            split = gsvSentence.split(',');
            for (int i=7; i<split.size(); i += 4) {
                if (split.at(i).toInt() > 0) cnoList.append(split.at(i).toInt());
            }
        }
        while (cnoList.size() > 4) { // reduce cno list to 4 highest values, and make avg
            int low = 99, iterator = 0;
            for (int i=0; i<cnoList.size(); i++) {
                if (cnoList.at(i) < low) {
                    low = cnoList.at(i);
                    iterator = i;
                }
            }
            cnoList.removeAt(iterator);
        }
        if (cnoList.size() > 0) {
            gnssData.cno = 0;
            for (auto &v : cnoList)
                gnssData.cno += v;
            gnssData.cno /= cnoList.size();
        }
        gsvSentences.clear();
    }
    return true;
}

bool GnssDevice::decodeGns(const QByteArray &val)
{
    gnssData.gnsValid = true;
    QList<QByteArray> split = val.split(',');
    if (split.size() > 7) {
        gnssData.satsTracked = split[7].toInt();
    }
    return true;
}

QDateTime GnssDevice::convFromGnssTimeToQDateTime(const QByteArray date, const QByteArray time)
{
    QDateTime dt;
    QByteArray baTime;
    if (time.contains(".")) baTime = time.split('.')[0];
    if (date.size() == 6 && baTime.size() == 6) {
        dt = QDateTime::fromString(date + baTime, "ddMMyyHHmmss");
        dt = dt.addYears(100);
        dt.setTimeSpec(Qt::UTC);
    }
    return dt;
}

void GnssDevice::decodeBinary(const QByteArray &val)
{
    QByteArray MONHW(QByteArray::fromHex("0a09"));
    QByteArray MONRF(QByteArray::fromHex("0a38"));
    //qDebug() << "BIN:" << (int)val.at(2) << (int)val.at(3);
    if (checkBinaryChecksum(val)) {
        if (val.indexOf(MONHW) == 2) {
            int agc = (quint8)val.at(24) + (quint16)(val.at(25) << 8);
            if (agc >= 0 && agc <= 8192) gnssData.agc = agc * 100 / 8192; // Changed 181123 JSK: Percentage value, 8192 = 100% gain

            int jamInd = (quint8)val.at(51);
            if (jamInd >= 0 && jamInd < 256) gnssData.jammingIndicator = jamInd * 100 / 255;

            uchar jamState = (val.at(28) & 0xc) >> 2;

            if (jamState == 0) gnssData.jammingState = JAMMINGSTATE::UNKNOWN;
            else if (jamState == 1) gnssData.jammingState = JAMMINGSTATE::NOJAMMING;
            else if (jamState == 2) gnssData.jammingState = JAMMINGSTATE::WARNINGFIXOK;
            else if (jamState == 3) gnssData.jammingState = JAMMINGSTATE::CRITICALNOFIX;
        }

        else if (val.indexOf(MONRF) == 2) {
            int agc = (quint8)val[24] + ((quint8)val[25] << 8);
            if (agc >= 0 && agc <= 8192) gnssData.agc = agc * 100 / 8192;

            int jamInd = (quint8)val.at(26);
            if (jamInd >= 0 && jamInd < 256) gnssData.jammingIndicator = jamInd * 100 / 255;

            int jamState = (int)val[11];
            if (jamState == 0) gnssData.jammingState = JAMMINGSTATE::UNKNOWN;
            else if (jamState == 1) gnssData.jammingState = JAMMINGSTATE::NOJAMMING;
            else if (jamState == 2) gnssData.jammingState = JAMMINGSTATE::WARNINGFIXOK;
            else if (jamState == 3) gnssData.jammingState = JAMMINGSTATE::CRITICALNOFIX;
        }
    }
}

bool GnssDevice::checkBinaryChecksum(const QByteArray &val)
{
    if (val.size() > 4) {
        quint8 calcChkA = 0, calcChkB = 0;
        quint16 size = val.length() - 4;
        quint8 chkA = (quint8)val.at(val.size()-2);
        quint8 chkB = (quint8)val.at(val.size()-1);

        for (int i = 2; i<size+2; i++) {
            calcChkA += (quint8)val.at(i);
            calcChkB += calcChkA;
        }
        if (calcChkA == chkA && calcChkB == chkB) {
            return true;
        }
        else {
            return false;
        }
    }
    else return false;
}


bool GnssDevice::checkChecksum(const QByteArray &val)
{
    uint checksum = 0;
    QByteArray calcSum, string;
    QList<QByteArray> split = val.split('*');

    if (split.size() == 2) { // if not this is not a valid sentence (and the rest of code will fail miserably)
        calcSum = split.at(1).simplified();
        string = split.at(0);

        bool ok;
        uint val = calcSum.toUInt(&ok, 16);
        if (!ok) return false;

        for(int i=1; i<string.size(); i++) // skip inital sign ($)
            checksum ^= static_cast<quint8>(string[i]);
        if (checksum != val) return false;
        else return true;
    }
    return false;
}



void GnssDevice::updSettings() // caching these settings in memory since they are used often
{
    if (gnssData.id == 1 && !config->getGnssSerialPort1Activate()) gnss->close();
    else if (gnssData.id == 2 && !config->getGnssSerialPort2Activate()) gnss->close();
    if (gnssData.id == 1) {
        activate = config->getGnssSerialPort1Activate();
        portName = config->getGnssSerialPort1Name();
        baudrate = config->getGnssSerialPort1Baudrate();
        logToFile = config->getGnssSerialPort1LogToFile();
        uBloxDevice = config->getGnssSerialPort1MonitorAgc();
    }
    else {
        activate = config->getGnssSerialPort2Activate();
        portName = config->getGnssSerialPort2Name();
        baudrate = config->getGnssSerialPort2Baudrate();
        logToFile = config->getGnssSerialPort2LogToFile();
        uBloxDevice = config->getGnssSerialPort2MonitorAgc();
    }

    if (!gnss->isOpen() && !tcpSocket->isOpen() && activate) start(); // reconnect if needed
    if (!activate && tcpSocket->isOpen()) {
        tcpSocket->close();
        gnssData.posValid = false;
        emit gnssDisabled();
    }
    if (!activate && gnss->isOpen()) {
        gnss->close();
        gnssData.posValid = false;
        emit gnssDisabled();
    }
}

void GnssDevice::appendToLogfile(const QByteArray &data)
{
    if (!logfile.isOpen()) {
        logfile.setFileName(config->getLogFolder() + "/" + "gnss_" + QString::number(gnssData.id) + QDate::currentDate().toString("_yyyyMMdd.log"));
        logfile.open(QIODevice::Append);
        logfileStartedDate = QDate::currentDate();
        qDebug() << "GNSS logfile opened" << logfile.fileName();
    }
    logfile.write(data);
    logfile.flush();

    if (logfileStartedDate.daysTo(QDate::currentDate())) {
        logfile.close();
        qDebug() << "GNSS logfile closed, starting new";
    }
}

void GnssDevice::checkPosValid()
{
    QString msg;
    QTextStream ts(&msg);
    ts.setRealNumberNotation(QTextStream::FixedNotation);
    ts.setRealNumberPrecision(1);

    if (!gnssData.posValid && !posInvalidTriggered && !delayedReportTimer->isActive()) { // any recording triggered because position goes invalid will only last for minutes set in sdef config (record time after incident).
        // this to not always record, in case gnss has failed somehow. other triggers will renew as long as the trigger is valid.
        delayedReportTimer->start(config->getNotifyTruncateTime() * 1e3);
    }
    else if (gnssData.posValid && posInvalidTriggered && !delayedReportTimer->isActive()) {
        delayedReportTimer->start(config->getNotifyTruncateTime() * 1e3);
    }
    if (!msg.isEmpty() && config->getGnssShowNotifications()) emit toIncidentLog(NOTIFY::TYPE::GNSSDEVICE, QString::number(gnssData.id), msg);
}

void GnssDevice::connectTcpSocket()
{
    if (!portName.isEmpty() && !baudrate.isEmpty() && tcpSocket->state() == QAbstractSocket::UnconnectedState && baudrate.toInt() > 0) {
        hostAddress->setAddress(portName);
        tcpSocket->connectToHost(*hostAddress, baudrate.toInt());
    }
}

void GnssDevice::handleTcpStateChange(QAbstractSocket::SocketState state)
{
    //qDebug() << "GNSS TCP debug" << state;
    if (state == QAbstractSocket::ConnectedState) {
        qDebug() << "Connected to" << portName << baudrate;
        if (!sendToAnalyzerTimer->isActive()) sendToAnalyzerTimer->start(1000);
        gnssData.inUse = true;
    }
    else {
        gnssData.inUse = false;
        sendToAnalyzerTimer->stop();
    }
    if (state == QAbstractSocket::UnconnectedState) { // retry conn. every 5 sec if valid settings
        QTimer::singleShot(5000, this, [this] {
            if (activate) connectTcpSocket();
        });
    }
}

void GnssDevice::askUbloxVersion()
{
    qDebug() << "asking for version info from USB device";
    if (gnss->write(QByteArray::fromHex("b5620a0400000e34")) == -1) { // Ask version!
        qDebug() << "GNSS write failed";
    }
    uBloxState = UBLOX::ASKEDVERSION;
}

void GnssDevice::decodeBinary0a04(const QByteArray &val)
{
    if (val.contains("M91")) { // Cool, this is a very fancy GPS chip!
        qDebug() << "uBlox: M9 chip identified";
        uBloxID = "M91";
    }
    else if (val.contains("M8") || val.contains("00080000")) { // Ok then
        qDebug() << "uBlox: M8 chip identified";
        uBloxID = "M8";
    }
    if (uBloxID.isEmpty()) {
        qDebug() << "Unknown GPS chip, trying to proceed anyway" << val;
        uBloxState = UBLOX::READY; // skip programming, we have no idea what to do
    }
    else {
        qDebug() << "Trying to set up uBlox chip according to version...";
        setupUbloxDevice();
    }
}

void GnssDevice::setupUbloxDevice()
{
    if (uBloxID.contains("M9")) {
        qDebug() << "uBlox M9 configuration called";
        gnss->write(QByteArray::fromHex("b56206090c000000000000000000ffffffff1785")); // load defaults
        gnss->write(QByteArray::fromHex("b562068a0900000100005c03912001abfd")); // ubx-mon-rf on
        gnss->write(QByteArray::fromHex("b56206010300f00500ff19")); // vtg off
        gnss->write(QByteArray::fromHex("b562068a090000010000cc009120001720")); // gll off
        gnss->write(QByteArray::fromHex("b562068a090000010000b80091200104bd")); // gns on
        gnss->write(QByteArray::fromHex("b562068a0900000100000d00411001f956")); // itfm on (interference/jamming
        gnss->write(QByteArray::fromHex("b562068a09000001000010004120020d86")); // itfm active ant.
        gnss->write(QByteArray::fromHex("b562068a0900000100008e03912001ddf7")); // mon-span on (spectrum)
    }

    else if (uBloxID.contains("M8")) {
        gnss->write(QByteArray::fromHex("b56206010300f00100fb11")); // GLL OFF
        gnss->write(QByteArray::fromHex("b562060103000a09011e70")); // mon-hw on
        gnss->write(QByteArray::fromHex("b56206010300f00500ff19")); // vtg off
        gnss->write(QByteArray::fromHex("b56206390800f3ac62ad231e000036ea")); // itfm on (interference/jamming
        gnss->write(QByteArray::fromHex("b56206010800f00d0101010101011293")); // gns on
    }
    uBloxState = UBLOX::READY;
}

void GnssDevice::delayedReportHandler()
{
    delayedReportTimer->stop();

    if (!posInvalidTriggered && gnssData.posValid) { // Position was invalid, but became valid while waiting the truncate time. Do nothing
        //
    }
    else if (!posInvalidTriggered && !gnssData.posValid) { // Position invalid for more than truncate time, report
        posInvalidTriggered = true;
        if (config->getGnssShowNotifications()) emit toIncidentLog(NOTIFY::TYPE::GNSSDEVICE, QString::number(gnssData.id), "Position invalid");
    }
    else if (posInvalidTriggered && !gnssData.posValid) { // Already reported invalid, do nothing
        //
    }
    else if (posInvalidTriggered && gnssData.posValid) { // Pos. was invalid, now ok. Report.
        posInvalidTriggered = false;
        if (config->getGnssShowNotifications()) emit toIncidentLog(NOTIFY::TYPE::GNSSDEVICE, QString::number(gnssData.id), "Position valid");
    }
}
