#include "gnssspectrumwindow.h"

#include <algorithm>
#include <cmath>
#include <limits>
#include <QHBoxLayout>
#include <QLabel>
#include <QGridLayout>
#include <QFile>
#include <QMenu>
#include <QPainter>
#include <QPixmap>
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
    plot->addGraph();
    plot->graph(1)->setPen(QPen(Qt::red));
    plot->graph(1)->setVisible(false);
    plot->xAxis->setLabel("MHz");
    plot->yAxis->setLabel("Level (relative, uncalibrated)");
    plot->addLayer("gnssBandOverlay", plot->layer("main"), QCustomPlot::limAbove);
    plot->addLayer("gnssMarkerOverlay", plot->layer("gnssBandOverlay"), QCustomPlot::limAbove);
    plot->setInteraction(QCP::iRangeDrag, true);
    loadBandOverlays();
    overlayEnabled->setEnabled(!bandPlanOverlays.isEmpty());
    if (bandPlanOverlays.isEmpty())
        overlayEnabled->setToolTip(tr("No bandplan.csv found in the configured work folder"));

    connect(bandSelector, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &GnssSpectrumWindow::bandSelectionChanged);
    connect(yMin, QOverload<int>::of(&QSpinBox::valueChanged), this, &GnssSpectrumWindow::manualYRangeChanged);
    connect(yMax, QOverload<int>::of(&QSpinBox::valueChanged), this, &GnssSpectrumWindow::manualYRangeChanged);
    connect(autoScale, &QCheckBox::toggled, this, &GnssSpectrumWindow::autoScaleChanged);
    connect(maxholdTime, QOverload<int>::of(&QSpinBox::valueChanged), this, &GnssSpectrumWindow::maxholdTimeChanged);
    connect(waterfallMode, &QComboBox::currentTextChanged, this, &GnssSpectrumWindow::waterfallModeChanged);
    connect(waterfallTime, QOverload<int>::of(&QSpinBox::valueChanged), this, &GnssSpectrumWindow::waterfallTimeChanged);
    connect(overlayEnabled, &QCheckBox::toggled, this, &GnssSpectrumWindow::overlayEnabledChanged);
    connect(plot, &QCustomPlot::mouseMove, this, &GnssSpectrumWindow::showSpectrumToolTip);
    connect(plot, &QCustomPlot::mousePress, this, &GnssSpectrumWindow::spectrumMousePress);
    connect(plot, &QCustomPlot::mouseRelease, this, &GnssSpectrumWindow::spectrumMouseRelease);

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

    updateBandHistory(block);
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
    updateBandOverlays();
    updateSpectrumMarkers();
    plot->replot();
}

void GnssSpectrumWindow::showSpectrumToolTip(QMouseEvent *event)
{
    if (draggedSpectrumMarker >= 0) {
        if ((event->pos() - markerDragStartPosition).manhattanLength() > 4) {
            markerDragMoved = true;
            spectrumMarkers[draggedSpectrumMarker].frequencyMHz = plot->xAxis->pixelToCoord(event->pos().x());
            updateSpectrumMarkers();
            plot->replot();
        }
        return;
    }

    if (!plot->axisRect()->rect().contains(event->pos())) {
        plot->setToolTip(QString());
        return;
    }

    const double frequencyMHz = plot->xAxis->pixelToCoord(event->pos().x());
    const double level = spectrumLevelAt(frequencyMHz);
    QString text = QString::number(frequencyMHz, 'f', 3) + tr(" MHz");
    if (std::isfinite(level))
        text += QString(" / %1").arg(level, 0, 'f', 1);
    plot->setToolTip(text);
}

void GnssSpectrumWindow::spectrumMousePress(QMouseEvent *event)
{
    if (event->button() != Qt::LeftButton) return;

    leftMousePressPosition = event->pos();
    markerDragStartPosition = event->pos();
    draggedSpectrumMarker = spectrumMarkerAt(event->pos());
    markerDragMoved = false;
    if (draggedSpectrumMarker >= 0)
        plot->setInteraction(QCP::iRangeDrag, false);
}

