#include "traceanalyzer.h"

TraceAnalyzer::TraceAnalyzer(QSharedPointer<Config> c)
{
    config = c;
    updTrigFrequencyTable();
}

void TraceAnalyzer::setTrace(const QVector<qint16> &data)
{
    if (recorderRunning) emit toRecorder(data);

    int valuesAboveLimit = 0;
    if (averageData.size() == data.size()) {
        for (int i=0; i<data.size(); i++) {
            if (checkIfFrequencyIsInTrigArea(startFreq + (resolution/1e3 * i)) && data.at(i) > averageData.at(i) + trigLevel * 10) // * 10 because we have values in 1/10 dBuV!
                valuesAboveLimit++;
        }
    }
    khzAboveLimit = valuesAboveLimit * resolution;
    if (khzAboveLimit > trigBandwidth) {
        if (trigTime == 0) { // trig time 0 means sound the alarm immediately
            emit alarm();
            if (!alarmEmitted) {
                alarmEmitted = true;
                emit toIncidentLog("Recording triggered by measurement receiver (" + QString::number(khzAboveLimit) + ")");
            }
        }
        else if (trigTime > 0 && !elapsedTimer->isValid()) // first time above limit, start the clock
            elapsedTimer->start();
        else if (trigTime > 0 && elapsedTimer->isValid()) {
            if (elapsedTimer->elapsed() >= trigTime) {
                emit alarm();
                if (!alarmEmitted) {
                    alarmEmitted = true;
                    emit toIncidentLog("Recording triggered by measurement receiver (" +QString::number(khzAboveLimit) + ")");
                }
            }
        }
    }
    else {
        if (elapsedTimer) elapsedTimer->invalidate();
    }
}

void TraceAnalyzer::setAverageTrace(const QVector<qint16> &data)
{
    averageData = data;
}

void TraceAnalyzer::updTrigFrequencyTable()
{
    QStringList freqList = config->getTrigFrequencies();
    if (freqList.isEmpty()) {
        trigFrequenciesList.append(QPair<double, double>(0, 0)); // nothing will be detected, ever!
    }
    else {
        for (int i=0; i<freqList.size();) {
            double sel1 = freqList.at(i++).toDouble();
            double sel2 = freqList.at(i++).toDouble();
            trigFrequenciesList.append(QPair<double, double>(sel1, sel2));
        }
    }
}

bool TraceAnalyzer::checkIfFrequencyIsInTrigArea(double freq)
{
    for (auto &val : trigFrequenciesList) {
        if (freq >= val.first && freq <= val.second)
            return true;
    }
    return false;
}

void TraceAnalyzer::updSettings()
{
    trigLevel = config->getInstrTrigLevel();
    trigBandwidth = config->getInstrMinTrigBW();
    trigTime = config->getInstrMinTrigTime();
    startFreq = config->getInstrStartFreq();
    stopFreq = config->getInstrStopFreq();
    resolution = config->getInstrResolution();
}
