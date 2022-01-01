#include "customplotcontroller.h"

CustomPlotController::CustomPlotController(QCustomPlot *ptr, QSharedPointer<Config> c)
{
    customPlotPtr = ptr;
    config = c;
    connect(customPlotPtr, &QCustomPlot::mouseMove, this, &CustomPlotController::showToolTip);
    connect(customPlotPtr->selectionRect(), &QCPSelectionRect::accepted, this, &CustomPlotController::showSelectionMenu);
    connect(flashTimer, &QTimer::timeout, this, &CustomPlotController::flashRoutine);
}

void CustomPlotController::init()
{
    setupBasics();
    updSettings();
}

void CustomPlotController::setupBasics()
{
    customPlotPtr->addGraph();
    customPlotPtr->addGraph();
    customPlotPtr->addGraph();
    customPlotPtr->addGraph();
    customPlotPtr->yAxis->setLabel("dBμV");
    customPlotPtr->xAxis->setLabel("MHz");
    customPlotPtr->graph(0)->setPen(QPen(Qt::blue));
    customPlotPtr->graph(1)->setPen(QPen(Qt::red));
    customPlotPtr->graph(2)->setSelectable(QCP::stWhole);
    customPlotPtr->graph(2)->setLineStyle(QCPGraph::lsLine);
    QPen pen;
    pen.setWidth(1);
    pen.setColor(QColor(0, 100, 0, 255));
    customPlotPtr->graph(2)->setPen(pen);
    customPlotPtr->graph(2)->setBrush(QColor(0, 150, 0, 30));
    customPlotPtr->graph(2)->setChannelFillGraph(customPlotPtr->graph(3));

    customPlotPtr->setInteraction(QCP::iRangeDrag, true);
    customPlotPtr->setSelectionRectMode(QCP::srmCustom);
    //customPlotPtr->setOpenGl(false);
    customPlotPtr->setNotAntialiasedElements(QCP::aePlottables);

    fill.fill(-200, screenResolution);
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

    if (!data.isEmpty()) {
        copy = data;
        for (int i=0; i<screenResolution; i++) {
            if (!freqSelection.at(i))
                copy[i] = -100;
        }
    }
    customPlotPtr->graph(2)->setData(keyValues, copy);
}

void CustomPlotController::doReplot()
{
    customPlotPtr->replot();
}

