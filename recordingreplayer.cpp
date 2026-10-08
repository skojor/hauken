#include "recordingreplayer.h"
#include "appclock.h"
#include "asciitranslator.h"
#include "JlCompress.h"
#include <QDirIterator>
#include <QFileInfo>
#include <QRegularExpression>
#include <QSet>
#include <QSettings>
#include <QtEndian>
#include <QTimeZone>
#include <algorithm>
#include <cstring>

RecordingReplayer::RecordingReplayer(QSharedPointer<Config> c, QObject *parent)
    : QObject(parent), config(c)
{
    tickTimer.setSingleShot(true);
    connect(&tickTimer, &QTimer::timeout, this, &RecordingReplayer::processDue);
    iqTimer.setInterval(0);
    connect(&iqTimer, &QTimer::timeout, this, &RecordingReplayer::sendIqChunk);
}

RecordingReplayer::~RecordingReplayer()
{
    close();
}

bool RecordingReplayer::open(const QString &path, QString &error)
{
    close();

    const QFileInfo info(path);
    QStringList folders;

    if (info.isFile() && info.suffix().compare("zip", Qt::CaseInsensitive) == 0) {
        tempDir = std::make_unique<QTemporaryDir>();
        if (!tempDir->isValid() || JlCompress::extractDir(path, tempDir->path()).isEmpty()) {
            error = "Could not extract " + info.fileName();
            close();
            return false;
        }
        folders.append(tempDir->path());
        recordingTitle = info.completeBaseName();
    }
    else if (info.isDir()) {
        folders.append(info.absoluteFilePath());
        recordingTitle = info.fileName();

        // The .cef may be zipped on its own (.cef.zip) or together with everything else (_incident.zip)
        QDirIterator cefIt(info.absoluteFilePath(), {"*.cef"}, QDir::Files, QDirIterator::Subdirectories);
        if (!cefIt.hasNext()) {
            QDirIterator zipIt(info.absoluteFilePath(), {"*.zip"}, QDir::Files);
            tempDir = std::make_unique<QTemporaryDir>();
            while (zipIt.hasNext() && tempDir->isValid())
                JlCompress::extractDir(zipIt.next(), tempDir->path());
            folders.append(tempDir->path());
        }
    }
    else {
        error = path + " is neither a folder nor a zip file";
        return false;
    }

    collectFiles(folders);

    if (cefFiles.isEmpty()) {
        error = "No usable .cef trace file found in " + info.fileName();
        close();
        return false;
    }

    std::sort(cefFiles.begin(), cefFiles.end(), [](const CefFile &a, const CefFile &b) {
        return a.firstTime < b.firstTime;
    });

    station = cefFiles.first().location;
    for (const CefFile &cef : std::as_const(cefFiles))
        totalLines += cef.lines;

    triggerSource = "Unknown";
    if (!snapshotFile.isEmpty()) {
        QSettings snapshot(snapshotFile, QSettings::IniFormat);
        triggerSource = snapshot.value("runtime/triggerSource", "Unknown").toString();
        incidentTime = QDateTime::fromString(snapshot.value("runtime/incidentTimestamp").toString(), Qt::ISODateWithMs);
        if (station.isEmpty())
            station = snapshot.value("station/Name").toString();
    }
    if (!incidentTime.isValid())
        incidentTime = incidentTimeFromName(recordingTitle);
    if (!incidentTime.isValid())
        incidentTime = incidentTimeFromName(QFileInfo(cefFiles.first().path).fileName());
    if (!incidentTime.isValid())
        incidentTime = cefFiles.first().firstTime.addSecs(config->getSdefPreRecordTime());

    parseIqFiles();
    parseGnssFiles();
    return true;
}