void GnssSpectrumWindow::spectrumMouseRelease(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton && draggedSpectrumMarker >= 0) {
        const int markerIndex = draggedSpectrumMarker;
        markerDragMoved = markerDragMoved || (event->pos() - markerDragStartPosition).manhattanLength() > 4;
        draggedSpectrumMarker = -1;
        plot->setInteraction(QCP::iRangeDrag, true);

        if (markerDragMoved) {
            spectrumMarkers[markerIndex].frequencyMHz = plot->xAxis->pixelToCoord(event->pos().x());
            updateSpectrumMarkers();
            plot->replot();
        }
        else {
            QMenu menu;
            menu.addAction(tr("Remove marker"), this, [this, markerIndex] { removeSpectrumMarker(markerIndex); });
            menu.exec(plot->mapToGlobal(event->pos()));
        }
        return;
    }

    if (!plot->axisRect()->rect().contains(event->pos())) return;
    if (event->button() != Qt::LeftButton && event->button() != Qt::RightButton) return;
    if (event->button() == Qt::LeftButton
        && (event->pos() - leftMousePressPosition).manhattanLength() > 4)
        return;

    const int markerIndex = spectrumMarkerAt(event->pos());
    const double frequencyMHz = plot->xAxis->pixelToCoord(event->pos().x());
    QMenu menu;
    if (markerIndex >= 0) {
        menu.addAction(tr("Remove marker"), this, [this, markerIndex] { removeSpectrumMarker(markerIndex); });
    }
    else {
        menu.addAction(tr("Add marker at %1 MHz").arg(frequencyMHz, 0, 'f', 3),
                       this,
                       [this, frequencyMHz] { addSpectrumMarker(frequencyMHz); });
    }
    menu.exec(plot->mapToGlobal(event->pos()));
}

void GnssSpectrumWindow::addSpectrumMarker(double frequencyMHz)
{
    const QVector<QColor> markerColors = {QColor(0, 90, 255), QColor(220, 120, 0), QColor(160, 0, 200)};
    SpectrumMarker marker;
    marker.frequencyMHz = frequencyMHz;
    marker.color = markerColors.at(spectrumMarkers.size() % markerColors.size());

    QPen markerPen(marker.color);
    markerPen.setWidth(2);
    marker.line = new QCPItemStraightLine(plot);
    marker.line->setLayer("gnssMarkerOverlay");
    marker.line->setPen(markerPen);
    marker.line->setSelectable(false);

    marker.label = new QCPItemText(plot);
    marker.label->setLayer("gnssMarkerOverlay");
    marker.label->setSelectable(false);
    marker.label->setColor(marker.color);
    marker.label->setPen(QPen(marker.color));
    marker.label->setBrush(config->getDarkMode() ? QColor(30, 30, 30, 210) : QColor(255, 255, 255, 220));
    marker.label->setPadding(QMargins(4, 2, 4, 2));
    marker.label->setPositionAlignment(Qt::AlignLeft | Qt::AlignTop);
    marker.label->setTextAlignment(Qt::AlignLeft);
    marker.label->setFont(QFont(QFont().family(), qMax(1, qRound(config->getOverlayFontSize() * 0.7))));

    spectrumMarkers.append(marker);
    updateSpectrumMarkers();
    plot->replot();
}

void GnssSpectrumWindow::removeSpectrumMarker(int markerIndex)
{
    if (markerIndex < 0 || markerIndex >= spectrumMarkers.size()) return;

    const SpectrumMarker marker = spectrumMarkers.takeAt(markerIndex);
    plot->removeItem(marker.line);
    plot->removeItem(marker.label);
    if (draggedSpectrumMarker == markerIndex) draggedSpectrumMarker = -1;
    else if (draggedSpectrumMarker > markerIndex) --draggedSpectrumMarker;
    updateSpectrumMarkers();
    plot->replot();
}

