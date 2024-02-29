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

    classes << "cw" << "jammer" << "wideband";

    reqTraceBufferTimer->setSingleShot(true);
    connect(reqTraceBufferTimer, &QTimer::timeout, this, [this] {
        int wait = WAITBEFOREANALYZING * 2;
        if (wait > 120) wait = 120;
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
    cv::imwrite(QString("c:/Hauken/test.png").toStdString(), frame);

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
    emit aiResult(classes[classId], probability);
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

void AI::receiveTraceBuffer(const QList<QVector<qint16> > data)
{
    if (data.size() > 50) {
        qint16 min, max, avg;
        findMinAvgMax(data, &min, &avg, &max);
        if (max < 30) max = 30;
        min = avg / 2;

        cv::Mat3f frame((int)data.size(), (int)data.front().size());

        for (int i = data.size() - 1; i >= 0; i--) {
            for (int j = 0, k = 0; j < data[0].size(); j++, k++) {
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

        cv::Mat frameToSave;
        frame.convertTo(frameToSave, CV_8UC3, 255);
        cv::imwrite("C:/hauken/test.png", frameToSave);
    }
}
