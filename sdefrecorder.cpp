#include "sdefrecorder.h"

SdefRecorder::SdefRecorder()
{
    qDebug() << "Working folder" << QDir(QCoreApplication::applicationDirPath()).absolutePath();
}

void SdefRecorder::start()
{
    process = new QProcess;
    recordingStartedTimer = new QTimer;
    recordingTimeoutTimer = new QTimer;
    reqPositionTimer = new QTimer;
    recordingStartedTimer->setSingleShot(true);
    recordingTimeoutTimer->setSingleShot(true);
    connect(recordingStartedTimer, &QTimer::timeout, this, &SdefRecorder::finishRecording);
    connect(recordingTimeoutTimer, &QTimer::timeout, this, &SdefRecorder::restartRecording);
    connect(process, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
        [=](int exitCode, QProcess::ExitStatus exitStatus) {
        qDebug() << "process exit code" << exitStatus << exitCode;
    });
    connect(reqPositionTimer, &QTimer::timeout, this, [this] {
        emit reqPositionFrom(positionSource);
    });
}

void SdefRecorder::updSettings()
{
    saveToSdef = getSdefSaveToFile();
    recordTime = getSdefRecordTime() * 60e3;
    maxRecordTime = getSdefMaxRecordTime() * 60e3;
    addPosition = getSdefAddPosition();
    QString src = getSdefGpsSource();
    if (src == "InstrumentGnss")
        positionSource = POSITIONSOURCE::INSTRUMENTGNSS;
    else if (src.contains("1"))
        positionSource = POSITIONSOURCE::GNSSDEVICE1;
    else
        positionSource = POSITIONSOURCE::GNSSDEVICE2;
    prevLat = getStnLatitude().toDouble(); // failsafe if position is missing
    prevLng = getStnLongitude().toDouble();

    if (addPosition && !reqPositionTimer->isActive()) reqPositionTimer->start(1000); // ask for position once per sec.
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
       << QString::number(getInstrStartFreq() * 1e3, 'f', 0) << "-"
       << QString::number(getInstrStopFreq() * 1e3, 'f', 0) << "_"
       << QDateTime::currentDateTime().toString("yyyyMMdd_hhmm")
       << ".cef";

    return filename;
}
void SdefRecorder::receiveTrace(const QVector<qint16> data)
{
    if (!historicDataSaved) {
        emit toIncidentLog(NOTIFY::TYPE::SDEFRECORDER, "", "Trace history data not saved correctly");
        failed = true;
        file.close();
    }

    QByteArray byteArray = QDateTime::currentDateTime().toString("hh:mm:ss,").toLocal8Bit();
    if (addPosition) {
        byteArray +=
            QByteArray::number(positionHistory.last().first, 'f', 6) + "," +
            QByteArray::number(positionHistory.last().second, 'f', 6) + ",";
    }

    for (auto val : data) {
        byteArray += QByteArray::number((int)(val / 10)) + ',';
    }
    byteArray.remove(byteArray.size() - 1, 1); // remove last comma
    byteArray += "\n";
    file.write(byteArray);
}

