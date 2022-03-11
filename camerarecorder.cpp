#include "camerarecorder.h"

CameraRecorder::CameraRecorder(QSharedPointer<Config> c)
{
    config = c;
}

void CameraRecorder::start()
{
    qDebug() << "Cam recorder thread started";
    updSettings();
    selectCamera();
}

void CameraRecorder::selectCamera()
{
    const QUrl url = QUrl("http://195.196.36.242/mjpg/video.mjpg");
    const QNetworkRequest requestRtsp(url);
    QMediaPlayer *player = new QMediaPlayer;
    player->setMedia(requestRtsp);
    player->play();

    recorder = new QMediaRecorder(player);
    /*QVideoEncoderSettings settings = recorder->videoSettings();
    settings.setResolution(1280,720);
    settings.setQuality(QMultimedia::VeryHighQuality);
    settings.setFrameRate(30.0);*/

    //recorder->setVideoSettings(settings);
    recorder->setOutputLocation(QUrl::fromLocalFile("test.mpg")); //config->getLogFolder() + "/" + QDateTime::currentDateTime().toString("yyyyMMdd_hhmmss") + ".mpg"));

    qDebug() << recorder->outputLocation() << player->availability() << recorder->isAvailable();

/*    qDebug() << usbCamName << rtspStream;
    if (!usbCamName.contains("None", Qt::CaseInsensitive) && !rtspStream.isEmpty())
        //emit toIncidentLog(NOTIFY::TYPE::CAMERARECORDER, "", "Both USB and network camera selected as source, which is it?");
        qDebug() << "Both USB and network camera selected as source, which is it?";
    else if (!usbCamName.contains("None")) {
        QList<QCameraInfo> cameraList = QCameraInfo::availableCameras();
        for (auto &val : cameraList) {
            if (val.description().contains(usbCamName)) {
                qDebug() << "Found cam source:" << usbCamName << val.deviceName();
                camera = new QCamera(val);
                camera->setCaptureMode(QCamera::CaptureVideo);
                camera->start();
                break;
            }
        }
        if (camera != nullptr) {
            qDebug() << " CMAM" << camera->isCaptureModeSupported(QCamera::CaptureVideo) << camera->errorString() << camera->supportedViewfinderResolutions();
            recorder = new QMediaRecorder(camera);
            QVideoEncoderSettings settings = recorder->videoSettings();
            settings.setResolution(1280,720);
            settings.setQuality(QMultimedia::VeryHighQuality);
            settings.setFrameRate(30.0);

            recorder->setVideoSettings(settings);
            recorder->setOutputLocation(QUrl::fromLocalFile(config->getLogFolder() + "/" + QDateTime::currentDateTime().toString("yyyyMMdd_hhmmss") + ".mpg"));
            qDebug() << QUrl::fromLocalFile(config->getLogFolder() + "/" + QDateTime::currentDateTime().toString("yyyyMMdd_hhmmss") + ".mpg") << camera->isAvailable() << recorder->isAvailable()
                     << recorder->status() << recorder->state() << recorder->supportedVideoCodecs() << recorder->supportedResolutions() << recorder->supportedContainers();
            QTimer::singleShot(5000, this, &CameraRecorder::startRecorder);
            QTimer::singleShot(15000, this, &CameraRecorder::stopRecorder);
        }
    }*/
}

void CameraRecorder::startRecorder()
{
    recorder->record();
    qDebug() << "Starting" << recorder->outputLocation();
    qDebug() << QUrl::fromLocalFile(config->getLogFolder() + "/" + QDateTime::currentDateTime().toString("yyyyMMdd_hhmmss") + ".mpg") << camera->isAvailable() << recorder->isAvailable()
             << recorder->status() << recorder->state();
}

void CameraRecorder::stopRecorder()
{
    recorder->stop();
    qDebug() << "juzed" << recorder->actualLocation();
}

void CameraRecorder::updSettings()
{
    usbCamName = config->getCameraName();
    rtspStream = config->getCameraStreamAddress();
    doRecord = config->getCameraDeviceTrigger();
    recordTime = config->getCameraRecordTime();
}
