#ifndef AI_H
#define AI_H

#include "opencv2/dnn.hpp"
#include "opencv2/highgui.hpp"
#include "opencv2/imgproc.hpp"

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
#include <iostream>
#include "config.h"
#include "typedefs.h"


#define WAITBEFOREANALYZING 20

class AI : public QObject
{
Q_OBJECT

signals:
    void aiResult(QString, int);
    void reqTraceBuffer(int seconds);
    void toIncidentLog(const NOTIFY::TYPE, const QString, const QString);

public:
    AI(QSharedPointer<Config> c);
    void receiveBuffer(QVector<QVector<float >> buffer);
    void receiveTraceBuffer(const QList<QVector<qint16> > &data);
    void startAiTimer() { if (recordingEnded && netLoaded && !reqTraceBufferTimer->isActive()) reqTraceBufferTimer->start(45 * 1e3); recordingEnded = false;} //getNotifyTruncateTime() * 0.75e3); recordingEnded = false; }
    void recordingHasEnded() { recordingEnded = true; }
    void freqChanged(double a, double b) { startfreq = a; stopfreq = b;}
    void resChanged(double a) { resolution = a;}
    void setTrigCenterFrequency(double a) { trigCenterFrequency = a * 1e6; qDebug() << "trig center" << trigCenterFrequency;}

private:
    QStringList classes;
    cv::dnn::Net net;
    QTimer *reqTraceBufferTimer = new QTimer;
    QTimer *testTimer = new QTimer;
    bool netLoaded = false;
    bool recordingEnded = true;
    double startfreq, stopfreq, resolution, startrange, stoprange;
    double trigCenterFrequency;
    QSharedPointer<Config> config;


private slots:
    void classifyData(cv::Mat frame);
    void findMinAvgMax(const QVector<QVector<float >> &buffer, float *min, float *avg, float *max);
    void findMinAvgMax(const QVector<QVector<qint16 >> &buffer, qint16 *min, qint16 *avg, qint16 *max);
    void findTrigRange();
    std::vector<float> sigmoid(const std::vector<float>& m1);
};

#endif // AI_H