void SdefRecorder::receiveTraceBuffer(const QList<QDateTime> datetime, const QList<QVector<qint16> > data)
{
    QElapsedTimer timer; timer.start();
    QByteArray byteArray;
    QDateTime startTime = datetime.last();
    int dateIterator = positionHistory.size() - 1 - getSdefPreRecordTime(); // to iterate through the pos. history. this one gets weird if recording is triggered before trace buffer is filled, but no worries
    if (dateIterator < 0) dateIterator = 0;

    for (int i=data.size() - 1; i >= 0; i--) {
        byteArray.clear();
        bool ok = true;

        byteArray += datetime.at(i).toString("hh:mm:ss").toLocal8Bit() + ',';
        if (addPosition) {
            if (startTime.secsTo(datetime.at(i)) > 0) { // one second of data has passed
                    dateIterator++;
                    startTime = datetime.at(i);
            }
            if (dateIterator > positionHistory.size() - 1) // should never happen except in early startup
                dateIterator = positionHistory.size() - 1;
            byteArray +=
                QByteArray::number(positionHistory.at(dateIterator).first, 'f', 6) + "," +
                QByteArray::number(positionHistory.at(dateIterator).second, 'f', 6) + ",";
            qDebug() << i << dateIterator;
        }

        for (auto val : data.at(i)) {
            byteArray += QByteArray::number((int)(val / 10)) + ',';
            if (val < -999 || val >= 2000) ok = false;
        }
        byteArray.remove(byteArray.size()-1, 1); // remove last comma
        byteArray += '\n';

        if (ok) file.write(byteArray);
        else qDebug() << "This backlog line failed:" << byteArray;
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
           << "FreqStart " << QString::number(getInstrStartFreq() * 1e3, 'f', 0) << '\n'
           << "FreqStop " << QString::number(getInstrStopFreq() * 1e3, 'f', 0) << '\n'
           << "AntennaType NoAntenna" << '\n'
           << "FilterBandwidth " << QString::number(getInstrResolution().toDouble(), 'f', 3) << '\n'
           << "LevelUnits dBuV" << '\n' \
           << "Date " << QDateTime::currentDateTime().toString("yyyy-M-d") << '\n'
           << "DataPoints " << QString::number(1 + ((getInstrStopFreq() - getInstrStartFreq()) / (getInstrResolution().toDouble() / 1e3))) << '\n'
           << "ScanTime " << QString::number((double)getInstrMeasurementTime() / 1e3, 'f', 3) << '\n'
           << "Detector FFM" << '\n'
           << "Note Instrument: " << getInstrId() << "; Hauken v." << QString(SW_VERSION).split('-').at(0) << "\n"
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
        if (recordingTimeoutTimer->isActive()) emit toIncidentLog(NOTIFY::TYPE::SDEFRECORDER, "", "Recording ended after "
                                                                  + (dateTimeRecordingStarted.secsTo(QDateTime::currentDateTime()) < 60 ?
                                                                       QString::number(dateTimeRecordingStarted.secsTo(QDateTime::currentDateTime()))
                                                                       + " seconds" :
                                                                   QString::number(dateTimeRecordingStarted.secsTo(QDateTime::currentDateTime()) / 60)
                                                                  + (dateTimeRecordingStarted.secsTo(QDateTime::currentDateTime()) / 60 == 1? " minute" : " minutes")));

    if (file.isOpen()) file.close();
    if (getSdefUploadFile() && recordingTimeoutTimer->isActive() && recording) uploadToCasper();
    recordingTimeoutTimer->stop();
    recordingStartedTimer->stop();

    if (!failed)
        recording = historicDataSaved = failed = false;
}

void SdefRecorder::restartRecording()
{
    finishRecording(); // basically the same operation, recording should restart anyway.
}

bool SdefRecorder::uploadToCasper()
{
    // parameters check
    if (getSdefStationInitals().isEmpty() or getSdefUsername().isEmpty() or getSdefPassword().isEmpty()
            or getStationName().isEmpty()) {
        emit toIncidentLog(NOTIFY::TYPE::SDEFRECORDER, "", "Upload requested, but some parameters are missing in the config. Check it out");
        return false;
    }
    if ((int)getStnLatitude().toDouble() * 1e6 == 0 or (int)getStnLongitude().toDouble() * 1e6 == 0) {
        emit toIncidentLog(NOTIFY::TYPE::SDEFRECORDER, "", "Upload requested, but the current position is set to equator. Somehow I doubt it");
        return false;
    }

    QStringList l;
    process->setWorkingDirectory(QDir(QCoreApplication::applicationDirPath()).absolutePath());
    process->setStandardOutputFile(getWorkFolder() + "/.process.out");
    process->setStandardErrorFile(getWorkFolder() + "/.process.err");

    if (QSysInfo::kernelType().contains("win")) {
        l << "/C" << "upload.bat" << file.fileName() << getSdefUsername() << getSdefPassword();
        process->setProgram("cmd.exe");
    }
    else if (QSysInfo::kernelType().contains("linux")) {
        l << "upload.sh" << file.fileName() << getSdefUsername() << getSdefPassword();
        process->setProgram("bash");
    }

    qDebug() << "starting upload process";
    process->setArguments(l);
    process->startDetached();

    return true;
}

void SdefRecorder::updPosition(bool b, double l1, double l2)
{
     if (b) {
         positionHistory.append(QPair<double, double>(l1, l2));
         prevLat = l1;
         prevLng = l2;
     }
     else {
         positionHistory.append(QPair<double, double>(prevLat, prevLng));
    }
     qDebug() << l1 << l2 << b << positionHistory.size();
     while (positionHistory.size() > 120) { // keeps a constant 120 values in buffer
         positionHistory.removeFirst();
     }
}
