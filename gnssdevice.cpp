#include "gnssdevice.h"

GnssDevice::GnssDevice(QObject *parent)
    : Config{parent}
{
    gnssData.append(GnssData());
    gnssData.append(GnssData());
    gnssData[0].id = 0;
    gnssData[1].id = 1;

    connect(gnss1, &QSerialPort::readyRead, this, [this]
    {
        this->gnss1Buffer.append(gnss1->readAll());
        this->handleBuffer();
    });
    connect(gnss2, &QSerialPort::readyRead, this,  [this]
    {
        this->gnss2Buffer.append(gnss2->readAll());
    });

    connect(updDisplayTimer, &QTimer::timeout, this, &GnssDevice::updDisplay);
    start();
}

void GnssDevice::start()
{
    //gnss1->close();
    //gnss2->close();
    if (getGnssSerialPort1Activate() && !gnss1->isOpen()) connectToPort(1);
    if (getGnssSerialPort2Activate() && !gnss2->isOpen()) connectToPort(2);

    updDisplayTimer->stop();
    if (gnss1->isOpen() or gnss2->isOpen()) updDisplayTimer->start(2000);
}

void GnssDevice::connectToPort(int portnr)
{
    serialPortList = QSerialPortInfo::availablePorts();
    QSerialPortInfo portInfo;
    // gnss 1 basic config check
    if (portnr == 1 && getGnssSerialPort1Activate() && !getGnssSerialPort1Name().isEmpty() && !getGnssSerialPort1Baudrate().isEmpty()) {
        for (auto &val : serialPortList) {
            if (!val.isNull() && val.portName().contains(getGnssSerialPort1Name(), Qt::CaseInsensitive))
                portInfo = val;
        }
        if (!portInfo.isNull())
            gnss1->setPort(portInfo);
        else {
            qDebug() << "Couldn't find serial port, check settings";
        }
        gnss1->setBaudRate(getGnssSerialPort1Baudrate().toUInt());
        if (gnss1->open(QIODevice::ReadWrite))
            qDebug() << "Connected to" << gnss1->portName();
        else
            qDebug() << "Cannot open" << gnss1->portName();
    }

    if (portnr == 2 && getGnssSerialPort2Activate() && !getGnssSerialPort2Name().isEmpty() && !getGnssSerialPort2Baudrate().isEmpty()) {
        for (auto &val : serialPortList) {
            if (!val.isNull() && val.portName().contains(getGnssSerialPort2Name(), Qt::CaseInsensitive))
                portInfo = val;
        }
        if (!portInfo.isNull())
            gnss2->setPort(portInfo);
        else {
            qDebug() << "Couldn't find serial port, check settings";
        }
        gnss2->setBaudRate(getGnssSerialPort2Baudrate().toUInt());
        if (gnss2->open(QIODevice::ReadWrite))
            qDebug() << "Connected to" << gnss2->portName();
        else
            qDebug() << "Cannot open" << gnss2->portName();

    }
}

void GnssDevice::handleBuffer()
{
    while (gnss1Buffer.contains('\n')) {
        QByteArray sentence = gnss1Buffer.split('\n').at(0);
        gnss1Buffer.remove(0, gnss1Buffer.indexOf('\n') + 1); // deletes one sentence from the buffer
        if (checkChecksum(sentence)) {
            if (sentence.contains("GGA"))
                decodeGga(sentence, 0);
            else if (sentence.contains("GSA"))
                decodeGsa(sentence, 0);
            else if (sentence.contains("RMC"))
                decodeRmc(sentence, 0);
            else if (sentence.contains("GSV"))
                decodeGsv(sentence, 0);
        }
        else qDebug() << "checksum error" << sentence;
    }
    while (gnss2Buffer.contains('\n')) {
        QByteArray sentence = gnss2Buffer.split('\n').at(0);
        gnss2Buffer.remove(0, gnss2Buffer.indexOf('\n') + 1); // deletes one sentence from the buffer
        if (checkChecksum(sentence)) {
            if (sentence.contains("GGA"))
                decodeGga(sentence, 1);
            else if (sentence.contains("GSA"))
                decodeGsa(sentence, 1);
            else if (sentence.contains("RMC"))
                decodeRmc(sentence, 1);
            else if (sentence.contains("GSV"))
                decodeGsv(sentence, 1);
        }
        else qDebug() << "checksum error" << sentence;
    }

}