void RecordingReplayer::close()
{
    stop();
    cefStream.close();
    cefFiles.clear();
    cefIndex = -1;
    nextTraceTime = QDateTime();
    nextTrace.clear();
    recordingTitle.clear();
    station.clear();
    snapshotFile.clear();
    avgDataFile.clear();
    triggerSource.clear();
    iqFileList.clear();
    gnssFileList.clear();
    incidentTime = QDateTime();
    totalLines = playedLines = 0;
    lastProgress = -1;
    iqRecordings.clear();
    gnssEvents.clear();
    gnssLabels.clear();
    gnssIndex = 0;
    AppClock::setOffsetMs(0);
    tempDir.reset();
}

void RecordingReplayer::collectFiles(const QStringList &folders)
{
    QSet<QString> seenNames;
    for (const QString &folder : folders) {
        QDirIterator it(folder, QDir::Files, QDirIterator::Subdirectories);
        while (it.hasNext()) {
            const QString file = it.next();
            const QString name = QFileInfo(file).fileName();
            if (seenNames.contains(name))
                continue;
            seenNames.insert(name);

            if (name.endsWith(".cef", Qt::CaseInsensitive)) {
                CefFile cef;
                cef.path = file;
                if (parseCef(cef))
                    cefFiles.append(cef);
                else
                    qWarning() << "Replay: could not parse" << file;
            }
            else if (name.endsWith(".avgdata", Qt::CaseInsensitive))
                avgDataFile = file;
            else if (name.endsWith("_settings.ini", Qt::CaseInsensitive))
                snapshotFile = file;
            else if (name.endsWith(".iq", Qt::CaseInsensitive))
                iqFileList.append(file);
            else if (name.endsWith(".log", Qt::CaseInsensitive) && name.contains("_gnss", Qt::CaseInsensitive))
                gnssFileList.append(file);
        }
    }
    iqFileList.sort();
    gnssFileList.sort();
}

bool RecordingReplayer::parseCef(CefFile &cef) const
{
    QFile file(cef.path);
    if (!file.open(QIODevice::ReadOnly))
        return false;

    while (!file.atEnd()) {
        const qint64 pos = file.pos();
        const QByteArray line = file.readLine().trimmed();
        if (line.isEmpty())
            break;
        if (!line.isEmpty() && QChar(line.at(0)).isDigit()) { // header without blank line terminator
            file.seek(pos);
            break;
        }
        const QString text = QString::fromLocal8Bit(line);
        const QString key = text.section(' ', 0, 0).toLower();
        const QString value = text.section(' ', 1).trimmed();

        if (key == "filetype")
            cef.mobile = value.contains("MobileData", Qt::CaseInsensitive);
        else if (key == "locationname")
            cef.location = value;
        else if (key == "freqstart")
            cef.startHz = value.toDouble() * 1e3;
        else if (key == "freqstop")
            cef.stopHz = value.toDouble() * 1e3;
        else if (key == "filterbandwidth")
            cef.resolutionHz = value.toDouble() * 1e3;
        else if (key == "datapoints")
            cef.dataPoints = value.toInt();
        else if (key == "saveinterval")
            cef.saveInterval = value.section(' ', 0, 0).toDouble();
        else if (key == "date")
            cef.date = QDate::fromString(value, "yyyy-M-d");
    }
    cef.dataOffset = file.pos();

    if (cef.startHz <= 0 || cef.stopHz <= cef.startHz || cef.dataPoints <= 0)
        return false;
    if (cef.resolutionHz <= 0 && cef.dataPoints > 1)
        cef.resolutionHz = (cef.stopHz - cef.startHz) / (cef.dataPoints - 1);

    while (!file.atEnd()) {
        const QByteArray line = file.readLine();
        const int comma = line.indexOf(',');
        if (comma <= 0)
            continue;
        const QDateTime dt = parseTimestamp(line.left(comma), cef.date);
        if (!dt.isValid())
            continue;
        if (!cef.firstTime.isValid())
            cef.firstTime = dt;
        cef.lastTime = dt;
        cef.lines++;
    }
    return cef.lines > 0;
}

