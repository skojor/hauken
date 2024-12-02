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
#include "config.h"
#include "typedefs.h"
#include "quazip/JlCompress.h"
#include <QNetworkAccessManager>


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
    void receiveTraceBuffer(const QList<QDateTime> datetime, const QList<QVector<qint16 >> data); // backlog arrives here, no reference as this should be thread safe
    void receiveTrace(const QVector<qint16> data); // new data arrives here
    void updTracesPerSecond(double d) { tracePerSecond = d;}
    void deviceDisconnected(bool b) { if (!b) finishRecording(); deviceConnected = b;}
    void endRecording() { finishRecording();}
    void updPosition(bool b, double l1, double l2);
    void finishRecording();
    void recPrediction(QString pred, int prob);
    void loginRequest() {  askedForLogin = true; curlLogin(); }
    void setModeUsed(QString s) { modeUsed = s;}
    void dataPointsChanged(int d) { datapoints = d;}

private slots:
    QByteArray createHeader();
    QString convertDdToddmmss(const double d, const bool lat = true);
    void restartRecording();
    QString createFilename();
    bool curlLogin();
    void curlUpload();
    void curlCallback(int exitCode, QProcess::ExitStatus exitStatus);
    void zipit();
    void updFileWithPrediction(const QString filename); // changes note in sdef file to reflect AI prediction

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
    void publishFilename(QString);
    void reqAuthentication();
    void fileReadyForUpload(QString);

private:
    //QSharedPointer<Config> config;
    QFile file;
    double tracePerSecond;
    QDateTime dateTimeRecordingStarted;
    QTimer *recordingStartedTimer;
    QTimer *recordingTimeoutTimer;
    QTimer *reqPositionTimer;
    QTimer *periodicCheckUploadsTimer;
    QTimer *autorecorderTimer;
    QTimer *finishedFileTimer;
    bool historicDataSaved = false;
    bool recording = false;
    bool failed = false;
    bool saveToSdef;
    int recordTime;
    int maxRecordTime;
    bool addPosition;
    QProcess *process;
    double prevLat, prevLng;
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

    QNetworkAccessManager *networkManager; // New for OAuth2/SSO


    // Config cache
    bool useNewMsFormat;
    double startfreq, stopfreq, resolution;
};

#endif // SDEFRECORDER_H
