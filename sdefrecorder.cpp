#include "sdefrecorder.h"
#include "version.h"
#include "asciitranslator.h"
#include "JlCompress.h"

SdefRecorder::SdefRecorder(QSharedPointer<Config> c)
{
    config = c;
}

void SdefRecorder::start()
{
    process = new QProcess(this);
    recordingStartedTimer = new QTimer(this);
    recordingTimeoutTimer = new QTimer(this);
    reqPositionTimer = new QTimer(this);
    periodicCheckUploadsTimer = new QTimer(this);
    autorecorderTimer = new QTimer(this);
    finishedFileTimer = new QTimer(this);
    tempFileTimer = new QTimer(this);
    tempFileCheckFilesize = new QTimer(this);
    finishedFileTimer->setSingleShot(true);

    recordingStartedTimer->setSingleShot(true);
    recordingTimeoutTimer->setSingleShot(true);
    connect(recordingStartedTimer, &QTimer::timeout, this, &SdefRecorder::endRecording);
    connect(recordingTimeoutTimer, &QTimer::timeout, this, &SdefRecorder::endRecording);
    connect(process,
            QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            this,
            &SdefRecorder::curlCallback);
    connect(reqPositionTimer, &QTimer::timeout, this, [this] {
        emit reqPositionFrom(positionSource);
    });
    connect(periodicCheckUploadsTimer, &QTimer::timeout, this, &SdefRecorder::curlUpload);
    connect(autorecorderTimer,
            &QTimer::timeout,
            this,
            &SdefRecorder::triggerRecording); // auto recording triggers recording here

    connect(tempFileTimer, &QTimer::timeout, this, &SdefRecorder::saveDataToTempFile);

    process->setWorkingDirectory(QDir(QCoreApplication::applicationDirPath()).absolutePath());
    process->setStandardOutputFile(config->getWorkFolder() + "/.process.out");
    process->setStandardErrorFile(config->getWorkFolder() + "/.process.err");
    if (QSysInfo::kernelType().contains("win")) {
        process->setProgram("curl.exe");
    }

    else if (QSysInfo::kernelType().contains("linux")) {
        process->setProgram("curl");
    }
    periodicCheckUploadsTimer->start(
        1800 * 1e3); // check if any files still waits for upload every half hour

    networkManager = new QNetworkAccessManager(this);

    connect(finishedFileTimer, &QTimer::timeout, this, [this]() {
        emit fileReadyForUpload(finishedFilename);
        finishedFilename.clear();
    });

    tempFileCheckFilesize->start(3600 * 1e3);
    connect(tempFileCheckFilesize, &QTimer::timeout, this, [this] {
        if (tempFile.isOpen() && tempFile.size() > 4096e3) {
            tempFile.close();
            tempFile.remove();
        }
    });
    emit recordingEnded();
}

void SdefRecorder::updSettings()
{
    if (autorecorderTimer) { // Don't do anything until thread has started!
        saveToSdef = config->getSdefSaveToFile();
        recordTime = config->getSdefRecordTime() * 60e3;
        maxRecordTime = config->getSdefMaxRecordTime() * 60e3;
        addPosition = config->getSdefAddPosition();
        QString src = config->getSdefGpsSource();
        if (src == "InstrumentGnss")
            positionSource = POSITIONSOURCE::INSTRUMENTGNSS;
        else if (src.contains("1"))
            positionSource = POSITIONSOURCE::GNSSDEVICE1;
        else
            positionSource = POSITIONSOURCE::GNSSDEVICE2;
        //prevLat = config->getStnLatitude().toDouble(); // failsafe if position is missing BUG, removed 010422: Creates header of position 0 in random occasions...
        //prevLng = config->getStnLongitude().toDouble();

        if (addPosition && !reqPositionTimer->isActive())
            reqPositionTimer->start(1000); // ask for position once per sec.

        if (saveToSdef)
            emit recordingEnabled();
        else
            emit recordingDisabled();

        if (config->getAutoRecorderActivate()) {
            if (!autorecorderTimer->isActive()) {
                autorecorderTimer->start(10000);
                emit toIncidentLog(NOTIFY::TYPE::SDEFRECORDER,
                                   "",
                                   "Auto recording is activated, starting recording of currently "
                                   "chosen frequency spectrum and resolution in 10 seconds");
            }
        } else {
            if (autorecorderTimer and autorecorderTimer->isActive())
                autorecorderTimer->stop();
        }
        useNewMsFormat = config->getSdefNewMsFormat();

        tempFileMaxhold = config->getSaveToTempFileMaxhold();
        if (tempFileTimer and tempFileMaxhold > 0)
            tempFileTimer->start(tempFileMaxhold * 1e3);
        else if (tempFileTimer)
            tempFileTimer->stop();
    }
}

