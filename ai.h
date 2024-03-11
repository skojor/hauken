#ifndef AI_H
#define AI_H

#include <opencv2/dnn.hpp>
#include <opencv2/imgproc.hpp>
#include <opencv2/highgui.hpp>

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

class AI : public Config
{
Q_OBJECT

signals:
    void aiResult(QString, int);
    void reqTraceBuffer(int seconds);
    void toIncidentLog(const NOTIFY::TYPE, const QString, const QString);

public:
    AI();
    void receiveBuffer(QVector<QVector<float >> buffer);
    void receiveTraceBuffer(const QList<QVector<qint16> > data);
    void startAiTimer() { if (recordingEnded && netLoaded && !reqTraceBufferTimer->isActive()) reqTraceBufferTimer->start(getNotifyTruncateTime() * 1.5e3); recordingEnded = false; }
    void recordingHasEnded() { recordingEnded = true; }

private:
    QStringList classes;
    cv::dnn::Net net;
    QTimer *reqTraceBufferTimer = new QTimer;
    QTimer *testTimer = new QTimer;
    bool netLoaded = false;
    bool recordingEnded = true;

private slots:
    void classifyData(cv::Mat frame);
    void findMinAvgMax(const QVector<QVector<float >> &buffer, float *min, float *avg, float *max);
    void findMinAvgMax(const QVector<QVector<qint16 >> &buffer, qint16 *min, qint16 *avg, qint16 *max);
};

#endif // AI_H
