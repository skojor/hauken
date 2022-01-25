#ifndef CAMERARECORDER_H
#define CAMERARECORDER_H

#include <QCamera>
#include <QCameraInfo>
#include <QMultimedia>
#include <QObject>
#include <QMediaRecorder>
#include <QDebug>
#include <QDateTime>
#include <QTimer>
#include <QUrl>
#include "config.h"
#include "typedefs.h"

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

signals:
    void toIncidentLog(const NOTIFY::TYPE, const QString, const QString);

private slots:
    void selectCamera();

private:
    QSharedPointer<Config> config;
    QString usbCamName, rtspStream;
    bool doRecord;
    int recordTime;
    QCamera *camera = nullptr;
    QMediaRecorder *recorder;
};

#endif // CAMERARECORDER_H
