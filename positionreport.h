#ifndef POSITIONREPORT_H
#define POSITIONREPORT_H

#include "config.h"
#include <QWidget>
#include <QDebug>
#include <QDateTime>
#include <QTimer>
#include <QElapsedTimer>
#include <QTextStream>
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QLineEdit>
#include <QCheckBox>
#include <QLabel>
#include <QDialogButtonBox>
#include <QPushButton>
#include "config.h"
#include "typedefs.h"
#include <QProcess>


class PositionReport : public Config
{
    Q_OBJECT
public:
    explicit PositionReport(QObject *parent = nullptr);
    void start();
    void updSettings();

private slots:
    void configReportTimer();
    void generateReport();
    void sendReport();
    void checkReturnValue(int exitCode, QProcess::ExitStatus);

public slots:
    void updPosition(GnssData &data) { gnssData = data;}

private:
    QTimer *reportTimer = new QTimer;
    QProcess *curlProcess = new QProcess;

    // config cache
    bool posReportActive, addPosition, addCogSog, addGnssStats;
    QString posSource, url;
    int reportInterval;
    GnssData gnssData;
    QStringList reportArgs;

signals:
    void reqPosition(QString);
};

#endif // POSITIONREPORT_H
