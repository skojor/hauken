#ifndef CAMERARECORDER_H
#define CAMERARECORDER_H

#include <QCamera>
#include <QCameraInfo>
#include <QMultimedia>
#include <QObject>
#include <QMediaRecorder>
#include <QNetworkRequest>
#include <QMediaPlayer>
#include <QDebug>
#include <QDateTime>
#include <QTimer>
#include <QUrl>
#include "config.h"
#include "typedefs.h"

#ifdef __linux__
extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavformat/avio.h>
#include <libswscale/swscale.h>
}
#endif

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
