#ifndef PLOTANDANALYZE_H
#define PLOTANDANALYZE_H

#include <QObject>
#include <QVector>
#include <QImage>
#include <QMutex>
#include <QTimer>
#include "config.h"
#include <QSharedPointer>
#include "typedefs.h"
#include "qcustomplot.h"
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include <QFile>
#include "opencv2/core.hpp"

class PlotAndAnalyze : public QObject
{
    Q_OBJECT
public:
    explicit PlotAndAnalyze(QSharedPointer<Config> c);
    void start(); // Called by thread
    void end(); // Called by thread
    void createIqDiagram();
    void receiveIqData(const QVector<complexInt16> &iq, IqMetadata meta) {m_iq16 = iq; m_iqMetadata = meta;};
    void receiveFftData(const QVector<QVector<double>> &fftVector, const IqMetadata &meta);
    void receiveClassification(cv::Mat allResults, QStringList classes, IqMetadata meta);
    void receiveTracedata(quint64 recordingId, TraceDataStruct traceData, QCustomPlot *plot = nullptr, QVector<qint16> avglevel = QVector<qint16>());
    void receiveTracedata(TraceDataStruct traceData, QCustomPlot *plot = nullptr, QVector<qint16> avglevel = QVector<qint16>());
    void receivePlot(quint64 recordingId, QPixmap *pixmap, QDateTime timestamp);
    void receiveWaterfallSnapshot(quint64 recordingId, const QImage &image);
    void receiveGnssSpectrumSnapshotsReady(quint64 recordingId);
    void recordingContextStarted(quint64 recordingId, const QDateTime &started, const QString &reason, bool includeAnalysisArtifacts);
    void recordingContextEnded(quint64 recordingId);
    void updFrequencies(quint64 a, quint64 b);
    void updResolution(int a) { m_resolution = a;}
    void readFile(QString filename);
    void findFreqsAboveAvgLevel(const QVector<double> maxholdData,
                                const QVector<double> avgData,
                                double startfreq,
                                double stopfreq);

public slots:
    void traceIncidentStarted();
    void traceIncidentEnded();

signals:
    void imagesReadyForClassification(QVector<QImage>, IqMetadata);
    void imageReady(QString, QString);
    void iqPlotImageReady(QString, quint64);
    void toIncidentLog(const NOTIFY::TYPE, const QString, const QString);
    void analyzerResult(QString, int);
    void reportIntentional(QString);
    void reqTracedata(quint64 recordingId);
    void reqTracePlot(quint64 recordingId);
    void reqWaterfallSnapshot(quint64 recordingId);
    void reqGnssBacklogs(quint64 recordingId);
    void reqGnssSpectrumSnapshots(quint64 recordingId);
    void reqMaxholdData();
    void recordingArtifactReady(quint64 recordingId, const QString &filename, const QString &description);
    void recordingImageReady(quint64 recordingId, const QImage &image, const QString &description);
    void recordingPlotsReady(quint64 recordingId);

private:
    float findMaxMagnitudeAndPosition(const QVector<complexInt16> &iq16, int &pos);
    QVector<QImage> createImages(const QVector<QVector<double>> &fftVector,
                                 double lenOfImage,
                                 int startAtPos = 0,
                                 int nrOfImages = 1,
                                 bool grayscale = false,
                                 bool increaseRange = false);
    void findIqFftMinMaxAvg(const QVector<QVector<double> > &iqFftResult, int from = 0, int to = 0, bool findMaxLoc = false);
    double calculateSpectralStructure(const QImage &image);
    void dumpMetadata() { qDebug() << m_metadata.spectral << m_metadata.avg << m_metadata.bandwidth << m_metadata.centerfreq << m_metadata.max
                                   << m_metadata.min << m_metadata.avg << m_metadata.maxLoc << m_metadata.samplerate; }
    void createGif(QVector<QImage> &images);
    void createMovie(QVector<QImage> &images);
    void createJpgWithInfo(QImage &image, const double secondsAnalyzed);
    void addText(QImage &image, const double secondsAnalyzed);
    void addLines(QImage &image, const double secondsAnalyzed);
    void createFilename();
    void processTraceData(TraceDataStruct traceData, QCustomPlot *plot, QVector<qint16> avglevel);
    void requestRecordingArtifacts(quint64 recordingId);
    void completeRecordingArtifactsIfReady();
    void findTracedataMinMaxAvg(const QVector<QVector<qint16>> &data, int &min, int &max, int &avg);
    void calcPeriodAndDensity(const QVector<QVector<double>> &data, int from, int to);
    double estimateSweepPeriodFromIq(int from, int to) const;
    void writeMetaToDisk(cv::Mat results, QStringList classes);
    bool shouldIncludePeriodEstimate() const;
    bool attachClassificationToPendingPlot(const QString &text);
    bool classificationHasRfiForRange(double startMHz, double stopMHz) const;
    bool withinL1(quint64 freq) { if (abs(freq - GPSL1) < 0.5e6) return true; else return false;}
    bool withinL2(quint64 freq) { if (abs(freq - GPSL2) < 5e6) return true; else return false;}
    bool withinAirRadar(quint64 freq) { if (abs(freq - AIRRADAR) < 2e6) return true; else return false;}

    QSharedPointer<Config> m_config;
    IqMetadata m_metadata;
    IqMetadata m_iqMetadata;
    QVector<complexInt16> m_iq16;
    bool m_flagRecordingState = false;
    QTimer *m_reqTraceplotTimer;
    QTimer *m_incidentEndTimer = nullptr;
    QVector<QString> m_plotsToSend;
    QVector<QString> m_plotsDescription;
    QMutex m_mutex;
    quint64 m_startfreq, m_stopfreq;
    int m_resolution;
    bool m_lastClassificationHasRfi = false;
    double m_lastClassificationStartMHz = 0;
    double m_lastClassificationStopMHz = 0;
    bool m_freqNotificationSentForIncident = false;
    bool m_collectNotificationArtifacts = false;
    quint64 m_recordingId = 0;
    bool m_captureRequested = false;
    bool m_tracePlotReceived = false;
    bool m_waterfallReceived = false;
    bool m_traceDataReceived = false;
    bool m_gnssSpectrumSnapshotsReceived = false;
};

#endif // PLOTANDANALYZE_H
