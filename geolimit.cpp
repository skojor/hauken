#include "geolimit.h"


GeoLimit::GeoLimit(QSharedPointer<Config> c)
{
    config = c;
    connect(timer, &QTimer::timeout, this, &GeoLimit::checkCurrentPosition);
}

void GeoLimit::activate()
{
    if (activated) {
        timer->start(1000); // check position validity every second
        weAreInsidePolygon = false;
        emit currentPositionOk(false);
    }

    else {
        timer->stop();
        weAreInsidePolygon = true;
        emit currentPositionOk(true);
    }
}

void GeoLimit::checkCurrentPosition()
{
    if (gnssData.posValid && !graceTime) { // no point in checking if not valid
        awaitingPosition = false;
        QGeoCoordinate coord(gnssData.latitude, gnssData.longitude);
        if (testMode) {
            if (!testCoordinates.isValid()) testCoordinates = coord;
            else {
                testCoordinates.setLatitude(testCoordinates.latitude() - QRandomGenerator64::global()->generateDouble());
                testCoordinates.setLongitude(testCoordinates.longitude() + QRandomGenerator64::global()->generateDouble() / 10);
            }
            qDebug() << "Test random coord" << testCoordinates.latitude() << testCoordinates.longitude();
            coord = testCoordinates;
        }

        if (polygon.contains(coord)) {
            if (!weAreInsidePolygon) {
                emit toIncidentLog(NOTIFY::TYPE::GEOLIMITER, "", "Current position within allowed area, enabling radio");
                emit currentPositionOk(true);
                weAreInsidePolygon = true;
                graceTime = 60;
            }
        }
        else {
            if (weAreInsidePolygon) {
                emit toIncidentLog(NOTIFY::TYPE::GEOLIMITER, "", "Current position is outside allowed area, pausing radio");
                emit currentPositionOk(false);
                weAreInsidePolygon = false;
                graceTime = 60;
            }
        }
    }
    if (awaitingPosition && !notifyWeAreWaiting) {
        notifyWeAreWaiting = true;
        emit toIncidentLog(NOTIFY::TYPE::GEOLIMITER, "", "Radio is paused until a valid position inside the given polygon is received");
    }

    else if (graceTime) {
        graceTime--;
        if (testMode && !weAreInsidePolygon && graceTime == 1) testMode = false;
    }

    emit requestPosition(); // update pos. every second
}

void GeoLimit::readKmlFile()
{
    QFile file(filename);
    if (!file.open(QIODevice::ReadOnly)) {
        qDebug() << "Cannot open KML" << filename;
        emit toIncidentLog(NOTIFY::TYPE::GEOLIMITER, QString(), "Geographic limit KML file " + filename + " could not be opened, disabling geographic limit");
        activated = false;
        triedReadingFileNoSuccess = true;
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
                            coord.setLatitude(latLng[1].toDouble());
                            coord.setLongitude(latLng[0].toDouble());

                            if (coord.isValid()) {
                                polygon.addCoordinate(coord);
                                //qDebug() << "Added to polygon" << coord.latitude() << coord.longitude();
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

void GeoLimit::restart()
{
    weAreInsidePolygon = false;
    triedReadingFileNoSuccess = false;
    awaitingPosition = true;
    notifyWeAreWaiting = false;
    activate();
}

void GeoLimit::updSettings()
{
    if (filename != config->getGeoLimitFilename()) {
        filename = config->getGeoLimitFilename();
        if (activated && !filename.isEmpty()) readKmlFile();
    }
    if (!activated && config->getGeoLimitActive()) {
        activated = true;
        if (!triedReadingFileNoSuccess) filename = config->getGeoLimitFilename();
        if (!filename.isEmpty()) readKmlFile();
        activate();
    }
    else if (activated && !config->getGeoLimitActive()) {
        activated = false;
        weAreInsidePolygon = true; // allow any pos.
        awaitingPosition = false;
        activate();
    }
}
