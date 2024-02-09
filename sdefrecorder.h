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

/*
 * Class to take care of recording to file in sdef format.
 * Should run in its own thread, as this can be rather I/O unfriendly, at least
 * on a tiny RPi with a slow SD card.
 * Also takes care of eventual uploading to Casper the friendly cat
 */

class SdefRecorder : public Config
{
    Q_OBJECT
public:
    explicit SdefRecorder();

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

private slots:
    QByteArray createHeader();
    QString convertDdToddmmss(const double d, const bool lat = true);
    void restartRecording();
    QString createFilename();
    bool curlLogin();
    void curlUpload();
    void curlCallback(int exitCode, QProcess::ExitStatus exitStatus);
    void zipit();

signals:
    void recordingStarted();
    void recordingEnded();
    void toIncidentLog(const NOTIFY::TYPE, const QString, const QString);
    void reqTraceHistory(int);
    void warning(QString);
    void reqPositionFrom(POSITIONSOURCE);
    void recordingEnabled();
    void recordingDisabled();

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
};

#endif // SDEFRECORDER_H