void SdefRecorder::triggerRecording()
{
    if (deviceConnected) { // never start a recording unless some device is connected!
        recordingStartedTimer->start(
            recordTime); // minutes. restarts every time routine is called, which means it will run for x minutes after incident ended

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
                endRecording();
            } else {
                file.write(createHeader());
                if (config->getSdefPreRecordTime() > 0)
                    emit reqTraceHistory(config->getSdefPreRecordTime());
                else
                    historicDataSaved = true;
            }
        }
    }
}

void SdefRecorder::manualTriggeredRecording()
{
    endRecording();
    emit toIncidentLog(NOTIFY::TYPE::SDEFRECORDER, "", "Recording triggered manually");
    triggerRecording();
}

QString SdefRecorder::createFilename()
{
    QString dir = config->incidentFolder();
    QString filename;
    QTextStream ts(&filename);

    if (!QDir().exists(dir))
        QDir().mkpath(dir);

        ts << dir << "/" << config->incidentTimestamp().toString("yyyyMMddhhmmss")
        << "_" << AsciiTranslator::toAscii(config->getStationName())
        << "_" << QString::number(1e-3 * startfreq, 'f', 0) << "-"
        << QString::number(1e-3 * stopfreq, 'f', 0)
        << ".cef";

    return filename;
}
void SdefRecorder::receiveTrace(const QVector<qint16> data)
{
    if (historicDataSaved && !iqRecordingInProgress && !skipTraces) { // TODO: Quickfix to see if random "not saved correctly" error message disappears. NB! This one will just skip trace(s) until backlog is saved!
        if (addPosition and !positionHistory.isEmpty())
            file.write(genSdefTraceLine(positionHistory.last().first, positionHistory.last().second, data));
        else
            file.write(genSdefTraceLine(0, 0, data));
    }
    if (skipTraces) skipTraces--;
}

void SdefRecorder::receiveTraceBuffer(const QList<QDateTime> datetime,
                                      const QList<QVector<qint16> > data)
{
    QElapsedTimer timer;
    timer.start();
    QByteArray buffer;
    QDateTime startTime;
    if (!datetime.isEmpty()) startTime = datetime.last();
    int dateIterator
        = positionHistory.size() - 1
          - config->getSdefPreRecordTime(); // to iterate through the pos. history. this one gets weird if recording is triggered before trace buffer is filled, but no worries
    if (dateIterator < 0)
        dateIterator = 0;

    for (int i = data.size() - 1; i >= 0; i--) {
        buffer.clear();
        bool ok = true;

        if (useNewMsFormat)
            buffer.append(datetime.at(i).toString("yyyy-MM-dd hh:mm:ss.zzz").toUtf8());
        else
            buffer.append(datetime.at(i).toString("hh:mm:ss").toUtf8());

        if (startTime.secsTo(datetime.at(i))
            > 0) { // one second of data has passed, counters updated in if below here
            QVector<float> tmpTrace;
            for (int arrayIterator = 0; arrayIterator < 1200; arrayIterator++)
                tmpTrace.append((float) data[i][(float) arrayIterator * data[i].size() / 1200.0]
                                / 10.0);
        }

        if (addPosition) {
            if (startTime.secsTo(datetime.at(i)) > 0) { // one second of data has passed
                dateIterator++;
                startTime = datetime.at(i);
            }
            if (dateIterator > positionHistory.size() - 1) // should never happen except in early startup
                dateIterator = positionHistory.size() - 1;

            buffer.append(",").append(QByteArray::number(positionHistory.at(dateIterator).first, 'f', 6));
            buffer.append(",").append(QByteArray::number(positionHistory.at(dateIterator).second, 'f', 6));
        }

        for (auto &&val : data.at(i)) {
            buffer.append(',').append(QByteArray::number(val / 10));
            if (val < -999 || val >= 2000)
                ok = false;
        }
        buffer.append('\n');

        if (ok) file.write(buffer);
        else qDebug() << "This backlog line failed:" << buffer;
    }
    historicDataSaved = true;
    qDebug() << "spent" << timer.elapsed();
}

