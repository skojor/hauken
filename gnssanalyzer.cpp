#include "gnssanalyzer.h"

GnssAnalyzer::GnssAnalyzer(QObject *parent, int i)
    : Config{parent}
{
    gnssData.id = i;
    connect(updDisplayTimer, &QTimer::timeout, this, &GnssAnalyzer::updDisplay);
}

void GnssAnalyzer::getData(GnssData &data)
{
    if (!updDisplayTimer->isActive()) updDisplayTimer->start(1000);

    mutex.lock();
    calcAvgs(data);
    analyze(data);

    gnssData = data; // keep a local copy for display purposes
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
}

void GnssAnalyzer::analyze(GnssData &data)
{
    data.posOffset = distanceInMeters(data);
    data.altOffset = abs(ownAltitude - data.altitude);
    data.timeOffset = abs(data.timestamp.msecsTo(QDateTime::currentDateTimeUtc()));
    data.cnoOffset = abs(data.cno - data.avgCno);
    data.agcOffset = abs(data.agc - data.avgAgc);

    checkPosValid(data);
    checkPosOffset(data);
    checkAltOffset(data);
    checkTimeOffset(data);
    checkCnoOffset(data);
    if (checkAgc) checkAgcOffset(data);
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
    cnoLimit = getGnssCnoDeviation();
    agcLimit = getGnssAgcDeviation();
    posOffsetLimit = getGnssPosOffset();
    altOffsetLimit = getGnssAltOffset();
    timeOffsetLimit = getGnssTimeOffset();

    ownLatitude = getStnLatitude().toDouble();
    ownLongitude = getStnLongitude().toDouble();
    ownAltitude = getStnAltitude().toDouble();
    if (gnssData.id == 1) {
        checkAgc = getGnssSerialPort1MonitorAgc();
        logToFile = getGnssSerialPort1TriggerRecording();
    }
    else {
        checkAgc = getGnssSerialPort2MonitorAgc();
        logToFile = getGnssSerialPort2TriggerRecording();
    }
}

void GnssAnalyzer::updDisplay()
{
    mutex.lock();
    if (gnssData.inUse) {
        QString out;
        QTextStream ts(&out);
        //bool valid = gnssData.posValid;
        if (gnssData.posValid)
            ts << "<table style='color:black'>";
        else
            ts << "<table style='color:grey'>";
        ts.setRealNumberNotation(QTextStream::FixedNotation);
        ts.setRealNumberPrecision(1);
        ts << "<tr><td>Pos. offset</td><td align=right>" << (gnssData.posOffset > 999?">999":QString::number(gnssData.posOffset, 'f', 1)) << "</td><td>m</td></tr>"
           << "<tr><td>Alt. offset</td><td align=right>" << gnssData.altOffset << "</td><td>m</td></tr>"
           << "<tr><td>Time offset</td><td align=right>" << (gnssData.timeOffset > 9999?">9999":QString::number(gnssData.timeOffset)) << "</td><td>ms</td></tr>"
           << "<tr><td>C/No (offset)</td><td align=right>" << gnssData.cno << " (" << gnssData.cnoOffset << ")</td><td>dB</td></tr>";
        if (gnssData.agc > 0)
            ts << "<tr><td>AGC (offset)</td><td align=right>" << gnssData.altOffset << " (" << gnssData.agcOffset << ")</td><td>m</td></tr>";

        ts << "<tr><td>Sats tracked</td><td align=right>" << gnssData.satsTracked << "</td></tr>"
           << "<tr><td>GNSS type</td><td align=right>" << gnssData.gnssType << "</td></tr>"
           << "</font></table>";
        emit displayGnssData(out, gnssData.id, gnssData.posValid);
    }
    mutex.unlock();
}

void GnssAnalyzer::checkPosValid(GnssData &data)
{
    QString msg;
    QTextStream ts(&msg);
    ts.setRealNumberNotation(QTextStream::FixedNotation);
    ts.setRealNumberPrecision(1);

    if (!data.posValid && !posInvalidTriggered) { // any recording triggered because position goes invalid will only last for minutes set in sdef config (record time after incident).
                                                  // this to not always record, in case gnss has failed somehow. other triggers will renew as long as the trigger is valid.
        ts << "GNSS " << data.id << ": Position invalid"
           ;//<< (logToFile ? ". Recording":"");
        posInvalidTriggered = true;
        /*if (logToFile) {
            emit alarm();
        }*/
    }
    else if (data.posValid && posInvalidTriggered) {
        ts << "GNSS " << data.id << ": Position valid";
        posInvalidTriggered = false;
    }
    if (!msg.isEmpty()) emit toIncidentLog(msg);
}

