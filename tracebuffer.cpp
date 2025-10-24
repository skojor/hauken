#include "tracebuffer.h"

TraceBuffer::TraceBuffer(QSharedPointer<Config> c)
{
    config = c;
    start();
}

void TraceBuffer::start()
{
    deleteOlderThanTimer = new QTimer;
    throttleTimer = new QElapsedTimer;
    connect(deleteOlderThanTimer, &QTimer::timeout, this, &TraceBuffer::deleteOlderThan);
    deleteOlderThanTimer->start(1000); // clean our house once per second, if not we will eat memory like hell!
    //connect(averageLevelMaintenanceTimer, &QTimer::timeout, this, &TraceBuffer::maintainAvgLevel);

    connect(maintenanceRestartTimer, &QTimer::timeout, this, [this]() {
        maintenanceRestartTimer->stop();
        averageLevelMaintenanceTimer->start(avgLevelMaintenanceTime);
        qDebug() << "Restarting avg calc. after incident";
    });
}

void TraceBuffer::deleteOlderThan()
{
    //mutex.lock();
    if (traceBuffer.size() > 2) {
        while (!traceBuffer.isEmpty() && !datetimeBuffer.isEmpty() && datetimeBuffer.first().secsTo(QDateTime::currentDateTime()) > bufferAge) {
            datetimeBuffer.removeFirst();
            traceBuffer.removeFirst();
            normTraceBuffer.removeFirst();
            //displayBuffer.removeFirst();
        }
        while (maxholdBuffer.size() > 120) maxholdBuffer.removeLast();
    }
    //mutex.unlock();
}

void TraceBuffer::addTrace(const QVector<qint16> &data)
{
    if (data.size() != nrOfDataPoints) {
        emit nrOfDatapointsChanged(data.size());
        nrOfDataPoints = data.size();
    }

    emit traceToAnalyzer(data); // unchanged data going to analyzer, together with unchanged avg. if normalized is on, buffer contains normalized data!
    mutex.lock();  // blocking access to containers, in case the cleanup timers wants to do work at the same time
    if (!traceBuffer.isEmpty()) {
        if (traceBuffer.last().size() != data.size())  // two different container sizes indicates freq/resolution changed, let's discard the buffer
            emptyBuffer();
    }
    
    if (normalizeSpectrum) {
        traceCopy = data;
        traceBuffer.append(calcNormalizedTrace(data));
    }
    else
        traceBuffer.append(data);

    if (recording)
        emit traceToRecorder(traceBuffer.last());

    emit traceData(traceBuffer.last());

    datetimeBuffer.append(QDateTime::currentDateTime());
    
    addDisplayBufferTrace(data);
    calcMaxhold();

    if (!throttleTimer->isValid() || throttleTimer->elapsed() > throttleTime) {
        throttleTimer->start();
        emit newDispTrace(displayBuffer);
        //emit reqReplot(); // Why this one? Not needed really?
    }
    mutex.unlock();
    
    if (tracesUsedInAvg <= tracesNeededForAvg)
        calcAvgLevel(data);
}

void TraceBuffer::getSecondsOfBuffer(int secs)
{
    QDateTime currentDateTime = QDateTime::currentDateTime();
    QList<QDateTime> dateBuffer;
    QList<QVector<qint16> > databuffer;
    int iterator = traceBuffer.size() - 1;
    while (iterator >= 0 && datetimeBuffer.at(iterator).secsTo(currentDateTime) < secs) {
        dateBuffer.append(datetimeBuffer.at(iterator));
        databuffer.append(traceBuffer.at(iterator--));
    }
    emit historicData(dateBuffer, databuffer);
}

void TraceBuffer::getAiData(int secs) // secs ignored ftm, just send all you have (2 min)
{
    (void)secs;
    emit aiData(normTraceBuffer);
}

