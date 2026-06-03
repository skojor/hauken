#include "customplotcontroller.h"

#include <algorithm>
#include <cmath>
#include <limits>
#include <QMenu>

CustomPlotController::CustomPlotController(QCustomPlot *ptr, QSharedPointer<Config> c)
{
    customPlotPtr = ptr;
    config = c;
    connect(customPlotPtr, &QCustomPlot::mouseMove, this, &CustomPlotController::showToolTip);
    connect(customPlotPtr, &QCustomPlot::mousePress, this, &CustomPlotController::onMousePress);
    connect(customPlotPtr->selectionRect(), &QCPSelectionRect::accepted, this, &CustomPlotController::showSelectionMenu);
    connect(customPlotPtr, &QCustomPlot::mouseRelease, this, &CustomPlotController::onMouseClick);
    connect(flashTimer, &QTimer::timeout, this, &CustomPlotController::flashRoutine);
    connect(customPlotPtr, &QCustomPlot::mouseDoubleClick, this, &CustomPlotController::doubleClickEvent);
    connect(customPlotPtr, &QCustomPlot::mouseWheel, this, &CustomPlotController::mouseWheel);

    QFile file(config->getWorkFolder() + "/bandplan.csv");
    if (file.exists()) {
        file.open(QIODevice::ReadOnly);
        while (!file.atEnd()) {
            QString line = file.readLine();
            QStringList split = line.split(',');
            if (!line.contains("#") && split.size() == 9) {
                gnssBands.append(split[0]);
                gnssBandfrequencies.append(split[1].toDouble());
                gnssBandfrequencies.append(split[2].toDouble());
                gnssBandColors.append(QColor(split[3].toInt(), split[4].toInt(), split[5].toInt(), split[7].toInt()));
                gnssTextLabelPos.append(split[6].toInt());
                gnssBandCenterFreq.append(split[8].toDouble());
            }
        }
    }
    file.close();

    centerFreqLine = new QCPItemStraightLine(customPlotPtr);
}

void CustomPlotController::init()
{
    setupBasics();
    updSettings();
    customPlotPtr->replot();
}

