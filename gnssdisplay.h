#ifndef GNSSDISPLAY_H
#define GNSSDISPLAY_H

#include <QWidget>
#include <QDebug>
#include <QObject>
#include <QDateTime>
#include <QTimer>
#include <QTextStream>
#include <QList>
#include <QMap>
#include <QStringList>
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QLineEdit>
#include <QCheckBox>
#include <QLabel>
#include <QDialogButtonBox>
#include <QPushButton>
#include <QGroupBox>
#include <QEvent>
#include <QGridLayout>
#include <QVBoxLayout>
#include "config.h"
#include "typedefs.h"
#include "qcustomplot.h"


/*
 * Class to display GNSS data in a window.
 *
 * GNSS data fed via slots
 *
*/

struct GnssReceiverCard
{
    QGroupBox *leftGroupBox = new QGroupBox;
    QGroupBox *rightGroupBox = new QGroupBox;
    QLabel *latitude = new QLabel;
    QLabel *longitude = new QLabel;
    QLabel *altitude = new QLabel;
    QLabel *time = new QLabel;
    QLabel *dop = new QLabel;
    QLabel *cno = new QLabel;
    QLabel *agc = new QLabel;
    QLabel *sats = new QLabel;
    QLabel *posValid = new QLabel;
    QLabel *posOffset = new QLabel;
    QLabel *altOffset = new QLabel;
    QLabel *timeOffset = new QLabel;
    QLabel *cwJamming = new QLabel;
    GnssData data;
};

class GnssDisplay : public QObject
{
    Q_OBJECT
public:
    explicit GnssDisplay(QSharedPointer<Config>);
    void start();
    void close();
    void updSettings();
    void updText();
    void updGnssData(GnssData data, QString id);
    QImage emailInfoImage() const;
    void updPpsData(const PpsData &data);
    void updPpsAvailability(bool online);
    void updPpsError(const QString &error);
    void reqGnssData();
    bool eventFilter(QObject *obj, QEvent *event) override;

private slots:
    void setupWidget();

private:
    QWidget *wdg = new QWidget;
    QGridLayout *receiversLayout = new QGridLayout;
    QMap<QString, GnssReceiverCard *> cards;
    bool isClosing = false;
    QGroupBox *ppsGroupBox = new QGroupBox;

    QTimer *updateGnssDataTimer = new QTimer;
    PpsData ppsData;
    bool ppsDataReceived = false;
    bool ppsAvailabilityKnown = false;
    bool ppsOnline = false;
    QString ppsLastError;

    QLabel *ppsAvailability = new QLabel;
    QLabel *ppsReference = new QLabel;
    QLabel *ppsGps = new QLabel;
    QLabel *ppsGalileo = new QLabel;
    QLabel *ppsGpsReference = new QLabel;
    QLabel *ppsGalileoReference = new QLabel;
    QLabel *ppsGalileoGps = new QLabel;
    QLabel *ppsJitter = new QLabel;
    QLabel *ppsSamples = new QLabel;
    QLabel *ppsBackend = new QLabel;
    QLabel *ppsQualified = new QLabel;
    QLabel *ppsError = new QLabel;
    QWidget *ppsPlotWindow = nullptr;
    QCustomPlot *ppsPlot = nullptr;
    QTimer *ppsPlotTimer = new QTimer;
    double ppsPlotStartTime = 0;

    void rebuildReceiverCards();
    void updateCardText(const QString &id, GnssReceiverCard *card);
    void updatePpsText();
    void updatePpsPlot();
    void resetPpsPlot();
    void setupPpsPlot();

    QSharedPointer<Config> config;

signals:
    void requestGnssData(QString);
};

#endif // GNSSDISPLAY_H
