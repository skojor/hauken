#include "gnssanalyzer.h"

GnssAnalyzer::GnssAnalyzer(QSharedPointer<Config> c, int id)
{
    gnssData.id = id;
    config = c;
    connect(updDisplayTimer, &QTimer::timeout, this, &GnssAnalyzer::updDisplay);
}

void GnssAnalyzer::getData(GnssData &data)
{
    if (!updDisplayTimer->isActive()) updDisplayTimer->start(1000);

    mutex.lock();
    calcAvgs(data);
    if (stateInstrumentGnss) {
        analyze(data);
        gnssData = data; // keep a local copy for display purposes
    }
    mutex.unlock();
}

void GnssAnalyzer::calcAvgs(GnssData &data)
{
    if (data.cno > 0) {
        data.avgCnoArray.append(data.cno);
        data.avgCno = 0;
        for (auto &val : data.avgCnoArray)
            data.avgCno += val;
        data.avgCno /= data.avgCnoArray.size();
        while (data.avgCnoArray.size() > 300)
            data.avgCnoArray.pop_front();
    }
    if (data.agc > 0) {
        data.avgAgcArray.append(data.agc);
        data.avgAgc = 0;
        for (auto &val : data.avgAgcArray)
            data.avgAgc += val;
        data.avgAgc /= data.avgAgcArray.size();
        while (data.avgAgcArray.size() > 300)
            data.avgAgcArray.pop_front();
    }
    if (data.jammingIndicator >= 0) {
        data.avgJammingIndicatorArray.append(data.jammingIndicator);
        data.avgJammingIndicator = 0;
        for (auto &val : data.avgJammingIndicatorArray)
            data.avgJammingIndicator += val;
        data.avgJammingIndicator /= data.avgJammingIndicatorArray.size();
        while (data.avgJammingIndicatorArray.size() > 300)
            data.avgJammingIndicatorArray.pop_front();
    }

}

void GnssAnalyzer::analyze(GnssData &data)
{
    data.posOffset = distanceInMeters(data);
    data.altOffset = abs(ownAltitude - data.altitude);
    data.timeOffset = abs(data.timestamp.msecsTo(QDateTime::currentDateTimeUtc()));
    if (data.id < 3) {
        data.cnoOffset = abs(data.cno - data.avgCno);
        data.agcOffset = abs(data.agc - data.avgAgc);
        data.jammingIndicatorOffset = abs(data.jammingIndicator - data.jammingIndicatorOffset);
        checkCnoOffset(data);
        if (checkAgc) {
            checkAgcOffset(data);
            checkJammingIndicator(data);
        }
    }
    checkPosOffset(data);
    checkAltOffset(data);
    checkTimeOffset(data);
}

double GnssAnalyzer::arcInRadians(GnssData &data)
{
    double latitudeArc  = (data.latitude - ownLatitude) * DEG_TO_RAD;
    double longitudeArc = (data.longitude - ownLongitude) * DEG_TO_RAD;
    double latitudeH = sin(latitudeArc * 0.5);
    latitudeH *= latitudeH;
    double lontitudeH = sin(longitudeArc * 0.5);
    lontitudeH *= lontitudeH;
    double tmp = cos(data.latitude * DEG_TO_RAD) * cos(data.latitude * DEG_TO_RAD);
    return 2.0 * asin(sqrt(latitudeH + tmp * lontitudeH));
}

double GnssAnalyzer::distanceInMeters(GnssData &data)
{
    return EARTH_RADIUS_IN_METERS * arcInRadians(data);
}

void GnssAnalyzer::updSettings()
{
    cnoLimit = config->getGnssCnoDeviation();
    agcLimit = config->getGnssAgcDeviation();
    posOffsetLimit = config->getGnssPosOffset();
    altOffsetLimit = config->getGnssAltOffset();
    timeOffsetLimit = config->getGnssTimeOffset();

    ownLatitude = config->getStnLatitude().toDouble();
    ownLongitude = config->getStnLongitude().toDouble();
    ownAltitude = config->getStnAltitude().toDouble();
    if (gnssData.id == 1) {
        checkAgc = config->getGnssSerialPort1MonitorAgc();
        logToFile = config->getGnssSerialPort1TriggerRecording();
    }
    else if (gnssData.id == 2) {
        checkAgc = config->getGnssSerialPort2MonitorAgc();
        logToFile = config->getGnssSerialPort2TriggerRecording();
    }
    else if (gnssData.id == 3) {
        logToFile = config->getGnssInstrumentGnssTriggerRecording();
    }
    emit alarmEnded(); // To update led indicator/text
    //qDebug() << "gnss updsettings";
}

