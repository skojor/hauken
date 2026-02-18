#ifndef PLOTANDANALYZE_H
#define PLOTANDANALYZE_H

#include <QObject>
#include <QVector>
#include <QImage>
#include <QMutex>
#include "config.h"
#include <QSharedPointer>
#include "typedefs.h"
#include "qcustomplot.h"

class PlotAndAnalyze : public QObject
{
    Q_OBJECT
public:
    explicit PlotAndAnalyze(QSharedPointer<Config> c);
    void start();
    void createIqDiagram();
    void receiveIqData(const QVector<complexInt16> &iq, IqMetadata) {m_iq16 = iq;};
    void receiveFftData(const QVector<QVector<double>> &fftVector, const IqMetadata &meta);
    void receiveClassification(int classId, double confid, QStringList classes);
    void receiveTracedata(QVector<QVector<qint16>> data, QCustomPlot *plot);
    void recordingState();

signals:
    void imagesReadyForClassification(QVector<QImage>);
    void imageReady(QString, QString);
    void toIncidentLog(const NOTIFY::TYPE, const QString, const QString);
    void analyzerResult(QString, int);
    void reportIntentional(QString);
    void reqTracedata();

private:
    float findMaxMagnitudeAndPosition(const QVector<complexInt16> &iq16, int &pos);
    QVector<QImage> createImages(const QVector<QVector<double>> &fftVector, double lenOfImage, int startAtPos = 0, int nrOfImages = 1, bool grayscale = false);
    void findIqFftMinMaxAvg(const QVector<QVector<double> > &iqFftResult);
    double calculateSpectralStructure(const QImage &image);
    void dumpMetadata() { qDebug() << m_metadata.spectral << m_metadata.avg << m_metadata.bandwidth << m_metadata.centerfreq << m_metadata.max
                                   << m_metadata.min << m_metadata.avg << m_metadata.maxLoc << m_metadata.samplerate; }
    void createGif(QVector<QImage> &images);
    void createMovie(QVector<QImage> &images);
    void createJpgWithInfo(QImage &image, const double secondsAnalyzed);
    void addText(QImage &image, const double secondsAnalyzed);
    void addLines(QImage &image, const double secondsAnalyzed);
    void createFilename();
    void findTracedataMinMaxAvg(const QVector<QVector<qint16>> &data, int &min, int &max, int &avg);

    QSharedPointer<Config> m_config;
    IqMetadata m_metadata;
    QVector<complexInt16> m_iq16;
    bool flagRecordingState = false;
    QTimer *reqTracedataTimer;
    QTimer *sendPlotsTimer;
    QVector<QString> plotsToSend;
    QVector<QString> plotsDescription;
    QMutex mutex;
};

#endif // PLOTANDANALYZE_H
