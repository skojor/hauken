#ifndef PLOTANDANALYZE_H
#define PLOTANDANALYZE_H

#include <QObject>
#include <QVector>
#include <QImage>
#include "config.h"
#include <QSharedPointer>
#include "typedefs.h"


class PlotAndAnalyze : public QObject
{
    Q_OBJECT
public:
    explicit PlotAndAnalyze(QSharedPointer<Config> c);
    void createIqDiagram();
    void receiveIqData(const QVector<complexInt16> &iq, IqMetadata) {iq16 = iq;};
    void receiveFftData(const QVector<QVector<double>> &fftVector, const IqMetadata &meta);
    void receiveClassification(int classId, double confid, QStringList classes);

signals:
    void imagesReadyForClassification(QVector<QImage>);
    void imageReady(QString);
    void toIncidentLog(const NOTIFY::TYPE, const QString, const QString);
    void analyzerResult(QString, int);
    void reportIntentional(QString);

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

    QSharedPointer<Config> m_config;
    IqMetadata m_metadata;
    QVector<complexInt16> iq16;
};

#endif // PLOTANDANALYZE_H
