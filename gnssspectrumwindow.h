#ifndef GNSSSPECTRUMWINDOW_H
#define GNSSSPECTRUMWINDOW_H

#include <QComboBox>
#include <QCheckBox>
#include <QDialog>
#include <QImage>
#include <QMap>
#include <QSharedPointer>
#include <QSpinBox>
#include "config.h"
#include "gnssspectrumblock.h"
#include "qcustomplot.h"

/*
 * Standalone, resizable window showing one GNSS receiver's spectrum trace, auto-scaled.
 * Receivers reporting several bands/front-ends per sweep (e.g. u-blox X20P MON-SPAN) show
 * a band selector; only the currently selected band's data is plotted.
 */
class GnssSpectrumWindow : public QDialog
{
    Q_OBJECT
public:
    explicit GnssSpectrumWindow(QSharedPointer<Config> config, const QString &profileId,
                                const QString &profileName, QWidget *parent = nullptr);

public slots:
    void receiveSpectrum(const GnssSpectrumBlock &block);

protected:
    void resizeEvent(QResizeEvent *event) override;
    void moveEvent(QMoveEvent *event) override;

private slots:
    void bandSelectionChanged(int index);
    void manualYRangeChanged();
    void autoScaleChanged(bool enabled);
    void maxholdTimeChanged(int seconds);
    void waterfallModeChanged(const QString &mode);
    void waterfallTimeChanged(int seconds);

private:
    void plotBlock(const GnssSpectrumBlock &block);
    void saveGeometryToConfig();
    void applyManualYRange();
    void updateBandHistory(const GnssSpectrumBlock &block);
    void updateWaterfall(int blockIndex, const GnssSpectrumBlock &block, qint64 timestampMs);
    void displaySelectedWaterfall();
    void clearWaterfallHistory();

    struct MaxholdFrame
    {
        qint64 timestampMs = 0;
        QVector<double> levels;
    };

    struct BandDisplayState
    {
        quint64 centerHz = 0;
        quint32 spanHz = 0;
        quint32 resolutionHz = 0;
        int levelCount = 0;
        bool geometryInitialized = false;
        qint64 lastTimestampMs = -1;
        qint64 maxholdBucketStartMs = -1;
        qint64 maxholdBucketLastMs = -1;
        QVector<double> maxholdBucket;
        QVector<MaxholdFrame> maxholdFrames;
        QImage waterfallImage;
        QVector<double> waterfallLastLevels;
        qint64 lastWaterfallRowMs = -1;
    };

    QSharedPointer<Config> config;
    QString profileId;
    bool initializing = true; // true while still being constructed, to avoid saving/emitting on the initial resize
    QCustomPlot *plot = new QCustomPlot;
    QComboBox *bandSelector = new QComboBox;
    QSpinBox *yMax = new QSpinBox;
    QSpinBox *yMin = new QSpinBox;
    QCheckBox *autoScale = new QCheckBox;
    QSpinBox *maxholdTime = new QSpinBox;
    QComboBox *waterfallMode = new QComboBox;
    QSpinBox *waterfallTime = new QSpinBox;
    QCheckBox *overlayEnabled = new QCheckBox;
    QMap<int, GnssSpectrumBlock> lastBlockByIndex; // blockIndex -> most recently received block
    QMap<int, BandDisplayState> bandDisplayState;
    int selectedBlockIndex = 0;
};

#endif // GNSSSPECTRUMWINDOW_H

