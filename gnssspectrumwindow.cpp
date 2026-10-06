#include "gnssspectrumwindow.h"

#include <QHBoxLayout>
#include <QLabel>
#include <QGridLayout>
#include <QVBoxLayout>

GnssSpectrumWindow::GnssSpectrumWindow(QSharedPointer<Config> c, const QString &id, const QString &profileName, QWidget *parent)
    : QDialog(parent), config(c), profileId(id)
{
    setWindowTitle(profileName + " - GNSS spectrum");
    setAttribute(Qt::WA_QuitOnClose, false); // closing this window alone must not quit the app
    setWindowFlags(windowFlags() | Qt::WindowMinMaxButtonsHint);
    setSizeGripEnabled(true);

    auto *topLayout = new QHBoxLayout;
    topLayout->addWidget(new QLabel(tr("Frequency range")));
    topLayout->addWidget(bandSelector, 1);
    autoScale->setText(tr("Auto Y scale"));
    topLayout->addWidget(autoScale);

    auto *mainLayout = new QVBoxLayout(this);
    mainLayout->addLayout(topLayout);

    auto *plotLayout = new QGridLayout;
    plotLayout->addWidget(new QLabel(tr("Max")), 0, 0, Qt::AlignHCenter);
    plotLayout->addWidget(yMax, 1, 0);
    plotLayout->addWidget(plot, 0, 1, 4, 1);
    plotLayout->addWidget(new QLabel(tr("Min")), 2, 0, Qt::AlignHCenter);
    plotLayout->addWidget(yMin, 3, 0);
    plotLayout->setColumnStretch(1, 1);
    plotLayout->setRowStretch(2, 1);
    mainLayout->addLayout(plotLayout, 1);

    auto *displayLayout = new QHBoxLayout;
    maxholdTime->setRange(0, 120);
    maxholdTime->setSuffix(tr(" s"));
    waterfallMode->addItems({tr("Off"), tr("Grey"), tr("Red"), tr("Blue"), tr("Pride")});
    waterfallTime->setRange(10, 86400);
    waterfallTime->setSuffix(tr(" s"));
    overlayEnabled->setText(tr("Show spectrum overlay"));
    displayLayout->addWidget(new QLabel(tr("Maxhold")));
    displayLayout->addWidget(maxholdTime);
    displayLayout->addWidget(new QLabel(tr("Waterfall")));
    displayLayout->addWidget(waterfallMode);
    displayLayout->addWidget(new QLabel(tr("Waterfall time")));
    displayLayout->addWidget(waterfallTime);
    displayLayout->addWidget(overlayEnabled);
    mainLayout->addLayout(displayLayout);

    yMin->setRange(-200, 199);
    yMax->setRange(-199, 200);
    yMin->setValue(qBound(-200, config->getGnssSpectrumYMin(profileId), 199));
    yMax->setValue(qBound(-199, config->getGnssSpectrumYMax(profileId), 200));
    if (yMin->value() >= yMax->value()) {
        yMin->setValue(-30);
        yMax->setValue(50);
    }
    autoScale->setChecked(config->getGnssSpectrumAutoScale(profileId));
    maxholdTime->setValue(qBound(0, config->getGnssSpectrumMaxholdTime(profileId), 120));
    const QString savedWaterfallMode = config->getGnssSpectrumWaterfallMode(profileId);
    const int waterfallIndex = waterfallMode->findText(savedWaterfallMode, Qt::MatchFixedString);
    waterfallMode->setCurrentIndex(waterfallIndex >= 0 ? waterfallIndex : 0);
    waterfallTime->setValue(qBound(10, config->getGnssSpectrumWaterfallTime(profileId), 86400));
    overlayEnabled->setChecked(config->getGnssSpectrumOverlayEnabled(profileId));
    yMin->setEnabled(!autoScale->isChecked());
    yMax->setEnabled(!autoScale->isChecked());

    plot->addGraph();
    plot->xAxis->setLabel("MHz");
    plot->yAxis->setLabel("Level (relative, uncalibrated)");

    connect(bandSelector, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &GnssSpectrumWindow::bandSelectionChanged);
    connect(yMin, QOverload<int>::of(&QSpinBox::valueChanged), this, &GnssSpectrumWindow::manualYRangeChanged);
    connect(yMax, QOverload<int>::of(&QSpinBox::valueChanged), this, &GnssSpectrumWindow::manualYRangeChanged);
    connect(autoScale, &QCheckBox::toggled, this, &GnssSpectrumWindow::autoScaleChanged);
    connect(maxholdTime, QOverload<int>::of(&QSpinBox::valueChanged), this, [this](int seconds) {
        config->setGnssSpectrumMaxholdTime(profileId, seconds);
    });
    connect(waterfallMode, &QComboBox::currentTextChanged, this, [this](const QString &mode) {
        config->setGnssSpectrumWaterfallMode(profileId, mode);
    });
    connect(waterfallTime, QOverload<int>::of(&QSpinBox::valueChanged), this, [this](int seconds) {
        config->setGnssSpectrumWaterfallTime(profileId, seconds);
    });
    connect(overlayEnabled, &QCheckBox::toggled, this, [this](bool enabled) {
        config->setGnssSpectrumOverlayEnabled(profileId, enabled);
    });

    const QByteArray savedGeometry = config->getGnssSpectrumWindowGeometry(profileId);
    if (!savedGeometry.isEmpty()) restoreGeometry(savedGeometry);
    else resize(700, 400);
    initializing = false;
}