void TraceBuffer::calcMaxhold()
{
    QVector<double> maxhold;
    maxhold.fill(-500, plotResolution);

    if (!maxholdBufferElapsedTimer->isValid()) maxholdBufferElapsedTimer->start();

    if (maxholdBuffer.isEmpty()) maxholdBuffer.append(displayBuffer);

    if (maxholdBufferAggregate.isEmpty()) {
        maxholdBufferElapsedTimer->start();
        maxholdBufferAggregate = displayBuffer;
    }

    else if (!maxholdBufferAggregate.isEmpty() && maxholdBufferElapsedTimer->elapsed() < 1000) {  // aggregates maxhold
        for (int i=0; i<plotResolution; i++) {
            if (maxholdBufferAggregate.at(i) < displayBuffer.at(i)) maxholdBufferAggregate[i] = displayBuffer.at(i);
        }
    }
    else if (maxholdBufferElapsedTimer->elapsed() >= 1000) {
        maxholdBufferElapsedTimer->start();
        maxholdBuffer.prepend(maxholdBufferAggregate);
        emit newDispMaxholdToWaterfall(maxholdBufferAggregate); // for waterfall calc.
        maxholdBufferAggregate.clear();
    }

    if (!maxholdBuffer.isEmpty()) {
        for (int seconds = 0; seconds < maxholdTime - 1 && seconds < maxholdBuffer.size(); seconds++) {
            for (int i=0; i<plotResolution; i++) {
                if (maxholdBuffer[seconds].size() == plotResolution &&
                    maxhold.at(i) < maxholdBuffer.at(seconds).at(i)) {
                    maxhold[i] = maxholdBuffer.at(seconds).at(i);
                }
            }
        }
    }
    if (maxholdBufferAggregate.size() == plotResolution) { // Comparing maxhold to changes which happened < 1 second ago
        for (int i=0; i<plotResolution; i++) {
            if (maxhold[i] < maxholdBufferAggregate[i])
                maxhold[i] = maxholdBufferAggregate[i];
        }
    }

    if ( maxholdTime > 0)
        emit newDispMaxhold(maxhold);
}

void TraceBuffer::addDisplayBufferTrace(const QVector<qint16> &data) // resample to plotResolution values, find max between points
{
    displayBuffer.clear();
    QVector<qint16> tmpNormTraceBuffer(plotResolution);
    double corr = config->getCorrValue();

    if (data.size() > plotResolution) {
        double rate = (double)data.size() / plotResolution;
        for (int i=0; i<plotResolution; i++) {
            //int val = data.at(rate * i);
            int top = -800;
            for (int j=1; j<=(int)rate; j++) {
                if (data.size() > rate * i + j && top < data.at(rate * i + j))
                    top = data.at(rate * i + j); // pick the strongest sample to show in plot
                //val += data.at(rate * i + j);
            }

            //val /= (int)rate + 1;
            //if (top > 150) val = top; // hack to boost display of small bw signals
            if (useDbm) top -= 1070;
            top += corr * 10;

            displayBuffer.append(((double)top / 10.0)); // / (int)rate + 1);
            tmpNormTraceBuffer[i] = top;
        }
    }
    else if (data.size()) {
        double rate = (double)plotResolution / data.size();
        for (int i=0; i<plotResolution; i++) {
            displayBuffer.append((double)data.at((int)((double)i / rate)) / 10.0);
            tmpNormTraceBuffer[i] = data.at((int)((double)i / rate));
        }
    }
    if (normalizeSpectrum && !averageDispLevel.isEmpty()) {
        for (int i=0; i<plotResolution; i++)
            displayBuffer[i] -= averageDispLevel.at(i);
    }
    for (int i=0; i<plotResolution; i++) {
        tmpNormTraceBuffer[i] -= averageDispLevel.at(i) * 10;
    }
    normTraceBuffer.append(tmpNormTraceBuffer);
    //emit aiData(normTraceBuffer); // debug, analyze live
}

void TraceBuffer::calcAvgLevel(const QVector<qint16> &data)
{
    if (!data.isEmpty()) {
        if (!traceBuffer.isEmpty()) { // never work on an empty buffer!
            if (averageLevel.isEmpty())
                averageLevel = data;
            else {
                for (int i=0; i<data.size(); i++) {
                    if (averageLevel.at(i) < data.at(i)) averageLevel[i] += (int)(avgFactor + 0.5);
                    else averageLevel[i] -= (int)(avgFactor + 0.5);
                }
                if (avgFactor > 1) avgFactor -= avgFactor * 10 / tracesNeededForAvg; //avgFactor *= 0.98;
                //qDebug() << "avgfactor:" << avgFactor;
            }
            if (data.size() > plotResolution) {
                double rate = (double)data.size() / plotResolution;
                for (int i=0; i<plotResolution; i++) {
                    int val = averageLevel.at(rate * i);
                    for (int j=1; j<(int)rate; j++) {
                        val += averageLevel.at(rate * i + j);
                    }
                    averageDispLevel[i] = ((double)val / 10.0) / (int)rate + 1;
                    
                }
            }
            else {
                double rate = (double)plotResolution / data.size();
                for (int i=0; i<plotResolution; i++) {
                    averageDispLevel[i] = (double)averageLevel.at((int)((double)i / rate)) / 10.0;
                }
            }
        }
    }
    
    if (normalizeSpectrum) {
        if (averageDispLevelNormalized.isEmpty())
            averageDispLevelNormalized.fill(trigLevel, plotResolution);
        emit newDispTriglevel(averageDispLevelNormalized);
    }
    else {
        QVector<double> copy = averageDispLevel;
        for (auto &val : copy)
            val += trigLevel;

        emit newDispTriglevel(copy); // Time consuming, why?
    }

    tracesUsedInAvg++;
    if (tracesUsedInAvg >= tracesNeededForAvg) finishAvgLevelCalc();
}

