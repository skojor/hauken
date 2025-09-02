#ifndef GNSSDISPLAY_H
#define GNSSDISPLAY_H

#include <QWidget>
#include <QDebug>
#include <QObject>
#include <QDateTime>
#include <QTimer>
#include <QTextStream>
#include <QList>
#include <QStringList>
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QLineEdit>
#include <QCheckBox>
#include <QLabel>
#include <QDialogButtonBox>
#include <QPushButton>
#include <QGroupBox>
#include "config.h"
#include "typedefs.h"


/*
 * Class to display GNSS data in a window.
 *
 * GNSS data fed via slots
 *
*/

class GnssDisplay : public QObject
{
    Q_OBJECT
public:
    explicit GnssDisplay(QSharedPointer<Config>);
    void start();
    void close();
    void updSettings();
    void updText();
    void updGnssData(GnssData data, int id);
    void reqGnssData();

private slots:
    void setupWidget();

private:
    QWidget *wdg = new QWidget;
    QFormLayout *mainLayout = new QFormLayout;
    GnssData gnss1, gnss2;
    bool isClosing = false;
    QGroupBox *gnss1LeftGroupBox = new QGroupBox;
    QGroupBox *gnss1RightGroupBox = new QGroupBox;
    QGroupBox *gnss2LeftGroupBox = new QGroupBox;
    QGroupBox *gnss2RightGroupBox = new QGroupBox;

    QLabel *gnss1Latitude = new QLabel, *gnss2Latitude = new QLabel;
    QLabel *gnss1Longitude = new QLabel, *gnss2Longitude = new QLabel;
    QLabel *gnss1Altitude = new QLabel, *gnss2Altitude = new QLabel;
    QLabel *gnss1Time = new QLabel, *gnss2Time = new QLabel;
    QLabel *gnss1Dop = new QLabel, *gnss2Dop = new QLabel;
    QLabel *gnss1CNo = new QLabel, *gnss2CNo = new QLabel;
    QLabel *gnss1Agc = new QLabel, *gnss2Agc = new QLabel;
    QLabel *gnss1Sats = new QLabel, *gnss2Sats = new QLabel;

    QLabel *gnss1PosValid = new QLabel, *gnss2PosValid = new QLabel;
    QLabel *gnss1PosOffset = new QLabel, *gnss2PosOffset = new QLabel;
    QLabel *gnss1AltOffset = new QLabel, *gnss2AltOffset = new QLabel;
    QLabel *gnss1TimeOffset = new QLabel, *gnss2TimeOffset = new QLabel;
    QLabel *gnss1CwJamming = new QLabel, *gnss2CwJamming = new QLabel;
    QLabel *gnss1JammingState = new QLabel, *gnss2JammingState = new QLabel;

    QTimer *updateGnssDataTimer = new QTimer;
    QString gnss1Name, gnss2Name;

    QSharedPointer<Config> config;

signals:
    void requestGnssData(int);
};

#endif // GNSSDISPLAY_H