void CustomPlotController::setupBasics()
{
    customPlotPtr->addGraph();
    customPlotPtr->addGraph();
    customPlotPtr->addGraph();
    customPlotPtr->addGraph();
    customPlotPtr->addGraph();
    customPlotPtr->addGraph();

    customPlotPtr->yAxis->setLabel((config->getUseDbm()?"dBm":"dBμV"));
    customPlotPtr->xAxis->setLabel("MHz");
    customPlotPtr->graph(0)->setPen(QPen(Qt::black));
    customPlotPtr->graph(1)->setPen(QPen(Qt::red));
    customPlotPtr->graph(2)->setSelectable(QCP::stWhole);
    customPlotPtr->graph(2)->setLineStyle(QCPGraph::lsLine);
    QPen pen;
    pen.setWidth(1);
    pen.setColor(QColor(0, 100, 0, 255));
    customPlotPtr->graph(2)->setPen(pen);
    customPlotPtr->graph(2)->setBrush(QColor(0, 150, 0, 127));
    customPlotPtr->graph(2)->setChannelFillGraph(customPlotPtr->graph(3));

    customPlotPtr->setInteraction(QCP::iRangeDrag, true);
    customPlotPtr->setSelectionRectMode(QCP::srmCustom);
    //customPlotPtr->setOpenGl(false);
    customPlotPtr->setNotAntialiasedElements(QCP::aePlottables);

    customPlotPtr->addLayer("liveGraph");
    customPlotPtr->layer("liveGraph")->setMode(QCPLayer::lmBuffered);
    customPlotPtr->addLayer("triggerLayer");
    customPlotPtr->layer("triggerLayer")->setMode(QCPLayer::lmBuffered);
    customPlotPtr->graph(0)->setLayer("liveGraph");
    customPlotPtr->graph(1)->setLayer("liveGraph");
    customPlotPtr->graph(2)->setLayer("triggerLayer");
    customPlotPtr->graph(3)->setLayer("triggerLayer");
    // Overlay
    customPlotPtr->addLayer("overlayLayer");
    customPlotPtr->layer("overlayLayer")->setMode(QCPLayer::lmBuffered);
    customPlotPtr->graph(4)->setLayer("overlayLayer");
    customPlotPtr->graph(4)->setData(QVector<double>() << 1 << 9900, QVector<double>() << -200 << -200);

    // Demod. bw overlay
    customPlotPtr->addLayer("bwLayer");
    customPlotPtr->layer("bwLayer")->setMode(QCPLayer::lmBuffered);
    customPlotPtr->graph(5)->setLayer("bwLayer");
    customPlotPtr->graph(5)->setData(QVector<double>() << 1 << 9900, QVector<double>() << -200 << -200);
    customPlotPtr->graph(5)->setChannelFillGraph(customPlotPtr->graph(4));

    customPlotPtr->xAxis->grid()->setVisible(false);
    plotIterator = customPlotPtr->graphCount() - 1;

    for (int i=0, j=0; i < gnssBands.size(); i++, j += 2) {
        customPlotPtr->addGraph();
        plotIterator++;
        customPlotPtr->graph(plotIterator)->setPen(pen);
        customPlotPtr->graph(plotIterator)->setBrush(gnssBandColors[i]);
        customPlotPtr->graph(plotIterator)->setLineStyle(QCPGraph::lsLine);
        customPlotPtr->graph(plotIterator)->setLayer("overlayLayer");
        customPlotPtr->graph(plotIterator)->setChannelFillGraph(customPlotPtr->graph(4));

        addOverlay(plotIterator, gnssBandfrequencies[j], gnssBandfrequencies[j+1]);
        QCPItemText *tmp = new QCPItemText(customPlotPtr);
        tmp->setText(gnssBands[i]);
        tmp->setFont(QFont(QFont().family(), config->getOverlayFontSize()));
        gnssTextLabels.append(tmp);
        gnssBandsSelectors.append(true);

        QCPItemStraightLine *tmpLine = new QCPItemStraightLine(customPlotPtr);
        pen.setStyle(Qt::DotLine);
        pen.setColor(Qt::gray);
        tmpLine->setPen(pen);
        tmpLine->point1->setCoords( gnssBandCenterFreq[i], 200);
        tmpLine->point2->setCoords( gnssBandCenterFreq[i], -200);
        gnssCenterLine.append(tmpLine);
    }

    customPlotPtr->addLayer("markerLayer");
    customPlotPtr->layer("markerLayer")->setMode(QCPLayer::lmBuffered);

    toggleOverlay();
    updTextLabelPositions();
}

void CustomPlotController::plotTrace(const QVector<double> &data)
{
    currentTraceData = data;
    if (keyValues.isEmpty()) reCalc();
    if (!keyValues.isEmpty() and !data.isEmpty()) {
        customPlotPtr->graph(0)->setData(keyValues, data);
        updateSpectrumMarkerLabels();
        customPlotPtr->layer("liveGraph")->replot();
        customPlotPtr->layer("markerLayer")->replot();
    }
}

void CustomPlotController::plotMaxhold(const QVector<double> &data)
{
    if (keyValues.isEmpty()) reCalc();
    if (!keyValues.isEmpty() and !data.isEmpty()) {
        customPlotPtr->graph(1)->setData(keyValues, data);
    }
}

void CustomPlotController::plotTriglevel(const QVector<double> &data)
{
    if (keyValues.isEmpty()) reCalc();
    QVector<double> copy;

    if (!data.isEmpty() && !freqSelection.isEmpty()) {
        copy = data;
        if (config->getUseDbm()) {
            for (int i=0; i < copy.size(); i++) copy[i] -= 107;
        }
        if ((int)(config->getCorrValue() * 10) != 0) {
            double corr = config->getCorrValue();
            for (int i=0; i < copy.size(); i++) copy[i] += corr;
        }

        for (int i=0; i<plotResolution; i++) {
            if (!freqSelection.at(i))
                copy[i] = -200;
        }
    }
    customPlotPtr->graph(2)->setData(keyValues, copy);
    customPlotPtr->layer("triggerLayer")->replot();
}

