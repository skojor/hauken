#ifndef IQPLOT_H
#define IQPLOT_H

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
#include "fftw3.h"
#include "typedefs.h"
#include <math.h>


class IqPlot : public QObject
{
    Q_OBJECT
public:
    explicit IqPlot(QSharedPointer<Config> c);

public slots:
    void receiveIqData(const QList<complexInt16> &iq16);
    void setFfmFrequency(double d) { ffmFrequency = d;}
    void requestIqData();
    void resetTimer() { lastIqRequestTimer.invalidate();}
    void setFilename(QString) {}
    bool readAndAnalyzeFile(const QString filename);
    void updSettings();
    void setFolderDateTime() { foldernameDateTime = QDateTime::currentDateTime();}

private slots:
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
    const QList<complexInt16> convertComplex8to16bit(const QList<complexInt8> &);
    void findIqMaxValue(const QList<complexInt16> &, qint16 &max);
    void parseFilename(const QString file);

signals:
    void iqPlotReady(QString filename);
    void requestIq(int minSamplesNeeded);
    void workerDone();
    void folderDateTimeSet();

private:
    QSharedPointer<Config> config;

    QVector<double> window;
    double ffmFrequency = 0;
    double samplerate = 0;
    double secsPerLine;
    double secsToAnalyze = 500e-6;
    const int fftSize = 64;
    const int imageYSize = fftSize * 16 * 2;
    QElapsedTimer lastIqRequestTimer;
    QString filename;
    bool dataFromFile = false;
    QDateTime foldernameDateTime = QDateTime::currentDateTime();

};

#endif // IQPLOT_H
