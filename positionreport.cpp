#include "positionreport.h"

PositionReport::PositionReport(QObject *parent)
    : Config{parent}
{
    connect(curlProcess, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished), this, &PositionReport::checkReturnValue);

    updSettings();

    connect(reportTimer, &QTimer::timeout, this, [this] {
        if (!posSource.isEmpty()) emit reqPosition(posSource);
        generateReport();
        sendReport();
    });
}

void PositionReport::configReportTimer()
{
    if (posReportActive) {
        reportTimer->start(reportInterval * 1e3);
    }
    else {
        reportTimer->stop();
    }
}

void PositionReport::generateReport()
{
    reportArgs << "-H" << "Content-Type: application/x-www-form-urlencoded";
    reportArgs << "--data" << "id=" + getSdefStationInitals()
         << "--data" << "timestamp=" + QString::number(gnssData.timestamp.toMSecsSinceEpoch() / 1000);
    if (gnssData.posValid) {
        reportArgs << "--data" << "posValid=true";
        if (addPosition) {
            reportArgs << "--data" << "lat=" + QString::number(gnssData.latitude)
                 << "--data" << "lon=" + QString::number(gnssData.longitude);
        }
        if (addCogSog) {
            reportArgs << "--data" << "cog=" + QString::number(gnssData.cog, 'f', 1)
                 << "--data" << "sog=" + QString::number(gnssData.sog, 'f', 1);
        }
    }
    else {
        reportArgs << "--data" << "posValid=false";
    }
    if (addGnssStats) {
        reportArgs << "--data" << "dop=" + QString::number(gnssData.hdop, 'f', 1)
             << "--data" << "sats=" + QString::number(gnssData.satsTracked);
    }
}

void PositionReport::sendReport()
{
    curlProcess->setArguments(reportArgs);
    curlProcess->start();
}

void PositionReport::updSettings()
{
    if (getPosReportActivated() != posReportActive) {
        posReportActive = getPosReportActivated();
        configReportTimer();
    }
    addPosition = getPosReportAddPos();
    addCogSog = getPosReportAddSogCog();
    addGnssStats = getPosReportAddGnssStats();
    posSource = getPosReportSource();
    url = getPosReportUrl();
    reportInterval = getPosReportSendInterval();

    configReportTimer();
}

void PositionReport::checkReturnValue(int exitCode, QProcess::ExitStatus)
{
    if (exitCode != 0) {
        qDebug() << "Report failed" << curlProcess->readAllStandardError() << curlProcess->readAllStandardOutput();
    }
    else {
        qDebug() << "Report sent";
    }
}
