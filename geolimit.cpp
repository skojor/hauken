#include "geolimit.h"


GeoLimit::GeoLimit(QObject *parent)
{
    this->setParent(parent);
    connect(timer, &QTimer::timeout, this, &GeoLimit::checkCurrentPosition);
}

void GeoLimit::activate()
{
    if (activated) {
        timer->start(1000); // check position validity every second
    }

    else {
        timer->stop();
    }
}

void GeoLimit::checkCurrentPosition()
{
    if (gnssData.posValid && !graceTime) { // no point in checking if not valid
        QGeoCoordinate coord(gnssData.longitude, gnssData.latitude);
        if (polygon.contains(coord)) {
            if (!stateOutsidePolygon) {
                emit toIncidentLog(NOTIFY::TYPE::GENERAL, "", "Geographic limiting enabled, current position within allowed area. Enabling radio");
                emit currentPositionOk(true);
                stateOutsidePolygon = true;
                graceTime = 60;
            }
        }
        else {
            if (stateOutsidePolygon) {
                emit toIncidentLog(NOTIFY::TYPE::GENERAL, "", "Current position is outside allowed area. Pausing radio");
                emit currentPositionOk(false);
                stateOutsidePolygon = false;
                graceTime = 60;
            }
        }
    }

    else if (graceTime) graceTime--;

    else {
        qDebug() << "No valid position found";
    }
    emit requestPosition(); // update pos. every second
}

void GeoLimit::readKmlFile()
{
    QFile file(filename);
    if (!file.open(QIODevice::ReadOnly)) {
        qDebug() << "Cannot open KML" << filename;
        emit toIncidentLog(NOTIFY::TYPE::GENERAL, QString(), "Geographic limit KML file " + filename + " could not be opened, disabling geographic limit");

    }
    else {
        QByteArray data;
        while (!file.atEnd()) {
            data = file.readLine();
            if (data.contains("coordinates")) {
                QList<QByteArray> split = file.readLine().split(' ');
                if (split.size() > 0) {
                    for (auto &val : split) {
                        val = val.simplified();
                        QGeoCoordinate coord;
                        QList<QByteArray> latLng = val.split(',');
                        if (latLng.size() > 1) {
                            coord.setLatitude(latLng[0].toDouble());
                            coord.setLongitude(latLng[1].toDouble());

                            if (coord.isValid()) {
                                polygon.addCoordinate(coord);
                                qDebug() << "Added to polygon" << coord.latitude() << coord.longitude();
                            }
                        }
                    }
                }
                else {
                    qDebug() << "Error parsing KML, no polygon inside?";
                }
                break;
            }
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