QByteArray SdefRecorder::createHeader(const int saveInterval)
{
    QString buf;
    QTextStream stream(&buf, QIODevice::ReadWrite);

    QString gain;
    if (config->getInstrId().contains("em200", Qt::CaseInsensitive)
        || config->getInstrId().contains("pr200", Qt::CaseInsensitive)
        || config->getInstrId().contains("esmw", Qt::CaseInsensitive)
        || config->getInstrId().contains("esmd", Qt::CaseInsensitive)) {
        if (config->getInstrGainControl() == 0)
            gain = "Low noise";
        else if (config->getInstrGainControl() == 2)
            gain = "Low distortion";
        else
            gain = "Normal";
    }

    double saveInt = 1.0 / tracePerSecond;
    if (saveInterval) {
        saveInt = saveInterval;
    }

    stream << (addPosition ? "FileType MobileData" : "FileType SDF 3.0") << '\n'
           << "User " << config->getSdefStationInitals() << "\n"
           << "LocationName " << config->getStationName() << "\n"
           << "Latitude "
           << convertDdToddmmss((addPosition ? prevLat : config->getStnLatitude().toDouble()))
           << "\n"
           << "Longitude "
           << convertDdToddmmss((addPosition ? prevLng : config->getStnLongitude().toDouble()),
                                false)
           << '\n'
           << "FreqStart " << QString::number(1e-3 * startfreq, 'f', 0) << '\n'
           << "FreqStop " << QString::number(1e-3 * stopfreq, 'f', 0) << '\n'
           << "AntennaType NoAntennaFactor" << '\n'
           << "AntennaGain NA" << "\n"
           << "FilterBandwidth " << QString::number(resolution / 1e3, 'f', 3) << '\n'
           << "LevelUnits dBuV" << '\n'
           << "Date " << QDateTime::currentDateTime().toString("yyyy-M-d") << '\n'
           << "DataPoints " << datapoints << '\n'
           << "ScanTime "
           << QString::number((double) scanTime, 'f', 3) << '\n'
           << "Instrument " << config->getInstrId() << "\n"
           << "MeasureApp Hauken v" << QString(PROJECT_VERSION) << "\n"
           << "SaveInterval " << QString::number(saveInt, 'f', 2) << " s\n"
           << "Attenuator "
           << (config->getInstrAutoAtt() ? "Auto"
                                         : QString::number(config->getInstrManAtt()) + " dB")
           << "\n"
           << "FFT "
           << (config->getInstrFftMode().contains("off", Qt::CaseInsensitive)
                   ? "Clear/write"
                   : config->getInstrFftMode())
           << "\n"
           << "AntennaPort " << antName << "\n" //<< QString::number(config->getInstrAntPort() + 1) << "\n"
           << (!gain.isEmpty() ? "GainControl " + gain + "\n" : "") << "Note "
           << config->getInstrId() << "\n"
           << "Note\n"
           << "Note " << config->getSdefStationInitals()
           << ", SaveInterval: " << QString::number(saveInt, 'f', 2) << " s, Attenuator: "
           << (config->getInstrAutoAtt() ? "Auto"
                                         : QString::number(config->getInstrManAtt()) + " dB")
           << "\n\n";

    return buf.toLocal8Bit();
}

