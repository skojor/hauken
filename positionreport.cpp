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
    connect(sensorDataTimer, &QTimer::timeout, this, [this] {
        emit reqSensorData(sensorTemp, sensorHumidity);
        //qDebug() << "Data received" << sensorTemp << sensorHumidity << "";
        if (sensorTemp > -30 && sensorHumidity > 0) sensorDataValid = true;
    });

    if (!posSource.isEmpty()) gnssReqTimer->start(1000);
    if (addSensorData && getArduinoReadDHT20()) sensorDataTimer->start(60 * 1e3);

    uptime.start();
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
        QString state = "disconnected";
        if (measurementDeviceConnected && !inUse) state = "connected";
        else if (inUse) state = "busy";
        if (!inUseBy.isEmpty()) state += " (" + inUseBy + ")";
        reportArgs << "--data" << "state=" + state
                   << "--data" << "startFreq=" + QString::number(devicePtr->pscanStartFrequency)
                   << "--data" << "stopFreq=" + QString::number(devicePtr->pscanStopFrequency)
                   << "--data" << "res=" + QString::number(devicePtr->pscanResolution)
                   << "--data" << "uptime=" + QString::number((int)(uptime.elapsed() / 1e3));
    }

    if (addSensorData && sensorDataValid) {
        reportArgs << "--data" << "temp=" + QString::number(sensorTemp, 'f', 1)
                   << "--data" << "humidity=" + QString::number((int)sensorHumidity);
    }

    if (addMqttData) {
        for (int i=0; i<mqttNames.size(); i++) {
            qDebug() << mqttLastUpdated[i].secsTo(QDateTime::currentDateTime()) << mqttNames[i];
            if (mqttLastUpdated[i].secsTo(QDateTime::currentDateTime()) < 600) {
                reportArgs << "--data" << mqttNames[i] + "=" + QString::number(mqttValues[i]);
            }
        }
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
    addSensorData = getPosReportAddSensorData();
    addMqttData = getPosReportAddMqttData();
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
        //qDebug() << "Report failed" << reportArgs << curlProcess->readAllStandardError() << curlProcess->readAllStandardOutput() << exitCode;
    }
    else {
        //qDebug() << "Report sent";
    }
}

void PositionReport::updMqttData(QString& name, double& val)
{
    if (mqttNames.isEmpty()) {
        mqttNames.append(name);
        mqttValues.append(val);
        mqttLastUpdated.append(QDateTime::currentDateTime());
    }
    else {
        int i;
        for (i=0; i<mqttNames.size(); i++) {
            if (mqttNames[i] == name) {
                mqttValues[i] = val;
                mqttLastUpdated[i] = QDateTime::currentDateTime();
                break;
            }
        }
        if (i == mqttNames.size()) { // not found, add it
            mqttNames.append(name);
            mqttValues.append(val);
            mqttLastUpdated.append(QDateTime::currentDateTime());
        }
    }
    //qDebug() << "sig" << name << val << mqttNames.size() << mqttValues.size();
}