void CustomPlotController::reCalc()
{
    if ((int)(resolution * 1e6) > 0 && customPlotPtr->xAxis->range().upper > 0 && customPlotPtr->xAxis->range().lower > 0) {
        nrOfValues = 1 + ((customPlotPtr->xAxis->range().upper - customPlotPtr->xAxis->range().lower) / resolution);
        if (nrOfValues > 1) {
            double rate = (customPlotPtr->xAxis->range().upper - customPlotPtr->xAxis->range().lower) / screenResolution;
            keyValues.clear();
            double freq = customPlotPtr->xAxis->range().lower;

            for (int i = 0; i < screenResolution; i++) {
                keyValues.append(freq);
                freq += rate;
            }
            readTrigSelectionFromConfig();
            customPlotPtr->graph(3)->setData(keyValues, fill);
            customPlotPtr->replot();
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

void CustomPlotController::showSelectionMenu(const QRect &rect, QMouseEvent *event)
{
    selMin = customPlotPtr->xAxis->pixelToCoord(rect.x());
    selMax = customPlotPtr->xAxis->pixelToCoord(rect.x() + rect.width());
    if (selMin > selMax) { // some smartypant tried to reverse the selection
        double tmp = selMin;
        selMin = selMax;
        selMax = tmp;
    }
    QMenu *menu = new QMenu;
    menu->addAction("Include " + QString::number(selMin, 'f', 3) +
                    "-" + QString::number(selMax, 'f', 3) + " MHz in the trigger area",
                    this, &CustomPlotController::trigInclude);
    menu->addAction("Exclude " + QString::number(selMin, 'f', 3) +
                    "-" + QString::number(selMax, 'f', 3) + " MHz in the trigger area",
                    this, &CustomPlotController::trigExclude);
    menu->addAction("Include all", this, &CustomPlotController::trigIncludeAll);
    menu->addAction("Exclude all", this, &CustomPlotController::trigExcludeAll);
    menu->exec(customPlotPtr->mapToGlobal(event->pos()));
}

void CustomPlotController::trigInclude()
{
    for (int i=0; i<screenResolution; i++) {
        if (keyValues.at(i) > selMin && keyValues.at(i) < selMax) {
            freqSelection[i] = 1;
        }
    }
    saveTrigSelectionToConfig();
}

void CustomPlotController::trigExclude()
{
    for (int i=0; i<screenResolution; i++) {
        if (keyValues.at(i) > selMin && keyValues.at(i) < selMax) {
            freqSelection[i] = 0;
        }
    }
    saveTrigSelectionToConfig();
}

void CustomPlotController::trigIncludeAll()
{
    for (int i=0; i<screenResolution; i++) {
        freqSelection[i] = 1;
    }
    saveTrigSelectionToConfig();
}

void CustomPlotController::trigExcludeAll()
{
    for (int i=0; i<screenResolution; i++) {
        freqSelection[i] = 0;
    }
    //saveTrigSelectionToConfig();
    QStringList tmp = { "0", "0" };
    config->setTrigFrequencies(tmp); // deletes all settings
}

void CustomPlotController::readTrigSelectionFromConfig()
{
    freqSelection.fill(0, screenResolution);
    QStringList freqList = config->getTrigFrequencies();

    if (!freqList.isEmpty()) {
        double range1 = 0, range2 = 0;
        for (int i=0; i<freqList.size();) {
            range1 = freqList.at(i++).toDouble();
            range2 = freqList.at(i++).toDouble();
            for (int j=0; j<screenResolution; j++) {
                if (keyValues.at(j) > range1 && keyValues.at(j) < range2)
                    freqSelection[j] = 1;
            }
        }
    }
    else { // initialize list in config, it shouldn't be empty
        freqSelection.fill(1, screenResolution);
        freqList << "0" << "9999";
        config->setTrigFrequencies(freqList);
    }
}

void CustomPlotController::saveTrigSelectionToConfig()
{
    QStringList tmp;

    double sel1 = 0, sel2 = 0;
    for (int i=0; i<screenResolution; i++) {
        if (freqSelection.at(i) == 1 && (int)sel1 == 0) { // first include value
            sel1 = keyValues.at(i);
        }
        else if (freqSelection.at(i) == 0 && (int)sel1 != 0) { // first exclude detected
            sel2 = keyValues.at(i);
            tmp << QString::number(sel1, 'f', 3) << QString::number(sel2, 'f', 3);
            sel1 = 0;
        }
    }
    if (tmp.isEmpty() && (int)sel1 != 0) tmp << QString::number(sel1, 'f', 3)
                                             << QString::number(keyValues.last(), 'f', 3); // all included

    emit reqTrigline(); // to update trig line drawing with new values
    if (!tmp.isEmpty())
        config->setTrigFrequencies(tmp);

    emit freqSelectionChanged();
}

void CustomPlotController::updSettings()
{
    if ((int)customPlotPtr->xAxis->range().lower != (int)config->getInstrStartFreq()
            || (int)customPlotPtr->xAxis->range().upper != (int)config->getInstrStopFreq()
            || (int)(resolution * 1000) != (int)(config->getInstrResolution() * 1000)
            || customPlotPtr->yAxis->range().lower != config->getPlotYMin()
            || customPlotPtr->yAxis->range().upper != config->getPlotYMax()) {
        startFreq = config->getInstrStartFreq();
        stopFreq = config->getInstrStopFreq();
        resolution = config->getInstrResolution();
        customPlotPtr->xAxis->setRangeLower(startFreq);
        customPlotPtr->xAxis->setRangeUpper(stopFreq);
        customPlotPtr->yAxis->setRangeLower(config->getPlotYMin());
        customPlotPtr->yAxis->setRangeUpper(config->getPlotYMax());
        reCalc();
    }
}

void CustomPlotController::flashTrigline()
{
    flashTimer->start(500);
}

void CustomPlotController::stopFlashTrigline(const QVector<qint16>)
{
    flashTimer->stop();
    customPlotPtr->graph(2)->setVisible(true);  //setBrush(colorNormal);
}

void CustomPlotController::flashRoutine()
{
    if (flip) customPlotPtr->graph(2)->setVisible(false); //customPlotPtr->graph(2)->setBrush(colorNormal);
    else customPlotPtr->graph(2)->setVisible(true);
    flip = !flip;
    customPlotPtr->replot();
}
