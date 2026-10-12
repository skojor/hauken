#include "sdefrecorder.h"
#include "version.h"
#include "asciitranslator.h"
#include "JlCompress.h"
#include <QMetaEnum>
#include <QFutureWatcher>
#include <QtConcurrent/QtConcurrentRun>
#include <utility>
#include "appclock.h"

namespace {
struct ArchiveResult
{
    bool success = false;
    QString error;
};
}

SdefRecorder::SdefRecorder(QSharedPointer<Config> c)
{
    config = c;
    archiveThreadPool.setMaxThreadCount(1);
}

void SdefRecorder::start()
{
    process = new QProcess(this);
    recordingStartedTimer = new QTimer(this);
    recordingTimeoutTimer = new QTimer(this);
    reqPositionTimer = new QTimer(this);
    periodicCheckUploadsTimer = new QTimer(this);
    autorecorderTimer = new QTimer(this);
    tempFileTimer = new QTimer(this);
    tempFileCheckFilesize = new QTimer(this);

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
    connect(autorecorderTimer, &QTimer::timeout, this, [this] {
        setTriggerSource(TriggerSource::AutoRecorder);
        triggerRecordingWithoutAnalysisArtifacts();
    });

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

    tempFileCheckFilesize->start(3600 * 1e3);
    connect(tempFileCheckFilesize, &QTimer::timeout, this, [this] {
        if (tempFile.isOpen() && tempFile.size() > 4096e3) {
            tempFile.close();
            tempFile.remove();
        }
    });
    emit recordingEnded();
}

void SdefRecorder::end()
{
    endRecording();
    process->deleteLater();
}