void GnssAnalyzer::updDisplay()
{
    mutex.lock();
    if (gnssData.inUse) {
        QString out;
        QTextStream ts(&out);
        if (gnssData.posValid)
            ts << "<table width=100% style='color:black;width:100%'>";
        else
            ts << "<table style='color:grey'>";
        ts.setRealNumberNotation(QTextStream::FixedNotation);
        ts.setRealNumberPrecision(1);
        ts << "<tr><td>Pos. offset</td><td align>" << (gnssData.posOffset > 999?">999":QString::number(gnssData.posOffset, 'f', 1)) << " m</td></tr>"
           << "<tr><td>Alt. offset</td><td align>" << gnssData.altOffset << " m</td></tr>"
           << "<tr><td>Time offset</td><td>" << (gnssData.timeOffset > 9999?">9999":QString::number(gnssData.timeOffset)) << " ms</td></tr>";
        if (gnssData.id != 3)
            ts << "<tr><td>C/No (offset)</td><td>" << gnssData.cno << " (" << gnssData.cnoOffset << ") dB</td></tr>";
        if (gnssData.agc >= 0)
            ts << "<tr><td>AGC (offset)</td><td>" << gnssData.agc << " (" << gnssData.agcOffset << ") %</td></tr>";
        if (gnssData.jammingIndicator >= 0)
            ts << "<tr><td>CW jammer ind.</td><td>" << gnssData.jammingIndicator  << " %</td></tr>";
        if (gnssData.jammingState != JAMMINGSTATE::UNKNOWN) {
            ts << "<tr><td>Jamming state</td><td>";
            if (gnssData.jammingState == JAMMINGSTATE::NOJAMMING) ts << "No jamming";
            else if (gnssData.jammingState == JAMMINGSTATE::WARNINGFIXOK) ts << "Warning, fix ok";
            else if (gnssData.jammingState == JAMMINGSTATE::CRITICALNOFIX) ts << "Critical, fix lost";
            ts << "</td></tr>";
        }
        ts << "<tr><td>Sats tracked</td><td>" << gnssData.satsTracked;
        if (gnssData.satsTracked == -1) ts << " (man.pos!)";
        ts << "</td></tr>";
        ts << "<tr><td>GNSS type</td><td>" << gnssData.gnssType << "</td></tr>"
           << "</font></table>";
        emit displayGnssData(out, gnssData.id, gnssData.posValid);
    }
    mutex.unlock();
}

void GnssAnalyzer::checkPosOffset(GnssData &data)
{
    QString msg;
    QTextStream ts(&msg);
    ts.setRealNumberNotation(QTextStream::FixedNotation);
    ts.setRealNumberPrecision(1);

    if (data.posValid && posOffsetLimit > 0 && data.posOffset > posOffsetLimit) {
        if (!posOffsetTriggered) {
            ts << "Position offset triggered, current offset: "
               << (data.posOffset > 999?">999":QString::number(data.posOffset, 'f', 1)) << " m"
               << (logToFile ? ". Recording":"");
            posOffsetTriggered = true;
            emit visualAlarm();
        }
        if (logToFile) {
            emit alarm();
        }
    }
    else if (data.posValid && posOffsetTriggered) {
        ts << "Position offset normal";
        posOffsetTriggered = false;
        emit alarmEnded();
    }
    if (!msg.isEmpty() && config->getGnssShowNotifications()) emit toIncidentLog(NOTIFY::TYPE::GNSSANALYZER, QString::number(data.id), msg);
}

void GnssAnalyzer::checkAltOffset(GnssData &data)
{
    QString msg;
    QTextStream ts(&msg);
    ts.setRealNumberNotation(QTextStream::FixedNotation);
    ts.setRealNumberPrecision(1);

    if (data.posValid && altOffsetLimit > 0 && data.altOffset > altOffsetLimit) {
        if (!altOffsetTriggered) {
            ts << "Altitude offset triggered, current offset: "
               << data.altOffset << " m" << (logToFile ? ". Recording":"");
            altOffsetTriggered = true;
            emit visualAlarm();
        }
        if (logToFile) {
            emit alarm();
        }
    }
    else if (data.posValid && altOffsetTriggered) {
        ts << "Altitude offset normal";
        altOffsetTriggered = false;
        emit alarmEnded();
    }
    if (!msg.isEmpty() && config->getGnssShowNotifications()) emit toIncidentLog(NOTIFY::TYPE::GNSSANALYZER, QString::number(data.id), msg);
}

