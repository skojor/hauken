#ifndef CAMERARECORDER_H
#define CAMERARECORDER_H

#include <QObject>
#include <QDebug>
#include <QDateTime>
#include <QTimer>
#include <QFile>
#include <QDataStream>
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
};

#endif // CAMERARECORDER_H
