#include "gnssdevice.h"

GnssDevice::GnssDevice(QObject *parent)
    : Config{parent}
{
    serialPortList = QSerialPortInfo::availablePorts();
}

void GnssDevice::start()
{

}
