#include "traceanalyzer.h"

TraceAnalyzer::TraceAnalyzer(QSharedPointer<Config> c)
{
    config = c;
    updTrigFrequencyTable();
}

void TraceAnalyzer::setTrace(const QVector<qint16> &data)
{
    if (ready) { // dont do anything until trace buffer says go

        khzAboveLimit = khzAboveLimitTotal = singleTrigCenterFrequency = 0;
        maxLevel = -999;
        int valuesAboveLimit = 0, valuesAboveLimitTotal = 0;
        if (averageData.size() == data.size()) {
            for (int i=0; i<data.size(); i++) {
                if (checkIfFrequencyIsInTrigArea(startFreq + (resolution/1e3 * i))
                        && data.at(i) > averageData.at(i) + trigLevel * 10) { // * 10 because we have values in 1/10 dBuV!
                    valuesAboveLimit++;
                    valuesAboveLimitTotal++;
                }
                else if ((valuesAboveLimit && !checkIfFrequencyIsInTrigArea(startFreq + (resolution/1e3 * i))) ||
                         (valuesAboveLimit && checkIfFrequencyIsInTrigArea(startFreq + (resolution/1e3 * i))
                         && data.at(i) < averageData.at(i) + trigLevel * 10)) { // previously trace point high, not this one. reset counter and calc BW of signal
                    double khzOfIncident = valuesAboveLimit * resolution;
                    if (khzOfIncident > khzAboveLimit) {
                        khzAboveLimit = khzOfIncident;
                        singleTrigCenterFrequency = startFreq + (resolution/1e3 * i) - (khzAboveLimit/2000);
                        if (pmrMode) {
                        }
                    }
                    pmrCheckUptime(singleTrigCenterFrequency * 1e6, (khzAboveLimit > khzOfIncident?true:false));

                    valuesAboveLimit = 0;
                }
                if (data[i] > maxLevel) maxLevel = data[i];
            }
            khzAboveLimitTotal = valuesAboveLimitTotal * resolution;
        }

        emit maxLevelMeasured((double)maxLevel * 0.1);

        if (khzAboveLimit > singleTrigBandwidth || khzAboveLimitTotal > totalTrigBandwidth) {
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
                if (!pmrMode) emit toIncidentLog(NOTIFY::TYPE::TRACEANALYZER, "", "Normal signal levels");
                alarmEmitted = false;
                emit alarmEnded();
            }
        }
    }
}

void TraceAnalyzer::alarmTriggered()
{
    if (!alarmEmitted) {
        alarmEmitted = true;
        QString alarmText;
        if (khzAboveLimit > singleTrigBandwidth && khzAboveLimitTotal <= totalTrigBandwidth)
            if (!pmrMode) alarmText = "single cont. signal above limit at center frequency " + QString::number(singleTrigCenterFrequency, 'f', 6) + " MHz (" + QString::number((int)khzAboveLimit) + " kHz span)";
            else alarmText = "Activity at " + QString::number(singleTrigCenterFrequency, 'f', 6) + " MHz (" + QString::number((int)khzAboveLimit) + " kHz span)";
        else if (khzAboveLimit <= singleTrigBandwidth && khzAboveLimitTotal > totalTrigBandwidth)
            alarmText = "total signal above limit: " + QString::number((int)khzAboveLimitTotal) + " kHz";
        else
            alarmText = "both single and total signal levels triggered (" + QString::number((int)khzAboveLimit) + " / " + QString::number((int)khzAboveLimitTotal) + " kHz)";

        if (config->getSdefSaveToFile())
            emit toIncidentLog(NOTIFY::TYPE::TRACEANALYZER, "", "Recording triggered by measurement receiver, " + alarmText);
        else {
            if (!pmrMode) emit toIncidentLog(NOTIFY::TYPE::TRACEANALYZER, "", "Incident registered, " + alarmText + ". Recording is not enabled");
            else emit toIncidentLog(NOTIFY::TYPE::TRACEANALYZER, "", "PMR " + alarmText);
        }
        emit trigRegistered(singleTrigCenterFrequency);
    }
    emit alarm();
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
    /*if (config->getUseDbm() != useDbm) {
        useDbm = config->getUseDbm();
        if (useDbm) trigLevel += 107;
        else trigLevel -= 107;
    }*/
    singleTrigBandwidth = config->getInstrMinTrigBW();
    totalTrigBandwidth = config->getInstrTotalTrigBW();
    trigTime = config->getInstrMinTrigTime();
    /*startFreq = config->getInstrStartFreq();
    stopFreq = config->getInstrStopFreq();
    resolution = config->getInstrResolution().toDouble();*/
    pmrMode = config->getPmrMode();
}

void TraceAnalyzer::pmrCheckUptime(const quint64 frequency, const bool active)
{
    (void)frequency;
    (void)active;
 /*   bool foundIt = false;
    if (!pmrTable.isEmpty()) {                  // check if this one was up before
        for (auto &row : pmrTable) {
            if (row.centerFrequency == frequency) { // already in the table;
                if (active) row.totalDurationInMilliseconds += QDateTime::currentDateTime().msecsTo(row.startTime);
                foundIt = true;
                break;
            }
        }
        if (!foundIt) {
            PmrTable table;
            table.centerFrequency = frequency;
            if (active) table.startTime = QDateTime::currentDateTime();

            pmrTable.append(table);
        }
    }
    else {
        PmrTable table;
        table.centerFrequency = frequency;
        if (active) table.startTime = QDateTime::currentDateTime();
        pmrTable.append(table);
    }*/
}