void CustomPlotController::doReplot()
{
    customPlotPtr->layer("liveGraph")->replot();
}

void CustomPlotController::reCalc()
{
    if ((int)(resolution * 1e6) > 0 && customPlotPtr->xAxis->range().upper > 0 && customPlotPtr->xAxis->range().lower > 0) {
        nrOfValues = 1 + ((customPlotPtr->xAxis->range().upper - customPlotPtr->xAxis->range().lower) / resolution);
        if (nrOfValues > 1) {
            double rate = ((customPlotPtr->xAxis->range().upper - customPlotPtr->xAxis->range().lower) * 1e6) / (plotResolution - 1);
            keyValues.clear();
            double freq = customPlotPtr->xAxis->range().lower * 1e6;

            for (int i = 0; i < plotResolution; i++) {
                keyValues.append(freq / 1e6);
                freq += rate;
            }
            readTrigSelectionFromConfig();
            customPlotPtr->graph(3)->setData(keyValues, fill);
            updateSpectrumMarkerLabels();
            customPlotPtr->layer("triggerLayer")->replot();
        }
    }
}

void CustomPlotController::showToolTip(QMouseEvent *event)
{
    if (draggedSpectrumMarker >= 0) {
        if ((event->pos() - markerDragStartPos).manhattanLength() > 4) {
            markerDragMoved = true;
            setSpectrumMarkerFrequency(draggedSpectrumMarker, customPlotPtr->xAxis->pixelToCoord(event->pos().x()));
            customPlotPtr->layer("markerLayer")->replot();
        }
        return;
    }

    double x = customPlotPtr->xAxis->pixelToCoord(event->pos().x());
    double y = customPlotPtr->yAxis->pixelToCoord(event->pos().y());

    QString text = QString::number(x, 'f', 3) + " MHz / " +
                   QString::number(y, 'f', 1) +
                   (config->getUseDbm()?" dBm":" dBμV");
    customPlotPtr->setToolTip(text);
}

void CustomPlotController::onMousePress(QMouseEvent *event)
{
    if (event->button() != Qt::LeftButton) return;

    leftMousePressPos = event->pos();
    draggedSpectrumMarker = spectrumMarkerAt(event->pos());
    markerDragStartPos = event->pos();
    markerDragMoved = false;

    if (draggedSpectrumMarker >= 0) {
        customPlotPtr->setInteraction(QCP::iRangeDrag, false);
    }
}

