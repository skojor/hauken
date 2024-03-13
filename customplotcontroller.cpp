#include "customplotcontroller.h"

CustomPlotController::CustomPlotController(QCustomPlot *ptr, QSharedPointer<Config> c)
{
    customPlotPtr = ptr;
    config = c;
    connect(customPlotPtr, &QCustomPlot::mouseMove, this, &CustomPlotController::showToolTip);
    connect(customPlotPtr->selectionRect(), &QCPSelectionRect::accepted, this, &CustomPlotController::showSelectionMenu);
    connect(customPlotPtr, &QCustomPlot::mouseRelease, this, &CustomPlotController::onMouseClick);
    connect(flashTimer, &QTimer::timeout, this, &CustomPlotController::flashRoutine);

    QFile file(config->getWorkFolder() + "/bandplan.csv");
    if (file.exists()) {
        file.open(QIODevice::ReadOnly);
        while (!file.atEnd()) {
            QString line = file.readLine();
            QStringList split = line.split(',');
            if (!line.contains("#") && split.size() == 6) {
                gnssBands.append(split[0]);
                gnssBandfrequencies.append(split[1].toDouble());
                gnssBandfrequencies.append(split[2].toDouble());
                gnssBandColors.append(QColor(split[3].toInt(), split[4].toInt(), split[5].toInt(), overlayTransparency));
            }
        }
    }
    else {
        qDebug() << "No bandplan file found, creating one";
        file.open(QIODevice::ReadWrite);
        file.write("G1,1593,1610,255,0,0\n");
        file.write("L1/E1,1559,1591,0,0,255\n");
        file.write("E6,1260,1300,0,0,255\n");
        file.write("G2,1237,1254,255,0,0\n");
        file.write("L2,1215,1239.6,0,0,255\n");
        file.write("G3/E5b,1189,1214,255,255,0\n");
        file.write("L5/E5a,1164,1189,255,0,0\n");

        file.seek(0); // start at top to read data to memory
        while (!file.atEnd()) {
            QString line = file.readLine();
            QStringList split = line.split(',');
            if (!line.contains("#") && split.size() == 6) {
                gnssBands.append(split[0]);
                gnssBandfrequencies.append(split[1].toDouble());
                gnssBandfrequencies.append(split[2].toDouble());
                gnssBandColors.append(QColor(split[3].toInt(), split[4].toInt(), split[5].toInt(), overlayTransparency));
            }
        }
    }
    file.close();
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

    customPlotPtr->yAxis->setLabel("dBμV");
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
        //tmp->position->setCoords( gnssBandfrequencies[j] + ((gnssBandfrequencies[j+1] - gnssBandfrequencies[j]) / 2), customPlotPtr->yAxis->range().upper);
        tmp->setText(gnssBands[i]);
        tmp->setFont(QFont(QFont().family(), 16));
        //tmp->setPen(QPen(Qt::gray));
        gnssTextLabels.append(tmp);
        gnssBandsSelectors.append(true);

        QCPItemStraightLine *tmpLine = new QCPItemStraightLine(customPlotPtr);
        pen.setStyle(Qt::DotLine);
        pen.setColor(Qt::gray);
        tmpLine->setPen(pen);
        tmpLine->point1->setCoords( gnssBandfrequencies[j] + ((gnssBandfrequencies[j+1] - gnssBandfrequencies[j]) / 2), 200);
        tmpLine->point2->setCoords( gnssBandfrequencies[j] + ((gnssBandfrequencies[j+1] - gnssBandfrequencies[j]) / 2), -200);
        gnssCenterLine.append(tmpLine);
    }
    toggleOverlay();
    updTextLabelPositions();
}

void CustomPlotController::plotTrace(const QVector<double> &data)
{
    customPlotPtr->graph(0)->setData(keyValues, data);
}

void CustomPlotController::plotMaxhold(const QVector<double> &data)
{
    customPlotPtr->graph(1)->setData(keyValues, data);
}

