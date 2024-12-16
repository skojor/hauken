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
#include <QFile>
#include <math.h>
#include <QApplication>
#include <QtConcurrent/QtConcurrent>
#include <QFuture>
#include <QFutureWatcher>
#include <QPromise>
#include "config.h"
#include <fftw3.h>
#include "typedefs.h"

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
    void receiveIqData(const QList<complexInt16> &iq16);
    void setFfmFrequency(double d) { ffmFrequency = d;}
    void requestIqData();
    void resetTimer() { lastIqRequestTimer.invalidate();}
    void setFilename(QString) {}
    bool readAndAnalyzeFile(const QString filename);

signals:
    void imageReady(QPixmap *);
    void iqPlotReady(QString filename);
    void requestIq(int minSamplesNeeded);

private slots:
    void updTimerCallback();
    void findIqFftMinMaxAvg(const QList<QList<double> > &iqFftResult, double &min, double &max, double &avg);
    void createIqPlot(const QList<QList<double> > &, const double secondsAnalyzed, const double secondsPerLine);
    void fillWindow();
    void addLines(QImage *image, const double secondsAnalyzed, const double secondsPerLine);
    void addText(QImage *image, const double secondsAnalyzed, const double secondsPerLine);
    void saveImage(const QImage *image, const double secondsAnalyzed);
    int analyzeIqStart(const QList<complexInt16> &iq);     // Find where sth happens in data, to not analyze only random noise. Return start point
    void saveIqData(const QList<complexInt16> &iq);
    void receiveIqDataWorker(const QList<complexInt16> iq, const double secondsToAnalyze = 500e-6);
    const QList<complexInt8> convertComplex16to8bit(const QList<complexInt16> &);
    void findIqMaxValue(const QList<complexInt16> &, qint16 &max);
    void parseFilename(const QString file);

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
    //QList<QList<double> > iqFftResult;
    QVector<double> window;
    double ffmFrequency = 0;
    double samplerate = 0;
    double secsPerLine;
    double secsToAnalyze = 500e-6;
    const int fftSize = 64;
    const int imageYSize = fftSize * 16 * 2;
    //fftw_complex *in = new fftw_complex[fftSize], *out = new fftw_complex[fftSize];
    QElapsedTimer lastIqRequestTimer;
    QString filename;
    bool dataFromFile = false;
};

#endif // WATERFALL_H