void CustomPlotController::onMouseClick(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton && draggedSpectrumMarker >= 0) {
        const int markerIndex = draggedSpectrumMarker;
        draggedSpectrumMarker = -1;
        customPlotPtr->setInteraction(QCP::iRangeDrag, true);

        if (markerDragMoved) {
            setSpectrumMarkerFrequency(markerIndex, customPlotPtr->xAxis->pixelToCoord(event->pos().x()));
            customPlotPtr->layer("markerLayer")->replot();
            return;
        }

        QMenu menu;
        menu.addAction("Remove marker", this, [this, markerIndex]() {
            removeSpectrumMarker(markerIndex);
        });
        menu.exec(customPlotPtr->mapToGlobal(event->pos()));
        return;
    }

    if (!customPlotPtr->axisRect()->rect().contains(event->pos())) return;

    const double clickedFrequencyMhz = customPlotPtr->xAxis->pixelToCoord(event->pos().x());

    if (event->button() == Qt::LeftButton && (event->pos() - leftMousePressPos).manhattanLength() <= 4) {
        QMenu menu;
        menu.addAction("Add marker", this, [this, clickedFrequencyMhz]() {
            addSpectrumMarker(clickedFrequencyMhz);
        });
        menu.exec(customPlotPtr->mapToGlobal(event->pos()));
        return;
    }

    if (event->button() == Qt::RightButton) {
        const int markerIndex = spectrumMarkerAt(event->pos());

        QMenu menu;
        if (markerIndex >= 0) {
            menu.addAction("Remove marker", this, [this, markerIndex]() {
                removeSpectrumMarker(markerIndex);
            });
        } else {
            menu.addAction("Add marker", this, [this, clickedFrequencyMhz]() {
                addSpectrumMarker(clickedFrequencyMhz);
            });
        }
        menu.addSeparator();
        menu.addAction("Exclude all trigger frequencies", this, &CustomPlotController::trigExcludeAll);
        menu.addAction("Include all trigger frequencies", this, &CustomPlotController::trigIncludeAll);

        menu.addSeparator();
        menu.addAction("Show/hide spectrum overlay", this, &CustomPlotController::toggleOverlay);

        /*for (int i=0; i < gnssBandsSelectors.size(); i++) {
            menu.addAction("Show/hide " + gnssBands[i], this, [this, i] () {
                gnssBandsSelectors[i] = !gnssBandsSelectors[i];
                customPlotPtr->graph(customPlotPtr->graphCount() - gnssBands.size() + i)->setVisible(gnssBandsSelectors[i]);
                gnssTextLabels[i]->setVisible(gnssBandsSelectors[i]);
                gnssCenterLine[i]->setVisible(gnssBandsSelectors[i]);
                customPlotPtr->replot();
            });
        }*/
        menu.exec(customPlotPtr->mapToGlobal(event->pos()));
    }
}

void CustomPlotController::showSelectionMenu(const QRect &rect, QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        selMin = customPlotPtr->xAxis->pixelToCoord(rect.x());
        selMax = customPlotPtr->xAxis->pixelToCoord(rect.x() + rect.width());
        if (selMin > selMax) { // some smartypant tried to reverse the selection
            double tmp = selMin;
            selMin = selMax;
            selMax = tmp;
        }
        if (!keyValues.isEmpty() && selMax >= stopFreq) selMax = keyValues.last(); //stopFreq - resolution / 1000;

        QMenu *menu = new QMenu;
        menu->addAction("Include " + QString::number(selMin, 'f', 3) +
                            "-" + QString::number(selMax, 'f', 3) + " MHz in the trigger area",
                        this, &CustomPlotController::trigInclude);
        menu->addAction("Exclude " + QString::number(selMin, 'f', 3) +
                            "-" + QString::number(selMax, 'f', 3) + " MHz in the trigger area",
                        this, &CustomPlotController::trigExclude);
        menu->addAction("Include all trigger frequencies", this, &CustomPlotController::trigIncludeAll);
        menu->addAction("Exclude all trigger frequencies", this, &CustomPlotController::trigExcludeAll);
        menu->exec(customPlotPtr->mapToGlobal(event->pos()));
    }
}

void CustomPlotController::trigInclude()
{
    for (int i=0; i<plotResolution; i++) {
        if (keyValues.at(i) > selMin && keyValues.at(i) <= selMax) {
            freqSelection[i] = 1;
        }
    }
    saveTrigSelectionToConfig();
}

void CustomPlotController::trigExclude()
{
    for (int i=0; i<plotResolution; i++) {
        if (keyValues.at(i) > selMin && keyValues.at(i) <= selMax) {
            freqSelection[i] = 0;
        }
    }
    saveTrigSelectionToConfig();
}

void CustomPlotController::trigIncludeAll()
{
    for (int i=0; i<plotResolution; i++) {
        freqSelection[i] = 1;
    }
    saveTrigSelectionToConfig();
}

