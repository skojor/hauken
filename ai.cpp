#include "ai.h"
#include <iostream>
#include <QImage>
#include <QThread>
//#include <onnxruntime/onnxruntime_cxx_api.h>


static cv::Mat toUint8(const cv::Mat& src) {
    cv::Mat norm, u8;
    double minv, maxv;
    cv::minMaxLoc(src, &minv, &maxv);
    if (maxv == minv) {
        // unngå deling på null; sett alt til 0
        return cv::Mat(src.size(), CV_8U, cv::Scalar(0));
    }
    src.convertTo(norm, CV_32F, 1.0 / (maxv - minv), -minv / (maxv - minv));
    norm.convertTo(u8, CV_8U, 255.0);
    return u8;
}

static void saveChannels(const cv::Mat& chw, const std::string& base, bool normalize = true) {
    // chw = CxHxW eller HxWxC? Her forventer vi CxHxW
    int C = chw.size[0], H = chw.size[1], W = chw.size[2];
    for (int c = 0; c < C; ++c) {
        cv::Mat ch(H, W, CV_32F, (void*)chw.ptr<float>(c));
        cv::Mat out = normalize ? toUint8(ch) : ch; // normaliser for visning
        if (!normalize) out.convertTo(out, CV_8U); // “rå” klipping
        cv::imwrite(base + "_ch" + std::to_string(c) + ".png", out);
    }
}


AI::AI(QSharedPointer<Config> c)
{
    config = c;

    QFile checkFile(config->getWorkFolder() + "/model_new.onnx");
    if (checkFile.exists()) {
        qDebug() << "Using model found at" << config->getWorkFolder();
        net = cv::dnn::readNetFromONNX(QString(config->getWorkFolder() +  "/model_new.onnx").toStdString());
        netLoaded = true;
    }
    if (!netLoaded) {
        checkFile.setFileName(QDir(QCoreApplication::applicationDirPath()).absolutePath() +  "/model_new.onnx");
        if (checkFile.exists()) {
            qDebug() << "Using model found at" << QDir(QCoreApplication::applicationDirPath()).absolutePath();
            net = cv::dnn::readNetFromONNX(QString(QDir(QCoreApplication::applicationDirPath()).absolutePath() +  "/model_new.onnx").toStdString());
            netLoaded = true;
        }
    }
    else if (!netLoaded) {
        qDebug() << "Classification model not found";
    }
    net.setPreferableBackend(cv::dnn::DNN_BACKEND_OPENCV);
    net.setPreferableTarget(cv::dnn::DNN_TARGET_CPU);

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
    const quint64 vectorSize = m1.size();
    std::vector<float> output(vectorSize);
    for (unsigned i = 0; i != vectorSize; ++i) {
        output[i] = 1 / (1 + exp(-m1[i]));
    }
    return output;
}

void AI::receiveImage(const QImage &image)
{
    Q_ASSERT(image.allGray());

    if (!netLoaded) {
        qDebug() << "AI image classification called without any valid model data, aborting";
        return;
    }
    QImage resized = image.scaled(256, 256, Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
    resized.save("c:/hauken/mod_1_orig.png");

    cv::Mat bgra(resized.height(), resized.width(), CV_8UC4,
                 const_cast<uchar*>(resized.constBits()),
                 resized.bytesPerLine());
    cv::imwrite("c:/hauken/mod_2_konv_til_mat.png", bgra);

    cv::Mat tmp = bgra.clone();
    double mn, mx;
    cv::minMaxLoc(tmp.reshape(1,1), &mn, &mx);
    qDebug() << "max min bgra før std:" << mn << mx;

    cv::cvtColor(bgra, bgra, cv::COLOR_BGRA2BGR); // Convert from 4 to 3 channel img
    cv::imwrite("c:/hauken/mod_3_konv_til_3ch.png", bgra);

    tmp = bgra.clone();
    cv::minMaxLoc(tmp.reshape(1,1), &mn, &mx);
    qDebug() << "max min etter 3-ch konv:" << mn << mx;

    bgra.convertTo(bgra, CV_32F, 1.0 / 255.0); // Convert to float vals, 0 - 1 range
    cv::Mat debugmat;
    bgra.convertTo(debugmat, CV_8UC4, 255);
    cv::imwrite("c:/hauken/mod_4_konv_til_f32.png", debugmat);

    tmp = bgra.clone();
    cv::minMaxLoc(tmp.reshape(1,1), &mn, &mx);
    qDebug() << "max min etter cv32_f konv:" << mn << mx;

    //cv::normalize(bgra, bgra, 0.0, 0.5, cv::NORM_MINMAX); // TEST

    const cv::Scalar mean(0.485, 0.456, 0.406);
    const cv::Scalar std (0.229, 0.224, 0.225);
    cv::Mat blob = cv::dnn::blobFromImage(
        bgra,
        1.0,                 // no scaling, handled before blobifying
        cv::Size(224, 224),
        mean,                // mean
        false,               // swapRB=false
        false,               // crop
        CV_32F               // Store as 32-bit float
        );

    // Normalize with std matrix here (blobifier cannot do this for some reason)
    blob /= std;

    CV_Assert(blob.dims == 4);
    int N = blob.size[0], C = blob.size[1], H = blob.size[2], W = blob.size[3];

    double minVal, maxVal;
    cv::minMaxLoc(blob.reshape(1,1), &minVal, &maxVal);
    qDebug() << "max min etter blobbing og mean/std" << minVal << maxVal;

    net.setInput(blob);
    cv::Mat output = net.forward();

    std::vector<float> logits;
    output.reshape(1, 1).copyTo(logits);
    auto probs = softmax(logits);

    int classId = int(std::distance(probs.begin(), std::max_element(probs.begin(), probs.end())));
    float confid = probs[classId] * 100.0f;

    for (int i=0; i<classes.size(); i++) {
        qDebug() << "probs" << classes[i] << probs[i];
    }
    qDebug() << "Class:" << classes[classId] << "Confidence:" << confid << "%";
    emit toIncidentLog(NOTIFY::TYPE::AI, "", "AI classification: " + classes[classId] + ", probability " + QString::number((int)confid) + " %");
    emit aiResult(classes[classId], confid);

    CV_Assert(blob.dims == 4);
    C = blob.size[1], H = blob.size[2], W = blob.size[3];
    auto toUint8 = [](const cv::Mat& src) {
        double minv, maxv; cv::minMaxLoc(src, &minv, &maxv);
        if (maxv == minv) return cv::Mat(src.size(), CV_8U, cv::Scalar(0));
        cv::Mat norm, u8;
        src.convertTo(norm, CV_32F, 1.0); // / (gmax - gmin), -gmin / (gmax - gmin));
        norm.convertTo(u8, CV_8U, 255.0);
        return u8;
    };

    // DEBUG - visualisering av blobb
    for (int c = 0; c < C; ++c) {
        cv::Mat ch(H, W, CV_32F, (void*)blob.ptr<float>(0, c));
        cv::Mat out = toUint8(ch);
        cv::imwrite("c:/hauken/mod_blob_channel_" + std::to_string(c) + ".png", out);
    }

    /*
    if (C == 3) {
        std::vector<cv::Mat> u8(3);
        for (int c = 0; c < 3; ++c) {
            cv::Mat ch(H, W, CV_32F, (void*)blob.ptr<float>(0, c));
            u8[c] = toUint8(ch);
        }
        cv::Mat pseudo_rgb;
        cv::merge(u8, pseudo_rgb);
        cv::imwrite("c:/hauken/blob_visual_rgb.png", pseudo_rgb);
    }*/
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
