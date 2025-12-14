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
#include <QVector>
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
    SdefRecorder(QSharedPointer<Config> c);

public slots:
    void start();
    void updSettings();
    void triggerRecording(); // to be called as long as incident is happening!
    void manualTriggeredRecording();
    void receiveTraceBuffer(const QVector<QDateTime> datetime, const QVector<QVector<qint16 >> data); // backlog arrives here, no reference as this should be thread safe
    void receiveTrace(const QVector<qint16> data); // new data arrives here
    void updTracesPerSecond(double d);
    void setDeviceCconnectedState(bool b);
    void endRecording();
    void updPosition(bool b, double l1, double l2);
    void recPrediction(QString pred, int prob);
    void loginRequest() {  askedForLogin = true; curlLogin(); }
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

private slots:
    QByteArray createHeader(const int saveInterval = 0);
    QString convertDdToddmmss(const double d, const bool lat = true);
    QString createFilename();
    bool curlLogin();
    void curlUpload();
    void curlCallback(int exitCode, QProcess::ExitStatus exitStatus);
    void zipit();
    void updFileWithPrediction(const QString filename); // changes note in sdef file to reflect AI prediction
    void startTempFile();
    QByteArray genSdefTraceLine(double lat = 0, double lng = 0, const QVector<qint16> = QVector<qint16>());

signals:
    void recordingStarted();
    void recordingEnded();
    void toIncidentLog(const NOTIFY::TYPE, const QString, const QString);
    void reqTraceHistory(int);
    void warning(QString);
    void reqPositionFrom(POSITIONSOURCE);
    void recordingEnabled();
    void recordingDisabled();
    void loginSuccessful();
   //void publishFilename(QString);
    void reqAuthentication();
    void fileReadyForUpload(QString);
    void folderDateTimeSet();

private:
    //QSharedPointer<Config> config;
    QFile file;
    double tracePerSecond = -1, tracePerSecondForTempFile = -1;
    QDateTime dateTimeRecordingStarted;
    QTimer *recordingStartedTimer = nullptr;
    QTimer *recordingTimeoutTimer = nullptr;
    QTimer *reqPositionTimer = nullptr;
    QTimer *periodicCheckUploadsTimer = nullptr;
    QTimer *autorecorderTimer = nullptr;
    QTimer *finishedFileTimer = nullptr;
    QTimer *tempFileTimer = nullptr;
    QTimer *tempFileCheckFilesize = nullptr;
    //QDateTime foldernameDateTime = QDateTime::currentDateTime();
    bool historicDataSaved = false;
    bool recording = false;
    bool failed = false;
    bool saveToSdef;
    int recordTime;
    int maxRecordTime;
    bool addPosition;
    QProcess *process;
    double prevLat = 0, prevLng = 0;
    POSITIONSOURCE positionSource;
    QList<QPair<double, double>> positionHistory;
    bool deviceConnected = false;
    QString finishedFilename;
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

    // Config cache
    bool useNewMsFormat;
    double startfreq, stopfreq, resolution;
    int tempFileMaxhold;
    double scanTime;
};

#endif // SDEFRECORDER_H