void CustomPlotController::trigExcludeAll()
{
    for (int i=0; i<plotResolution; i++) {
        freqSelection[i] = 0;
    }
    QStringList tmp = { "0", "1" };
    config->setTrigFrequencies(tmp); // deletes all settings
    saveTrigSelectionToConfig();
}

void CustomPlotController::readTrigSelectionFromConfig()
{
    freqSelection.fill(0, plotResolution);
    QStringList freqList = config->getTrigFrequencies();

    if (!freqList.isEmpty()) {
        double range1 = 0, range2 = 0;
        for (int i=0; i<freqList.size();) {
            range1 = freqList.at(i++).toDouble();
            range2 = freqList.at(i++).toDouble();
            for (int j=0; j<plotResolution; j++) {
                if (keyValues.at(j) > range1 && keyValues.at(j) < range2)
                    freqSelection[j] = 1;
            }
        }
    }
    else { // initialize list in config, it shouldn't be empty
        freqSelection.fill(1, plotResolution);
        freqList << "0" << "9999";
        config->setTrigFrequencies(freqList);
    }
}

void CustomPlotController::saveTrigSelectionToConfig()
{
    QStringList tmp;

    double sel1 = 0, sel2 = 0;
    for (int i=0; i<plotResolution; i++) {
        if (freqSelection.at(i) == 1 && (int)sel1 == 0) { // first include value
            sel1 = keyValues.at(i);
        }
        else if ((freqSelection.at(i) == 0 || i == plotResolution - 1) && (int)sel1 != 0) { // first exclude detected
            sel2 = keyValues.at(i);
            tmp << QString::number(sel1, 'f', 3) << QString::number(sel2, 'f', 3);
            sel1 = 0;
        }
    }
    if (tmp.isEmpty() && (int)sel1 != 0) tmp << QString::number(sel1, 'f', 3)
            << QString::number(keyValues.last(), 'f', 3); // all included

    //qDebug() << "trig debug:" << tmp;
    emit reqTrigline(); // to update trig line drawing with new values
    if (!tmp.isEmpty())
        config->setTrigFrequencies(tmp);

    reCalc();
    emit freqSelectionChanged();
}

void CustomPlotController::updSettings()
{
    if (plotResolution == 0) plotResolution = config->getPlotResolution();
    fill.fill(-200, plotResolution);

    if (config->getInstrNormalizeSpectrum())
        customPlotPtr->yAxis->setLabel((config->getUseDbm()?"dBm (normalized)":"dBμV (normalized)"));
    else
        customPlotPtr->yAxis->setLabel((config->getUseDbm()?"dBm":"dBμV"));

    updTextLabelPositions();

    customPlotPtr->yAxis->setRangeLower(config->getPlotYMin());
    customPlotPtr->yAxis->setRangeUpper(config->getPlotYMax());

    reCalc();
    updateSpectrumMarkerLabels();
    customPlotPtr->replot();
    updOverlayText();
    emit reqTrigline(); // to update trig line drawing with new values
}

void CustomPlotController::flashTrigline()
{
    if (deviceConnected) flashTimer->start(500);
}

void CustomPlotController::stopFlashTrigline()
{
    flashTimer->stop();
    customPlotPtr->graph(2)->setVisible(true);
    customPlotPtr->layer("triggerLayer")->replot();
}

void CustomPlotController::flashRoutine()
{
    customPlotPtr->graph(2)->setVisible(flip = !flip);
    customPlotPtr->layer("triggerLayer")->replot();
}

void CustomPlotController::updDeviceConnected(bool b)
{
    deviceConnected = b;
    if (!b) {
        flashTimer->stop();
    }
}

void CustomPlotController::toggleOverlay()
{
    markGnss = !markGnss;

    for (int i=plotIterator, j=gnssBands.size() - 1; i > plotIterator - gnssBands.size(); i--, j--) {
        if (gnssBandsSelectors[j] && markGnss) {
            customPlotPtr->graph(i)->setVisible(markGnss); // only show overlay if not deactivated separately before
            gnssTextLabels[j]->setVisible(markGnss);
            gnssCenterLine[j]->setVisible(markGnss);
        }
        else if (!markGnss) {
            customPlotPtr->graph(i)->setVisible(markGnss);
            gnssTextLabels[j]->setVisible(markGnss);
            gnssCenterLine[j]->setVisible(markGnss);
        }
    }
    customPlotPtr->replot();
}

