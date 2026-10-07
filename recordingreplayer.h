#ifndef RECORDINGREPLAYER_H
#define RECORDINGREPLAYER_H

#include <QDate>
#include <QDateTime>
#include <QFile>
#include <QList>
#include <QObject>
#include <QSharedPointer>
#include <QStringList>
#include <QTemporaryDir>
#include <QTimer>
#include <QVector>
#include <memory>
#include "config.h"
#include "gnssreceiverprofile.h"
#include "typedefs.h"

// Real-time playback of a recorded incident folder/zip through the live signal paths, with AppClock shifted to the original time
class RecordingReplayer : public QObject
{
    Q_OBJECT
public:
    explicit RecordingReplayer(QSharedPointer<Config> c, QObject *parent = nullptr);
    ~RecordingReplayer();

    bool open(const QString &path, QString &error);
    void close();
    bool isOpen() const { return !cefFiles.isEmpty(); }
    bool isRunning() const { return running; }
    bool isFeeding() const { return feeding; }

    QString recordingName() const { return recordingTitle; }
    QString stationName() const { return station; }
    QString snapshotPath() const { return snapshotFile; }
    QString avgDataPath() const { return avgDataFile; }
    QString originalTriggerSource() const { return triggerSource; }
    QStringList iqFiles() const { return iqFileList; }
    QStringList gnssFiles() const { return gnssFileList; }
    double startFrequencyHz() const { return cefFiles.isEmpty() ? 0 : cefFiles.first().startHz; }
    double stopFrequencyHz() const { return cefFiles.isEmpty() ? 0 : cefFiles.first().stopHz; }
    int dataPoints() const { return cefFiles.isEmpty() ? 0 : cefFiles.first().dataPoints; }

    QVector<double> iqFrequenciesMHz() const;
    QVector<qint64> iqSampleCounts() const;
    quint32 iqBandwidthHz() const { return iqRecordings.isEmpty() ? 0 : iqRecordings.first().bandwidthHz; }
    QList<GnssReceiverProfile> gnssReplayProfiles() const;
    QString gnssPositionSource() const;

public slots:
    void configureStream();
    void play(bool baselineLoaded);
    void stop();
    void requestIqFrequency(double frequencyMHz);
    void endIqStream();

signals:
    void frequencyChanged(double startHz, double stopHz);
    void resolutionChanged(double resolutionHz);
    void tracesPerSecondChanged(double tracesPerSecond);
    void traceReady(const QVector<qint16> &data);
    void playbackRunning(bool running);
    void triggerAlarm(const QString &source);
    void triggerAlarmEnded();
    void progress(int percent);
    void finished();
    void toIncidentLog(const NOTIFY::TYPE, const QString, const QString);
    void iqHeader(quint64 frequencyHz, quint64 bandwidthHz, quint64 sampleRate, quint64 timestamp);
    void iqData(const QVector<complexInt16> &samples);
    void gnssData(const QString &profileId, const QByteArray &data);

private:
    struct IqRecording
    {
        QString path;
        double frequencyMHz = 0;
        quint64 sampleRate = 0;
        quint32 bandwidthHz = 0;
        bool is16bit = false;
        qint64 samples = 0;
    };

    struct GnssEvent
    {
        QDateTime time;
        QString profileId;
        QByteArray data;
    };
    struct CefFile
    {
        QString path;
        double startHz = 0, stopHz = 0, resolutionHz = 0;
        int dataPoints = 0;
        double saveInterval = 0;
        bool mobile = false;
        QDate date;
        QString location;
        QDateTime firstTime, lastTime;
        qint64 dataOffset = 0;
        int lines = 0;
    };

    void collectFiles(const QString &folder);
    bool parseCef(CefFile &cef) const;
    QDateTime parseTimestamp(const QByteArray &field, const QDate &date) const;
    bool openCef(int index);
    bool readNextTrace();
    double tracesPerSecondFor(const CefFile &cef) const;
    QDateTime incidentTimeFromName(const QString &name) const;
    QDateTime nextEventTime() const;
    void scheduleNext();
    void processDue();
    void finish();
    void parseIqFiles();
    void sendIqChunk();
    void parseGnssFiles();
    static QString gnssLabelFromFilename(const QString &name);
    static QString gnssProfileId(const QString &label) { return "replay-" + label; }
    void parseTimestampedNmea(const QString &path, const QString &profileId);
    void parseTimestampedBinary(const QString &path, const QString &profileId);
    void parseLegacyGnss(const QString &path, const QString &profileId);

    QSharedPointer<Config> config;
    std::unique_ptr<QTemporaryDir> tempDir;
    QList<CefFile> cefFiles;
    int cefIndex = -1;
    QFile cefStream;
    QDateTime nextTraceTime;
    QVector<qint16> nextTrace;

    QString recordingTitle, station, snapshotFile, avgDataFile, triggerSource;
    QStringList iqFileList, gnssFileList;
    QDateTime incidentTime;
    QList<QDateTime> syntheticTriggers;
    QDateTime syntheticAlarmEnd;

    QTimer tickTimer;
    bool running = false;
    bool feeding = false;
    int totalLines = 0, playedLines = 0, lastProgress = -1;

    QList<IqRecording> iqRecordings;
    QVector<complexInt16> iqPending;
    qsizetype iqPendingPos = 0;
    QTimer iqTimer;

    QList<GnssEvent> gnssEvents;
    int gnssIndex = 0;
    QStringList gnssLabels;
};

#endif // RECORDINGREPLAYER_H
