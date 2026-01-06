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
#include <QPixmap>
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
    QPixmap * retWaterfall() { return pixmap;}
    void updSize(QRect s);
    void updSettings();
    void restartPlot();
    void stopPlot(bool b) { if (!b) updIntervalTimer->stop(); else updIntervalTimer->start(100);}
    void pausePlot(bool b) {
        if (b) updIntervalTimer->stop();
        else {
            traceCopy.clear();
            updIntervalTimer->start(100);
        }
    }
    void updTimerCallback();

signals:
    void imageReady(QPixmap *);

private slots:

private:
    QTimer *testDraw;
    QTimer *updIntervalTimer;
    QVector<double> traceCopy;
    QSharedPointer<Config> config;
    QPixmap *pixmap;

    // config cache
    int scaleMin, scaleMax;
    quint64 startfreq, stopfreq;
    QString resolution;
    QString fftMode;
    int waterfallTime = 120;
    QString mode;
    COLORS colorset;
    bool greyscale = false;
    QMutex mutex;
    bool timeout = true;
    bool initial = true;
};

#endif // WATERFALL_H