void CustomPlotController::addOverlay(const int graph, const double startfreq, const double stopfreq)
{
    customPlotPtr->graph(graph)->setData(QVector<double>() << startfreq << stopfreq, QVector<double>() << 200 << 200);
}

void CustomPlotController::updTextLabelPositions()
{
    for (int i=0, j=0; i < gnssTextLabels.size(); i++, j += 2) {
        gnssTextLabels[i]->position->setCoords(gnssBandCenterFreq[i],
                                               customPlotPtr->yAxis->range().upper - 2 - gnssTextLabelPos[i]);
    }

}

void CustomPlotController::updOverlay()
{

}


void CustomPlotController::addSpectrumMarker(double frequencyMhz)
{
    const QVector<QColor> markerColors = { QColor(0, 90, 255), QColor(220, 120, 0), QColor(160, 0, 200) };
    const int markerIndex = spectrumMarkers.size();

    SpectrumMarker marker;
    marker.color = markerColors.at(markerIndex % markerColors.size());

    QPen markerPen(marker.color);
    markerPen.setWidth(2);
    marker.line = new QCPItemStraightLine(customPlotPtr);
    marker.line->setPen(markerPen);
    marker.line->setSelectedPen(markerPen);
    marker.line->setLayer("markerLayer");
    marker.line->setSelectable(false);
    marker.line->point1->setCoords(0, -200);
    marker.line->point2->setCoords(0, 200);

    marker.label = new QCPItemText(customPlotPtr);
    marker.label->setLayer("markerLayer");
    marker.label->setSelectable(false);
    marker.label->setColor(marker.color);
    marker.label->setPen(QPen(marker.color));
    marker.label->setBrush(config->getDarkMode() ? QColor(30, 30, 30, 210) : QColor(255, 255, 255, 220));
    marker.label->setPadding(QMargins(4, 2, 4, 2));
    marker.label->setPositionAlignment(Qt::AlignLeft | Qt::AlignTop);
    marker.label->setTextAlignment(Qt::AlignLeft);
    marker.label->setFont(QFont(QFont().family(), config->getOverlayFontSize()));

    spectrumMarkers.append(marker);
    setSpectrumMarkerFrequency(markerIndex, frequencyMhz);

    customPlotPtr->replot();
}

void CustomPlotController::removeSpectrumMarker(int markerIndex)
{
    if (markerIndex < 0 || markerIndex >= spectrumMarkers.size()) return;

    SpectrumMarker marker = spectrumMarkers.takeAt(markerIndex);
    if (marker.line) customPlotPtr->removeItem(marker.line);
    if (marker.label) customPlotPtr->removeItem(marker.label);

    if (draggedSpectrumMarker == markerIndex) draggedSpectrumMarker = -1;
    else if (draggedSpectrumMarker > markerIndex) --draggedSpectrumMarker;

    updateSpectrumMarkerLabels();
    customPlotPtr->replot();
}

void CustomPlotController::setSpectrumMarkerFrequency(int markerIndex, double frequencyMhz, bool clampToRange)
{
    if (markerIndex < 0 || markerIndex >= spectrumMarkers.size()) return;

    if (clampToRange) {
        const QCPRange range = customPlotPtr->xAxis->range();
        frequencyMhz = std::max(range.lower, std::min(range.upper, frequencyMhz));
    }

    SpectrumMarker &marker = spectrumMarkers[markerIndex];
    marker.frequencyMhz = frequencyMhz;
    marker.line->point1->setCoords(frequencyMhz, customPlotPtr->yAxis->range().lower);
    marker.line->point2->setCoords(frequencyMhz, customPlotPtr->yAxis->range().upper);
    updateSpectrumMarkerLabel(markerIndex);
}

