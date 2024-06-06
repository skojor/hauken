#include "ai.h"

AI::AI()
{
    QFile checkFile(QDir(QCoreApplication::applicationDirPath()).absolutePath() +  "/model.onnx");
    if (checkFile.exists()) {
        net = cv::dnn::readNet(QString(QDir(QCoreApplication::applicationDirPath()).absolutePath() +  "/model.onnx").toStdString());
        netLoaded = true;
    }
    else {
        qDebug() << "Classification model not found at" << QDir(QCoreApplication::applicationDirPath()).absolutePath() +  "/model.onnx";
    }

    classes << "cw" << "jammer" << "other" << "wideband";

    reqTraceBufferTimer->setSingleShot(true);
    connect(reqTraceBufferTimer, &QTimer::timeout, this, [this] {
        int wait = 120;
        //if (wait > 120) wait = 120;
        emit reqTraceBuffer(wait);
    });

    /*testTimer->start(1000);
    connect(testTimer, &QTimer::timeout, this, [this]  {
        emit reqTraceBuffer(0);
    });*/
    /*cv::Mat image = cv::imread("C:/Kode/Custom_Object_Classification/test_data/1052.png");

    cv::Mat blob;

    cv::Mat imgFloat;
    image.convertTo(imgFloat, CV_32F);
    imgFloat /= 255.0f;
    cv::Scalar mean = { 0.485, 0.456, 0.406 };
    cv::Scalar std = { 0.229, 0.224, 0.225 };
    imgFloat -= mean;
    imgFloat /= std;

    blob = cv::dnn::blobFromImage(imgFloat, 1, cv::Size(224, 224));

    net.setInput(blob);
    cv::Mat prob = net.forward();
    double minVal;
    double maxVal;
    cv::Point minLoc;
    cv::Point maxLoc;
    minMaxLoc(prob, &minVal, &maxVal, &minLoc, &maxLoc );

    double sum = 0;
    for (int i = 0; i < classes.size(); i++) sum += prob.at<float>(i);

    double probability = maxVal * 100 / sum;
    for (int i = 0; i < classes.size(); i++) {
        qDebug() << classes[i] << prob.at<float>(i) * 100 / sum;
    }
    qDebug() << classes[maxLoc.x] << probability;
    std::cerr << prob << std::endl;*/

}


void AI::receiveBuffer(QVector<QVector<float >> buffer)
{
    float min = 90, max = -90, avg = 0;
    findMinAvgMax(buffer, &min, &avg, &max);
    if (max < 30) max = 30;
    min = avg / 2;
    cv::Mat frame((int)buffer.size(), (int)buffer.front().size(), CV_8UC3, cv::Scalar());

    for (int i = 0; i < buffer.size(); i++) {
        for (int j = 0, k = 0; j < buffer[0].size(); j++, k++) {
            if (max - min == 0) max += 1;
            int val = (1.5 * 255 * (buffer[i][j] - min)) / (max - min); // / (max - min));
            if (val < 0) val = 0;
            if (val > 255) val = 255;
            frame.at<uchar>(i, k++) = 255 - val;
            frame.at<uchar>(i, k++) = 255 - val;
            frame.at<uchar>(i, k) = 255 - val;
        }
    }
    cv::resize(frame, frame, cv::Size(369, 369), 0, 0, cv::INTER_CUBIC);

    classifyData(frame);
}

void AI::classifyData(cv::Mat frame)
{
    cv::Mat blob;
    cv::dnn::blobFromImage(frame, blob, 1.0, cv::Size(224, 224), cv::Scalar(0), false, false);

    net.setInput(blob);
    int classId;
    double confidence;
    cv::Mat prob = net.forward();
    //std::cerr << prob << std::endl;
    cv::Point classIdPoint;
    cv::minMaxLoc(prob.reshape(1, 1), 0, &confidence, 0, &classIdPoint);
    classId = classIdPoint.x;
    double minVal;
    double maxVal;
    cv::Point minLoc;
    cv::Point maxLoc;
    minMaxLoc(prob, &minVal, &maxVal, &minLoc, &maxLoc );

    double sum = 0;
    for (int i = 0; i < classes.size(); i++) {
        sum += prob.at<float>(i);
        qDebug() << i << prob.at<float>(i);
    }
    double probability = maxVal * 100 / sum;

    qDebug() << "Classification" << classes[classId] << probability;
    if (probability >= 50) {
        emit toIncidentLog(NOTIFY::TYPE::AI, "", "AI classification: " + classes[classId] + ", probability " + QString::number((int)probability) + " %");
        emit aiResult(classes[classId], probability);
    }
    else {
        emit toIncidentLog(NOTIFY::TYPE::AI, "", "AI classification: other/uncertain");
        emit aiResult("other", 0);
    }
}

