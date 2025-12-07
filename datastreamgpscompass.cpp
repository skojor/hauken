#include "datastreamgpscompass.h"

DatastreamGpsCompass::DatastreamGpsCompass(QObject *parent)
{

}

bool DatastreamGpsCompass::checkHeaders()
{
    if (m_eb200Header.magicNumber != 0x000eb200 or m_eb200Header.dataSize > 65536)
        return false;
    if (m_attrHeader.tag != (int)Instrument::Tags::GPSC)
        return false;
    return true;
}

void DatastreamGpsCompass::readData(QDataStream &ds)
{
    if (checkHeaders())
        m_optHeader.readData(ds, m_attrHeader.optHeaderLength);
    checkOptHeader();
    if (!m_gpsCompassData.readData(ds))
        qWarning() << "Error reading GPSCompass stream";
    else {
        GpsData gpsData;

        gpsData.latitude = m_gpsCompassData.latDeg + (m_gpsCompassData.latMin * 0.0166666667);
        if (m_gpsCompassData.latRef == 'S')
            gpsData.latitude *= -1;
        gpsData.longitude = m_gpsCompassData.lonDeg + (m_gpsCompassData.lonMin * 0.0166666667);
        if (m_gpsCompassData.lonRef == 'W')
            gpsData.longitude *= -1;
        gpsData.altitude = m_gpsCompassData.altitude;
        gpsData.dop = m_gpsCompassData.dilution;
        gpsData.sog = (float) ( (m_gpsCompassData.sog) / 10.0 ) * 1.94384449;
        gpsData.timestamp = QDateTime::fromMSecsSinceEpoch(m_gpsCompassData.gpsTimestamp / 1e6);
        gpsData.sats = m_gpsCompassData.noOfSatInView;
        gpsData.valid = (bool)m_gpsCompassData.gpsValid;

        emit gpsdataReady(gpsData);
    }
}

void DatastreamGpsCompass::checkOptHeader()
{
    if (m_optHeader.timestamp == 0) // Maybe actually do some checking here?? TODO
        qWarning() << "GPSCompass stream: Opt header not set?";
}