void CustomPlotController::plotTriglevel(const QVector<double> &data)
{
    QVector<double> copy;

    if (!data.isEmpty() && !freqSelection.isEmpty()) {
        copy = data;
        for (int i=0; i<plotResolution; i++) {
            if (!freqSelection.at(i))
                copy[i] = -100;
        }
    }
    customPlotPtr->graph(2)->setData(keyValues, copy);
}

void CustomPlotController::doReplot()
{
    customPlotPtr->layer("liveGraph")->replot();
}

void CustomPlotController::reCalc()
{
    if ((int)(resolution * 1e6) > 0 && customPlotPtr->xAxis->range().upper > 0 && customPlotPtr->xAxis->range().lower > 0) {
        nrOfValues = 1 + ( 1e3 * (customPlotPtr->xAxis->range().upper - customPlotPtr->xAxis->range().lower) / resolution);
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
            customPlotPtr->layer("triggerLayer")->replot();
        }
    }
}

void CustomPlotController::showToolTip(QMouseEvent *event)
{
    double x = customPlotPtr->xAxis->pixelToCoord(event->pos().x());
    double y = customPlotPtr->yAxis->pixelToCoord(event->pos().y());

    QString text = QString::number(x, 'f', 3) + " MHz / " +
                   QString::number(y, 'f', 1) + " dBuV";
    customPlotPtr->setToolTip(text);
}

void CustomPlotController::onMouseClick(QMouseEvent *event)
{
    (void)event;
    if (event->button() == Qt::RightButton) {

        QMenu menu;
        menu.addAction("Exclude all trigger frequencies", this, &CustomPlotController::trigExcludeAll);
        menu.addAction("Include all trigger frequencies", this, &CustomPlotController::trigIncludeAll);

        menu.addSeparator();
        menu.addAction("Show/hide GNSS frequency spectrum overlay", this, &CustomPlotController::toggleOverlay);

        for (int i=0; i < gnssBandsSelectors.size(); i++) {
            menu.addAction("Show/hide " + gnssBands[i], this, [this, i] () {
                gnssBandsSelectors[i] = !gnssBandsSelectors[i];
                customPlotPtr->graph(customPlotPtr->graphCount() - gnssBands.size() + i)->setVisible(gnssBandsSelectors[i]);
                gnssTextLabels[i]->setVisible(gnssBandsSelectors[i]);
                gnssCenterLine[i]->setVisible(gnssBandsSelectors[i]);
                customPlotPtr->replot();
            });
        }

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
        if (selMax >= stopFreq) selMax = keyValues.last(); //stopFreq - resolution / 1000;
        //qDebug() << selMin << selMax;

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
    plotResolution = config->getPlotResolution();
    fill.fill(-200, plotResolution);

    if ((int)customPlotPtr->xAxis->range().lower != (int)config->getInstrStartFreq()
        || (int)customPlotPtr->xAxis->range().upper != (int)config->getInstrStopFreq()
        || (int)(resolution * 1000) != (int)(config->getInstrResolution().toDouble() * 1000)
        || customPlotPtr->yAxis->range().lower != config->getPlotYMin()
        || customPlotPtr->yAxis->range().upper != config->getPlotYMax()) {
        startFreq = config->getInstrStartFreq();
        stopFreq = config->getInstrStopFreq();
        resolution = config->getInstrResolution().toDouble();
        customPlotPtr->xAxis->setRangeLower(startFreq);
        customPlotPtr->xAxis->setRangeUpper(stopFreq);
        customPlotPtr->yAxis->setRangeLower(config->getPlotYMin());
        customPlotPtr->yAxis->setRangeUpper(config->getPlotYMax());
        reCalc();
        customPlotPtr->replot();
    }
    if (config->getInstrNormalizeSpectrum())
        customPlotPtr->yAxis->setLabel("dBμV (normalized)");
    else
        customPlotPtr->yAxis->setLabel("dBμV");
    // qDebug() << customPlotPtr->axisRect()->rect();
    updTextLabelPositions();
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
        gnssTextLabels[i]->position->setCoords( gnssBandfrequencies[j] + ((gnssBandfrequencies[j+1] - gnssBandfrequencies[j]) / 2),
                                               customPlotPtr->yAxis->range().upper - 5);

    }

}

void CustomPlotController::updOverlay()
{
}
