#include "camerarecorder.h"

CameraRecorder::CameraRecorder(QSharedPointer<Config> c)
{
    config = c;
}

void CameraRecorder::start()
{
    //qDebug() << "Cam recorder thread started";
    //updSettings();
    recordTimer = new QTimer();
    snapTimer = new QTimer();
    alarmTimer = new QTimer();
    alarmPulse = new QTimer();
    bufferTimer = new QTimer();
    reqPositionTimer = new QTimer();

    camOpened = false;

    connect(recordTimer, &QTimer::timeout, this, &CameraRecorder::endRecording);
    connect(snapTimer, &QTimer::timeout, this, &CameraRecorder::trigCapture);
    connect(alarmTimer, &QTimer::timeout, this, &CameraRecorder::alarmOff);
    connect(alarmPulse, &QTimer::timeout, this, &CameraRecorder::alarmOn);
    connect(bufferTimer, &QTimer::timeout, this, &CameraRecorder::grabCam);  // stupid, but needed to empty the openCV frame buffer...
    connect(reqPositionTimer, &QTimer::timeout, this, [this] {
        emit reqPosition();
    });
    reqPositionTimer->start(1000);

    signalLevel = 0.0;

    //selectCamera();
    //bufferTimer->start(20);

    //QTimer::singleShot(15000, this, &CameraRecorder::startRecorder);
}

void CameraRecorder::selectCamera()
{
    if (config->getCameraDeviceTrigger() && !config->getCameraStreamAddress().isEmpty()) {
        if (config->getCameraStreamAddress().contains("first", Qt::CaseInsensitive))
            cap.open(0, cv::CAP_DSHOW);
        else if (config->getCameraStreamAddress().contains("second", Qt::CaseInsensitive))
            cap.open(1, cv::CAP_DSHOW);
        else
            cap.open(config->getCameraStreamAddress().toStdString());

        cap.set(cv::CAP_PROP_BUFFERSIZE, 3);
        if (cap.isOpened() == false)
            qDebug() << "Cannot open camera" << config->getCameraStreamAddress();
        else {
            camOpened = true;
        }
    }
    cap.set(cv::CAP_PROP_FRAME_WIDTH, 1280); //FIXME
    cap.set(cv::CAP_PROP_FRAME_HEIGHT, 720);

    qDebug() << "Camera resolution: " << cap.get(cv::CAP_PROP_FRAME_WIDTH) << " x " << cap.get(cv::CAP_PROP_FRAME_HEIGHT);
}

void CameraRecorder::startRecorder()
{
    if (!camOpened && !recordTimer->isActive()) {
        selectCamera();
        bufferTimer->start(20);
        recordTimer->start((4 + config->getCameraRecordTime()) * 1000); // 3-4 sec. init. time of camera

        imageCtr = 1;
        if (config->getCameraRecordTime() > 0) {
            snapTimer->start(1000.0 / 25.0);
            QString filename;
            QTextStream ts2(&filename);
            ts2 << config->getWorkFolder() << "/" << QDateTime::currentDateTime().toString("yyyyMMdd_hhmmss") << ".avi";

            qDebug() << "Camera recording triggered, saving to" << filename << ", resolution"
                     << cap.get(cv::CAP_PROP_FRAME_WIDTH) << "x" << cap.get(cv::CAP_PROP_FRAME_HEIGHT);

            video.open(filename.toStdString(), cv::VideoWriter::fourcc('M','J','P','G'), 25, cv::Size(cap.get(cv::CAP_PROP_FRAME_WIDTH),cap.get(cv::CAP_PROP_FRAME_HEIGHT)));
        }
    }
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

void CameraRecorder::endRecording()
{
    snapTimer->stop();
    recordTimer->stop();
    bufferTimer->stop();
    video.release();
    cap.release();
    camOpened = false;
}

void CameraRecorder::trigCapture()
{
    if (!frame.empty()) {
        QString txt;
        QTextStream ts(&txt);
        ts << QDateTime::currentDateTime().toString("dd.MM.yyyy hh:mm:ss") << ":  "
           << gnss.latitude << " " << gnss.longitude << ", tracked sats: "
           << gnss.satsTracked << ", max. signal level: " << QString::number(signalLevel, 'f', 1);
        cv::rectangle(frame, cv::Point(90, frame.rows - 5), cv::Point(frame.cols - 90, frame.rows - 35), CV_RGB(20, 20, 20), -1);
        cv::putText(frame, txt.toStdString(), cv::Point(100, frame.rows - 10), cv::QT_FONT_NORMAL, 0.6, CV_RGB(255, 0, 0));
        video.write(frame);
    }
}

void CameraRecorder::updPosition(GnssData g)
{
    gnss = g;
}

void CameraRecorder::alarmOn()
{
}

void CameraRecorder::alarmOff()
{
}

void CameraRecorder::grabCam()
{
    cap >> frame;
}
