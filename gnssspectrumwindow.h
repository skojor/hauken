#ifndef GNSSSPECTRUMWINDOW_H
#define GNSSSPECTRUMWINDOW_H

#include <QComboBox>
#include <QCheckBox>
#include <QDialog>
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

private:
    void plotBlock(const GnssSpectrumBlock &block);
    void saveGeometryToConfig();
    void applyManualYRange();

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
    int selectedBlockIndex = 0;
};

#endif // GNSSSPECTRUMWINDOW_H