QDateTime RecordingReplayer::parseTimestamp(const QByteArray &field, const QDate &date) const
{
    const QString text = QString::fromLatin1(field).trimmed();
    QDateTime dt = QDateTime::fromString(text, "yyyy-MM-dd hh:mm:ss.zzz");
    if (!dt.isValid())
        dt = QDateTime::fromString(text, "yyyy-MM-dd hh:mm:ss");
    if (!dt.isValid() && date.isValid()) { // legacy time-only format
        const QTime time = QTime::fromString(text, "hh:mm:ss");
        if (time.isValid())
            dt = QDateTime(date, time);
    }
    return dt;
}

QDateTime RecordingReplayer::incidentTimeFromName(const QString &name) const
{
    static const QRegularExpression re("(\\d{8})_?(\\d{6})_");
    const QRegularExpressionMatch match = re.match(name);
    if (!match.hasMatch())
        return QDateTime();
    return QDateTime::fromString(match.captured(1) + match.captured(2), "yyyyMMddhhmmss");
}

double RecordingReplayer::tracesPerSecondFor(const CefFile &cef) const
{
    const qint64 ms = cef.firstTime.msecsTo(cef.lastTime);
    if (cef.lines > 1 && ms > 0)
        return (cef.lines - 1) * 1e3 / ms;
    if (cef.saveInterval > 0)
        return 1.0 / cef.saveInterval;
    return 1.0;
}

bool RecordingReplayer::openCef(int index)
{
    cefStream.close();
    if (index < 0 || index >= cefFiles.size())
        return false;

    const bool changed = cefIndex < 0
                         || cefFiles[cefIndex].startHz != cefFiles[index].startHz
                         || cefFiles[cefIndex].stopHz != cefFiles[index].stopHz
                         || cefFiles[cefIndex].resolutionHz != cefFiles[index].resolutionHz;
    cefIndex = index;
    const CefFile &cef = cefFiles[index];

    cefStream.setFileName(cef.path);
    if (!cefStream.open(QIODevice::ReadOnly) || !cefStream.seek(cef.dataOffset))
        return false;

    if (changed) {
        // Resolution first, plot controller needs it before recalculating the frequency axis
        emit resolutionChanged(cef.resolutionHz);
        emit frequencyChanged(cef.startHz, cef.stopHz);
    }
    emit tracesPerSecondChanged(tracesPerSecondFor(cef));
    return true;
}

bool RecordingReplayer::readNextTrace()
{
    while (cefIndex >= 0 && cefIndex < cefFiles.size()) {
        const CefFile &cef = cefFiles[cefIndex];
        while (!cefStream.atEnd()) {
            const QList<QByteArray> fields = cefStream.readLine().trimmed().split(',');
            const int skip = cef.mobile ? 3 : 1;
            if (fields.size() - skip != cef.dataPoints)
                continue;
            const QDateTime dt = parseTimestamp(fields.first(), cef.date);
            if (!dt.isValid())
                continue;

            QVector<qint16> trace(cef.dataPoints);
            for (int i = 0; i < cef.dataPoints; i++)
                trace[i] = (qint16)(fields.at(i + skip).toInt() * 10); // stored as whole dB
            nextTrace = trace;
            nextTraceTime = dt;
            return true;
        }
        if (!openCef(cefIndex + 1))
            break;
    }
    nextTraceTime = QDateTime();
    nextTrace.clear();
    return false;
}

void RecordingReplayer::configureStream()
{
    cefIndex = -1;
    playedLines = 0;
    lastProgress = -1;
    if (openCef(0))
        readNextTrace();
}