void GnssSpectrumWindow::resizeEvent(QResizeEvent *event)
{
    QDialog::resizeEvent(event);
    if (!initializing) saveGeometryToConfig();
}

void GnssSpectrumWindow::moveEvent(QMoveEvent *event)
{
    QDialog::moveEvent(event);
    if (!initializing) saveGeometryToConfig();
}

void GnssSpectrumWindow::saveGeometryToConfig()
{
    config->setGnssSpectrumWindowGeometry(profileId, saveGeometry());
}

void GnssSpectrumWindow::receiveSpectrum(const GnssSpectrumBlock &block)
{
    lastBlockByIndex[block.blockIndex] = block;

    if (bandSelector->count() != block.blockCount) {
        const int previousSelection = selectedBlockIndex;
        bandSelector->blockSignals(true);
        bandSelector->clear();
        for (int i = 0; i < block.blockCount; i++)
            bandSelector->addItem(tr("Band %1").arg(i + 1), i);
        const int restoreIndex = bandSelector->findData(previousSelection);
        bandSelector->setCurrentIndex(restoreIndex >= 0 ? restoreIndex : 0);
        selectedBlockIndex = bandSelector->currentData().toInt();
        bandSelector->blockSignals(false);
    }

    // Keep the band's label showing its actual current center frequency
    bandSelector->setItemText(block.blockIndex,
                               tr("Band %1 (%2 MHz)").arg(block.blockIndex + 1).arg(block.centerHz / 1e6, 0, 'f', 2));

    if (block.blockIndex == selectedBlockIndex) plotBlock(block);
}

void GnssSpectrumWindow::bandSelectionChanged(int index)
{
    selectedBlockIndex = bandSelector->itemData(index).toInt();
    if (lastBlockByIndex.contains(selectedBlockIndex)) plotBlock(lastBlockByIndex.value(selectedBlockIndex));
}

void GnssSpectrumWindow::manualYRangeChanged()
{
    if (yMin->value() >= yMax->value()) {
        if (sender() == yMin)
            yMax->setValue(yMin->value() + 1);
        else
            yMin->setValue(yMax->value() - 1);
    }

    config->setGnssSpectrumYMin(profileId, yMin->value());
    config->setGnssSpectrumYMax(profileId, yMax->value());
    if (!autoScale->isChecked()) applyManualYRange();
}

void GnssSpectrumWindow::autoScaleChanged(bool enabled)
{
    yMin->setEnabled(!enabled);
    yMax->setEnabled(!enabled);
    config->setGnssSpectrumAutoScale(profileId, enabled);

    if (enabled && lastBlockByIndex.contains(selectedBlockIndex))
        plotBlock(lastBlockByIndex.value(selectedBlockIndex));
    else if (!enabled)
        applyManualYRange();
}

void GnssSpectrumWindow::applyManualYRange()
{
    plot->yAxis->setRange(yMin->value(), yMax->value());
    plot->replot();
}

void GnssSpectrumWindow::plotBlock(const GnssSpectrumBlock &block)
{
    if (block.levels.isEmpty() || block.resolutionHz == 0) return;

    QVector<double> keys, values;
    keys.reserve(block.levels.size());
    values.reserve(block.levels.size());
    const double startMHz = (double)(block.centerHz - block.spanHz / 2) / 1e6;
    const double stepMHz = (double)block.resolutionHz / 1e6;
    for (int i = 0; i < block.levels.size(); i++) {
        keys.append(startMHz + i * stepMHz);
        values.append(block.levels.at(i));
    }

    plot->graph(0)->setData(keys, values, true);
    plot->xAxis->setRange(keys.first(), keys.last());
    if (autoScale->isChecked())
        plot->graph(0)->rescaleValueAxis(false, true);
    else
        plot->yAxis->setRange(yMin->value(), yMax->value());
    plot->replot();
}
