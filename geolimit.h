#ifndef GEOLIMIT_H
#define GEOLIMIT_H

#include <QWidget>
#include <QFile>
#include <QDir>
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QLineEdit>
#include <QCheckBox>
#include <QLabel>
#include <QDialogButtonBox>
#include <QPushButton>
#include <QXmlStreamReader>
#include <QList>
#include <QGeoPolygon>
#include <QGeoCoordinate>
#include <QTimer>
#include <QRandomGenerator64>
#include "config.h"
#include "typedefs.h"

class GeoLimit : public QObject
{
    Q_OBJECT
public:
    explicit GeoLimit(QSharedPointer<Config> c);
    void updSettings();
    void receivePosition(GnssData data) { if (data.posValid) gnssData = data; } // only update if valid position
    bool waitingForPosition() { return awaitingPosition;}
    bool areWeInsidePolygon() { return weAreInsidePolygon;}
    void restart();

private:
    QGeoPolygon polygon;
    QTimer *timer = new QTimer;
    bool weAreInsidePolygon = false, triedReadingFileNoSuccess = false, awaitingPosition = true, notifyWeAreWaiting = false;
    bool testMode = false;
    QGeoCoordinate testCoordinates;

    QString filename;
    bool activated = false;
    GnssData gnssData;
    int graceTime = 0; // time in seconds to not check, to not change state too fast
    QSharedPointer<Config> config;

private slots:
    void activate();
    void readKmlFile();
    void checkCurrentPosition();

signals:
    void warning(QString);
    void toIncidentLog(const NOTIFY::TYPE, const QString, const QString);
    void requestPosition();
    void currentPositionOk(bool b);
};

#endif // GEOLIMIT_H
