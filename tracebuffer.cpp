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
    connect(averageLevelMaintenanceTimer, &QTimer::timeout, this, &TraceBuffer::calcAvgLevel);
}

void TraceBuffer::deleteOlderThan()
{
    mutex.lock();
    if (traceBuffer.size() > 1) { // failsafe, don't ever delete an empty buffer line!
        while (datetimeBuffer.first().secsTo(QDateTime::currentDateTime()) > bufferAge) {
            datetimeBuffer.removeFirst();
            traceBuffer.removeFirst();
            displayBuffer.removeFirst();
        }
    }
    mutex.unlock();
}

void TraceBuffer::addTrace(const QVector<qint16> &data)
{
    if (!throttleTimer->isValid())
        throttleTimer->start();
    mutex.lock();
    if (!traceBuffer.isEmpty()) {
        if (traceBuffer.last().size() != data.size())  // two different container sizes indicates freq/resolution changed, let's discard the buffer
            emptyBuffer();
    }
    traceBuffer.append(data);
    datetimeBuffer.append(QDateTime::currentDateTime());
    addDisplayBufferTrace(data);

    if (throttleTimer->elapsed() > throttleTime) {
        throttleTimer->start();
        emit newDispTrace(displayBuffer.last());
        if (maxholdTime > 0) calcMaxhold();
        emit reqReplot();
    }
    mutex.unlock();

    if (averageLevelDoneTimer->isActive())
        calcAvgLevel();
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

    displayBuffer.append(displayData);
}

void TraceBuffer::calcAvgLevel()
{
    if (traceBuffer.size() > 1) { // never work on an empty buffer!
        if (averageLevel.isEmpty())
            averageLevel = traceBuffer.last();
        else {
            for (int i=0; i<traceBuffer.last().size(); i++) {
                if (averageLevel.at(i) < traceBuffer.last().at(i)) averageLevel[i] += avgFactor;
                else averageLevel[i] -= avgFactor;
            }
            avgFactor *= 0.96;
        }
        if (traceBuffer.last().size() > plotResolution) {
            double rate = (double)traceBuffer.last().size() / plotResolution;
            int iterator = 0;
            for (int i=0; i<plotResolution; i++) {
                /*int val = 0;
                for (int j=0; j<(int)rate; j++)
                    val += averageLevel.at(iterator++);
                val /= (int)rate;*/
                averageDispLevel[i] = trigLevel + (double)averageLevel.at(iterator += rate) / 10;
            }
        }
        else {
            double rate = (double)plotResolution / traceBuffer.last().size();
            for (int i=0; i<plotResolution; i++) {
                averageDispLevel[i] = (double)averageLevel.at((int)((double)i / rate)) / 10;
            }
        }
        emit newDispTriglevel(averageDispLevel);
    }
}

void TraceBuffer::emptyBuffer()
{
    traceBuffer.clear();
    datetimeBuffer.clear();
    displayBuffer.clear();
    restartCalcAvgLevel();
}

void TraceBuffer::finishAvgLevelCalc()
{
    averageLevelDoneTimer->stop();
    averageLevelMaintenanceTimer->start(avgLevelMaintenanceTime * 1e3); // routine to keep updating the average level at a very slow interval
    emit averageLevelReady(averageLevel);
    //emit toIncidentLog("Trig level calculated");
}

void TraceBuffer::restartCalcAvgLevel()
{
    averageLevelDoneTimer->start(calcAvgLevelTime * 1e3);
    averageLevelMaintenanceTimer->stop();
    averageLevel.clear();
    averageDispLevel.clear();
    averageDispLevel.resize(plotResolution);
    //emit toIncidentLog("Recalculating trig level");
    emit averageLevelCalculating();
    avgFactor = 40;
}

void TraceBuffer::updSettings()
{
    if (trigLevel != (int)config->getInstrTrigLevel()) {
        trigLevel = config->getInstrTrigLevel();
        //restartCalcAvgLevel();
        calcAvgLevel();
    }
    maxholdTime = config->getPlotMaxholdTime();
    emit showMaxhold((bool)maxholdTime);
    if (fftMode != (int)config->getInstrFftMode()) {
        fftMode = config->getInstrFftMode();
        restartCalcAvgLevel();
    }
}

void TraceBuffer::deviceDisconnected()
{
    averageLevelDoneTimer->stop();
    averageLevelMaintenanceTimer->stop();
}