void GnssSpectrumWindow::updateSpectrumMarkers()
{
    const QCPRange yRange = plot->yAxis->range();
    for (int i = 0; i < spectrumMarkers.size(); ++i) {
        SpectrumMarker &marker = spectrumMarkers[i];
        marker.line->point1->setCoords(marker.frequencyMHz, yRange.lower);
        marker.line->point2->setCoords(marker.frequencyMHz, yRange.upper);

        const double level = spectrumLevelAt(marker.frequencyMHz);
        const QString levelText = std::isfinite(level) ? QString::number(level, 'f', 1) : QStringLiteral("--");
        marker.label->setText(QString("%1 MHz\n%2 (relative)")
                                  .arg(marker.frequencyMHz, 0, 'f', 3)
                                  .arg(levelText));
        const double verticalOffset = yRange.size() * (0.04 + 0.08 * (i % 10));
        marker.label->position->setCoords(marker.frequencyMHz, yRange.upper - verticalOffset);
    }
}

int GnssSpectrumWindow::spectrumMarkerAt(const QPoint &position) const
{
    if (!plot->axisRect()->rect().contains(position)) return -1;

    for (int i = 0; i < spectrumMarkers.size(); ++i) {
        const int markerX = plot->xAxis->coordToPixel(spectrumMarkers.at(i).frequencyMHz);
        if (qAbs(markerX - position.x()) <= 6) return i;

        const double labelDistance = spectrumMarkers.at(i).label->selectTest(position, false);
        if (std::isfinite(labelDistance) && labelDistance <= plot->selectionTolerance()) return i;
    }
    return -1;
}

double GnssSpectrumWindow::spectrumLevelAt(double frequencyMHz) const
{
    const int sampleCount = qMin(currentTraceFrequencies.size(), currentTraceLevels.size());
    if (sampleCount <= 0 || frequencyMHz < currentTraceFrequencies.first()
        || frequencyMHz > currentTraceFrequencies.at(sampleCount - 1))
        return std::numeric_limits<double>::quiet_NaN();

    const auto begin = currentTraceFrequencies.constBegin();
    const auto end = begin + sampleCount;
    const auto upper = std::lower_bound(begin, end, frequencyMHz);
    const int upperIndex = static_cast<int>(upper - begin);
    if (upperIndex <= 0) return currentTraceLevels.first();
    if (upperIndex >= sampleCount) return currentTraceLevels.at(sampleCount - 1);

    const int lowerIndex = upperIndex - 1;
    const double lowerFrequency = currentTraceFrequencies.at(lowerIndex);
    const double upperFrequency = currentTraceFrequencies.at(upperIndex);
    if (upperFrequency <= lowerFrequency) return currentTraceLevels.at(lowerIndex);

    const double ratio = (frequencyMHz - lowerFrequency) / (upperFrequency - lowerFrequency);
    return currentTraceLevels.at(lowerIndex)
           + ratio * (currentTraceLevels.at(upperIndex) - currentTraceLevels.at(lowerIndex));
}

void GnssSpectrumWindow::overlayEnabledChanged(bool enabled)
{
    config->setGnssSpectrumOverlayEnabled(profileId, enabled);
    updateBandOverlays();
    plot->replot();
}

void GnssSpectrumWindow::loadBandOverlays()
{
    QFile file(config->getWorkFolder() + "/bandplan.csv");
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) return;

    while (!file.atEnd()) {
        const QString line = QString::fromLocal8Bit(file.readLine()).trimmed();
        const QStringList fields = line.split(',');
        if (line.contains('#') || fields.size() != 9) continue;

        bool lowerOk = false;
        bool upperOk = false;
        bool centerOk = false;
        const double lowerMHz = fields.at(1).toDouble(&lowerOk);
        const double upperMHz = fields.at(2).toDouble(&upperOk);
        const double centerMHz = fields.at(8).toDouble(&centerOk);
        if (!lowerOk || !upperOk || !centerOk || lowerMHz >= upperMHz) continue;

        BandPlanOverlay overlay;
        overlay.name = fields.at(0).trimmed();
        overlay.lowerMHz = lowerMHz;
        overlay.upperMHz = upperMHz;
        overlay.centerMHz = centerMHz;
        overlay.labelOffset = fields.at(6).toInt();
        overlay.color = QColor(fields.at(3).toInt(),
                               fields.at(4).toInt(),
                               fields.at(5).toInt(),
                               fields.at(7).toInt());

        overlay.area = new QCPItemRect(plot);
        overlay.area->setLayer("gnssBandOverlay");
        overlay.area->setPen(Qt::NoPen);
        overlay.area->setBrush(overlay.color);
        overlay.area->setSelectable(false);

        overlay.label = new QCPItemText(plot);
        overlay.label->setLayer("gnssBandOverlay");
        overlay.label->setText(overlay.name);
        overlay.label->setFont(QFont(QFont().family(), config->getOverlayFontSize()));
        overlay.label->setPen(Qt::NoPen);
        overlay.label->setBrush(Qt::NoBrush);
        overlay.label->setSelectable(false);

        overlay.centerLine = new QCPItemStraightLine(plot);
        overlay.centerLine->setLayer("gnssBandOverlay");
        QPen centerPen(Qt::gray);
        centerPen.setStyle(Qt::DotLine);
        overlay.centerLine->setPen(centerPen);
        overlay.centerLine->setSelectable(false);

        bandPlanOverlays.append(overlay);
    }
    updateBandOverlays();
}

