#ifndef CAMERARECORDER_H
#define CAMERARECORDER_H

#include <QObject>
#include <QDebug>
#include <QDateTime>
#include <QTimer>
#include <QFile>
#include <QDataStream>
#include <QElapsedTimer>
#include "config.h"
#include "typedefs.h"
#include <opencv2/opencv.hpp>

class CameraRecorder : public QObject
{
    Q_OBJECT
public:
    CameraRecorder(QSharedPointer<Config> c);

public slots:
    void start();
    void updSettings();
    void startRecorder();
    void stopRecorder();
    void receivedSignalLevel(double d) { signalLevel = d;}
    void updPosition(GnssData);

signals:
    void toIncidentLog(const NOTIFY::TYPE, const QString, const QString);
    void reqPosition();

private slots:
    void selectCamera();
    void trigCapture();
    void endRecording();
    void alarmOn();
    void alarmOff();
    void grabCam();

private:
    QSharedPointer<Config> config;
    QString usbCamName, rtspStream;
    bool doRecord;
    int recordTime;
    cv::VideoWriter video;
    cv::Mat frame;
    double signalLevel;
    GnssData gnss;
    QTimer *recordTimer, *snapTimer, *alarmTimer, *alarmPulse, *bufferTimer, *reqPositionTimer;
    cv::VideoCapture cap;
    int imageCtr;
    bool useAlarm;
    bool camOpened;
    bool flipHor, flipVert;
    QElapsedTimer debugTimer;

};

#endif // CAMERARECORDER_H
