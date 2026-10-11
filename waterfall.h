#ifndef WATERFALL_H
#define WATERFALL_H

#include <QApplication>
#include <QDateTime>
#include <QDebug>
#include <QFile>
#include <QFuture>
#include <QFutureWatcher>
#include <QMutex>
#include <QObject>
#include <QPainter>
#include <QImage>
#include <QPromise>
#include <QSharedPointer>
#include <QTimer>
#include <QtConcurrent>
#include "config.h"
//#include "fftw3.h"
//#include "typedefs.h"
#include <math.h>

enum class COLORS
{
    OFF,
    GREY,
    RED,
    BLUE,
    PRIDE
};

class Waterfall : public QObject
{
    Q_OBJECT
public:
    explicit Waterfall(QSharedPointer<Config> c);

public slots:
    void start();
    void receiveTrace(const QVector<double> &trace);
    void requestSnapshot(quint64 recordingId);
    void updSize(QSize size);
    void updSettings();
    void setAutoYRange(double lower, double upper);
    void restartPlot();
    void stopPlot(bool b) {
        if (!b) {
            initial = 2;
            if (updIntervalTimer) updIntervalTimer->stop();
        }
        else {
            if (updIntervalTimer) updIntervalTimer->start(100);
            initial = 2;
        }
    }
    void pausePlot(bool b) {
        if (b) {
            if (updIntervalTimer) updIntervalTimer->stop();
            initial = 2;
        }
        else {
            traceCopy.clear();
            initial = 2;
        }
    }
    void updTimerCallback();

signals:
    void imageReady(const QImage &);
    void snapshotReady(quint64 recordingId, const QImage &image);

private slots:

private:
    QTimer *testDraw;
    QTimer *updIntervalTimer = nullptr;
    QVector<double> traceCopy;
    QSharedPointer<Config> config;
    QImage waterfallImage;

    // config cache
    double scaleMin = 0;
    double scaleMax = 0;
    bool autoRangeReceived = false;
    quint64 startfreq, stopfreq;
    QString resolution;
    QString fftMode;
    int waterfallTime = 120;
    QString mode;
    COLORS colorset;
    bool greyscale = false;
    QMutex mutex;
    int initial = 2;
};

#endif // WATERFALL_H