QString SdefRecorder::convertDdToddmmss(const double d, const bool lat)
{
    QString ret;
    ret = QString::asprintf("%02i", abs((int) d)) + '.'
          + QString::asprintf("%02i", int(((abs(d) - abs((int) d)) * 60))) + '.'
          + QString::asprintf("%02i",
                              int(((((abs(d) - abs((int) d)) * 60))
                                   - (int) ((abs(d) - abs((int) d)) * 60))
                                  * 60))
          + (lat ? (d > 0 ? "N" : "S") : (d > 0 ? "E" : "W"));
    return ret;
}

void SdefRecorder::endRecording()
{
    if (file.isOpen()) {
        emit recordingEnded();
        if (config->getSdefSaveToFile()) {
            if (recordingTimeoutTimer->isActive()) {
                emit toIncidentLog(
                    NOTIFY::TYPE::SDEFRECORDER,
                    "",
                    "Recording ended after "
                        + (dateTimeRecordingStarted.secsTo(QDateTime::currentDateTime()) < 60
                               ? QString::number(
                                     dateTimeRecordingStarted.secsTo(QDateTime::currentDateTime()))
                                     + " seconds"
                               : QString::number(
                                     dateTimeRecordingStarted.secsTo(QDateTime::currentDateTime()) / 60)
                                     + (dateTimeRecordingStarted.secsTo(QDateTime::currentDateTime()) / 60
                                                == 1
                                            ? " minute"
                                            : " minutes")));
                if (autorecorderTimer->isActive())
                    autorecorderTimer->stop();
            }
        }

        if (file.isOpen()) {
            file.close();
            tracePerSecond = -1;
            finishedFilename = file.fileName(); // copy the name, in case a new recording starts immediately
        }

        if (predictionReceived)
            updFileWithPrediction(file.fileName());

        if (config->getSdefUploadFile() && recordingTimeoutTimer->isActive() && recording) {
            QTimer::singleShot(
                10000,
                this,
                &SdefRecorder::curlLogin); // 10 secs to allow AI to process the file before zipping
        } else {
            if (config->getSdefZipFiles() && recording)
                zipit(); // Zip the file anyways, we don't shit storage space here
        }
        if (!finishedFilename.isEmpty())
            finishedFileTimer->start(10000);

        recordingTimeoutTimer->stop();
        recordingStartedTimer->stop();

        if (!failed)
            recording = historicDataSaved = failed = false;
    }
}

bool SdefRecorder::curlLogin()
{
    if (!process->atEnd()) {
        qDebug() << "Curl process stuck, closing";
        process->close();
    }

    // parameters check
    if (!askedForLogin) {
        if (config->getSdefStationInitals().isEmpty() or config->getSdefUsername().isEmpty()
            or config->getSdefPassword().isEmpty() or config->getStationName().isEmpty()) {
            emit toIncidentLog(
                NOTIFY::TYPE::SDEFRECORDER,
                "",
                "Upload requested, but some parameters are missing in the config. Check it out");
            return false;
        }
        if (!config->getSdefAddPosition()
            and ((int) config->getStnLatitude().toDouble() * 1e6 == 0
                 or (int) config->getStnLongitude().toDouble() * 1e6 == 0)) {
            emit toIncidentLog(
                NOTIFY::TYPE::SDEFRECORDER,
                "",
                "Upload requested, but the current position is set to equator. Somehow I doubt it");
            return false;
        }
    }
    if (config->getSdefZipFiles())
        zipit();
    if (!askedForLogin && !finishedFilename.isEmpty())
        filesAwaitingUpload.append(finishedFilename); // add to transmit queue

    QStringList l;
    l << "-H"
      << "'application/x-www-form-url-encoded'"
      << "-F"
      << "brukernavn=" + config->getSdefUsername() << "-F"
      << "passord=" + config->getSdefPassword() << "--cookie-jar"
      << config->getWorkFolder() + "/.kake"
      << "-k" << config->getSdefAuthAddress();

    process->setArguments(l);
    process->start();
    stateCurlAwaitingLogin = true;

    //qDebug() << "Calling curl login"; // << process->program() << process->arguments();

    return true;
}