void CustomPlotController::updateSpectrumMarkerLabel(int markerIndex)
{
    if (markerIndex < 0 || markerIndex >= spectrumMarkers.size()) return;

    SpectrumMarker &marker = spectrumMarkers[markerIndex];
    const double level = spectrumMarkerLevel(marker.frequencyMhz);
    const QString levelText = std::isnan(level) ? QStringLiteral("--") : QString::number(level, 'f', 1);
    const QString unit = config->getUseDbm() ? QStringLiteral("dBm") : QStringLiteral("dBμV");
    marker.label->setText(QString("%1 MHz\n%2 %3")
                              .arg(marker.frequencyMhz, 0, 'f', 6)
                              .arg(levelText)
                              .arg(unit));

    const QCPRange yRange = customPlotPtr->yAxis->range();
    const double verticalOffset = yRange.size() * (0.04 + 0.08 * (markerIndex % 10));
    marker.label->position->setCoords(marker.frequencyMhz, yRange.upper - verticalOffset);
}

void CustomPlotController::updateSpectrumMarkerLabels()
{
    for (int i = 0; i < spectrumMarkers.size(); ++i) {
        setSpectrumMarkerFrequency(i, spectrumMarkers[i].frequencyMhz, false);
    }
}

double CustomPlotController::spectrumMarkerLevel(double frequencyMhz) const
{
    if (keyValues.isEmpty() || currentTraceData.isEmpty()) return std::numeric_limits<double>::quiet_NaN();

    const int sampleCount = std::min(keyValues.size(), currentTraceData.size());
    if (sampleCount <= 0) return std::numeric_limits<double>::quiet_NaN();
    if (frequencyMhz <= keyValues.first()) return currentTraceData.first();
    if (frequencyMhz >= keyValues.at(sampleCount - 1)) return currentTraceData.at(sampleCount - 1);

    const auto begin = keyValues.constBegin();
    const auto end = begin + sampleCount;
    const auto it = std::lower_bound(begin, end, frequencyMhz);
    const int upperIndex = static_cast<int>(it - begin);
    if (upperIndex <= 0) return currentTraceData.first();

    const int lowerIndex = upperIndex - 1;
    const double lowerFrequency = keyValues.at(lowerIndex);
    const double upperFrequency = keyValues.at(upperIndex);
    if (upperFrequency <= lowerFrequency) return currentTraceData.at(lowerIndex);

    const double ratio = (frequencyMhz - lowerFrequency) / (upperFrequency - lowerFrequency);
    return currentTraceData.at(lowerIndex) + ratio * (currentTraceData.at(upperIndex) - currentTraceData.at(lowerIndex));
}

int CustomPlotController::spectrumMarkerAt(const QPoint &pos) const
{
    if (!customPlotPtr->axisRect()->rect().contains(pos)) return -1;

    for (int i = 0; i < spectrumMarkers.size(); ++i) {
        const int markerX = customPlotPtr->xAxis->coordToPixel(spectrumMarkers[i].frequencyMhz);
        if (std::abs(markerX - pos.x()) <= 6) return i;

        const double labelDistance = spectrumMarkers[i].label->selectTest(pos, false);
        if (std::isfinite(labelDistance) && labelDistance <= customPlotPtr->selectionTolerance()) return i;
    }

    return -1;
}

void CustomPlotController::freqChanged(double a, double b)
{
    stopFreq = b;
    startFreq = a;

    if (a < b) {
        if (customPlotPtr->xAxis->range().lower > b / 1e6) {
            customPlotPtr->xAxis->setRangeLower(a / 1e6);
            //customPlotPtr->replot();
        }
        customPlotPtr->xAxis->setRangeUpper(b / 1e6);
        customPlotPtr->xAxis->setRangeLower(a / 1e6);
        customPlotPtr->yAxis->setRangeUpper(config->getPlotYMax());
        customPlotPtr->yAxis->setRangeLower(config->getPlotYMin());
        reCalc();
        customPlotPtr->replot();
    }
}

