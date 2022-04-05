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
#ifdef __linux__
    AVFormatContext* format_ctx = avformat_alloc_context();
    AVCodecContext* codec_ctx = NULL;
    int video_stream_index;
    if (avformat_open_input(&format_ctx, "http://195.196.36.242/mjpg/video.mjpg",
                            NULL, NULL) != 0) {
        qDebug() << "Cannot open RTSP stream";
    }
    else
        qDebug() << "RTSP stream opened with great success";
#endif
}

void CameraRecorder::startRecorder()
{
}

void CameraRecorder::stopRecorder()
{
}

void CameraRecorder::updSettings()
{
    usbCamName = config->getCameraName();
    rtspStream = config->getCameraStreamAddress();
    doRecord = config->getCameraDeviceTrigger();
    recordTime = config->getCameraRecordTime();
}
