#ifndef GNSSSPECTRUMWINDOW_H
#define GNSSSPECTRUMWINDOW_H

#include <QComboBox>
#include <QCheckBox>
#include <QDialog>
#include <QColor>
#include <QImage>
#include <QMap>
#include <QMouseEvent>
#include <QSharedPointer>
#include <QSpinBox>
#include <QVector>
#include "config.h"
#include "gnssspectrumblock.h"
#include "qcustomplot.h"

/*
 * Standalone, resizable window showing one GNSS receiver's spectrum trace, auto-scaled.
 * Receivers reporting several bands/front-ends per sweep (e.g. u-blox X20P MON-SPAN) show
 * a band selector; only the currently selected band's data is plotted.
 */
struct GnssSpectrumSnapshot
{
    int bandIndex = -1;
    QImage image;
    QString description;
};

class GnssSpectrumWindow : public QDialog
{
    Q_OBJECT
public:
    explicit GnssSpectrumWindow(QSharedPointer<Config> config, const QString &profileId,
                                const QString &profileName, QWidget *parent = nullptr);

public slots:
    void receiveSpectrum(const GnssSpectrumBlock &block);

public:
    bool hasSpectrumData() const { return !currentTraceLevels.isEmpty(); }
    QVector<GnssSpectrumSnapshot> createSpectrumSnapshots();

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
    void showSpectrumToolTip(QMouseEvent *event);
    void spectrumMousePress(QMouseEvent *event);
    void spectrumMouseRelease(QMouseEvent *event);

private:
    void plotBlock(const GnssSpectrumBlock &block);
    void saveGeometryToConfig();
    void applyManualYRange();
    void updateBandHistory(const GnssSpectrumBlock &block);
    void updateWaterfall(int blockIndex, const GnssSpectrumBlock &block, qint64 timestampMs);
    void displaySelectedWaterfall();
    void clearWaterfallHistory();
    void loadBandOverlays();
    void updateBandOverlays();
    void overlayEnabledChanged(bool enabled);
    void addSpectrumMarker(double frequencyMHz);
    void removeSpectrumMarker(int markerIndex);
    void updateSpectrumMarkers();
    int spectrumMarkerAt(const QPoint &position) const;
    double spectrumLevelAt(double frequencyMHz) const;

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

    struct BandPlanOverlay
    {
        double lowerMHz = 0;
        double upperMHz = 0;
        double centerMHz = 0;
        int labelOffset = 0;
        QString name;
        QColor color;
        QCPItemRect *area = nullptr;
        QCPItemText *label = nullptr;
        QCPItemStraightLine *centerLine = nullptr;
    };

    struct SpectrumMarker
    {
        double frequencyMHz = 0;
        QColor color;
        QCPItemStraightLine *line = nullptr;
        QCPItemText *label = nullptr;
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
    QVector<BandPlanOverlay> bandPlanOverlays;
    QVector<double> currentTraceFrequencies;
    QVector<double> currentTraceLevels;
    QVector<SpectrumMarker> spectrumMarkers;
    int draggedSpectrumMarker = -1;
    QPoint markerDragStartPosition;
    QPoint leftMousePressPosition;
    bool markerDragMoved = false;
    int selectedBlockIndex = 0;
};

#endif // GNSSSPECTRUMWINDOW_H

