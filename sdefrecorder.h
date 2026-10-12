#ifndef SDEFRECORDER_H
#define SDEFRECORDER_H

//#include <QObject>
#include <QFile>
#include <QCoreApplication>
#include <QProcess>
#include <QDir>
#include <QDataStream>
#include <QDebug>
#include <QTextStream>
#include <QDateTime>
#include <QMessageBox>
#include <QTimer>
#include <QDateTime>
#include <QElapsedTimer>
#include <QHash>
#include <QSet>
#include <QThreadPool>
#include <QVector>
#include <functional>
#include "config.h"
#include "typedefs.h"
#include <QNetworkAccessManager>

#define TEMPFILENAME "feed.cef"

/*
 * Class to take care of recording to file in sdef format.
 * Should run in its own thread, as this can be rather I/O unfriendly, at least
 * on a tiny RPi with a slow SD card.
 * Also takes care of eventual uploading to a server
 */

class SdefRecorder : public QObject
{
    Q_OBJECT
public:
    enum class TriggerSource { Unknown, TraceAnalyzer, Gnss, Manual, Mqtt, AutoRecorder };
    Q_ENUM(TriggerSource)

    SdefRecorder(QSharedPointer<Config> c);

public slots:
    void start();
    void end();
    void updSettings();
    void triggerRecording(); // to be called as long as incident is happening!
    void triggerRecordingWithoutAnalysisArtifacts();
    void manualTriggeredRecording();
    void stopRecordingManually(bool suppressIncidentTriggers);
    void clearIncidentTriggerSuppression();
    void receiveTraceBuffer(const QVector<QDateTime> datetime, const QVector<QVector<qint16 >> data); // backlog arrives here, no reference as this should be thread safe
    void receiveTrace(const QDateTime &timestamp, const QVector<qint16> &data); // new data arrives here
    void updTracesPerSecond(double d);
    void setDeviceCconnectedState(bool b);
    void endRecording();
    void updPosition(bool b, double l1, double l2);
    void recPrediction(QString pred, int prob);
    void loginRequest() { askedForLogin = true; curlLogin(finishedFilename); }
    void setModeUsed(QString s) { modeUsed = s;}
    void dataPointsChanged(int d) { datapoints = d;}
    void tempFileData(const QVector<qint16> data);
    void saveDataToTempFile();
    void closeTempFile();
    void setIqRecordingInProgress(bool b);
    void skipNextNTraces(int i) { skipTraces = i;}
    void updFrequencies(quint64 sta, quint64 stop);
    void updResolution(quint32 res);
    void updAntName(QString s) {antName = s.remove(' ');}
    void updScanTime(int i);
    void setIntentional(QString s) {flagUploadIntentionalOnly = true;} // FIXME - shortcut in code!
    void setTriggerSource(SdefRecorder::TriggerSource source) { if (!recording) triggerSource = source; }

private slots:
    QByteArray createHeader(const int saveInterval = 0);
    QString convertDdToddmmss(const double d, const bool lat = true);
    QString createFilename();
    QString incidentBasePath();
    void writeSettingsSnapshot();
    bool curlLogin(QString filename = {}, bool notifyOAuth = false, bool intentional = false);
    void curlUpload();
    void curlCallback(int exitCode, QProcess::ExitStatus exitStatus);
    void updFileWithPrediction(const QString filename); // changes note in sdef file to reflect AI prediction
    void startTempFile();
    QByteArray genSdefTraceLine(double lat = 0,
                                double lng = 0,
                                const QVector<qint16> = QVector<qint16>(),
                                const QDateTime &timestamp = QDateTime());

signals:
    void recordingStarted();
    void recordingStartedWithAnalysisArtifacts();
    void recordingStartedWithoutAnalysisArtifacts();
    void recordingEnded();
    void recordingContextStarted(quint64 recordingId, QDateTime started, QString reason,
                                 bool includeAnalysisArtifacts, bool gnssTriggered, bool measurementTriggered);
    void recordingContextEnded(quint64 recordingId);
    void toIncidentLog(const NOTIFY::TYPE, const QString, const QString);
    void reqTraceHistory(int);
    void warning(QString);
    void reqPositionFrom(QString);
    void recordingEnabled();
    void recordingDisabled();
    void loginSuccessful();
   //void publishFilename(QString);
    void reqAuthentication();
    void fileReadyForUpload(QString);
    void folderDateTimeSet();
    void incidentFilesBasePath(const QString &basePath);

private:
    using ArchiveCallback = std::function<void(bool, const QString &)>;

    void startRecording(bool includeAnalysisArtifacts);
    void archiveFile(const QString &filename, bool incidentArchive, ArchiveCallback callback);
    void notifyFileReadyForUpload(const QString &filename, bool intentional, int delayMs = 10000);
    void startCurlLoginProcess();
    //QSharedPointer<Config> config;
    QFile file;
    double tracePerSecond = -1, tracePerSecondForTempFile = -1;
    QDateTime dateTimeRecordingStarted;
    QTimer *recordingStartedTimer = nullptr;
    QTimer *recordingTimeoutTimer = nullptr;
    QTimer *reqPositionTimer = nullptr;
    QTimer *periodicCheckUploadsTimer = nullptr;
    QTimer *autorecorderTimer = nullptr;
    QTimer *tempFileTimer = nullptr;
    QTimer *tempFileCheckFilesize = nullptr;
    //QDateTime foldernameDateTime = QDateTime::currentDateTime();
    bool historicDataSaved = false;
    bool recording = false;
    bool failed = false;
    bool incidentTriggersSuppressed = false;
    bool preserveAutoRecorderSchedule = false;
    bool saveToSdef;
    int recordTime;
    int maxRecordTime;
    bool addPosition;
    QProcess *process;
    double prevLat = 0, prevLng = 0;
    QString positionSource;
    QList<QPair<double, double>> positionHistory;
    bool deviceConnected = false;
    QString finishedFilename;
    QSet<QString> archivePathsInProgress;
    QHash<QString, QVector<ArchiveCallback>> archiveCallbacksInProgress;
    QThreadPool archiveThreadPool;
    bool stateCurlAwaitingLogin = false;
    bool stateCurlAwaitingFileUpload = false;
    QStringList filesAwaitingUpload;
    QString prediction;
    int probability = 0;
    bool predictionReceived = false;
    bool askedForLogin = false;
    QSharedPointer<Config> config;
    QString modeUsed = "pscan";
    int datapoints = 0;
    QFile tempFile;

    QNetworkAccessManager *networkManager; // New for OAuth2/SSO

    QVector<qint16> tempFileTracedata;
    bool iqRecordingInProgress = false;
    int skipTraces = 0;
    QString antName = "NA";

    bool flagUploadIntentionalOnly = false;
    TriggerSource triggerSource = TriggerSource::Unknown;
    quint64 recordingSequence = 0;
    quint64 activeRecordingId = 0;

    // Config cache
    bool useNewMsFormat;
    double startfreq, stopfreq, resolution;
    int tempFileMaxhold;
    double scanTime;
};

#endif // SDEFRECORDER_H
