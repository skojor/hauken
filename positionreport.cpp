#include "positionreport.h"

PositionReport::PositionReport(QObject *parent)
    : Config{parent}
{
    connect(curlProcess, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished), this, &PositionReport::checkReturnValue);
    curlProcess->setWorkingDirectory(QDir(QCoreApplication::applicationDirPath()).absolutePath());

    if (QSysInfo::kernelType().contains("win")) {
        curlProcess->setProgram("curl.exe");
    }

    else if (QSysInfo::kernelType().contains("linux")) {
        curlProcess->setProgram("curl");
    }
    updSettings();

    connect(reportTimer, &QTimer::timeout, this, [this] {
        generateReport();
        sendReport();
    });
    connect(gnssReqTimer, &QTimer::timeout, this, [this] {
        emit reqPosition(posSource);
    });

    if (!posSource.isEmpty()) gnssReqTimer->start(1000);
}

void PositionReport::configReportTimer()
{
    if (posReportActive and reportInterval > 0) {
        reportTimer->start(reportInterval * 1e3);
    }
    else {
        reportTimer->stop();
    }
}

void PositionReport::generateReport()
{
    reportArgs.clear();
    reportArgs << "-H" << "Content-Type:application/x-www-form-urlencoded"
               << "-s" << "-w" << "%{http_code}";
    reportArgs << "--data" << "id=" + (id.isEmpty() ? getSdefStationInitals() : id)
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
    if (addConnStats) {
        reportArgs << "--data" << "state=" + QString((measurementDeviceConnected?"connected":"disconnected"))
                   << "--data" << "startFreq=" + QString::number(devicePtr->pscanStartFrequency)
                   << "--data" << "stopFreq=" + QString::number(devicePtr->pscanStopFrequency)
                   << "--data" << "res=" + QString::number(devicePtr->pscanResolution);
    }

    reportArgs << url;
}

void PositionReport::sendReport()
{
    curlProcess->setArguments(reportArgs);
    //qDebug() << "req to send" << reportArgs;
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
    addConnStats = getPosReportAddConnStats();
    posSource = getPosReportSource();
    url = getPosReportUrl();
    reportInterval = getPosReportSendInterval();
    id = getPosReportId();

    configReportTimer();
    if (!devicePtr) emit reqMeasurementDevicePtr();
}

void PositionReport::checkReturnValue(int exitCode, QProcess::ExitStatus)
{
    if (exitCode != 0 || !curlProcess->readAllStandardOutput().contains("200")) {
        qDebug() << "Report failed" << reportArgs << curlProcess->readAllStandardError() << curlProcess->readAllStandardOutput() << exitCode;
    }
    else {
        //qDebug() << "Report sent";
    }
}