void GnssSpectrumWindow::updateBandOverlays()
{
    const bool visible = overlayEnabled->isChecked();
    const QCPRange yRange = plot->yAxis->range();
    for (BandPlanOverlay &overlay : bandPlanOverlays) {
        overlay.area->topLeft->setCoords(overlay.lowerMHz, yRange.upper);
        overlay.area->bottomRight->setCoords(overlay.upperMHz, yRange.lower);
        overlay.label->position->setCoords(overlay.centerMHz,
                                           yRange.upper - 2 - overlay.labelOffset);
        overlay.centerLine->point1->setCoords(overlay.centerMHz, yRange.upper);
        overlay.centerLine->point2->setCoords(overlay.centerMHz, yRange.lower);
        overlay.area->setVisible(visible);
        overlay.label->setVisible(visible);
        overlay.centerLine->setVisible(visible);
    }
}

void GnssSpectrumWindow::maxholdTimeChanged(int seconds)
{
    config->setGnssSpectrumMaxholdTime(profileId, seconds);
    if (seconds == 0) {
        for (auto iterator = bandDisplayState.begin(); iterator != bandDisplayState.end(); ++iterator) {
            iterator->maxholdBucket.clear();
            iterator->maxholdFrames.clear();
            iterator->maxholdBucketStartMs = -1;
            iterator->maxholdBucketLastMs = -1;
        }
    }

    if (lastBlockByIndex.contains(selectedBlockIndex))
        plotBlock(lastBlockByIndex.value(selectedBlockIndex));
}

void GnssSpectrumWindow::waterfallModeChanged(const QString &mode)
{
    config->setGnssSpectrumWaterfallMode(profileId, mode);
    clearWaterfallHistory();
    displaySelectedWaterfall();
}

void GnssSpectrumWindow::waterfallTimeChanged(int seconds)
{
    config->setGnssSpectrumWaterfallTime(profileId, seconds);
    clearWaterfallHistory();
    displaySelectedWaterfall();
}

