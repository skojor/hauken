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
#include "config.h"
#include "typedefs.h"

class GeoLimit : public Config
{
    Q_OBJECT
public:
    explicit GeoLimit(QObject *parent = nullptr);
    void updSettings();
    void receivePosition(GnssData data) { if (data.posValid) gnssData = data; } // only update if valid position
    bool waitingForPosition() { return awaitingPosition;}

private:
    QGeoPolygon polygon;
    QTimer *timer = new QTimer;
    bool stateOutsidePolygon = false, awaitingPosition = true;

    QString filename;
    bool activated = false;
    GnssData gnssData;
    int graceTime = 0; // time in seconds to not check, to not change state too fast

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
