#ifndef WATERFALL_H
#define WATERFALL_H

#include <QObject>
#include <QPainter>
#include <QDebug>
#include <QSharedPointer>
#include <QPixmap>
#include <QDateTime>
#include <QTimer>
#include <QMutex>
#include <math.h>
#include "config.h"
#include <fftw3.h>

#define FFT_SIZE 1024

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
    void receiveIqData(QList<qint16>, QList<qint16>);
    void setFfmFrequency(double d) { ffmFrequency = d;}

signals:
    void imageReady(QPixmap *);

private slots:
    void updTimerCallback();
    void findIqFftMinMax(double &min, double &max);
    void storeIqTrace(QList<double>);
    void createIqPlot();
    void fillWindow();
    void addLines(QPixmap *pixmap);
    void addText(QPixmap *pixmap);
    void saveImage(QPixmap *pixmap);

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
    QList<QList<double> > iqFftResult;
    QVector<double> window;
    double ffmFrequency = 0;
    double samplerate = 0;
    double secsPerLine;
    double secsToAnalyze = 500e-6;
};

#endif // WATERFALL_H