void SdefRecorder::curlCallback(int exitCode, QProcess::ExitStatus exitStatus)
{
    if (exitCode != 0 && !config->getSdefUsername().isEmpty()
        && !config->getSdefPassword().isEmpty()) {
        /*emit toIncidentLog(NOTIFY::TYPE::SDEFRECORDER,
                           "",
                           "Uploading of SDeF data failed, trying again later");*/
        if (stateCurlAwaitingLogin)
            qDebug() << "Curl login failed:" << exitStatus << exitCode;
        else if (stateCurlAwaitingFileUpload) {
            //qDebug() << "Curl file upload failed:" << exitStatus << exitCode;
            stateCurlAwaitingFileUpload = false;
        }
    } else if (stateCurlAwaitingLogin) { // curl has successfully logged in, continue
        stateCurlAwaitingLogin = false;
        if (!askedForLogin)
            QTimer::singleShot(5000, this, &SdefRecorder::curlUpload);
        else {
            emit loginSuccessful();
            askedForLogin = false;
        }
    } else if (stateCurlAwaitingFileUpload) {
        stateCurlAwaitingFileUpload = false;
        qDebug() << "Curl upload successful, removing" << filesAwaitingUpload.first()
                 << "from queue";
        filesAwaitingUpload.removeFirst();
    }
}

void SdefRecorder::curlUpload()
{
    if (process->state() != QProcess::NotRunning) {
        qDebug() << "Curl process stuck, closing" << process->processId();
        process->close();
    }

    if (filesAwaitingUpload.size()) {
        QString filename = filesAwaitingUpload.first();

        QStringList l;
        l << "--cookie" << config->getWorkFolder() + "/.kake"
          << "-X"
          << "POST"
          << "-F"
          << "file=@" + filename // + ".zip"
          << "-f"
          << "-k" << config->getSdefServer();

        process->setArguments(l);
        process->start();
        stateCurlAwaitingFileUpload = true;
        qDebug() << "Calling curl file upload" << process->program() << process->arguments();
    }
}

void SdefRecorder::updPosition(bool b, double l1, double l2)
{
    //qDebug() << b << l1 << l2;
    if (b) {
        positionHistory.append(QPair<double, double>(l1, l2));
        prevLat = l1;
        prevLng = l2;
    } else {
        positionHistory.append(QPair<double, double>(prevLat, prevLng));
    }
    while (positionHistory.size() > 120) { // keeps a constant 120 values in buffer
        positionHistory.removeFirst();
    }
}

void SdefRecorder::zipit()
{
    if (finishedFilename.size() > 1
        && !JlCompress::compressFile(finishedFilename + ".zip", finishedFilename)) {
        qDebug() << "Compression of" << finishedFilename << "failed";
    } else if (finishedFilename.size() > 1) {
        QFile::remove(file.fileName());
        finishedFilename += ".zip";
    }
}

void SdefRecorder::recPrediction(QString pred, int prob)
{
    prediction = pred;
    probability = prob;
    predictionReceived = true;
}

void SdefRecorder::updFileWithPrediction(const QString filename)
{
    QFile file(filename);
    QFile tempFile(filename + ".tmp");
    if (file.open(QIODevice::ReadOnly) && tempFile.open(QIODevice::WriteOnly)) {
        QByteArray data;

        do {
            data = file.readLine();
            if (data.contains("Note\n")) { // Found empty note line, editing
                if (probability > 0)
                    data = "Note " + prediction.toLocal8Bit() + " ["
                           + QByteArray::number(probability) + " % probability]\n";
                else
                    data = "Note " + prediction.toLocal8Bit() + "\n";
            }
            tempFile.write(data);
        } while (!file.atEnd());
        file.close();
        tempFile.close();
        tracePerSecond = tracePerSecondForTempFile = -1;
        file.remove();
        tempFile.rename(filename);
    } else {
        qDebug() << "Couldn't open file" << filename << "for reading and/or" << filename + ".tmp"
                 << "for writing. Wanted to add AI prediction, but giving up";
    }
    predictionReceived = false; // reset for next file
    prediction.clear();
    probability = 0;
}