void RecordingReplayer::play(bool baselineLoaded)
{
    if (running || !nextTraceTime.isValid())
        return;

    gnssIndex = 0;
    QDateTime firstEvent = nextTraceTime;
    if (!gnssEvents.isEmpty() && gnssEvents.first().time < firstEvent)
        firstEvent = gnssEvents.first().time;
    AppClock::setOffsetMs(firstEvent.toMSecsSinceEpoch() - QDateTime::currentMSecsSinceEpoch() - 1000);

    syntheticTriggers.clear();
    syntheticAlarmEnd = QDateTime();
    // Without a baseline the analyzer cannot trigger, and non-analyzer triggers can never be reproduced from traces
    if (!baselineLoaded || triggerSource != "TraceAnalyzer") {
        const QDateTime lastTraceTime = cefFiles.last().lastTime;
        const QDateTime lastTrigger = lastTraceTime.addSecs(-60LL * config->getSdefRecordTime());
        QDateTime t = incidentTime;
        syntheticTriggers.append(t);
        while ((t = t.addSecs(10)) < lastTrigger)
            syntheticTriggers.append(t);
        syntheticAlarmEnd = syntheticTriggers.last().addSecs(1);
    }

    running = feeding = true;
    emit playbackRunning(true);
    emit toIncidentLog(NOTIFY::TYPE::GENERAL, "", "Replay of recorded data " + recordingTitle + " started"
                       + (syntheticTriggers.isEmpty() ? "" : ", using the original trigger time")
                       + (iqRecordings.isEmpty() ? ", no raw IQ data in recording, IQ capture disabled" : ""));
    scheduleNext();
}

void RecordingReplayer::stop()
{
    tickTimer.stop();
    endIqStream();
    if (!running)
        return;
    running = feeding = false;
    if (syntheticAlarmEnd.isValid()) {
        syntheticAlarmEnd = QDateTime();
        emit triggerAlarmEnded();
    }
    syntheticTriggers.clear();
    emit playbackRunning(false);
}

QDateTime RecordingReplayer::nextEventTime() const
{
    QDateTime next = nextTraceTime;
    if (!syntheticTriggers.isEmpty() && (!next.isValid() || syntheticTriggers.first() < next))
        next = syntheticTriggers.first();
    if (syntheticAlarmEnd.isValid() && (!next.isValid() || syntheticAlarmEnd < next))
        next = syntheticAlarmEnd;
    if (gnssIndex < gnssEvents.size() && (!next.isValid() || gnssEvents[gnssIndex].time < next))
        next = gnssEvents[gnssIndex].time;
    return next;
}

void RecordingReplayer::scheduleNext()
{
    const QDateTime next = nextEventTime();
    if (!next.isValid()) {
        finish();
        return;
    }
    tickTimer.start((int)std::clamp(AppClock::now().msecsTo(next), qint64(0), qint64(60000)));
}

void RecordingReplayer::processDue()
{
    if (!feeding)
        return;

    const QDateTime now = AppClock::now();
    for (int handled = 0; handled < 200; handled++) {
        if (!syntheticTriggers.isEmpty() && syntheticTriggers.first() <= now) {
            syntheticTriggers.removeFirst();
            emit triggerAlarm(triggerSource);
        }
        else if (syntheticAlarmEnd.isValid() && syntheticAlarmEnd <= now) {
            syntheticAlarmEnd = QDateTime();
            emit triggerAlarmEnded();
        }
        else if (gnssIndex < gnssEvents.size() && gnssEvents[gnssIndex].time <= now) {
            const GnssEvent &event = gnssEvents[gnssIndex++];
            emit gnssData(event.profileId, event.data);
        }
        else if (nextTraceTime.isValid() && nextTraceTime <= now) {
            emit traceReady(nextTrace);
            playedLines++;
            readNextTrace();
        }
        else
            break;
    }

    const int percent = totalLines > 0 ? playedLines * 100 / totalLines : 100;
    if (percent != lastProgress) {
        lastProgress = percent;
        emit progress(percent);
    }
    scheduleNext();
}

void RecordingReplayer::finish()
{
    if (!feeding)
        return;
    feeding = false;
    emit toIncidentLog(NOTIFY::TYPE::GENERAL, "", "Replay of recorded data " + recordingTitle + " finished");
    emit finished();
}

