#include "sdefrecorder.h"

SdefRecorder::SdefRecorder()
{
}

void SdefRecorder::start()
{
    recordingStartedTimer = new QTimer;
    recordingTimeoutTimer = new QTimer;
    recordingStartedTimer->setSingleShot(true);
    recordingTimeoutTimer->setSingleShot(true);
    connect(recordingStartedTimer, &QTimer::timeout, this, &SdefRecorder::finishRecording);
    connect(recordingTimeoutTimer, &QTimer::timeout, this, &SdefRecorder::restartRecording);
}

void SdefRecorder::updSettings()
{
    saveToSdef = getSdefSaveToFile();
    recordTime = getSdefRecordTime() * 60e3;
    maxRecordTime = getSdefMaxRecordTime() * 60e3;
}

void SdefRecorder::triggerRecording()
{
    recordingStartedTimer->start(recordTime); // minutes. restarts every time routine is called, which means it will run for x minutes after incident ended

    if (!recording && saveToSdef) {
        recording = true;
        dateTimeRecordingStarted = QDateTime::currentDateTime(); // for incident log

        emit recordingStarted();

        if (!recordingTimeoutTimer->isActive())
            recordingTimeoutTimer->start(maxRecordTime); // only call once

        file.setFileName(createFilename());
        if (failed || !file.open(QIODevice::WriteOnly)) {
            emit warning("Error creating file " + file.fileName());
            failed = true;
            finishRecording();
        }
        else {
            file.write(createHeader());
            if (getSdefPreRecordTime() > 0) emit reqTraceHistory(getSdefPreRecordTime());
            else historicDataSaved = true;
        }
    }
}

QString SdefRecorder::createFilename()
{
    QString dir = getLogFolder();
    QString filename;
    QTextStream ts(&filename);

    if (getNewLogFolder()) { // create new folder for incident
        dir = getLogFolder() + "/" + QDateTime::currentDateTime().toString("yyyyMMdd_hhmm");
        if (!QDir().mkpath(dir)) {
            emit warning("Cannot create folder " + dir + ", check your settings!");
            failed = true;
        }
    }

    ts << dir << "/" << getSdefStationInitals() << "_"
       << QString::number(getInstrStartFreq() * 1e6, 'f', 0) << "-"
       << QString::number(getInstrStopFreq() * 1e6, 'f', 0) << "_"
       << QDateTime::currentDateTime().toString("yyyyMMdd_hhmm")
       << ".cef";

    return filename;
}
void SdefRecorder::receiveTrace(const QVector<qint16> data)
{
    if (!historicDataSaved) {
        emit toIncidentLog("Trace history data not saved correctly");
        failed = true;
        file.close();
    }

    QByteArray buf = QDateTime::currentDateTime().toString("hh:mm:ss,").toLocal8Bit();
    for (auto val : data) {
        buf += QByteArray::number((int)(val / 10)) + ',';
    }
    buf.remove(buf.size() - 1, 1); // remove last comma
    buf += "\n";
    file.write(buf);
}

void SdefRecorder::receiveTraceBuffer(const QList<QDateTime> datetime, const QList<QVector<qint16> > data)
{
    QElapsedTimer timer; timer.start();
    QByteArray tmpBuf;
    for (int i=data.size() - 1; i >= 0; i--) {
        tmpBuf.clear();
        bool ok = true;

        tmpBuf += datetime.at(i).toString("hh:mm:ss").toLocal8Bit() + ',';
        //if (mobileData) tmpBuf += QString::number(lat->at(i), 'f', 6) + "," + QString::number(lng->at(i), 'f', 6) + ",";

        for (auto val : data.at(i)) {
            tmpBuf += QByteArray::number((int)(val / 10)) + ',';
            if (val < -999 || val >= 2000) ok = false;
        }
        tmpBuf.remove(tmpBuf.size()-1, 1); // remove last comma
        tmpBuf += '\n';

        if (ok) file.write(tmpBuf);
        else qDebug() << "This backlog line failed:" << tmpBuf;
    }
    historicDataSaved = true;
}

QByteArray SdefRecorder::createHeader() //TODO: Own dynamic position update!
{
    QString buf;
    QTextStream stream(&buf, QIODevice::ReadWrite);

    stream << (getSdefAddPosition()?"FileType MobileData":"FileType Standard Data exchange Format 2.0") << '\n'
           << "LocationName " << getStationName() << "\n"
           << "Latitude " << convertDdToddmmss(getStnLatitude().toDouble()) << "\n"
           << "Longitude " << convertDdToddmmss(getStnLongitude().toDouble(), false) << '\n'
           << "FreqStart " << QString::number(getInstrStartFreq() * 1e6, 'f', 0) << '\n'
           << "FreqStop " << QString::number(getInstrStopFreq() * 1e6, 'f', 0) << '\n'
           << "AntennaType NoAntenna" << '\n'
           << "FilterBandwidth " << QString::number(getInstrResolution() * 1e3) << '\n'
           << "LevelUnits dBuV" << '\n' \
           << "Date " << QDateTime::currentDateTime().toString("yyyy-M-d") << '\n'
           << "DataPoints " << QString::number(1 + ((getInstrStopFreq() - getInstrStartFreq()) / (getInstrResolution() / 1e3))) << '\n'
           << "ScanTime " << QString::number((double)getInstrMeasurementTime() / 1e3, 'f', 3) << '\n'
           << "Detector FFM" << '\n'
           << "Note Instrument: " << getInstrId() << "; Hauken_" << QString(SW_VERSION).split('-').at(0) << "\n"
           << "Note\nNote " << getSdefStationInitals() << ", MaxHoldTime: " << QString::number(1 / tracePerSecond, 'f', 2)
           << ", Attenuator: " << (getInstrAutoAtt()? "Auto" : QString::number(getInstrManAtt()))
           << "\n\n";

    return buf.toLocal8Bit();
}

QString SdefRecorder::convertDdToddmmss(const double d, const bool lat)
{
    QString ret;
    ret = QString::asprintf("%02i", abs((int)d)) + '.'
            + QString::asprintf("%02i", int(((abs(d) - abs((int)d)) * 60))) + '.'
            + QString::asprintf("%02i", int(((((abs(d) - abs((int)d)) * 60)) - (int)((abs(d) - abs((int)d)) * 60)) * 60))
            + ( lat ? (d > 0 ? "N": "S") : (d > 0 ? "E": "W")) ;
    return ret;
}

void SdefRecorder::finishRecording()
{
    emit recordingEnded();
    if (getSdefSaveToFile())
        if (recordingTimeoutTimer->isActive()) emit toIncidentLog("Recording ended after "
                           + QString::number(dateTimeRecordingStarted.secsTo(QDateTime::currentDateTime()) / 60)
                           + (dateTimeRecordingStarted.secsTo(QDateTime::currentDateTime()) / 60 == 1? " minute" : " minutes"));

    recordingTimeoutTimer->stop();
    recordingStartedTimer->stop();
    if (file.isOpen()) file.close();
    if (!failed)
        recording = historicDataSaved = failed = false;
}

void SdefRecorder::restartRecording()
{
    finishRecording(); // basically the same operation, recording should restart anyway.
}
