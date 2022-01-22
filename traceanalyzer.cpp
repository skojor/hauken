#include "traceanalyzer.h"

TraceAnalyzer::TraceAnalyzer(QSharedPointer<Config> c)
{
    config = c;
    updTrigFrequencyTable();
}

void TraceAnalyzer::setTrace(const QVector<qint16> &data)
{
    if (ready) { // dont do anything until trace buffer says go

        khzAboveLimit = 0;
        int valuesAboveLimit = 0;
        if (averageData.size() == data.size()) {
            for (int i=0; i<data.size(); i++) {
                if (checkIfFrequencyIsInTrigArea(startFreq + (resolution/1e3 * i))
                        && data.at(i) > averageData.at(i) + trigLevel * 10) // * 10 because we have values in 1/10 dBuV!
                    valuesAboveLimit++;
                else if ((valuesAboveLimit && !checkIfFrequencyIsInTrigArea(startFreq + (resolution/1e3 * i))) ||
                         (valuesAboveLimit && checkIfFrequencyIsInTrigArea(startFreq + (resolution/1e3 * i))
                         && data.at(i) < averageData.at(i) + trigLevel * 10)) { // previously trace point high, not this one. reset counter and calc BW of signal
                    double khzOfIncident = valuesAboveLimit * resolution;
                    if (khzOfIncident > khzAboveLimit)
                        khzAboveLimit = khzOfIncident;
                    valuesAboveLimit = 0;
                }
            }
        }
        if (khzAboveLimit > trigBandwidth) {
            if (trigTime == 0) { // trig time 0 means sound the alarm immediately
                alarmTriggered();
            }
            else if (trigTime > 0 && !elapsedTimer->isValid()) // first time above limit, start the clock
                elapsedTimer->start();
            else if (trigTime > 0 && elapsedTimer->isValid() && elapsedTimer->elapsed() >= trigTime) {
                alarmTriggered();
            }
        }
        else {
            if (elapsedTimer) elapsedTimer->invalidate();
            if (alarmEmitted) {
                emit toIncidentLog(NOTIFY::TYPE::TRACEANALYZER, "", "Normal signal levels");
                alarmEmitted = false;
            }
        }
    }
}

void TraceAnalyzer::alarmTriggered()
{
    emit alarm();
    if (!alarmEmitted) {
        alarmEmitted = true;
        if (config->getSdefSaveToFile())
            emit toIncidentLog(NOTIFY::TYPE::TRACEANALYZER, "", "Recording triggered by measurement receiver (" +
                               QString::number((int)khzAboveLimit) + " kHz above limit)");
        else
            emit toIncidentLog(NOTIFY::TYPE::TRACEANALYZER, "", "Incident registered, " + QString::number((int)khzAboveLimit) +
                               " kHz above limit. Recording is not enabled.");
    }
}

void TraceAnalyzer::setAverageTrace(const QVector<qint16> &data)
{
    averageData = data;
}

void TraceAnalyzer::updTrigFrequencyTable()
{
    trigFrequenciesList.clear();
    QStringList freqList = config->getTrigFrequencies();
    if (freqList.isEmpty()) {
        trigFrequenciesList.append(QPair<double, double>(0, 0.001)); // nothing will be detected, ever!
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
    resolution = config->getInstrResolution().toDouble();
}