void CustomPlotController::resChanged(double a)
{
    resolution = a / 1e6;
}

void CustomPlotController::reqTracePlot()
{
    // New, remove waterfall before making a snapshot
    mutex.lock();
    customPlotPtr->axisRect()->setBackground(QPixmap(), false);
    bool flagVisible = false;
    if (customPlotPtr->graph(2)->visible()) {
        flagVisible = true;
        customPlotPtr->graph(2)->setVisible(false);
    }
    customPlotPtr->replot();
    *tracePlot = customPlotPtr->toPixmap();
    if (flagVisible) {
        customPlotPtr->graph(2)->setVisible(true);
        customPlotPtr->layer("triggerLayer")->replot();
        customPlotPtr->replot();
    }
    mutex.unlock();
    emit retTracePlot(tracePlot, QDateTime::currentDateTime());
}

void CustomPlotController::updOverlayText()
{
    for (auto && row : gnssTextLabels) {
        row->setFont(QFont(QFont().family(), config->getOverlayFontSize()));
    }
    for (auto && marker : spectrumMarkers) {
        marker.label->setFont(QFont(QFont().family(), config->getOverlayFontSize()));
        marker.label->setBrush(config->getDarkMode() ? QColor(30, 30, 30, 210) : QColor(255, 255, 255, 220));
    }
    customPlotPtr->replot();
}

void CustomPlotController::doubleClickEvent(QMouseEvent *event)
{
    double x = customPlotPtr->xAxis->pixelToCoord(event->pos().x());
    emit centerFrequency(x);
}

void CustomPlotController::mouseWheel(QWheelEvent *event)
{
    QPoint degrees = event->angleDelta() / 8;
    emit scroll(degrees.y() / 15);
}

void CustomPlotController::modeChanged(Instrument::Mode m)
{
    if (m == Instrument::Mode::FFM) {
        centerFreqLine->setVisible(true);
        customPlotPtr->layer("bwLayer")->setVisible(true);
    }
    else {
        centerFreqLine->setVisible(false);
        customPlotPtr->layer("bwLayer")->setVisible(false);
    }
}

void CustomPlotController::ffmCenterFreqChanged(quint64 f)
{
    QPen pen;
    pen.setWidth(1);
    pen.setStyle(Qt::SolidLine);
    pen.setColor(Qt::black);
    centerFreqLine->setPen(pen);
    centerFreqLine->point1->setCoords(f / 1e6, -200);
    centerFreqLine->point2->setCoords(f / 1e6, 200);
    demodBwChanged();
    customPlotPtr->replot();
}

void CustomPlotController::demodBwChanged(quint32 f)
{
    if (!f) { // Reuse bw
        f = m_bandwidth;
    }
    else
        m_bandwidth = f;

    QPen pen;
    pen.setWidth(1);
    pen.setStyle(Qt::SolidLine);
    pen.setColor(Qt::black);

    double start = centerFreqLine->point1->key() - (double)f / 1e3 / 2;
    double stop = centerFreqLine->point1->key() + (double)f / 1e3 / 2;
    customPlotPtr->graph(5)->setPen(pen);
    QBrush brush(Qt::Dense4Pattern);
    if (config->getDarkMode())
        brush.setColor(QColor(217, 217, 217, 90));
    else
        brush.setColor(QColor(67, 67, 67, 90));
    customPlotPtr->graph(5)->setBrush(brush);
    customPlotPtr->graph(5)->setLineStyle(QCPGraph::lsLine);
    customPlotPtr->graph(5)->setLayer("bwLayer");
    customPlotPtr->graph(5)->setData(QVector<double>() << start << stop, QVector<double>() << 200 << 200);
    customPlotPtr->layer("bwLayer")->replot();
}