void RecordingReplayer::parseIqFiles()
{
    for (const QString &path : std::as_const(iqFileList)) {
        IqRecording rec;
        rec.path = path;
        rec.is16bit = path.contains("16bit", Qt::CaseInsensitive);
        const QStringList tokens = QFileInfo(path).completeBaseName().split('_');
        for (const QString &token : tokens) {
            if (token.startsWith("bw", Qt::CaseInsensitive) && token.endsWith("kHz", Qt::CaseInsensitive))
                rec.bandwidthHz = (quint32)(token.mid(2).chopped(3).toDouble() * 1e3);
            else if (token.endsWith("MHz", Qt::CaseInsensitive))
                rec.frequencyMHz = token.chopped(3).toDouble();
            else if (token.endsWith("Msps", Qt::CaseInsensitive))
                rec.sampleRate = (quint64)(token.chopped(4).toDouble() * 1e6);
        }
        if (rec.frequencyMHz <= 0 || rec.sampleRate == 0) {
            qWarning() << "Replay: cannot interpret IQ file name" << path;
            continue;
        }
        if (!rec.bandwidthHz)
            rec.bandwidthHz = (quint32)(rec.sampleRate / 1.28);
        rec.samples = QFileInfo(path).size() / (rec.is16bit ? 4 : 2);
        iqRecordings.append(rec);
    }
}

QVector<double> RecordingReplayer::iqFrequenciesMHz() const
{
    QVector<double> freqs;
    for (const IqRecording &rec : iqRecordings)
        freqs.append(rec.frequencyMHz);
    return freqs;
}

QVector<qint64> RecordingReplayer::iqSampleCounts() const
{
    QVector<qint64> counts;
    for (const IqRecording &rec : iqRecordings)
        counts.append(rec.samples);
    return counts;
}

void RecordingReplayer::requestIqFrequency(double frequencyMHz)
{
    endIqStream();
    if (!running)
        return;

    const auto rec = std::find_if(iqRecordings.cbegin(), iqRecordings.cend(), [frequencyMHz](const IqRecording &r) {
        return qAbs(r.frequencyMHz - frequencyMHz) < 0.0005;
    });
    if (rec == iqRecordings.cend()) {
        qWarning() << "Replay: no recorded IQ data for" << frequencyMHz << "MHz";
        return;
    }

    QFile file(rec->path);
    if (!file.open(QIODevice::ReadOnly))
        return;
    const QByteArray raw = file.readAll();

    if (rec->is16bit) {
        iqPending.resize(raw.size() / 4);
        std::memcpy(iqPending.data(), raw.constData(), iqPending.size() * 4); // written in host (little endian) order
    }
    else {
        iqPending.resize(raw.size() / 2);
        for (qsizetype i = 0; i < iqPending.size(); i++) {
            iqPending[i].real = (qint16)((qint8)raw.at(2 * i) * 128);
            iqPending[i].imag = (qint16)((qint8)raw.at(2 * i + 1) * 128);
        }
    }
    iqPendingPos = 0;

    emit iqHeader((quint64)(frequencyMHz * 1e6), rec->bandwidthHz, rec->sampleRate, 0);
    iqTimer.start();
}

void RecordingReplayer::sendIqChunk()
{
    constexpr qsizetype chunkSize = 1 << 20;
    if (iqPendingPos >= iqPending.size()) {
        endIqStream();
        return;
    }
    emit iqData(iqPending.mid(iqPendingPos, chunkSize));
    iqPendingPos += chunkSize;
}

void RecordingReplayer::endIqStream()
{
    iqTimer.stop();
    iqPending.clear();
    iqPendingPos = 0;
}

QString RecordingReplayer::gnssLabelFromFilename(const QString &name)
{
    static const QRegularExpression re("_gnss-?(.*?)(?:_nmea|_binary)?\\.log$", QRegularExpression::CaseInsensitiveOption);
    const QRegularExpressionMatch match = re.match(name);
    const QString label = match.hasMatch() ? match.captured(1) : QString();
    return label.isEmpty() ? "gnss" : label;
}

