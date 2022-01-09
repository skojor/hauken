#include "gnssdevice.h"

GnssDevice::GnssDevice(QObject *parent, int val)
    : Config{parent}
{
    gnssData.id = val;

    connect(gnss, &QSerialPort::readyRead, this, [this]
    {
        this->gnssBuffer.append(gnss->readAll());
        this->handleBuffer();
    });

    connect(sendToAnalyzerTimer, &QTimer::timeout, this, [this]
    {
        if (this->gnss->isOpen()) emit analyzeThisData(gnssData);
    });

    start();
}

void GnssDevice::start()
{
    if (gnssData.id == 1 && getGnssSerialPort1Activate() && !gnss->isOpen()) connectToPort();
    else if (gnssData.id == 2 && getGnssSerialPort2Activate() && !gnss->isOpen()) connectToPort();
    sendToAnalyzerTimer->stop();

    if (gnss->isOpen()) {
        sendToAnalyzerTimer->start(1000);
    }
}

void GnssDevice::connectToPort()
{
    QSerialPortInfo portInfo;
    bool activate;
    QString portName;
    QString baudrate;

    if (gnssData.id == 1) {
        activate = getGnssSerialPort1Activate();
        portName = getGnssSerialPort1Name();
        baudrate = getGnssSerialPort1Baudrate();
    }
    else {
        activate = getGnssSerialPort2Activate();
        portName = getGnssSerialPort2Name();
        baudrate = getGnssSerialPort2Baudrate();
    }

    if (activate && !portName.isEmpty() && !baudrate.isEmpty()) {
        for (auto &val : QSerialPortInfo::availablePorts()) {
            if (!val.isNull() && val.portName().contains(portName, Qt::CaseInsensitive))
                portInfo = val;
        }
        if (!portInfo.isNull())
            gnss->setPort(portInfo);
        else {
            qDebug() << "Couldn't find serial port" << portInfo.portName() << ", check settings";
        }
        gnss->setBaudRate(baudrate.toUInt());
        if (gnss->open(QIODevice::ReadWrite))
            qDebug() << "Connected to" << gnss->portName();
        else
            qDebug() << "Cannot open" << gnss->portName();
        if (gnss->isOpen()) gnssData.inUse = true;
        else gnssData.inUse = false;
    }
}

void GnssDevice::handleBuffer()
{
    while (gnssBuffer.contains('\n')) {
        if (gnssBuffer.at(0) == (char)0xb5 && gnssBuffer.at(1) == (char)0x62) { //binary header
            quint16 size = 8 + (quint16)gnssBuffer.at(4) + (quint16)(gnssBuffer.at(5) << 8);
            if (gnssBuffer.size() >= size) {
                QByteArray tmp = gnssBuffer.left(size);
                gnssBuffer.remove(0, size);
                if (checkBinaryChecksum(tmp)) decodeBinary(tmp);
            }
        }

        QByteArray sentence = gnssBuffer.split('\n').at(0);
        gnssBuffer.remove(0, gnssBuffer.indexOf('\n') + 1); // deletes one sentence from the buffer
        if (logToFile) appendToLogfile(sentence);

        if (sentence.at(0) == '$' && sentence.contains('\r')) { // looks like nmea
            if (checkChecksum(sentence)) {
                if (sentence.contains("GGA"))
                    decodeGga(sentence);
                else if (sentence.contains("GSA"))
                    decodeGsa(sentence);
                else if (sentence.contains("RMC"))
                    decodeRmc(sentence);
                else if (sentence.contains("GSV"))
                    decodeGsv(sentence);
            }
            else qDebug() << "checksum error" << sentence;
        }
    }
}

bool GnssDevice::decodeGsa(const QByteArray &val)
{
    QList<QByteArray> split = val.split(',');
    if (split.count() > 16) {
        gnssData.gsaValid = true;
        gnssData.hdop = split.at(16).toDouble();

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
    if (split.size() > 7) {
        gnssData.ggaValid = true;

        gnssData.satsTracked = split.at(7).toInt();
        gnssData.fixQuality = split.at(6).toInt();
        gnssData.altitude = split.at(9).toDouble();
        //gnssData.hdop = split.at(8).toDouble();
        gnssData.latitude =
                int(split.at(2).toDouble() / 100) +
                ((split.at(2).toDouble() / 100) - int(split.at(2).toDouble() / 100)) * 1.6667;
        gnssData.longitude =
                int(split.at(4).toDouble() / 100) +
                ((split.at(4).toDouble() / 100) - int(split.at(4).toDouble() / 100)) * 1.6667;

        if (split[3][0] == 'S') gnssData.latitude *= -1;
        if (split[5][0] == 'W') gnssData.longitude *= -1;
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
    }
    return true;
}

bool GnssDevice::decodeGsv(const QByteArray &val)
{
    QList<QByteArray> split = val.split(',');
    if (split.size() > 4) {
        if (val.contains("$GN")) gnssData.gnssType = "GPS+GLONASS";
        else if (val.contains("$GP")) gnssData.gnssType = "GPS";
        else if (val.contains("$GL")) gnssData.gnssType = "GLONASS";
        else gnssData.gnssType = "Other";

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

QDateTime GnssDevice::convFromGnssTimeToQDateTime(const QByteArray date, const QByteArray time)
{
    QDateTime dt;
    dt = QDateTime::fromString(date + time.split('.').at(0), "ddMMyyHHmmss");
    dt = dt.addYears(100);
    dt.setTimeSpec(Qt::UTC);
    return dt;
}

void GnssDevice::decodeBinary(const QByteArray &val)
{
    if (checkBinaryChecksum(val)) {
        gnssData.agc = (quint8)val.at(24) + (quint16)(val.at(25) << 8);
        //jamInd = (quint8)buffer.at(51);
    }
}

bool GnssDevice::checkBinaryChecksum(const QByteArray &val)
{
    quint8 calcChkA = 0, calcChkB = 0;
    quint16 size = val.length() - 4;
    quint8 chkA = (quint8)val.at(val.size()-2);
    quint8 chkB = (quint8)val.at(val.size()-1);

    for (int i = 2; i<size+2; i++) {
        calcChkA += (quint8)val.at(i);
        calcChkB += calcChkA;
    }
    if (calcChkA == chkA && calcChkB == chkB) return true;
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
    }
    return true;
}



void GnssDevice::updSettings() // caching these settings in memory since they are used often
{
    if (gnssData.id == 1 && !getGnssSerialPort1Activate()) gnss->close();
    else if (gnssData.id == 2 && !getGnssSerialPort2Activate()) gnss->close();
    if (gnssData.id == 1) {
        logToFile = getGnssSerialPort1LogToFile();
    }
    else {
        logToFile = getGnssSerialPort2LogToFile();
    }
    start(); // reconnect if needed
}

void GnssDevice::appendToLogfile(const QByteArray &data)
{
    if (!logfile.isOpen()) {
        logfile.setFileName(getLogFolder() + "/" + "gnss_" + QString::number(gnssData.id) + QDate::currentDate().toString("_yyyyMMdd.log"));
        logfile.open(QIODevice::Append | QIODevice::Text);
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