void GnssAnalyzer::checkTimeOffset(GnssData &data)
{
    QString msg;
    QTextStream ts(&msg);
    ts.setRealNumberNotation(QTextStream::FixedNotation);
    ts.setRealNumberPrecision(1);

    if (data.posValid && timeOffsetLimit > 0 && data.timeOffset > timeOffsetLimit) {
        if (!timeOffsetTriggered) {
            ts << "Time offset triggered, current offset: "
               << (data.timeOffset > 9999?">9999":QString::number(data.timeOffset)) << " ms"
               << (logToFile ? ". Recording":"");
            timeOffsetTriggered = true;
            emit visualAlarm();
        }
        if (logToFile) {
            emit alarm();
        }
    }
    else if (data.posValid && timeOffsetTriggered) {
        ts << "Time offset normal";
        timeOffsetTriggered = false;
        emit alarmEnded();
    }
    if (!msg.isEmpty() && config->getGnssShowNotifications()) emit toIncidentLog(NOTIFY::TYPE::GNSSANALYZER, QString::number(data.id), msg);
}

void GnssAnalyzer::checkCnoOffset(GnssData &data)
{
    QString msg;
    QTextStream ts(&msg);
    ts.setRealNumberNotation(QTextStream::FixedNotation);
    ts.setRealNumberPrecision(1);

    if (data.posValid && cnoLimit > 0 && data.cnoOffset > cnoLimit) {
        if (!cnoLimitTriggered) {
            ts << "C/No offset triggered, current offset: " << data.cnoOffset << " dB"
               << (logToFile ? ". Recording":"");
            cnoLimitTriggered = true;
            emit visualAlarm();
        }
        if (logToFile) {
            emit alarm();
        }
    }
    else if (data.posValid && cnoLimitTriggered) {
        ts << "C/No offset normal";
        cnoLimitTriggered = false;
        emit alarmEnded();
    }
    if (!msg.isEmpty() && config->getGnssShowNotifications()) emit toIncidentLog(NOTIFY::TYPE::GNSSANALYZER, QString::number(data.id), msg);
}

void GnssAnalyzer::checkAgcOffset(GnssData &data)
{
    QString msg;
    QTextStream ts(&msg);
    ts.setRealNumberNotation(QTextStream::FixedNotation);
    ts.setRealNumberPrecision(1);

    if (data.posValid && agcLimit > 0 && data.agcOffset > agcLimit) {
        if (!agcLimitTriggered) {
            ts << "AGC offset triggered, current offset: "
               << data.agcOffset << (logToFile ? ". Recording":"");
            agcLimitTriggered = true;
            emit visualAlarm();
        }
        if (logToFile) {
            emit alarm();
        }
    }
    else if (data.posValid && agcLimitTriggered) {
        ts << "AGC offset normal";
        agcLimitTriggered = false;
        emit alarmEnded();
    }
    if (!msg.isEmpty() && config->getGnssShowNotifications()) emit toIncidentLog(NOTIFY::TYPE::GNSSANALYZER, QString::number(data.id), msg);
}

void GnssAnalyzer::checkJammingIndicator(GnssData &data)
{
    QString msg;
    QTextStream ts(&msg);
    ts.setRealNumberNotation(QTextStream::FixedNotation);
    ts.setRealNumberPrecision(1);

    if (data.posValid && data.jammingIndicator > jammingIndicatorTriggerValue) {
        if (!jamIndTriggered) {
            ts << "Jamming indicator high: " << data.avgJammingIndicator << (logToFile ? ". Recording":"");
            jamIndTriggered = true;
            emit visualAlarm();
        }
        if (logToFile) {
            emit alarm();
        }
    }
    else if (data.posValid && jamIndTriggered) {
        ts << "Jamming indicator normal";
        jamIndTriggered = false;
        emit alarmEnded();
    }
    if (!msg.isEmpty() && config->getGnssShowNotifications()) emit toIncidentLog(NOTIFY::TYPE::GNSSANALYZER, QString::number(data.id), msg);
}