void RecordingReplayer::parseGnssFiles()
{
    for (const QString &path : std::as_const(gnssFileList)) {
        const QString name = QFileInfo(path).fileName();
        const QString label = gnssLabelFromFilename(name);
        const QString profileId = gnssProfileId(label);
        if (!gnssLabels.contains(label))
            gnssLabels.append(label);

        if (name.endsWith("_nmea.log", Qt::CaseInsensitive))
            parseTimestampedNmea(path, profileId);
        else if (name.endsWith("_binary.log", Qt::CaseInsensitive))
            parseTimestampedBinary(path, profileId);
        else
            parseLegacyGnss(path, profileId);
    }
    std::stable_sort(gnssEvents.begin(), gnssEvents.end(), [](const GnssEvent &a, const GnssEvent &b) {
        return a.time < b.time;
    });

    const QDateTime replayStart = cefFiles.first().firstTime;
    QDateTime replayEnd = replayStart;
    for (const CefFile &cef : std::as_const(cefFiles))
        replayEnd = qMax(replayEnd, cef.lastTime);
    gnssEvents.erase(std::remove_if(gnssEvents.begin(), gnssEvents.end(), [&](const GnssEvent &event) {
        return event.time < replayStart || event.time > replayEnd;
    }), gnssEvents.end());
}

void RecordingReplayer::parseTimestampedNmea(const QString &path, const QString &profileId)
{
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly))
        return;
    while (!file.atEnd()) {
        const QByteArray line = file.readLine().trimmed();
        const int comma = line.indexOf(',');
        if (comma <= 0)
            continue;
        const QDateTime time = QDateTime::fromString(QString::fromLatin1(line.left(comma)), Qt::ISODateWithMs);
        if (time.isValid())
            gnssEvents.append({time, profileId, line.mid(comma + 1) + "\r\n"});
    }
}

void RecordingReplayer::parseTimestampedBinary(const QString &path, const QString &profileId)
{
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly))
        return;
    const QByteArray raw = file.readAll();
    const uchar *data = reinterpret_cast<const uchar *>(raw.constData());
    qsizetype pos = 0;
    while (pos + 12 <= raw.size()) {
        const quint64 ms = qFromLittleEndian<quint64>(data + pos);
        const quint32 length = qFromLittleEndian<quint32>(data + pos + 8);
        pos += 12;
        if (length > 65536 || pos + length > raw.size())
            break;
        gnssEvents.append({QDateTime::fromMSecsSinceEpoch(ms, QTimeZone::UTC), profileId, raw.mid(pos, length)});
        pos += length;
    }
}

