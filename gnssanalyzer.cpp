#include "gnssanalyzer.h"

GnssAnalyzer::GnssAnalyzer(QSharedPointer<Config> c, const QString &profileId)
{
    gnssData.id = profileId;
    config = c;
    connect(updDisplayTimer, &QTimer::timeout, this, &GnssAnalyzer::updDisplay);

    if (RUNTESTS) tests();
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
    const bool pauseSignalAverages = shouldPauseSignalAverages();

    if (!pauseSignalAverages && data.cno > 0) {
        data.avgCnoArray.append(data.cno);
        data.avgCno = 0;
        for (auto &val : data.avgCnoArray)
            data.avgCno += val;
        data.avgCno /= data.avgCnoArray.size();
        while (data.avgCnoArray.size() > 300)
            data.avgCnoArray.pop_front();
    }
    if (!pauseSignalAverages && data.agc >= 0) {
        data.avgAgcArray.append(data.agc);
        data.avgAgc = 0;
        for (auto &val : data.avgAgcArray)
            data.avgAgc += val;
        data.avgAgc /= data.avgAgcArray.size();
        while (data.avgAgcArray.size() > 300)
            data.avgAgcArray.pop_front();
    }
    if (!pauseSignalAverages) {
        for (auto iterator = data.agcBlocks.begin(); iterator != data.agcBlocks.end(); ++iterator) {
            GnssAgcBlockData &block = iterator.value();
            if (block.agc < 0) continue;

            block.avgAgcArray.append(block.agc);
            block.avgAgc = 0;
            for (int value : block.avgAgcArray)
                block.avgAgc += value;
            block.avgAgc /= block.avgAgcArray.size();
            while (block.avgAgcArray.size() > 300)
                block.avgAgcArray.pop_front();
        }
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

void GnssAnalyzer::traceIncidentStarted()
{
    QMutexLocker locker(&mutex);
    traceIncidentActive = true;
    traceIncidentGraceTimer.invalidate();
}

void GnssAnalyzer::traceIncidentEnded()
{
    QMutexLocker locker(&mutex);
    traceIncidentActive = false;
    traceIncidentGraceTimer.start();
}

bool GnssAnalyzer::shouldPauseSignalAverages()
{
    if (traceIncidentActive) return true;

    if (!traceIncidentGraceTimer.isValid()) return false;

    if (traceIncidentGraceTimer.elapsed() < traceIncidentAverageGraceTimeMs) return true;

    traceIncidentGraceTimer.invalidate();
    return false;
}

void GnssAnalyzer::analyze(GnssData &data)
{
    data.posOffset = distanceInMeters(data);
    data.altOffset = abs(ownAltitude - data.altitude);
    data.timeOffset = abs(data.timestamp.msecsTo(QDateTime::currentDateTimeUtc()));
    if (data.id != "instrument") {
        data.cnoOffset = abs(data.cno - data.avgCno);
        data.agcOffset = abs(data.agc - data.avgAgc);
        for (auto iterator = data.agcBlocks.begin(); iterator != data.agcBlocks.end(); ++iterator) {
            GnssAgcBlockData &block = iterator.value();
            if (block.avgAgc >= 0) block.agcOffset = abs(block.agc - block.avgAgc);
        }
        data.jammingIndicatorOffset = abs(data.jammingIndicator - data.jammingIndicatorOffset);
        checkCnoOffset(data);
        if (checkAgc) {
            if (data.agcBlocks.isEmpty()) checkAgcOffset(data);
            else checkAgcBlockOffsets(data);
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

    const GnssReceiverProfile profile = config->getGnssReceiverProfile(gnssData.id);
    isInstrumentType = profile.type == GnssReceiverProfile::Type::Instrument;
    logToFile = profile.triggerRecording;
    checkAgc = !isInstrumentType && profile.ublox;
    if (!anyOffsetTriggered()) emit alarmEnded(); // To update led indicator/text
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
        if (!isInstrumentType)
            ts << "<tr><td>C/No (offset)</td><td>" << gnssData.cno << " (" << gnssData.cnoOffset << ") dB</td></tr>";
        if (!gnssData.agcBlocks.isEmpty()) {
            for (auto iterator = gnssData.agcBlocks.cbegin(); iterator != gnssData.agcBlocks.cend(); ++iterator) {
                const QString label = iterator.value().frequencyMapped
                                          ? QString("AGC %1").arg(iterator.value().frequencyLabel)
                                          : QString("AGC block %1").arg(iterator.key());
                ts << "<tr><td>" << label << "</td><td>"
                   << iterator.value().agc << " (" << iterator.value().agcOffset << ") %</td></tr>";
            }
        }
        else if (gnssData.agc >= 0)
            ts << "<tr><td>AGC (offset)</td><td>" << gnssData.agc << " (" << gnssData.agcOffset << ") %</td></tr>";
        ts << "<tr><td>Sats tracked</td><td>" << gnssData.satsTracked;
        if (gnssData.satsTracked == -1) ts << " (man.pos!)";
        ts << "</td></tr>";
        ts << "<tr><td>GNSS type</td><td>" << gnssData.gnssType << "</td></tr>"
           << "</font></table>";
        emit displayGnssData(out, gnssData.id, gnssData.posValid);
    }
    mutex.unlock();
}


bool GnssAnalyzer::anyOffsetTriggered() const
{
    return posOffsetTriggered || altOffsetTriggered || timeOffsetTriggered
           || cnoLimitTriggered || agcLimitTriggered || jamIndTriggered;
}

void GnssAnalyzer::emitAlarmEndedIfNoOffsetTriggered()
{
    if (!anyOffsetTriggered()) emit alarmEnded();
}

void GnssAnalyzer::checkPosOffset(GnssData &data)
{
    QString msg;
    QTextStream ts(&msg);
    ts.setRealNumberNotation(QTextStream::FixedNotation);
    ts.setRealNumberPrecision(1);

    if (data.posValid && posOffsetLimit > 0 && data.posOffset > posOffsetLimit) {
        if (!posOffsetTriggered && !posOffsetFilterTimer.isValid()) { // First activation, start timer to filter short term events
            posOffsetFilterTimer.start();
        }
        else if (!posOffsetTriggered && posOffsetFilterTimer.elapsed() > config->getGnssTimeFilter() * 1e3) {
            ts << "Position offset triggered, current offset: "
               << (data.posOffset > 999?">999":QString::number(data.posOffset, 'f', 1)) << " m"
               << (logToFile ? ". Recording":"");
            posOffsetTriggered = true;
            posOffsetTimer.start();
            posOffsetFilterTimer.invalidate();
            emit visualAlarm();
        }
        if (logToFile) {
            emit alarm();
        }
    }
    else if (data.posValid && posOffsetTriggered) {
        ts << "Position offset normal after " << posOffsetTimer.elapsed() / 1e3 << " seconds";
        posOffsetTimer.invalidate();
        posOffsetTriggered = false;
        emitAlarmEndedIfNoOffsetTriggered();
    }
    else if (data.posValid && posOffsetFilterTimer.isValid()) { // Normal sit. before incidence reported. Deactivate timer
        posOffsetFilterTimer.invalidate();
        qDebug() << "Inc. gone before filter time";
    }
    if (!msg.isEmpty() && config->getGnssShowNotifications()) emit toIncidentLog(NOTIFY::TYPE::GNSSANALYZER, config->getGnssReceiverProfile(data.id).name, msg);
}

void GnssAnalyzer::checkAltOffset(GnssData &data)
{
    QString msg;
    QTextStream ts(&msg);
    ts.setRealNumberNotation(QTextStream::FixedNotation);
    ts.setRealNumberPrecision(1);

    if (data.posValid && altOffsetLimit > 0 && data.altOffset > altOffsetLimit) {
        if (!altOffsetTriggered && !altOffsetFilterTimer.isValid())
            altOffsetFilterTimer.start();

        else if (!altOffsetTriggered && altOffsetFilterTimer.elapsed() > config->getGnssTimeFilter() * 1e3) {
            ts << "Altitude offset triggered, current offset: "
               << data.altOffset << " m" << (logToFile ? ". Recording":"");
            altOffsetTriggered = true;
            altOffsetTimer.start();
            altOffsetFilterTimer.invalidate();
            emit visualAlarm();
        }
        if (logToFile) {
            emit alarm();
        }
    }
    else if (data.posValid && altOffsetTriggered) {
        ts << "Altitude offset normal after " << altOffsetTimer.elapsed() / 1e3 << " seconds";
        altOffsetTimer.invalidate();
        altOffsetTriggered = false;
        emitAlarmEndedIfNoOffsetTriggered();
    }
    else if (altOffsetFilterTimer.isValid() && data.posValid)
        altOffsetFilterTimer.invalidate();

    if (!msg.isEmpty() && config->getGnssShowNotifications()) emit toIncidentLog(NOTIFY::TYPE::GNSSANALYZER, config->getGnssReceiverProfile(data.id).name, msg);
}

void GnssAnalyzer::checkTimeOffset(GnssData &data)
{
    QString msg;
    QTextStream ts(&msg);
    ts.setRealNumberNotation(QTextStream::FixedNotation);
    ts.setRealNumberPrecision(1);

    if (data.posValid && timeOffsetLimit > 0 && data.timeOffset > timeOffsetLimit) {
        if (!timeOffsetTriggered && !timeOffsetFilterTimer.isValid())
            timeOffsetFilterTimer.start();

        else if (!timeOffsetTriggered && timeOffsetFilterTimer.elapsed() > config->getGnssTimeFilter() * 1e3) {
            ts << "Time offset triggered, current offset: "
               << (data.timeOffset > 9999?">9999":QString::number(data.timeOffset)) << " ms"
               << (logToFile ? ". Recording":"");
            timeOffsetTriggered = true;
            timeOffsetTimer.start();
            timeOffsetFilterTimer.invalidate();
            emit visualAlarm();
        }
        if (logToFile) {
            emit alarm();
        }
    }
    else if (data.posValid && timeOffsetTriggered) {
        ts << "Time offset normal after " << timeOffsetTimer.elapsed() / 1e3 << " seconds";
        timeOffsetTimer.invalidate();
        timeOffsetTriggered = false;
        emitAlarmEndedIfNoOffsetTriggered();
    }
    else if (timeOffsetFilterTimer.isValid() && data.posValid)
        timeOffsetFilterTimer.invalidate();

    if (!msg.isEmpty() && config->getGnssShowNotifications()) emit toIncidentLog(NOTIFY::TYPE::GNSSANALYZER, config->getGnssReceiverProfile(data.id).name, msg);
}

void GnssAnalyzer::checkCnoOffset(GnssData &data)
{
    QString msg;
    QTextStream ts(&msg);
    ts.setRealNumberNotation(QTextStream::FixedNotation);
    ts.setRealNumberPrecision(1);

    if (data.posValid && cnoLimit > 0 && data.cnoOffset > cnoLimit) {
        if (!cnoLimitTriggered && !cnoFilterTimer.isValid())
            cnoFilterTimer.start();

        else if (!cnoLimitTriggered && cnoFilterTimer.elapsed() > config->getGnssTimeFilter() * 1e3) {
            ts << "C/No offset triggered, current offset: " << data.cnoOffset << " dB"
               << (logToFile ? ". Recording":"");
            cnoLimitTriggered = true;
            cnoTimer.start();
            cnoFilterTimer.invalidate();
            emit visualAlarm();
        }
        if (logToFile) {
            emit alarm();
        }
    }
    else if (data.posValid && cnoLimitTriggered) {
        ts << "C/No offset normal after " << cnoTimer.elapsed() / 1e3 << " seconds";
        cnoTimer.invalidate();
        cnoLimitTriggered = false;
        emitAlarmEndedIfNoOffsetTriggered();
    }
    else if (cnoFilterTimer.isValid() && data.posValid)
        cnoFilterTimer.invalidate();

    if (!msg.isEmpty() && config->getGnssShowNotifications()) emit toIncidentLog(NOTIFY::TYPE::GNSSANALYZER, config->getGnssReceiverProfile(data.id).name, msg);
}

void GnssAnalyzer::checkAgcOffset(GnssData &data)
{
    QString msg;
    QTextStream ts(&msg);
    ts.setRealNumberNotation(QTextStream::FixedNotation);
    ts.setRealNumberPrecision(1);

    if (data.posValid && agcLimit > 0 && data.agcOffset > agcLimit) {
        if (!agcLimitTriggered && !agcFilterTimer.isValid())
            agcFilterTimer.start();

        else if (!agcLimitTriggered && agcFilterTimer.elapsed() > config->getGnssTimeFilter() * 1e3) {
            ts << "AGC offset triggered, current offset: "
               << data.agcOffset << (logToFile ? ". Recording":"");
            agcLimitTriggered = true;
            agcTimer.start();
            agcFilterTimer.invalidate();
            emit visualAlarm();
        }
        if (logToFile) {
            emit alarm();
        }
    }
    else if (data.posValid && agcLimitTriggered) {
        ts << "AGC offset normal after " << agcTimer.elapsed() / 1e3 << " seconds";
        agcTimer.invalidate();
        agcLimitTriggered = false;
        emitAlarmEndedIfNoOffsetTriggered();
    }
    else if (agcFilterTimer.isValid() && data.posValid)
        agcFilterTimer.invalidate();

    if (!msg.isEmpty() && config->getGnssShowNotifications()) emit toIncidentLog(NOTIFY::TYPE::GNSSANALYZER, config->getGnssReceiverProfile(data.id).name, msg);
}

void GnssAnalyzer::checkAgcBlockOffsets(GnssData &data)
{
    QStringList messages;
    const bool previouslyTriggered = agcLimitTriggered;
    const auto knownBlockIds = agcBlockLimitTriggered.keys();
    for (int blockId : knownBlockIds) {
        if (!data.agcBlocks.contains(blockId)) {
            agcBlockLimitTriggered.remove(blockId);
            agcBlockTimers.remove(blockId);
            agcBlockFilterTimers.remove(blockId);
        }
    }

    for (auto iterator = data.agcBlocks.cbegin(); iterator != data.agcBlocks.cend(); ++iterator) {
        const int blockId = iterator.key();
        const GnssAgcBlockData &block = iterator.value();
        bool &triggered = agcBlockLimitTriggered[blockId];
        QElapsedTimer &filterTimer = agcBlockFilterTimers[blockId];

        if (data.posValid && agcLimit > 0 && block.agcOffset > agcLimit) {
            if (!triggered && !filterTimer.isValid()) {
                filterTimer.start();
            }
            else if (!triggered && filterTimer.elapsed() > config->getGnssTimeFilter() * 1e3) {
                const QString blockLabel = block.frequencyMapped
                                               ? QString("%1 - %2").arg(blockId).arg(block.frequencyLabel)
                                               : QString::number(blockId);
                messages.append(QString("AGC block %1 offset triggered, current offset: %2%3")
                                    .arg(blockLabel).arg(block.agcOffset)
                                    .arg(logToFile ? ". Recording" : ""));
                triggered = true;
                agcBlockTimers[blockId].start();
                filterTimer.invalidate();
                emit visualAlarm();
            }
            if (logToFile) emit alarm();
        }
        else if (data.posValid && triggered) {
            const QString blockLabel = block.frequencyMapped
                                           ? QString("%1 - %2").arg(blockId).arg(block.frequencyLabel)
                                           : QString::number(blockId);
            messages.append(QString("AGC block %1 offset normal after %2 seconds")
                                .arg(blockLabel).arg(agcBlockTimers[blockId].elapsed() / 1e3));
            agcBlockTimers.remove(blockId);
            triggered = false;
        }
        else if (data.posValid && filterTimer.isValid()) {
            filterTimer.invalidate();
        }
    }

    agcLimitTriggered = false;
    for (bool triggered : std::as_const(agcBlockLimitTriggered))
        agcLimitTriggered |= triggered;
    if (previouslyTriggered && !agcLimitTriggered) emitAlarmEndedIfNoOffsetTriggered();

    if (!messages.isEmpty() && config->getGnssShowNotifications())
        emit toIncidentLog(NOTIFY::TYPE::GNSSANALYZER, config->getGnssReceiverProfile(data.id).name, messages.join("; "));
}

void GnssAnalyzer::checkJammingIndicator(GnssData &data)
{
    QString msg;
    QTextStream ts(&msg);
    ts.setRealNumberNotation(QTextStream::FixedNotation);
    ts.setRealNumberPrecision(1);

    if (data.posValid && data.jammingIndicator > jammingIndicatorTriggerValue) {
        if (!jamIndTriggered && !jamIndFilterTimer.isValid())
            jamIndFilterTimer.start();

        else if (!jamIndTriggered && jamIndFilterTimer.elapsed() > config->getGnssTimeFilter() * 1e3) {
            ts << "Jamming indicator high: " << data.avgJammingIndicator << (logToFile ? ". Recording":"");
            jamIndTriggered = true;
            jamIndTimer.start();
            jamIndFilterTimer.invalidate();
            emit visualAlarm();
        }
        if (logToFile) {
            emit alarm();
        }
    }
    else if (data.posValid && jamIndTriggered) {
        ts << "Jamming indicator normal after " << jamIndTimer.elapsed() / 1e3 << " seconds";
        jamIndTimer.invalidate();
        jamIndTriggered = false;
        emitAlarmEndedIfNoOffsetTriggered();
    }
    else if (jamIndFilterTimer.isValid() && data.posValid)
        jamIndFilterTimer.invalidate();

    if (!msg.isEmpty() && config->getGnssShowNotifications()) emit toIncidentLog(NOTIFY::TYPE::GNSSANALYZER, config->getGnssReceiverProfile(data.id).name, msg);
}

void GnssAnalyzer::tests()
{
    testData.gsaValid = testData.gnsValid = testData.ggaValid = testData.gsvValid = testData.posValid = testData.inUse = true;
    testData.latitude = config->getStnLatitude().toDouble();
    testData.longitude = config->getStnLongitude().toDouble();
    testData.altitude = config->getStnAltitude().toDouble();
    testData.agc = 40;

    QTimer *t = new QTimer(this);
    qDebug() << "Starting gnss simulation";
    connect(t, &QTimer::timeout, this, [this] () {
        getData(testData);
    });
    t->start(1000);

    QTimer::singleShot(config->getGnssTimeFilter() * 2e3, this, [this] () {
        qDebug() << "Now adding pos. offset";
        testData.latitude += 0.01;
        testData.longitude -= 0.01;
        testData.altitude -= 100;
        testData.timestamp = QDateTime::currentDateTime().addSecs(10);
    });
    QTimer::singleShot((config->getGnssTimeFilter() * 2e3) + 2e3, this, [this] () {
        qDebug() << "Removing pos. offset after 2 secs";
        testData.latitude -= 0.01;
        testData.longitude += 0.01;
        testData.altitude += 100;
    });
    QTimer::singleShot((config->getGnssTimeFilter() * 2e3) + 6e3, this, [this] () {
        qDebug() << "Adding pos. offset again, this time left on for > filter time";
        testData.latitude -= 0.01;
        testData.longitude += 0.01;
        testData.altitude -= 200;
        testData.timestamp = QDateTime::currentDateTime().addSecs(1e3);
        testData.agc = 10;
    });
    QTimer::singleShot((config->getGnssTimeFilter() * 2e3) + 12e3 + config->getGnssTimeFilter() * 1e3, this, [this] () {
        qDebug() << "Removing pos. offset";
        testData.latitude += 0.01;
        testData.longitude -= 0.01;
        testData.altitude += 100;
    });
}