void SdefRecorder::startTempFile()
{
    if (!tempFile.isOpen()) {
        tempFile.setFileName(config->getLogFolder() + "/" + TEMPFILENAME);

        if (tempFile.exists()) {
            tempFile.remove();
        }
        if (!tempFile.open(QIODevice::WriteOnly))
            qDebug() << "Could not open file" << tempFile.fileName() << "for writing,"
                     << tempFile.errorString();
    } else {
        qDebug() << "Temp file" << tempFile.fileName()
        << "already opened, what did you try to do here?";
    }
    tempFile.write(createHeader(config->getSaveToTempFileMaxhold()));
}

void SdefRecorder::closeTempFile()
{
    if (tempFile.isOpen()) {
        tempFile.close();
        tracePerSecondForTempFile = -1;
        qDebug() << "Temp file" << tempFile.fileName() << "closed";
    }
}

void SdefRecorder::tempFileData(const QVector<qint16> data)
{
    if (tempFileMaxhold == 0 && !tempFileTimer->isActive()) {
        tempFileTracedata = data;
        saveDataToTempFile(); // save immediately if AFAP mode
    } else { // if not keep a track of maxhold values
        if (tempFileTracedata.isEmpty())
            tempFileTracedata = data;
        else if (tempFileTracedata.size() == data.size()) {
            for (int i = 0; i < tempFileTracedata.size(); i++) {
                if (data[i] > tempFileTracedata[i])
                    tempFileTracedata[i] = data[i];
            }
        }
    }
}

void SdefRecorder::saveDataToTempFile()
{
    if (!tempFile.isOpen() and tempFileTracedata.size() > 0 and tracePerSecondForTempFile > 0) {
        tempFileTracedata.clear();
        startTempFile();
    }

    if (tempFile.isOpen() && tempFileTracedata.size()) {
        if (addPosition and !positionHistory.isEmpty())
            tempFile.write(genSdefTraceLine(positionHistory.last().first, positionHistory.last().second, tempFileTracedata));
        else
            tempFile.write(genSdefTraceLine(0, 0, tempFileTracedata));

        tempFile.flush();
        tempFileTracedata.clear();
    }
}

void SdefRecorder::setDeviceCconnectedState(bool b)
{
    if (!b) {
        endRecording();
        closeTempFile();
    }
    deviceConnected = b;
}

void SdefRecorder::updTracesPerSecond(double d)
{
    if (!iqRecordingInProgress) {
        tracePerSecond = tracePerSecondForTempFile = d;
    }
}

void SdefRecorder::setIqRecordingInProgress(bool b)
{
    if (b) iqRecordingInProgress = b;
    else QTimer::singleShot(1000, this, [this] {
            iqRecordingInProgress = false;
        });
}

void SdefRecorder::updFrequencies(quint64 sta, quint64 stop)
{
    if (!iqRecordingInProgress) {
        closeTempFile();
        endRecording();
        startfreq = sta;
        stopfreq = stop;
    }
}

void SdefRecorder::updResolution(quint32 res)
{
    if (!iqRecordingInProgress) {
        closeTempFile();
        endRecording();
        resolution = res;
    }
}

void SdefRecorder::updScanTime(int i)
{
    if (!iqRecordingInProgress) {
        closeTempFile();
        scanTime = (double)i / 1e3;
    }
}

QByteArray SdefRecorder::genSdefTraceLine(double lat, double lng, const QVector<qint16> line)
{
    QByteArray buffer;
    buffer.reserve(line.size() * 4 + 128);

    if (useNewMsFormat)
        buffer.append(QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss.zzz").toUtf8());
    else
        buffer.append(QDateTime::currentDateTime().toString("hh:mm:ss").toUtf8());

    if (addPosition) {
        buffer.append(',').append(QByteArray::number(lat,  'f', 6));
        buffer.append(',').append(QByteArray::number(lng, 'f', 6));
    }

    for (auto &&v : line)
        buffer.append(',').append(QByteArray::number(v / 10));

    buffer.append('\n');
    return buffer;
}
