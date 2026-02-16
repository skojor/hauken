#include "ai.h"
#include <iostream>
#include <QImage>
#include <QThread>
#include <QtConcurrent/QtConcurrentRun>
#include <opencv2/core/utils/logger.hpp>
#include <QElapsedTimer>

AI::AI(QSharedPointer<Config> c)
{
    config = c;
}

void AI::start()
{
    reqTraceBufferTimer = new QTimer;
    testTimer = new QTimer;
    cv::utils::logging::setLogLevel(cv::utils::logging::LogLevel::LOG_LEVEL_ERROR);

    QFile checkFile(config->getWorkFolder() + "/model_new.onnx");
    if (checkFile.exists()) {
        qDebug() << "Using model found at" << config->getWorkFolder();
        net = new cv::dnn::Net(cv::dnn::readNetFromONNX(checkFile.fileName().toStdString()));
        netLoaded = true;
    }
    if (!netLoaded) {
        checkFile.setFileName(QDir(QCoreApplication::applicationDirPath()).absolutePath() +  "/model_new.onnx");
        if (checkFile.exists()) {
            qDebug() << "Using model found at" << QDir(QCoreApplication::applicationDirPath()).absolutePath();
            net = new cv::dnn::Net(cv::dnn::readNetFromONNX(checkFile.fileName().toStdString()));
            netLoaded = true;
        }
    }
    else if (!netLoaded) {
        qDebug() << "Classification model not found";
    }
    net->setPreferableBackend(cv::dnn::DNN_BACKEND_OPENCV);
    net->setPreferableTarget(cv::dnn::DNN_TARGET_CPU);

    if (netLoaded) {
        QFile classFile(config->getWorkFolder() + "/classes.txt");
        if (classFile.exists()) {
            classFile.open(QIODevice::ReadOnly);
            QTextStream ts(&classFile);
            while (!ts.atEnd()) {
                QString className;
                ts >> className;
                if (!className.isEmpty()) classes.append(className);
            }
            qDebug() << "Model using the following classes:" << classes;
        }
        else {
            qDebug() << "Couldn't load classes file for AI classification model" << classFile.fileName();
            netLoaded = false;
        }
    }
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
    net->setInput(blob);

    cv::Mat prob = net->forward();
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
    const quint64 vectorSize = m1.size();
    std::vector<float> output(vectorSize);
    for (unsigned i = 0; i != vectorSize; ++i) {
        output[i] = 1 / (1 + exp(-m1[i]));
    }
    return output;
}

void AI::receiveImages(QVector<QImage> images)
{
    if (!netLoaded) {
        qDebug() << "AI image classification called without any valid model data, aborting";
        return;
    }
    std::vector<cv::Mat> imgVector;

    for (auto && image : images) {
        QImage resized = image.scaled(256, 256, Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
        cv::Mat bgra(resized.height(), resized.width(), CV_8UC4,
                     const_cast<uchar*>(resized.constBits()),
                     resized.bytesPerLine());
        cv::cvtColor(bgra, bgra, cv::COLOR_BGRA2BGR); // Convert from 4 to 3 channel img
        bgra.convertTo(bgra, CV_32F, 1.0 / 255.0); // Convert to float vals, 0 - 1 range
        imgVector.push_back(bgra);
    }

    const cv::Scalar mean(0.485, 0.456, 0.406);
    const cv::Scalar std (0.229, 0.224, 0.225);

    cv::Mat blob = cv::dnn::blobFromImages(
        imgVector,
        1.0,                 // no scaling, handled before blobifying
        cv::Size(224, 224),
        mean,                // mean
        false,               // swapRB=false
        false,               // crop
        CV_32F               // Store as 32-bit float
        );

    blob /= std;
    net->setInput(blob);
    cv::Mat outputs = net->forward();

    std::vector<float> result(outputs.cols, 0);
    //QVector<int> resultPerImage(imgVector.size(), 0);
    int imageIterator = 0;

    for (int row = 0; row < outputs.rows; row++) {
        cv::Mat output = outputs.row(row);
        std::vector<float> logits;
        output.reshape(1, 1).copyTo(logits);
        auto probs = softmax(logits);
        for (int i=0; i<probs.size(); i++)
            result[i] += probs[i];
        if (row == 0) { // Weighing first image higher prob than the following
            for (int i=0; i<probs.size(); i++)
                result[i] += 2 * probs[i];
        }
        /*int classId = int(std::distance(probs.begin(), std::max_element(probs.begin(), probs.end())));
        resultPerImage[imageIterator++] = classId;*/
    }
    for (int i=0; i < result.size(); i++)
        result[i] /= outputs.rows + 2;

    /*for (auto && res : resultPerImage)
        qDebug() << "per image classif." << res;*/

    int classId = int(std::distance(result.begin(), std::max_element(result.begin(), result.end())));
    float confid = result[classId] * 100.0;
    emit toIncidentLog(NOTIFY::TYPE::AI, "", "AI classification: " + classes[classId] + ", probability " + QString::number((int)confid) + " %");
    emit aiResult(classes[classId], confid);
    emit aiResultToAnalyzer(classId, confid, classes);

    /*for (int i = 0; i < result.size(); i++)
        qDebug() << "res" << classes[i] << result[i] * 100.0;*/
}

std::vector<float> AI::softmax(const std::vector<float>& logits)
{
    std::vector<float> probs(logits.size());

    float maxLogit = *std::max_element(logits.begin(), logits.end());

    float sum = 0.0f;
    for (float v : logits)
        sum += std::exp(v - maxLogit);

    for (size_t i = 0; i < logits.size(); ++i)
        probs[i] = std::exp(logits[i] - maxLogit) / sum;

    return probs;
}