void GnssSpectrumWindow::updateBandHistory(const GnssSpectrumBlock &block)
{
    if (block.blockIndex < 0 || block.levels.isEmpty()) return;

    BandDisplayState &state = bandDisplayState[block.blockIndex];
    const bool geometryChanged = state.geometryInitialized
                                 && (state.centerHz != block.centerHz
                                     || state.spanHz != block.spanHz
                                     || state.resolutionHz != block.resolutionHz
                                     || state.levelCount != block.levels.size());
    if (geometryChanged) state = BandDisplayState();

    state.centerHz = block.centerHz;
    state.spanHz = block.spanHz;
    state.resolutionHz = block.resolutionHz;
    state.levelCount = block.levels.size();
    state.geometryInitialized = true;

    qint64 timestampMs = block.timestamp.isValid()
                             ? block.timestamp.toMSecsSinceEpoch()
                             : QDateTime::currentMSecsSinceEpoch();
    if (state.lastTimestampMs >= 0 && timestampMs < state.lastTimestampMs) {
        state.maxholdBucket.clear();
        state.maxholdFrames.clear();
        state.maxholdBucketStartMs = -1;
        state.maxholdBucketLastMs = -1;
        state.waterfallImage = QImage();
        state.waterfallLastLevels.clear();
        state.lastWaterfallRowMs = -1;
    }
    state.lastTimestampMs = timestampMs;

    const int holdSeconds = maxholdTime->value();
    if (holdSeconds > 0) {
        if (state.maxholdBucket.isEmpty() || state.maxholdBucketStartMs < 0
            || timestampMs - state.maxholdBucketStartMs >= 1000) {
            if (!state.maxholdBucket.isEmpty()) {
                state.maxholdFrames.append({state.maxholdBucketLastMs, state.maxholdBucket});
            }
            state.maxholdBucket = block.levels;
            state.maxholdBucketStartMs = timestampMs;
        }
        else if (state.maxholdBucket.size() == block.levels.size()) {
            for (int i = 0; i < block.levels.size(); ++i)
                state.maxholdBucket[i] = qMax(state.maxholdBucket.at(i), block.levels.at(i));
        }
        state.maxholdBucketLastMs = timestampMs;

        const qint64 oldestAllowedMs = timestampMs - qint64(holdSeconds) * 1000;
        while (!state.maxholdFrames.isEmpty()
               && state.maxholdFrames.first().timestampMs < oldestAllowedMs)
            state.maxholdFrames.removeFirst();
    }
    else {
        state.maxholdBucket.clear();
        state.maxholdFrames.clear();
        state.maxholdBucketStartMs = -1;
        state.maxholdBucketLastMs = -1;
    }

    if (waterfallMode->currentText() != "Off")
        updateWaterfall(block.blockIndex, block, timestampMs);
}

void GnssSpectrumWindow::updateWaterfall(int blockIndex, const GnssSpectrumBlock &block, qint64 timestampMs)
{
    BandDisplayState &state = bandDisplayState[blockIndex];
    constexpr int imageWidth = 512;
    constexpr int imageHeight = 256;
    if (state.waterfallImage.size() != QSize(imageWidth, imageHeight)) {
        state.waterfallImage = QImage(imageWidth, imageHeight, QImage::Format_ARGB32);
        state.waterfallImage.fill(Qt::transparent);
        state.lastWaterfallRowMs = -1;
    }

    const qint64 rowIntervalMs = qMax<qint64>(1, qint64(waterfallTime->value()) * 1000 / imageHeight);
    const QVector<double> previousLevels = state.waterfallLastLevels;
    const bool canInterpolate = previousLevels.size() == block.levels.size();
    state.waterfallLastLevels = block.levels;

    qint64 rowsElapsed = 1;
    if (state.lastWaterfallRowMs >= 0) {
        rowsElapsed = (timestampMs - state.lastWaterfallRowMs) / rowIntervalMs;
        if (rowsElapsed <= 0) return;
    }
    const int rowsToRender = static_cast<int>(qMin<qint64>(rowsElapsed, imageHeight));

    if (rowsElapsed >= imageHeight) {
        state.waterfallImage.fill(Qt::transparent);
    }
    else {
        QImage shifted(state.waterfallImage.size(), QImage::Format_ARGB32);
        shifted.fill(Qt::transparent);
        QPainter painter(&shifted);
        painter.drawImage(0, static_cast<int>(rowsElapsed), state.waterfallImage);
        state.waterfallImage = shifted;
    }

    double scaleMin = plot->yAxis->range().lower;
    double scaleMax = plot->yAxis->range().upper;
    if (autoScale->isChecked()) {
        const auto minMax = std::minmax_element(block.levels.cbegin(), block.levels.cend());
        scaleMin = *minMax.first;
        scaleMax = *minMax.second;
    }
    if (!std::isfinite(scaleMin) || !std::isfinite(scaleMax) || scaleMax <= scaleMin) {
        scaleMin = 0;
        scaleMax = 1;
    }

    const double sampleRate = static_cast<double>(block.levels.size()) / imageWidth;
    const QString palette = waterfallMode->currentText();
    const qint64 firstVisibleStep = rowsElapsed - rowsToRender + 1;
    for (int row = 0; row < rowsToRender; ++row) {
        const qint64 elapsedStep = rowsElapsed - row;
        const double interpolation = canInterpolate
                                         ? qBound(0.0,
                                                  static_cast<double>(elapsedStep - firstVisibleStep + 1)
                                                      / static_cast<double>(rowsToRender),
                                                  1.0)
                                         : 1.0;
        for (int x = 0; x < imageWidth; ++x) {
            const int sampleIndex = qBound(0, static_cast<int>(x * sampleRate), block.levels.size() - 1);
            const double level = canInterpolate
                                     ? previousLevels.at(sampleIndex)
                                           + (block.levels.at(sampleIndex) - previousLevels.at(sampleIndex)) * interpolation
                                     : block.levels.at(sampleIndex);
            const double percent = qBound(0.0, (level - scaleMin) / (scaleMax - scaleMin), 1.0);
            QColor color;
            if (palette == "Grey")
                color.setHsv(180, 0, qRound(255 * (1.0 - percent)));
            else if (palette == "Red")
                color.setHsv(0, qRound(255 * percent), 255);
            else if (palette == "Blue")
                color.setHsv(240, qRound(255 * percent), 255);
            else
                color.setHsv(qRound(255 * (1.0 - percent)), 255, 240);
            state.waterfallImage.setPixelColor(x, row, color);
        }
    }

    state.lastWaterfallRowMs = state.lastWaterfallRowMs < 0
                                   ? timestampMs
                                   : state.lastWaterfallRowMs + rowsElapsed * rowIntervalMs;
    if (blockIndex == selectedBlockIndex) displaySelectedWaterfall();
}

