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
    void receiveTraceBuffer(const QList<QDateTime> datetime, const QList<QVector<qint16 >> data); // backlog arrives here, no reference as this should be thread safe
    void receiveTrace(const QVector<qint16> data); // new data arrives here
    void updTracesPerSecond(double d) { tracePerSecond = d;}
    void deviceDisconnected(bool b) { if (!b) finishRecording();}
    void endRecording() { finishRecording();}

private slots:
    QByteArray createHeader();
    QString convertDdToddmmss(const double d, const bool lat = true);
    void finishRecording();
    void restartRecording();
    QString createFilename();
    bool uploadToCasper();

signals:
    void recordingStarted();
    void recordingEnded();
    void toIncidentLog(QString);
    void reqTraceHistory(int);
    void warning(QString);

private:
    //QSharedPointer<Config> config;
    QFile file;
    double tracePerSecond;
    QDateTime dateTimeRecordingStarted;
    QTimer *recordingStartedTimer;
    QTimer *recordingTimeoutTimer;
    bool historicDataSaved = false;
    bool recording = false;
    bool failed = false;
    bool saveToSdef;
    int recordTime;
    int maxRecordTime;
    QProcess *process;
};

#endif // SDEFRECORDER_H
