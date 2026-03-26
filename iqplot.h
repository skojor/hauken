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
#include <QtConcurrent/QtConcurrentRun>
#include "config.h"
#include "typedefs.h"
#include <math.h>

#define IQTRANSFERTIMEOUT_MS 15000

class IqPlot : public QObject
{
    Q_OBJECT
public:
    explicit IqPlot(QSharedPointer<Config> c);

public slots:
    void requestIqData(); // Function call to set up I/Q transfer
    void getIqData(const QVector<complexInt16> iq16); // New, gather parts of data before doing sth with them
    void parseIqData(const QVector<complexInt16> &iq16, IqMetadata meta); // Data from vifstream class
    void validateHeader(quint64 freq, quint64 bw, quint64 rate, quint64 timestamp);
    void setFfmFrequency(double d) { m_iqMetadata.centerfreq = d;}
    void resetTimer() { flagOngoingAlarm = false; lastIqRequestTimer->stop();} // In case of manual recording request, this will allow < 120 sec between I/Q transfers
    bool readAndAnalyzeFile(const QString filename);
    void readFolder(const QString &folder);
    void updSettings();
    void setCurrentMode(Instrument::Mode m) { instrMode = m;}
    void setCurrentFfmCenterFrequency(quint64 f) { oldFfmCenterFrequency = f;}
    void setCurrentFfmBandwidth(quint32 f) { oldFfmBandwidth = f;}
    void setTrigFrequency(double f) { trigFrequency = f;} // Set by trace analyzer class
    void getIqCenterFrequency(double f) { centerFrequency = f; }
    void start(); // Called by thread
    void end(); // Called by thread
    void ifStreamStarted(){} // { stopIqStreamTimer->start(500 + config->getIqLogTime() * 8e3);}

private slots:
    void fillWindow();
    void saveIqData(const QVector<complexInt16> &iq);
    QVector<QVector<double>>  doFft(const QVector<complexInt16> &iq, int samplesToAnalyze = 0);
    const QVector<complexInt8> convertComplex16to8bit(const QVector<complexInt16> &);
    const QVector<complexInt16> convertComplex8to16bit(const QVector<complexInt8> &);
    void findIqMaxValue(const QVector<complexInt16> &, qint16 &max);
    void parseFilename(const QString file);
    void receiverControl();

signals:
    void reqVifConnection(); // This signal goes to meas.device, to set up tcp link
    void endVifConnection(); // To remove the TCP connection
    void setFfmCenterFrequency(double);
    void iqPlotReady(QString filename);
    void iqAnimeReady(QString filename);
    void workerDone();
    void folderDateTimeSet();
    void busyRecording(bool);
    void headerValidated(bool); // Approve I/Q header, start gathering data
    void resetTimeoutTimer(); // To hold TCP/UDP conn. up while gathering I/Q
    void reqIqCenterFrequency();
    void rawPlotReady(const QImage &image);
    void imagesForClassification(QVector<QImage>);
    void iqdataReady(const QVector<complexInt16>, IqMetadata);
    void fftdataReady(const QVector<QVector<double>> &, const IqMetadata &);

private:
    QSharedPointer<Config> config;

    QVector<double> window;
    double secsPerLine;
    double secsToAnalyze = 500e-6;
    QTimer *lastIqRequestTimer;
    QTimer *stopIqStreamTimer;
    QString filename;
    bool dataFromFile = false;
    QString filenameFromFile;

    Instrument::Mode instrMode;
    quint64 oldFfmCenterFrequency;
    quint32 oldFfmBandwidth;
    double trigFrequency = 0;
    QVector<double> listFreqs;
    QVector<complexInt16> iqSamples;
    QVector<IqSamplesStruct> iqSamplesVector;
    quint64 samplesNeeded = 0;
    bool flagRequestedEndVifConnection = false;
    bool flagHeaderValidated = false;
    int throwFirstSamples = 0;
    QTimer *timeoutTimer;
    double centerFrequency = 0;
    bool flagOngoingAlarm = false;
    IqMetadata m_iqMetadata;
};

#endif // IQPLOT_H
