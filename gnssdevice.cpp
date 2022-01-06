#include "gnssdevice.h"

GnssDevice::GnssDevice(QObject *parent)
    : Config{parent}
{
    connect(gnss1, &QSerialPort::readyRead, this, &GnssDevice::incomingDataGnss1);
    connect(gnss2, &QSerialPort::readyRead, this, &GnssDevice::incomingDataGnss2);
    connectToPort(1);
    connectToPort(2);

}

void GnssDevice::start()
{

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
            if (!val.isNull() && val.portName().contains(getGnssSerialPort1Name(), Qt::CaseInsensitive))
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
    }
}

void GnssDevice::handleBuffer()
{
    qDebug() << "buffer size " << gnss1Buffer.size() << gnss2Buffer.size();
}

void GnssDevice::incomingDataGnss1()
{
    gnss1Buffer.append(gnss1->readAll());
    handleBuffer();
}


void GnssDevice::incomingDataGnss2()
{
    gnss2Buffer.append(gnss2->readAll());
    handleBuffer();
}

void GnssDevice::updSettings() // caching these settings in memory since they are used often
{
    cnoLimit = getGnssCnoDeviation();
    agcLimit = getGnssAgcDeviation();
    posOffsetLimit = getGnssPosOffset();
    altOffsetLimit = getGnssAltOffset();
    timeOffsetLimit = getGnssTimeOffset();
}
