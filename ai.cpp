#include "ai.h"

AI::AI(QSharedPointer<Config> c)
{
    config = c;

    QFile checkFile(config->getWorkFolder() + "/model.onnx.disabled");
    if (checkFile.exists()) {
        qDebug() << "Using model found at" << config->getWorkFolder();
        net = cv::dnn::readNet(QString(config->getWorkFolder() +  "/model.onnx").toStdString());
        netLoaded = true;
    }
    if (!netLoaded) {
    checkFile.setFileName(QDir(QCoreApplication::applicationDirPath()).absolutePath() +  "/model.onnx");
    if (checkFile.exists()) {
        qDebug() << "Using model found at" << QDir(QCoreApplication::applicationDirPath()).absolutePath();
        net = cv::dnn::readNet(QString(QDir(QCoreApplication::applicationDirPath()).absolutePath() +  "/model.onnx").toStdString());
        netLoaded = true;
    }
    }
    else if (!netLoaded) {
        qDebug() << "Classification model not found";
    }

    //classes << "cw" << "jammer" << "other" << "wideband";
    classes << "jammer" << "other";

    reqTraceBufferTimer->setSingleShot(true);
    /*connect(reqTraceBufferTimer, &QTimer::timeout, this, [this] {
        int wait = 120;
        //if (wait > 120) wait = 120;
        emit reqTraceBuffer(wait);
    });*/
    /*
    cv::Mat frame;
    frame = cv::imread("c:/hauken/other1.png", cv::ImreadModes::IMREAD_COLOR);
    cv::Mat3b conv;
    frame.convertTo(conv, CV_8UC3);
    classifyData(conv);*/
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
    cv::Mat3f convFrame;

    frame.convertTo(convFrame, CV_32FC3);
    cv::normalize(convFrame, convFrame, 0, 1, cv::NORM_MINMAX);

    cv::Mat3b conv;
    convFrame.convertTo(conv, CV_8UC3, 255);
    cv::imwrite("c:/hauken/test.png", conv);

    cv::Mat blob;
    cv::Scalar mean{0.485, 0.456, 0.406};
    cv::Scalar std{0.229, 0.224, 0.225};
    bool swapRB = false;
    bool crop = false;

    cv::dnn::blobFromImage(convFrame, blob, 1.0, cv::Size(224, 224), mean, swapRB, crop);
    if (std.val[0] != 0.0 && std.val[1] != 0.0 && std.val[2] != 0.0) {
        cv::divide(blob, std, blob);
    }
    net.setInput(blob);

    cv::Mat prob = net.forward();
    std::cerr << prob << std::endl;
    // Apply sigmoid
    //cv::Mat probReshaped = prob.reshape(1, (int)prob.total() * prob.channels());
    //std::vector<float> probVec =
    //    probReshaped.isContinuous() ? probReshaped : probReshaped.clone();
    //std::vector<float> probNormalized = sigmoid(probVec);
    //qDebug() << probNormalized;
    cv::Point classIdPoint;
    double confidence;
    minMaxLoc(prob.reshape(1, 1), 0, &confidence, 0, &classIdPoint);
    int classId = classIdPoint.x;
    qDebug() << " ID " << classId << " - " << classes[classId] << " confidence " << confidence;

    /*cv::Point classIdPoint;
    cv::minMaxLoc(prob.reshape(1, 1), 0, &confidence, 0, &classIdPoint);
    classId = classIdPoint.x;*/
    double minVal;
    double maxVal;
    cv::Point minLoc;
    cv::Point maxLoc;
    minMaxLoc(prob, &minVal, &maxVal, &minLoc, &maxLoc );

    double sum = 0;
    for (int i = 0; i < classes.size(); i++) {
        sum += prob.at<float>(i);
        //qDebug() << i << prob.at<float>(i);
    }
    double probability = maxVal * 100 / sum;

    //qDebug() << "Classification" << classes[classId] << probability;
    if (classId == 0 && probability >= 39) {
        emit toIncidentLog(NOTIFY::TYPE::AI, "", "AI classification: " + classes[classId] + ", probability " + QString::number((int)probability) + " %");
        emit aiResult(classes[classId], probability);
    }
    else {
        emit toIncidentLog(NOTIFY::TYPE::AI, "", "AI classification: other");
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
        //if (max < 30) max = 30;
        min = avg;
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
                float val = 1.1 * (1.0 - ((((float)data[i][j] / 10.0) - min) / (max - min)));
                if (val < 0) val = 0;
                if (val > 1) val = 1;
                frame.at<float>(i, k++) = val;
                frame.at<float>(i, k++) = val;
                frame.at<float>(i, k) = val;
            }
        }

        classifyData(frame);
    }
}

void AI::findTrigRange()
{
    QStringList trigFrequencies = config->getTrigFrequencies();
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

std::vector<float> AI::sigmoid(const std::vector<float>& m1) {
    const unsigned long vectorSize = m1.size();
    std::vector<float> output(vectorSize);
    for (unsigned i = 0; i != vectorSize; ++i) {
        output[i] = 1 / (1 + exp(-m1[i]));
    }
    return output;
}
