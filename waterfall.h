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
    void requestIqData();
    void resetTimer() { lastIqRequestTimer.invalidate();}
    void setFilename(QString s) { filename = s;}

signals:
    void imageReady(QPixmap *);
    void iqPlotReady(QString filename);
    void requestIq(int minSamplesNeeded);

private slots:
    void updTimerCallback();
    void findIqFftMinMax(double &min, double &max);
    void storeIqTrace(QList<double>);
    void createIqPlot();
    void fillWindow();
    void addLines(QPixmap *pixmap);
    void addText(QPixmap *pixmap);
    void saveImage(QPixmap *pixmap);
    int analyzeIqStart(const QList<qint16> cmpI, const QList<qint16> cmpQ);     // Find where sth happens in data, to not analyze only random noise. Return start point

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
    int fftSize = 512;
    int imageYSize = 1024;
    fftw_complex *in = new fftw_complex[fftSize], *out = new fftw_complex[fftSize];
    QElapsedTimer lastIqRequestTimer;
    QString filename;
};

#endif // WATERFALL_H
