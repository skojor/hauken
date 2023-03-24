#include "geolimit.h"


GeoLimit::GeoLimit(QObject *parent)
{
    this->setParent(parent);

}

void GeoLimit::activate()
{

}

void GeoLimit::readKmlFile()
{
    QFile file(filename);
    if (!file.open(QIODevice::ReadOnly)) {
        qDebug() << "Cannot open KML" << filename;
        emit toIncidentLog(NOTIFY::TYPE::GENERAL, QString(), "Geographic limit KML file " + filename + " could not be opened, disabling geographic limit");

    }
    else {
        QXmlStreamReader xml(&file);
        while (xml.readNextStartElement()) {
            qDebug() << xml.name();

            xml.skipCurrentElement();
        }
    }
    file.close();
}

void GeoLimit::updSettings()
{
    if (!activated && getGeoLimitActive()) {
        activated = true;
        filename = getGeoLimitFilename();
        readKmlFile();
        activate();
    }
    else if (activated && !getGeoLimitActive()) {
        activated = false;
        activate();
    }
    if (filename != getGeoLimitFilename()) {
        filename = getGeoLimitFilename();
        readKmlFile();
    }
}