void TraceBuffer::emptyBuffer()
{
    traceBuffer.clear();
    datetimeBuffer.clear();
    displayBuffer.clear();
    averageDispLevel.clear();
    averageDispLevelNormalized.clear();
    maxholdBuffer.clear();
    maxholdBufferAggregate.clear();
    restartCalcAvgLevel();
}

void TraceBuffer::finishAvgLevelCalc()
{
    averageLevelMaintenanceTimer->start(avgLevelMaintenanceTime); // routine to keep updating the average level at a very slow interval
    emit averageLevelReady(averageLevel);
    emit stopAvgLevelFlash();
}

void TraceBuffer::restartCalcAvgLevel()
{
    tracesUsedInAvg = 0;
    averageLevelMaintenanceTimer->stop();
    averageLevel.clear();
    averageDispLevel.clear();
    averageDispLevel.resize(plotResolution);
    averageDispLevelNormalized.clear();
    maxholdBuffer.clear();
    maxholdBufferAggregate.clear();
    emit averageLevelCalculating();
    avgFactor = 40;
}

void TraceBuffer::updSettings()
{
    plotResolution = config->getPlotResolution();
    if (trigLevel != (int)config->getInstrTrigLevel()) {
        trigLevel = config->getInstrTrigLevel();
    }
    maxholdTime = config->getPlotMaxholdTime();
    emit showMaxhold((bool)maxholdTime);
    if (fftMode != config->getInstrFftMode() || antPort != QString::number(config->getInstrAntPort()) || // any of these changes should invalidate average level
        autoAtt != config->getInstrAutoAtt() || att != config->getInstrManAtt()) {
        fftMode = config->getInstrFftMode();
        antPort = QString::number(config->getInstrAntPort());
        autoAtt = config->getInstrAutoAtt();
        att = config->getInstrManAtt();
        restartCalcAvgLevel();
    }
    normalizeSpectrum = config->getInstrNormalizeSpectrum();
    averageDispLevelNormalized.clear();
    tracesNeededForAvg = config->getInstrTracesNeededForAverage();
    useDbm = config->getUseDbm();
    //maintainAvgLevel();
}

void TraceBuffer::deviceDisconnected()
{
    averageLevelMaintenanceTimer->stop();
}

QVector<qint16> TraceBuffer::calcNormalizedTrace(const QVector<qint16> &data)
{
    QVector<qint16> copy = data;
    if (!averageLevel.isEmpty()) {
        for (int i=0; i<copy.size(); i++)
            copy[i] -= averageLevel.at(i);
    }
    return copy;
}

void TraceBuffer::maintainAvgLevel()
{
    if (avgFactor < 3) avgFactor = 1;
    if (!normalizeSpectrum && !traceBuffer.isEmpty()) calcAvgLevel(traceBuffer.last());
    else if (normalizeSpectrum && !traceCopy.isEmpty()) calcAvgLevel(traceCopy);
    //qDebug() << "avg level maintenance update";
    emit averageLevelReady(averageLevel); // update trace analyzer with the new avg level data
}

void TraceBuffer::recorderStarted()
{
    recording = true;
    /*if (averageLevelMaintenanceTimer->isActive()) { // stop averaging if incident started. add timer to restart it later when recording ends
        averageLevelMaintenanceTimer->stop();
        qDebug() << "TraceBuffer: Incidence reported, halting level averaging until it ends";
    }*/
}

void TraceBuffer::recorderEnded()
{
    recording = false;
    /*if (!averageLevelMaintenanceTimer->isActive() && avgFactor <= 1) {
        averageLevelMaintenanceTimer->start(avgLevelMaintenanceTime);
        qDebug() << "TraceBuffer: Incidence ended, resuming level averaging";
    }*/
}

void TraceBuffer::incidenceTriggered() // called whenever sth is above trig line in spectrum
{
    //qDebug() << "TraceBuffer: Incidence triggered, status" << averageLevelMaintenanceTimer->isActive() << maintenanceRestartTimer->isActive();

    if (!maintenanceRestartTimer->isActive() && averageLevelMaintenanceTimer->isActive() && avgFactor <= 1) { // we have a situation, halt averaging until incidence has gone away
        averageLevelMaintenanceTimer->stop();
        maintenanceRestartTimer->start(120000);
    }
}