void SdefRecorder::updSettings()
{
    if (autorecorderTimer) { // Don't do anything until thread has started!
        saveToSdef = config->getSdefSaveToFile();
        recordTime = config->getSdefRecordTime() * 60e3;
        maxRecordTime = config->getSdefMaxRecordTime() * 60e3;
        addPosition = config->getSdefAddPosition();
        QString src = config->getSdefGpsSource();
        positionSource = src;
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
                    if (!config->getHideRecordingStartStopMessages())
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
    if (incidentTriggersSuppressed) return;
    startRecording(true);
}

void SdefRecorder::triggerRecordingWithoutAnalysisArtifacts()
{
    startRecording(false);
}

void SdefRecorder::startRecording(bool includeAnalysisArtifacts)
{
    if (deviceConnected) { // never start a recording unless some device is connected!
        recordingStartedTimer->start(
            recordTime); // minutes. restarts every time routine is called, which means it will run for x minutes after incident ended

        if (!recording && saveToSdef) {
            recording = true;
            pendingPositionTraces.clear();
            waitingForPositionWarningEmitted = false;
            pendingPositionOverflowWarningEmitted = false;
            positionedCefWriteLogged = false;
            dateTimeRecordingStarted = AppClock::now(); // for incident log

            emit recordingStarted();

            if (!recordingTimeoutTimer->isActive())
                recordingTimeoutTimer->start(maxRecordTime); // only call once

            file.setFileName(createFilename());
            if (failed || !file.open(QIODevice::WriteOnly)) {
                emit warning("Error creating file " + file.fileName());
                recording = historicDataSaved = false;
                recordingStartedTimer->stop();
                recordingTimeoutTimer->stop();
                emit recordingEnded();
                failed = false;
            } else {
                file.write(createHeader());
                writeSettingsSnapshot();
                activeRecordingId = ++recordingSequence;
                QString reason;
                switch (triggerSource) {
                case TriggerSource::TraceAnalyzer: reason = "Measurement receiver spectrum trigger"; break;
                case TriggerSource::Gnss: reason = "GNSS trigger"; break;
                case TriggerSource::Manual: reason = "Manual recording trigger"; break;
                case TriggerSource::Mqtt: reason = "MQTT trigger"; break;
                case TriggerSource::AutoRecorder: reason = "Automatic recording"; break;
                case TriggerSource::Unknown: reason = "Recording trigger not identified"; break;
                }
                emit recordingContextStarted(activeRecordingId, dateTimeRecordingStarted, reason,
                                             includeAnalysisArtifacts,
                                             triggerSource == TriggerSource::Gnss,
                                             triggerSource == TriggerSource::TraceAnalyzer);
                if (includeAnalysisArtifacts)
                    emit recordingStartedWithAnalysisArtifacts();
                else
                    emit recordingStartedWithoutAnalysisArtifacts();
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
    incidentTriggersSuppressed = false;
    endRecording();
    if (!config->getHideRecordingStartStopMessages())
        emit toIncidentLog(NOTIFY::TYPE::SDEFRECORDER, "", "Recording triggered manually");
    setTriggerSource(TriggerSource::Manual);
    triggerRecordingWithoutAnalysisArtifacts();
}

void SdefRecorder::stopRecordingManually(bool suppressIncidentTriggers)
{
    incidentTriggersSuppressed = suppressIncidentTriggers;
    preserveAutoRecorderSchedule = autorecorderTimer && autorecorderTimer->isActive();
    endRecording();
    preserveAutoRecorderSchedule = false;
}

void SdefRecorder::clearIncidentTriggerSuppression()
{
    incidentTriggersSuppressed = false;
}

QString SdefRecorder::incidentBasePath()
{
    QString dir = config->incidentFolder();

    if (!QDir().exists(dir))
        QDir().mkpath(dir);

    return dir + "/" + config->incidentTimestamp().toString("yyyyMMddhhmmss")
           + "_" + AsciiTranslator::toAscii(config->getStationName());
}

QString SdefRecorder::createFilename()
{
    const QString basePath = incidentBasePath()
                             + "_" + QString::number(1e-3 * startfreq, 'f', 0) + "-"
                             + QString::number(1e-3 * stopfreq, 'f', 0);
    QString filename = basePath + ".cef";
    int suffix = 1;
    while (QFileInfo::exists(filename) || QFileInfo::exists(filename + ".zip")
           || archivePathsInProgress.contains(filename)
           || archivePathsInProgress.contains(filename + ".zip")) {
        filename = basePath + "_part" + QString::number(suffix++) + ".cef";
    }
    return filename;
}

void SdefRecorder::writeSettingsSnapshot()
{
    const QString basePath = incidentBasePath();
    const QMetaEnum sourceEnum = QMetaEnum::fromType<TriggerSource>();

    QVariantMap runtime;
    runtime["swVersion"] = QString(PROJECT_VERSION);
    runtime["instrId"] = config->getInstrId();
    runtime["streamStartFreqHz"] = QString::number(startfreq, 'f', 0);
    runtime["streamStopFreqHz"] = QString::number(stopfreq, 'f', 0);
    runtime["streamResolutionHz"] = resolution;
    runtime["dataPoints"] = datapoints;
    runtime["tracesPerSecond"] = tracePerSecond;
    runtime["plotResolution"] = config->getPlotResolution();
    runtime["modeUsed"] = modeUsed;
    runtime["antennaName"] = antName;
    runtime["triggerSource"] = QString(sourceEnum.valueToKey(static_cast<int>(triggerSource)));
    runtime["recordingStarted"] = dateTimeRecordingStarted.toString(Qt::ISODateWithMs);
    runtime["incidentTimestamp"] = config->incidentTimestamp().toString(Qt::ISODateWithMs);
    runtime["cefFile"] = QFileInfo(file.fileName()).fileName();
    runtime["levelsUnit"] = "dBuV, integer (0.1 dB internal values divided by 10)";
    runtime["normalizedTraces"] = config->getInstrNormalizeSpectrum();

    if (!config->writeSettingsSnapshot(basePath + "_settings.ini", runtime))
        qWarning() << "Could not write settings snapshot" << basePath + "_settings.ini";

    emit incidentFilesBasePath(basePath);
}
void SdefRecorder::receiveTrace(const QDateTime &timestamp, const QVector<qint16> &data)
{
    if (historicDataSaved && !iqRecordingInProgress && !skipTraces) { // TODO: Quickfix to see if random "not saved correctly" error message disappears. NB! This one will just skip trace(s) until backlog is saved!
        if (addPosition && !positionEstablished) {
            queueTraceUntilPosition(timestamp, data);
        } else if (addPosition and !positionHistory.isEmpty())
            logPositionedTraceWritten(
                timestamp,
                file.write(genSdefTraceLine(positionHistory.last().first,
                                            positionHistory.last().second,
                                            data,
                                            timestamp)));
        else
            file.write(genSdefTraceLine(0, 0, data, timestamp));
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

        for (const qint16 value : data.at(i)) {
            if (value < -999 || value >= 2000) {
                ok = false;
                break;
            }
        }
        if (!ok) {
            qDebug() << "This backlog line failed validation at" << datetime.at(i);
            continue;
        }

        if (addPosition && !positionEstablished) {
            queueTraceUntilPosition(datetime.at(i), data.at(i));
            continue;
        }

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

        if (addPosition && !positionHistory.isEmpty()) {
            if (startTime.secsTo(datetime.at(i)) > 0) { // one second of data has passed
                dateIterator++;
                startTime = datetime.at(i);
            }
            if (dateIterator > positionHistory.size() - 1) // should never happen except in early startup
                dateIterator = positionHistory.size() - 1;

            buffer.append(",").append(QByteArray::number(positionHistory.at(dateIterator).first, 'f', 6));
            buffer.append(",").append(QByteArray::number(positionHistory.at(dateIterator).second, 'f', 6));
        } else if (addPosition) {
            buffer.append(",").append(QByteArray::number(prevLat, 'f', 6));
            buffer.append(",").append(QByteArray::number(prevLng, 'f', 6));
        }

        for (const qint16 val : data.at(i))
            buffer.append(',').append(QByteArray::number(val / 10));
        buffer.append('\n');

        const qint64 bytesWritten = file.write(buffer);
        if (addPosition)
            logPositionedTraceWritten(datetime.at(i), bytesWritten);
    }
    historicDataSaved = true;
    qDebug() << "SdefRecorder: Backlog used" << timer.elapsed() << "ms to write to disk";
}

QByteArray SdefRecorder::createHeader(const int saveInterval)
{
    QString buf;
    QTextStream stream(&buf, QIODevice::ReadWrite);

    QString gain;
    if (config->getInstrId().contains("em200", Qt::CaseInsensitive)
        || config->getInstrId().contains("eb500", Qt::CaseInsensitive)
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
           << "Date " << AppClock::now().toString("yyyy-M-d") << '\n'
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
        if (activeRecordingId) {
            emit recordingContextEnded(activeRecordingId);
            activeRecordingId = 0;
        }
        emit recordingEnded();
        if (config->getSdefSaveToFile()) {
            if (recordingTimeoutTimer->isActive()) {
                const qint64 secs = dateTimeRecordingStarted.secsTo(AppClock::now());
                if (!config->getHideRecordingStartStopMessages())
                    emit toIncidentLog(
                        NOTIFY::TYPE::SDEFRECORDER,
                        "",
                        "Recording ended after "
                            + (secs < 60 ? QString::number(secs) + " seconds"
                                         : QString::number(secs / 60) + (secs / 60 == 1 ? " minute" : " minutes")));
                if (autorecorderTimer->isActive() && !preserveAutoRecorderSchedule)
                    autorecorderTimer->stop();
            }
        }

        if (file.isOpen()) {
            file.close();
            tracePerSecond = -1;
            finishedFilename = file.fileName();
        }
        const QString closedFilename = finishedFilename;
        const bool intentional = flagUploadIntentionalOnly;
        flagUploadIntentionalOnly = false;

        if (predictionReceived)
            updFileWithPrediction(closedFilename);

        if (config->getSdefUploadFile() && recordingTimeoutTimer->isActive() && recording && !config->isReplayMode()) {
            QTimer::singleShot(10000, this, [this, closedFilename, intentional] {
                if (!curlLogin(closedFilename, true, intentional))
                    notifyFileReadyForUpload(closedFilename, intentional, 0);
            }); // allow AI to process the file before zipping
        } else {
            const bool shouldZip = recording && !config->isReplayMode()
                                   && (config->getSdefZipAllIncidentFiles()
                                       || config->getSdefZipFiles());
            if (shouldZip) {
                archiveFile(closedFilename, config->getSdefZipAllIncidentFiles(),
                            [this, closedFilename, intentional](bool success,
                                                                 const QString &resultFilename) {
                    const QString readyFilename = success ? resultFilename : closedFilename;
                    if (finishedFilename == closedFilename)
                        finishedFilename = readyFilename;
                    if (!config->isReplayMode())
                        notifyFileReadyForUpload(readyFilename, intentional);
                });
            } else if (!closedFilename.isEmpty() && !config->isReplayMode()) {
                notifyFileReadyForUpload(closedFilename, intentional);
            }
        }

        recordingTimeoutTimer->stop();
        recordingStartedTimer->stop();

        if (!failed)
            recording = historicDataSaved = failed = false;
        if (!pendingPositionTraces.isEmpty()) {
            qWarning() << "Discarding" << pendingPositionTraces.size()
                       << "CEF traces without a valid GNSS position at recording end";
            pendingPositionTraces.clear();
        }
        waitingForPositionWarningEmitted = false;
        pendingPositionOverflowWarningEmitted = false;
        triggerSource = TriggerSource::Unknown;
    }
}

bool SdefRecorder::curlLogin(QString filename, bool notifyOAuth, bool intentional)
{
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

    if (filename.isEmpty())
        filename = finishedFilename;
    const bool loginOnly = askedForLogin;
    const bool shouldZip = !filename.isEmpty()
                           && (config->getSdefZipAllIncidentFiles() || config->getSdefZipFiles());
    auto continueLogin = [this, filename, notifyOAuth, intentional, loginOnly,
                          incidentArchive = config->getSdefZipAllIncidentFiles()]
                         (bool success, const QString &resultFilename) {
        if (finishedFilename == filename && success)
            finishedFilename = resultFilename;
        if (notifyOAuth && !filename.isEmpty())
            notifyFileReadyForUpload(success ? resultFilename : filename, intentional, 0);
        if (!success && incidentArchive)
            return;
        if (!loginOnly && !filename.isEmpty())
            filesAwaitingUpload.append(success ? resultFilename : filename);
        startCurlLoginProcess();
    };

    if (shouldZip)
        archiveFile(filename, config->getSdefZipAllIncidentFiles(), std::move(continueLogin));
    else
        continueLogin(true, filename);

    return true;
}

void SdefRecorder::startCurlLoginProcess()
{
    if (!process->atEnd()) {
        qDebug() << "Curl process stuck, closing";
        process->close();
    }

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
    const bool firstValidPosition = b && !positionEstablished;
    if (b) {
        positionEstablished = true;
        positionHistory.append(QPair<double, double>(l1, l2));
        prevLat = l1;
        prevLng = l2;
    } else {
        if (!positionEstablished)
            return;
        positionHistory.append(QPair<double, double>(prevLat, prevLng));
    }
    while (positionHistory.size() > 120) { // keeps a constant 120 values in buffer
        positionHistory.removeFirst();
    }
    if (firstValidPosition && addPosition && recording) {
        qDebug() << "SdefRecorder: Valid GNSS position established for CEF recording"
                 << file.fileName() << "position" << l1 << l2
                 << "pending traces" << pendingPositionTraces.size();
    }
    if (b && !pendingPositionTraces.isEmpty())
        flushTracesWaitingForPosition(l1, l2);
}

void SdefRecorder::queueTraceUntilPosition(const QDateTime &timestamp,
                                           const QVector<qint16> &data)
{
    if (!waitingForPositionWarningEmitted) {
        emit warning("Waiting for a valid GNSS position before writing CEF data");
        qDebug() << "SdefRecorder: Deferring CEF trace until valid GNSS position"
                 << file.fileName() << "first pending timestamp" << timestamp;
        waitingForPositionWarningEmitted = true;
    }
    if (pendingPositionTraces.size() >= maximumPendingPositionTraces) {
        pendingPositionTraces.removeFirst();
        if (!pendingPositionOverflowWarningEmitted) {
            emit warning("Pending CEF data limit reached while waiting for GNSS; oldest traces will be dropped");
            pendingPositionOverflowWarningEmitted = true;
        }
    }
    pendingPositionTraces.append({timestamp, data});
}

void SdefRecorder::flushTracesWaitingForPosition(double latitude, double longitude)
{
    if (!recording || !file.isOpen()) {
        pendingPositionTraces.clear();
        return;
    }

    const int pendingCount = pendingPositionTraces.size();
    for (const PendingPositionTrace &trace : std::as_const(pendingPositionTraces)) {
        const qint64 bytesWritten = file.write(
            genSdefTraceLine(latitude, longitude, trace.data, trace.timestamp));
        logPositionedTraceWritten(trace.timestamp, bytesWritten);
    }

    pendingPositionTraces.clear();
    waitingForPositionWarningEmitted = false;
    pendingPositionOverflowWarningEmitted = false;
    qDebug() << "SdefRecorder: CEF log updated with queued traces"
             << file.fileName() << "written traces" << pendingCount
             << "position" << latitude << longitude;
}

void SdefRecorder::logPositionedTraceWritten(const QDateTime &timestamp, qint64 bytesWritten)
{
    if (positionedCefWriteLogged)
        return;

    if (bytesWritten > 0) {
        qDebug() << "SdefRecorder: First CEF trace written with valid GNSS position"
                 << file.fileName() << "timestamp" << timestamp
                 << "bytes" << bytesWritten;
        positionedCefWriteLogged = true;
    } else {
        qWarning() << "SdefRecorder: Failed writing positioned CEF trace"
                   << file.fileName() << "timestamp" << timestamp
                   << file.errorString();
    }
}

void SdefRecorder::notifyFileReadyForUpload(const QString &filename, bool intentional, int delayMs)
{
    QTimer::singleShot(delayMs, this, [this, filename, intentional] {
        if (!config->getSdefUploadIntentionalOnly() || intentional) {
            qDebug() << "Preparing upload of" << filename << intentional;
            emit fileReadyForUpload(filename);
        } else {
            qDebug() << "Classified as non-intentional or no classification received. Not uploading";
        }
        if (finishedFilename == filename)
            finishedFilename.clear();
    });
}

void SdefRecorder::archiveFile(const QString &filename,
                               bool incidentArchive,
                               ArchiveCallback callback)
{
    if (filename.isEmpty()) {
        callback(false, filename);
        return;
    }
    if (archivePathsInProgress.contains(filename)) {
        archiveCallbacksInProgress[filename].append(std::move(callback));
        return;
    }

    QStringList filesToArchive;
    QString archiveFilename;
    if (incidentArchive) {
        const QFileInfo sourceInfo(filename);
        const QDir incidentFolder = sourceInfo.absoluteDir();
        const QString preferredName = incidentFolder.absoluteFilePath(
            sourceInfo.completeBaseName() + "_incident.zip");
        const QFileInfo preferredInfo(preferredName);
        const QFileInfoList incidentFiles = incidentFolder.entryInfoList(
            QDir::Files | QDir::Readable, QDir::Name);
        for (const QFileInfo &fileInfo : incidentFiles) {
            if (fileInfo.suffix().compare("zip", Qt::CaseInsensitive) != 0)
                filesToArchive.append(fileInfo.absoluteFilePath());
        }
        if (filesToArchive.isEmpty()) {
            qDebug() << "No incident files found to compress in" << incidentFolder.absolutePath();
            callback(false, filename);
            return;
        }
        archiveFilename = preferredInfo.absoluteFilePath();
    } else {
        filesToArchive.append(filename);
        archiveFilename = filename + ".zip";
    }

    const QFileInfo archiveInfo(archiveFilename);
    const QString archiveBase = archiveInfo.completeBaseName();
    const QString archiveSuffix = archiveInfo.suffix();
    int suffix = 1;
    while (QFileInfo::exists(archiveFilename) || archivePathsInProgress.contains(archiveFilename)) {
        archiveFilename = archiveInfo.absoluteDir().absoluteFilePath(
            archiveBase + "_" + QString::number(suffix++) + "." + archiveSuffix);
    }

    for (const QString &source : std::as_const(filesToArchive))
        archivePathsInProgress.insert(source);
    archivePathsInProgress.insert(archiveFilename);
    archiveCallbacksInProgress[filename].append(std::move(callback));

    auto *watcher = new QFutureWatcher<ArchiveResult>(this);
    connect(watcher, &QFutureWatcher<ArchiveResult>::finished, this,
            [this, watcher, filename, filesToArchive, archiveFilename] {
        const ArchiveResult result = watcher->result();
        watcher->deleteLater();
        for (const QString &source : filesToArchive)
            archivePathsInProgress.remove(source);
        archivePathsInProgress.remove(archiveFilename);

        if (!result.success)
            qWarning() << "Compression of" << filename << "failed:" << result.error;
        const QString resultFilename = result.success ? archiveFilename : filename;
        const auto callbacks = archiveCallbacksInProgress.take(filename);
        for (const ArchiveCallback &completion : callbacks)
            completion(result.success, resultFilename);
    });

    watcher->setFuture(QtConcurrent::run(&archiveThreadPool, [filesToArchive, archiveFilename] {
        ArchiveResult result;
        result.success = filesToArchive.size() == 1
                             ? JlCompress::compressFile(archiveFilename, filesToArchive.first())
                             : JlCompress::compressFiles(archiveFilename, filesToArchive);
        if (!result.success) {
            result.error = QStringLiteral("archive creation failed");
            QFile::remove(archiveFilename);
            return result;
        }
        for (const QString &source : filesToArchive)
            QFile::remove(source);
        return result;
    }));
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
        //qDebug() << "Temp file" << tempFile.fileName() << "closed";
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
    if (addPosition && !positionEstablished)
        return;

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
    //if (!iqRecordingInProgress) {
    closeTempFile();
    endRecording();
    startfreq = sta;
    stopfreq = stop;
    //}
}

void SdefRecorder::updResolution(quint32 res)
{
    //if (!iqRecordingInProgress) {
    closeTempFile();
    endRecording();
    resolution = res;
    //}
}

void SdefRecorder::updScanTime(int i)
{
    //if (!iqRecordingInProgress) {
    closeTempFile();
    scanTime = (double)i / 1e3;
    //}
}

QByteArray SdefRecorder::genSdefTraceLine(double lat,
                                         double lng,
                                         const QVector<qint16> line,
                                         const QDateTime &timestamp)
{
    QByteArray buffer;
    buffer.reserve(line.size() * 4 + 128);

    if (useNewMsFormat)
        buffer.append((timestamp.isValid() ? timestamp : AppClock::now())
                          .toString("yyyy-MM-dd hh:mm:ss.zzz").toUtf8());
    else
        buffer.append((timestamp.isValid() ? timestamp : AppClock::now())
                          .toString("hh:mm:ss").toUtf8());

    if (addPosition) {
        buffer.append(',').append(QByteArray::number(lat,  'f', 6));
        buffer.append(',').append(QByteArray::number(lng, 'f', 6));
    }

    for (auto &&v : line)
        buffer.append(',').append(QByteArray::number(v / 10));

    buffer.append('\n');
    return buffer;
}
