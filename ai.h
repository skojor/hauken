#ifndef AI_H
#define AI_H

#include <opencv2/opencv.hpp>
#include <opencv2/dnn.hpp>
#include <opencv2/highgui.hpp>
#include <opencv2/imgproc.hpp>

#include <QString>
#include <QDebug>
#include <QStringList>
#include <QList>
#include <QElapsedTimer>
#include <QTimer>
#include <QObject>
#include <QFile>
#include <QDateTime>
#include <QVector>
#include <QCoreApplication>
#include <QBuffer>
#include "config.h"
#include "typedefs.h"


#define WAITBEFOREANALYZING 20

class AI : public QObject
{
Q_OBJECT

signals:
    void aiResult(QString, int);
    void aiResultToAnalyzer(cv::Mat allResults, QStringList classes, IqMetadata);
    void reqTraceBuffer(int seconds);
    void toIncidentLog(const NOTIFY::TYPE, const QString, const QString);

public:
    AI(QSharedPointer<Config> c);
    ~AI() override;
    void receiveBuffer(QVector<QVector<float >> buffer);
    void receiveImages(QVector<QImage> images, IqMetadata meta);
    void receiveTraceBuffer(const QList<QVector<qint16> > &data);
    void startAiTimer() { if (m_recordingEnded && m_netLoaded && m_reqTraceBufferTimer != nullptr && !m_reqTraceBufferTimer->isActive()) m_reqTraceBufferTimer->start(45 * 1e3); m_recordingEnded = false;} //getNotifyTruncateTime() * 0.75e3); m_recordingEnded = false; }
    void recordingHasEnded() { m_recordingEnded = true; }
    void freqChanged(double a, double b) { m_startfreq = a; m_stopfreq = b;}
    void resChanged(double a) { m_resolution = a;}
    void setTrigCenterFrequency(double a) { m_trigCenterFrequency = a * 1e6; qDebug() << "trig center" << m_trigCenterFrequency;}
    void start(); // Thread calls

private:
    QStringList m_classes;
    cv::dnn::Net *m_net = nullptr;
    QTimer *m_reqTraceBufferTimer = nullptr;
    QTimer *m_testTimer = nullptr;
    bool m_netLoaded = false;
    bool m_recordingEnded = true;
    double m_startfreq, m_stopfreq, m_resolution, m_startrange, m_stoprange;
    double m_trigCenterFrequency;
    QSharedPointer<Config> m_config;


private slots:
    void classifyData(cv::Mat frame);
    void findMinAvgMax(const QVector<QVector<float >> &buffer, float *min, float *avg, float *max);
    void findMinAvgMax(const QVector<QVector<qint16 >> &buffer, qint16 *min, qint16 *avg, qint16 *max);
    void findTrigRange();
    std::vector<float> sigmoid(const std::vector<float>& m1);
    std::vector<float> softmax(const std::vector<float>& logits);
};

#endif // AI_H