bool GnssDevice::decodeGsa(const QByteArray &val, const int nr)
{
    QList<QByteArray> split = val.split(',');
    if (split.count() > 16) {
        gnssData[nr].gsaValid = true;
        gnssData[nr].hdop = split.at(16).toDouble();

        if (split.at(2).toInt() == 3) gnssData[nr].fixType = "3D";
        else if (split.at(2).toInt() == 2) gnssData[nr].fixType = "2D";
        else gnssData[nr].fixType = "No fix";
    }
    else
        gnssData[nr].gsaValid = false;

    return gnssData[nr].gsaValid;
}

bool GnssDevice::decodeGga(const QByteArray &val, const int nr)
{
    QList<QByteArray> split = val.split(',');
    if (split.size() > 7) {
        gnssData[nr].ggaValid = true;

        gnssData[nr].satsTracked = split.at(7).toInt();
        gnssData[nr].fixQuality = split.at(6).toInt();
        gnssData[nr].altitude = split.at(9).toDouble();
        //gnssData[nr].hdop = split.at(8).toDouble();
        gnssData[nr].latitude =
                int(split.at(2).toDouble() / 100) +
                ((split.at(2).toDouble() / 100) - int(split.at(2).toDouble() / 100)) * 1.6667;
        gnssData[nr].longitude =
                int(split.at(4).toDouble() / 100) +
                ((split.at(4).toDouble() / 100) - int(split.at(4).toDouble() / 100)) * 1.6667;

        if (split[3][0] == 'S') gnssData[nr].latitude *= -1;
        if (split[5][0] == 'W') gnssData[nr].longitude *= -1;
    }
    else gnssData[nr].ggaValid = false;
    return gnssData[nr].ggaValid;
}

bool GnssDevice::decodeRmc(const QByteArray &val, const int nr)
{
    QList<QByteArray> split = val.split(',');
    if (split.size() > 11) {
        gnssData[nr].timestamp = convFromGnssTimeToQDateTime(split.at(9), split.at(1));
        if (split.at(2).contains('A'))
            gnssData[nr].posValid = true;
        else
            gnssData[nr].posValid = false;
    }
    return true;
}

bool GnssDevice::decodeGsv(const QByteArray &val, const int nr)
{
    QList<QByteArray> split = val.split(',');
    if (split.size() > 11) {
        if (split.at(2).toInt() == gsvSentences.size() + 1)
            gsvSentences.append(val);
    }
    if (split.at(1).toInt() == gsvSentences.size()) { // received all x sentences, let's analyze!
        QList<int> cnoList;
        for (auto &gsvSentence : gsvSentences) {
            split = gsvSentence.split(',');
            for (int i=4; i<split.size(); i += 3) {
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
        gnssData[nr].cno = 0;
        for (auto &val : cnoList)
            gnssData[nr].cno += val;
        gnssData[nr].cno /= cnoList.size();
    }
    gsvSentences.clear();
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
    if (!getGnssSerialPort1Activate()) gnss1->close();
    if (!getGnssSerialPort2Activate()) gnss2->close();
    start(); // reconnect if needed
}

void GnssDevice::updDisplay()
{
    if (gnss1->isOpen() && gnss2->isOpen()) { // alternates once per update (second)
        if (display) display = 0;
        else display = 1;
    }
    else if (gnss1->isOpen() && !gnss2->isOpen())
        display = 0;
    else if (!gnss1->isOpen() && gnss2->isOpen())
        display = 1;
    else display = -1;

    if (display >= 0) {
        QString out;
        QTextStream ts(&out);

        ts << "<table><tr><td>Latitude</td><td>" << gnssData.at(display).latitude << "</td></tr>"
           << "<tr><td>Longitude</td><td>" << gnssData.at(display).longitude << "</td></tr>"
           << "<tr><td>Altitude</td><td>" << gnssData.at(display).altitude << "</td></tr>"
           << "<tr><td>C/No</td><td>" << gnssData.at(display).cno << "</td></tr>"
           << "<tr><td>Sats tracked</td><td>" << gnssData.at(display).satsTracked << "</td></tr>"
           << "<tr><td>HDOP</td><td>" << gnssData.at(display).hdop << "</td></tr>"
           << "<tr><td>GNSS type</td><td>" << gnssData.at(display).fixType << "</td></tr>"
           << "</table>";
        emit displayGnssData(out, display + 1);
    }
}