void AI::findMinAvgMax(const QVector<QVector<float >> &buffer, float *min, float *avg, float *max)
{
    float _min = 99999, _max = -99999, _avg = 0;

    for (int i=0; i<buffer.size(); i++) { // find min and max values, used for normalizing values
        for (int j=0; j<buffer[i].size(); j++) {
            if (_min > buffer[i][j]) _min = buffer[i][j];
            else if (_max < buffer[i][j]) _max = buffer[i][j];
            _avg += buffer[i][j];
        }
    }

    *min = _min / 10;
    *max = _max / 10;
    _avg = _avg / (buffer.size() * buffer[0].size());
    *avg = _avg / 10;
}

void AI::findMinAvgMax(const QVector<QVector<qint16 >> &buffer, qint16 *min, qint16 *avg, qint16 *max)
{
    qint16 _min = 32767, _max = -32767;
    long _avg = 0;

    for (int i=0; i<buffer.size(); i++) { // find min and max values, used for normalizing values
        for (int j=0; j<buffer[i].size(); j++) {
            if (_min > buffer[i][j]) _min = buffer[i][j];
            else if (_max < buffer[i][j]) _max = buffer[i][j];
            _avg += buffer[i][j];
        }
    }
    *min = _min / 10;
    *max = _max / 10;
    _avg = _avg / (buffer.size() * buffer[0].size());
    *avg = (qint16) (_avg / 10);
}

void AI::receiveTraceBuffer(const QList<QVector<qint16> > &data)
{
    if (data.size() > 50) {
        findTrigRange();
        qint16 min, max, avg;
        findMinAvgMax(data, &min, &avg, &max);
        if (max < 30) max = 30;
        min = avg / 2;
        double displayResolution = (stopfreq - startfreq) / data.front().size();
        int jFirst = (startrange - startfreq) / displayResolution;
        int jLast = data.front().size() - ((stopfreq - stoprange) / displayResolution);

        if (jLast - jFirst <= 0) { // sth very wrong happened here
            jFirst = 0;
            jLast = data.front().size();
        }
        cv::Mat3f frame((int)data.size(), jLast - jFirst);

        for (int i = data.size() - 1; i >= 0; i--) {
            for (int j = jFirst, k = 0; j < jLast; j++, k++) {
                if (max - min == 0) max += 1;
                float val = 1 - ((((float)data[i][j] / 10.0) - min) / (max - min)); // * 1.5?
                if (val < 0) val = 0;
                if (val > 1) val = 1;
                frame.at<float>(i, k++) = (val - 0.485) / 0.229;
                frame.at<float>(i, k++) = (val - 0.456) / 0.224;
                frame.at<float>(i, k) = (val - 0.406) / 0.225;
            }
        }
        classifyData(frame);
    }
}

void AI::findTrigRange()
{
    QStringList trigFrequencies = getTrigFrequencies();
    startrange = 0;
    stoprange = 0;

    if (trigFrequencies.size() >= 2) {
        for (int i = 0; i < trigFrequencies.size(); i += 2) {
            if (trigCenterFrequency >= trigFrequencies[i].toDouble() * 1e6 && trigCenterFrequency <= trigFrequencies[i+1].toDouble() * 1e6) {
                startrange = trigFrequencies[i].toDouble() * 1e6;
                stoprange = trigFrequencies[i+1].toDouble() * 1e6;
                qDebug() << "Trig in the range" << startrange << stoprange;
                break;
            }
        }
        if (startrange > 0 && stoprange > 0) { // found a valid range

        }
    }
    if ((int)startrange == 0 || (int)stoprange == 0) {
        // no trig area set? sth wrong here, but proceed as if we know what we are doing
        qDebug() << "AI debug: Trig not found in a valid range, proceeding anyway" << startrange << stoprange << startfreq / 1e6 << stopfreq / 1e6;
        startrange = startfreq;
        stoprange = stopfreq;
    }
}