void GnssSpectrumWindow::displaySelectedWaterfall()
{
    if (waterfallMode->currentText() == "Off"
        || !bandDisplayState.contains(selectedBlockIndex)
        || bandDisplayState.value(selectedBlockIndex).waterfallImage.isNull()) {
        plot->axisRect()->setBackground(QPixmap());
    }
    else {
        plot->axisRect()->setBackground(
            QPixmap::fromImage(bandDisplayState.value(selectedBlockIndex).waterfallImage),
            true,
            Qt::IgnoreAspectRatio);
    }
    plot->replot();
}

void GnssSpectrumWindow::clearWaterfallHistory()
{
    for (auto iterator = bandDisplayState.begin(); iterator != bandDisplayState.end(); ++iterator) {
        iterator->waterfallImage = QImage();
        iterator->waterfallLastLevels.clear();
        iterator->lastWaterfallRowMs = -1;
    }
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
    currentTraceFrequencies = keys;
    currentTraceLevels = values;

    plot->graph(0)->setData(keys, values, true);
    plot->xAxis->setRange(keys.first(), keys.last());
    BandDisplayState &state = bandDisplayState[block.blockIndex];
    const int holdSeconds = maxholdTime->value();
    if (holdSeconds > 0 && state.geometryInitialized) {
        QVector<double> maxhold(block.levels.size(), std::numeric_limits<double>::lowest());
        auto mergeLevels = [&maxhold](const QVector<double> &levels) {
            if (levels.size() != maxhold.size()) return;
            for (int i = 0; i < levels.size(); ++i)
                maxhold[i] = qMax(maxhold.at(i), levels.at(i));
        };
        for (const MaxholdFrame &frame : state.maxholdFrames)
            mergeLevels(frame.levels);
        mergeLevels(state.maxholdBucket);
        plot->graph(1)->setData(keys, maxhold, true);
        plot->graph(1)->setVisible(true);
    }
    else {
        plot->graph(1)->setData(QVector<double>(), QVector<double>());
        plot->graph(1)->setVisible(false);
    }
    if (autoScale->isChecked())
        plot->graph(0)->rescaleValueAxis(false, true);
    else
        plot->yAxis->setRange(yMin->value(), yMax->value());
    updateBandOverlays();
    updateSpectrumMarkers();
    displaySelectedWaterfall();
    plot->replot();
}
