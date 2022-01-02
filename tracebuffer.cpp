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
    connect(averageLevelDoneTimer, &QTimer::timeout, this, &TraceBuffer::finishAvgLevelCalc);
    connect(averageLevelMaintenanceTimer, &QTimer::timeout, this, &TraceBuffer::maintainAvgLevel);
}

void TraceBuffer::deleteOlderThan()
{
    mutex.lock();
    if (traceBuffer.size() > 2) { // failsafe, don't ever delete an empty buffer line!
        while (!datetimeBuffer.isEmpty() && datetimeBuffer.first().secsTo(QDateTime::currentDateTime()) > bufferAge) {
            datetimeBuffer.removeFirst();
            traceBuffer.removeFirst();
            displayBuffer.removeFirst();
        }
    }
    mutex.unlock();
}

void TraceBuffer::addTrace(const QVector<qint16> &data)
{
    emit traceToAnalyzer(data); // unchanged data going to analyzer, together with unchanged avg. if normalized is on, buffer contains normalized data!

    mutex.lock();  // blocking access to containers, in case the cleanup timers wants to do work at the same time
    if (!traceBuffer.isEmpty()) {
        if (traceBuffer.last().size() != data.size())  // two different container sizes indicates freq/resolution changed, let's discard the buffer
            emptyBuffer();
    }

    if (normalizeSpectrum)
        traceBuffer.append(calcNormalizedTrace(data));
    else
        traceBuffer.append(data);

    if (recording)
        emit traceToRecorder(traceBuffer.last());

    datetimeBuffer.append(QDateTime::currentDateTime());
    addDisplayBufferTrace(data);

    if (!throttleTimer->isValid())
        throttleTimer->start();

    if (throttleTimer->elapsed() > throttleTime) {
        throttleTimer->start();
        emit newDispTrace(displayBuffer.last());
        if (maxholdTime > 0) calcMaxhold();
        emit reqReplot();
    }
    mutex.unlock();

    if (averageLevelDoneTimer->isActive())
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

void TraceBuffer::calcMaxhold()
{
    if (displayBuffer.size() > 2) {
        QVector<double> maxhold(displayBuffer.last());
        int iterator = displayBuffer.size() - 2;
        while (iterator >= 0 && datetimeBuffer.at(iterator).secsTo(QDateTime::currentDateTime()) < maxholdTime) {
            for (int i=0; i<displayBuffer.at(iterator).size(); i++) {
                if (maxhold.at(i) < displayBuffer.at(iterator).at(i))
                    maxhold[i] = displayBuffer.at(iterator).at(i);
            }
            iterator--;
        }
        emit newDispMaxhold(maxhold);
    }
}

void TraceBuffer::addDisplayBufferTrace(const QVector<qint16> &data) // resample to plotResolution values, find average between points
{
    QVector<double>displayData(plotResolution);

    if (data.size() > plotResolution) {
        double rate = (double)data.size() / plotResolution;
        int iterator = 0;
        for (int i=0; i<plotResolution; i++) {
            /*int val = 0;
            for (int j=0; j<(int)rate; j++)
                val += data.at(iterator++);
            val /= (int)rate;*/
            displayData[i] = (double)data.at(iterator += rate) / 10;
        }
    }
    else {
        double rate = (double)plotResolution / data.size();
        for (int i=0; i<plotResolution; i++) {
            displayData[i] = (double)data.at((int)((double)i / rate)) / 10;
        }
    }
    if (normalizeSpectrum && !averageDispLevel.isEmpty()) {
        for (int i=0; i<displayData.size(); i++)
            displayData[i] -= averageDispLevel.at(i);
    }

    displayBuffer.append(displayData);
}

void TraceBuffer::calcAvgLevel(const QVector<qint16> &data)
{
    if (!data.isEmpty()) {
        if (!traceBuffer.isEmpty()) { // never work on an empty buffer!
            if (averageLevel.isEmpty())
                averageLevel = data;
            else {
                for (int i=0; i<data.size(); i++) {
                    if (averageLevel.at(i) < data.at(i)) averageLevel[i] += avgFactor;
                    else averageLevel[i] -= avgFactor;
                }
                if (avgFactor > 1) avgFactor *= 0.96;
            }
            if (data.size() > plotResolution) {
                double rate = (double)data.size() / plotResolution;
                int iterator = 0;
                for (int i=0; i<plotResolution; i++) {
                    averageDispLevel[i] = (double)averageLevel.at(iterator += rate) / 10;
                }
            }
            else {
                double rate = (double)plotResolution / data.size();
                for (int i=0; i<plotResolution; i++) {
                    averageDispLevel[i] = (double)averageLevel.at((int)((double)i / rate)) / 10;
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

        emit newDispTriglevel(copy);
    }
}

void TraceBuffer::emptyBuffer()
{
    traceBuffer.clear();
    datetimeBuffer.clear();
    displayBuffer.clear();
    averageDispLevel.clear();
    averageDispLevelNormalized.clear();
    restartCalcAvgLevel();
}

void TraceBuffer::finishAvgLevelCalc()
{
    averageLevelDoneTimer->stop();
    averageLevelMaintenanceTimer->start(avgLevelMaintenanceTime * 1e3); // routine to keep updating the average level at a very slow interval
    emit averageLevelReady(averageLevel);
}

void TraceBuffer::restartCalcAvgLevel()
{
    averageLevelDoneTimer->start(calcAvgLevelTime * 1e3);
    averageLevelMaintenanceTimer->stop();
    averageLevel.clear();
    averageDispLevel.clear();
    averageDispLevel.resize(plotResolution);
    averageDispLevelNormalized.clear();
    //emit toIncidentLog("Recalculating trig level");
    emit averageLevelCalculating();
    avgFactor = 40;
}

void TraceBuffer::updSettings()
{
    if (trigLevel != (int)config->getInstrTrigLevel()) {
        trigLevel = config->getInstrTrigLevel();
    }
    maxholdTime = config->getPlotMaxholdTime();
    emit showMaxhold((bool)maxholdTime);
    if (fftMode != (int)config->getInstrFftMode()) {
        fftMode = config->getInstrFftMode();
        restartCalcAvgLevel();
    }
    normalizeSpectrum = config->getInstrNormalizeSpectrum();
    averageDispLevelNormalized.clear();
    maintainAvgLevel();
}

void TraceBuffer::deviceDisconnected()
{
    averageLevelDoneTimer->stop();
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
    calcAvgLevel();
    emit averageLevelReady(averageLevel); // update trace analyzer with the new avg level data
}