void GnssAnalyzer::checkPosOffset(GnssData &data)
{
    QString msg;
    QTextStream ts(&msg);
    ts.setRealNumberNotation(QTextStream::FixedNotation);
    ts.setRealNumberPrecision(1);

    if (data.posValid && posOffsetLimit > 0 && data.posOffset > posOffsetLimit) {
        if (!posOffsetTriggered) ts << "GNSS " << data.id << ": Position offset triggered, current offset: "
                                    << (data.posOffset > 999?">999":QString::number(data.posOffset, 'f', 1)) << " m"
                                    << (logToFile ? ". Recording":"");
        posOffsetTriggered = true;
        if (logToFile) {
            emit alarm();
        }
    }
    else {
        if (data.posValid && posOffsetTriggered) ts << "GNSS " << data.id << ": Position offset normal";
        posOffsetTriggered = false;
    }
    if (!msg.isEmpty()) emit toIncidentLog(msg);
}

void GnssAnalyzer::checkAltOffset(GnssData &data)
{
    QString msg;
    QTextStream ts(&msg);
    ts.setRealNumberNotation(QTextStream::FixedNotation);
    ts.setRealNumberPrecision(1);

    if (data.posValid && altOffsetLimit > 0 && data.altOffset > altOffsetLimit) {
        if (!altOffsetTriggered) ts << "GNSS " << data.id << ": Altitude offset triggered, current offset: "
                                    << data.altOffset << " m" << (logToFile ? ". Recording":"");
        altOffsetTriggered = true;
        if (logToFile) {
            emit alarm();
        }
    }
    else {
        if (data.posValid && altOffsetTriggered) ts << "GNSS " << data.id << ": Altitude offset normal";
        altOffsetTriggered = false;
    }
    if (!msg.isEmpty()) emit toIncidentLog(msg);
}

void GnssAnalyzer::checkTimeOffset(GnssData &data)
{
    QString msg;
    QTextStream ts(&msg);
    ts.setRealNumberNotation(QTextStream::FixedNotation);
    ts.setRealNumberPrecision(1);

    if (data.posValid && timeOffsetLimit > 0 && data.timeOffset > timeOffsetLimit) {
        if (!timeOffsetTriggered) ts << "GNSS " << data.id << ": Time offset triggered, current offset: "
                                     << (data.timeOffset > 9999?">9999":QString::number(data.timeOffset)) << " ms"
                                     << (logToFile ? ". Recording":"");
        timeOffsetTriggered = true;
        if (logToFile) {
            emit alarm();
        }
    }
    else {
        if (data.posValid && timeOffsetTriggered) ts << "GNSS " << data.id << ": Time offset normal";
        timeOffsetTriggered = false;
    }
    if (!msg.isEmpty()) emit toIncidentLog(msg);
}

void GnssAnalyzer::checkCnoOffset(GnssData &data)
{
    QString msg;
    QTextStream ts(&msg);
    ts.setRealNumberNotation(QTextStream::FixedNotation);
    ts.setRealNumberPrecision(1);

    if (data.posValid && cnoLimit > 0 && data.cnoOffset > cnoLimit) {
        if (!cnoLimitTriggered) ts << "GNSS " << data.id << ": C/No offset triggered, current offset: " << data.cnoOffset << " dB"
                                   << (logToFile ? ". Recording":"");
        cnoLimitTriggered = true;
        if (logToFile) {
            emit alarm();
        }
    }
    else {
        if (data.posValid && cnoLimitTriggered) ts << "GNSS " << data.id << ": C/No offset normal";
        cnoLimitTriggered = false;
    }
    if (!msg.isEmpty()) emit toIncidentLog(msg);
}

void GnssAnalyzer::checkAgcOffset(GnssData &data)
{
    QString msg;
    QTextStream ts(&msg);
    ts.setRealNumberNotation(QTextStream::FixedNotation);
    ts.setRealNumberPrecision(1);

    if (data.posValid && agcLimit > 0 && data.agcOffset > agcLimit) {
        if (!agcLimitTriggered) ts << "GNSS " << data.id << ": AGC offset triggered, current offset: "
                                   << data.agcOffset << " m" << (logToFile ? ". Recording":"");
        agcLimitTriggered = true;
        if (logToFile) {
            emit alarm();
        }
    }
    else {
        if (data.posValid && agcLimitTriggered) ts << "GNSS " << data.id << ": AGC offset normal";
        agcLimitTriggered = false;
    }
    if (!msg.isEmpty()) emit toIncidentLog(msg);
}