void RecordingReplayer::parseLegacyGnss(const QString &path, const QString &profileId)
{
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly))
        return;
    const QByteArray raw = file.readAll();
    if (raw.isEmpty())
        return;

    QDate date = cefFiles.first().firstTime.toUTC().date();
    for (const CefFile &cef : std::as_const(cefFiles))
        if (cef.firstTime.toUTC().date() < date)
            date = cef.firstTime.toUTC().date();

    // Segment boundaries wherever the NMEA time of day changes, the text/binary mix between them is replayed as-is
    QList<QPair<qsizetype, QDateTime>> boundaries = {{0, QDateTime()}};
    QDateTime lastTime;
    qsizetype idx = 0;
    while ((idx = raw.indexOf("$G", idx)) != -1) {
        const qsizetype end = raw.indexOf('\n', idx);
        if (end == -1)
            break;
        const QByteArray sentence = raw.mid(idx, end - idx).trimmed();
        const QByteArray type = sentence.mid(3, 3);
        if (sentence.size() < 200 && sentence.contains('*') && (type == "RMC" || type == "GGA" || type == "GNS")) {
            const QList<QByteArray> fields = sentence.split(',');
            if (type == "RMC" && fields.size() > 9 && fields[9].size() == 6) {
                const QDate rmcDate = QDate::fromString("20" + QString::fromLatin1(fields[9].right(2) + fields[9].mid(2, 2) + fields[9].left(2)), "yyyyMMdd");
                if (rmcDate.isValid())
                    date = rmcDate;
            }
            const QTime time = fields.size() > 1 && fields[1].size() >= 6
                                   ? QTime::fromString(QString::fromLatin1(fields[1].left(6)), "hhmmss")
                                   : QTime();
            if (time.isValid()) {
                QDateTime t(date, time, QTimeZone::UTC);
                if (lastTime.isValid() && t < lastTime.addSecs(-43200))
                    t = t.addDays(1);
                if (!lastTime.isValid() || t != lastTime) {
                    if (!boundaries.last().second.isValid())
                        boundaries.last().second = t;
                    else
                        boundaries.append({idx, t});
                    lastTime = t;
                }
            }
        }
        idx = end + 1;
    }

    if (!boundaries.first().second.isValid()) { // no usable NMEA time, spread over the trace recording at 1 Hz
        const QDateTime start = cefFiles.first().firstTime;
        const qint64 secs = qMax<qint64>(1, start.secsTo(cefFiles.last().lastTime));
        const qsizetype step = qMax<qsizetype>(1, raw.size() / secs);
        for (qsizetype pos = 0, i = 0; pos < raw.size(); pos += step, i++)
            gnssEvents.append({start.addSecs(i), profileId, raw.mid(pos, step)});
        return;
    }

    for (int i = 0; i < boundaries.size(); i++) {
        const qsizetype from = boundaries[i].first;
        const qsizetype to = i + 1 < boundaries.size() ? boundaries[i + 1].first : raw.size();
        gnssEvents.append({boundaries[i].second, profileId, raw.mid(from, to - from)});
    }
}

QList<GnssReceiverProfile> RecordingReplayer::gnssReplayProfiles() const
{
    QHash<QString, QPair<bool, bool>> snapshotFlags; // label -> ublox, triggerRecording
    if (!snapshotFile.isEmpty()) {
        QSettings snapshot(snapshotFile, QSettings::IniFormat);
        AsciiTranslator::Options options;
        options.restrictToSafeAscii = true;
        for (const QString &id : snapshot.value("gnss/receiverProfileIds").toStringList()) {
            const QString prefix = "gnss/receivers/" + id + "/";
            QString label = AsciiTranslator::toAscii(snapshot.value(prefix + "name", id).toString().trimmed(), options).simplified().replace(' ', '_');
            if (label.isEmpty())
                label = id;
            snapshotFlags.insert(label, {snapshot.value(prefix + "ublox", true).toBool(),
                                         snapshot.value(prefix + "triggerRecording", false).toBool()});
        }
    }

    QList<GnssReceiverProfile> profiles;
    for (const QString &label : gnssLabels) {
        GnssReceiverProfile profile;
        profile.id = gnssProfileId(label);
        profile.name = label;
        profile.type = GnssReceiverProfile::Type::Serial;
        profile.enabled = true;
        profile.ublox = snapshotFlags.value(label, {true, false}).first;
        profile.triggerRecording = snapshotFlags.value(label, {true, false}).second;
        profiles.append(profile);
    }
    return profiles;
}

QString RecordingReplayer::gnssPositionSource() const
{
    if (gnssLabels.isEmpty())
        return QString();
    if (!snapshotFile.isEmpty()) {
        QSettings snapshot(snapshotFile, QSettings::IniFormat);
        const QString sourceId = snapshot.value("sdef/GpsSource").toString();
        AsciiTranslator::Options options;
        options.restrictToSafeAscii = true;
        const QString label = AsciiTranslator::toAscii(snapshot.value("gnss/receivers/" + sourceId + "/name", sourceId).toString().trimmed(), options).simplified().replace(' ', '_');
        if (gnssLabels.contains(label))
            return gnssProfileId(label);
    }
    return gnssProfileId(gnssLabels.first());
}
