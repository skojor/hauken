#include "gnssdisplay.h"

namespace {
QString coloredText(const QString &text, const QString &color)
{
    return QString("<span style='font-size:normal;color:%1;'>%2</span>").arg(color, text.toHtmlEscaped());
}

QString formatOffset(const PpsOffsetData &offset)
{
    if (!offset.valid) return coloredText("n/a", "red");
    return coloredText(QString::number(offset.currentNs / 1000.0, 'f', 3) + " us", "green");
}

QString formatFixType(int fixMode)
{
    switch (fixMode) {
    case 2: return "2D";
    case 3: return "3D";
    default: return "no fix";
    }
}

QString formatSource(const PpsSourceData &source, bool showMetadata)
{
    if (!source.valid) return coloredText(source.reason.isEmpty() ? "invalid" : source.reason, "red");

    QString text = QString("valid, age %1 s").arg(source.ageNs / 1e9, 0, 'f', 3);
    if (showMetadata) {
        if (source.metadataValid)
            text += QString(", fix %1").arg(formatFixType(source.fixMode));
        else
            text += ", metadata invalid";
    }
    return coloredText(text, source.metadataValid || !showMetadata ? "green" : "orange");
}
}

GnssDisplay::GnssDisplay(QSharedPointer<Config> c)
{
    config = c;
    wdg->setAttribute(Qt::WA_QuitOnClose);
    wdg->installEventFilter(this);

    connect(updateGnssDataTimer, &QTimer::timeout, this, &GnssDisplay::reqGnssData);
    connect(updateGnssDataTimer, &QTimer::timeout, this, &GnssDisplay::updatePpsText);
    ppsPlotTimer->setInterval(1000);
    connect(ppsPlotTimer, &QTimer::timeout, this, &GnssDisplay::updatePpsPlot);
}

void GnssDisplay::start()
{
    setupWidget();
    if (config->getGnssPpsPlotEnabled()) {
        setupPpsPlot();
        ppsPlotWindow->show();
        ppsPlotTimer->start();
    }
    updText();
    updateGnssDataTimer->start(250);
}

void GnssDisplay::close()
{
    isClosing = true;   // don't reopen widget after shutdown

    qDebug() << "Closing Gnss Display widget...";
    config->setGnssDisplayWindowState(wdg->saveGeometry());
    wdg->close();
    if (ppsPlotWindow) ppsPlotWindow->close();

}

void GnssDisplay::updGnssData(GnssData g, int id)
{
    if (id == 1) gnss1 = g;
    else if (id == 2) gnss2 = g;
    else instrumentGnss = g;

    updText();
}

void GnssDisplay::updPpsData(const PpsData &data)
{
    ppsData = data;
    ppsDataReceived = true;
    ppsLastError.clear();
    updatePpsText();
}

void GnssDisplay::updPpsAvailability(bool online)
{
    ppsAvailabilityKnown = true;
    ppsOnline = online;
    updatePpsText();
}

void GnssDisplay::updPpsError(const QString &error)
{
    ppsLastError = error;
    updatePpsText();
}

void GnssDisplay::setupWidget()
{
    //wdg->resize(640, 480);
    wdg->setWindowTitle("GNSS data");
    QGridLayout *mainLayout = new QGridLayout;

    gnss1LeftGroupBox->setTitle("GNSS receiver 1");
    QFormLayout *gnss1LeftLayout = new QFormLayout;
    gnss1LeftLayout->addRow("Latitude", gnss1Latitude);
    gnss1LeftLayout->addRow("Longitude", gnss1Longitude);
    gnss1LeftLayout->addRow("Altitude", gnss1Altitude);
    gnss1LeftLayout->addRow("GNSS time", gnss1Time);
    gnss1LeftLayout->addRow("DOP", gnss1Dop);
    gnss1LeftLayout->addRow("C/No", gnss1CNo);
    //gnss1LeftLayout->addRow("AGC", gnss1Agc);
    gnss1LeftLayout->addRow("Sats tracked", gnss1Sats);
    //gnss1LeftLayout->setFormAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    gnss1LeftGroupBox->setLayout(gnss1LeftLayout);

    gnss1RightGroupBox->setTitle("Info/calculations");
    QFormLayout *gnss1RightLayout = new QFormLayout;
    gnss1RightLayout->addRow("Position valid", gnss1PosValid);
    gnss1RightLayout->addRow("Position offset", gnss1PosOffset);
    gnss1RightLayout->addRow("Altitude offset", gnss1AltOffset);
    gnss1RightLayout->addRow("Time offset", gnss1TimeOffset);
    gnss1RightLayout->addRow("AGC", gnss1Agc);
    /*gnss1RightLayout->addRow("CW jamming indicator", gnss1CwJamming);
    gnss1RightLayout->addRow("Jamming state", gnss1JammingState);*/
    gnss1RightGroupBox->setLayout(gnss1RightLayout);

    gnss2LeftGroupBox->setTitle("GNSS receiver 2");
    QFormLayout *gnss2LeftLayout = new QFormLayout;
    gnss2LeftLayout->addRow("Latitude", gnss2Latitude);
    gnss2LeftLayout->addRow("Longitude", gnss2Longitude);
    gnss2LeftLayout->addRow("Altitude", gnss2Altitude);
    gnss2LeftLayout->addRow("GNSS time", gnss2Time);
    gnss2LeftLayout->addRow("DOP", gnss2Dop);
    gnss2LeftLayout->addRow("C/No", gnss2CNo);
    gnss2LeftLayout->addRow("Sats tracked", gnss2Sats);
    //gnss1LeftLayout->setFormAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    gnss2LeftGroupBox->setLayout(gnss2LeftLayout);

    gnss2RightGroupBox->setTitle("Info/calculations");
    QFormLayout *gnss2RightLayout = new QFormLayout;
    gnss2RightLayout->addRow("Position valid", gnss2PosValid);
    gnss2RightLayout->addRow("Position offset", gnss2PosOffset);
    gnss2RightLayout->addRow("Altitude offset", gnss2AltOffset);
    gnss2RightLayout->addRow("Time offset", gnss2TimeOffset);
    gnss2RightLayout->addRow("AGC", gnss2Agc);
    /*gnss2RightLayout->addRow("CW jamming indicator", gnss2CwJamming);
    gnss2RightLayout->addRow("Jamming state", gnss2JammingState);*/
    gnss2RightGroupBox->setLayout(gnss2RightLayout);

    mainLayout->addWidget(gnss1LeftGroupBox, 0, 0);
    mainLayout->addWidget(gnss1RightGroupBox, 0, 1);

    mainLayout->addWidget(gnss2LeftGroupBox, 1, 0);
    mainLayout->addWidget(gnss2RightGroupBox, 1, 1);

    ppsGroupBox->setTitle(tr("PPS timing"));
    auto ppsLayout = new QGridLayout(ppsGroupBox);
    auto ppsSourceLayout = new QFormLayout;
    ppsSourceLayout->addRow(tr("Broker/daemon"), ppsAvailability);
    ppsSourceLayout->addRow(tr("Reference"), ppsReference);
    ppsSourceLayout->addRow(tr("GPS"), ppsGps);
    ppsSourceLayout->addRow(tr("Galileo"), ppsGalileo);
    ppsSourceLayout->addRow(tr("Backend"), ppsBackend);
    ppsSourceLayout->addRow(tr("Qualified"), ppsQualified);

    auto ppsOffsetLayout = new QFormLayout;
    ppsOffsetLayout->addRow(tr("GPS - reference"), ppsGpsReference);
    ppsOffsetLayout->addRow(tr("Galileo - reference"), ppsGalileoReference);
    ppsOffsetLayout->addRow(tr("Galileo - GPS"), ppsGalileoGps);
    ppsOffsetLayout->addRow(tr("Jitter GPS / Galileo"), ppsJitter);
    ppsOffsetLayout->addRow(tr("Samples GPS / Galileo"), ppsSamples);
    ppsOffsetLayout->addRow(tr("Status"), ppsError);
    ppsLayout->addLayout(ppsSourceLayout, 0, 0);
    ppsLayout->addLayout(ppsOffsetLayout, 0, 1);
    mainLayout->addWidget(ppsGroupBox, 2, 0, 1, 2);
    wdg->setLayout(mainLayout);
    updateReceiverVisibility();
    //wdg->adjustSize();
}

bool GnssDisplay::eventFilter(QObject *obj, QEvent *event)
{
    if (obj == wdg && event->type() == QEvent::Close) {
        config->setGnssDisplayWindowState(wdg->saveGeometry());
        if (!isClosing && config->getGnssDisplayWidget())
            config->setGnssDisplayWidget(false);
    }

    return QObject::eventFilter(obj, event);
}

void GnssDisplay::updateReceiverVisibility()
{
    const bool useInstrumentGnss = config->getGnssUseInstrumentGnss();
    const bool showGnss1 = useInstrumentGnss || config->getGnssSerialPort1Activate();
    const bool showGnss2 = !useInstrumentGnss && config->getGnssSerialPort2Activate();

    gnss1LeftGroupBox->setVisible(showGnss1);
    gnss1RightGroupBox->setVisible(showGnss1);
    gnss2LeftGroupBox->setVisible(showGnss2);
    gnss2RightGroupBox->setVisible(showGnss2);
    ppsGroupBox->setVisible(config->getGnssPpsDisplayEnabled());
}

void GnssDisplay::setupPpsPlot()
{
    if (ppsPlotWindow) return;

    ppsPlotWindow = new QWidget;
    ppsPlotWindow->setWindowTitle(tr("PPS difference"));
    ppsPlot = new QCustomPlot(ppsPlotWindow);
    ppsPlot->addGraph();
    ppsPlot->addGraph();
    ppsPlot->graph(0)->setPen(QPen(Qt::blue));
    ppsPlot->graph(0)->setName(tr("Reference - GPS"));
    ppsPlot->graph(1)->setPen(QPen(Qt::red));
    ppsPlot->graph(1)->setName(tr("Reference - Galileo"));
    ppsPlot->legend->setVisible(true);
    ppsPlot->xAxis->setLabel(tr("Seconds"));
    ppsPlot->yAxis->setLabel(tr("Difference (us)"));
    ppsPlot->xAxis->setRange(0, 10);

    auto resetButton = new QPushButton(tr("Reset plot"), ppsPlotWindow);
    connect(resetButton, &QPushButton::clicked, this, &GnssDisplay::resetPpsPlot);
    auto layout = new QVBoxLayout(ppsPlotWindow);
    layout->addWidget(ppsPlot);
    layout->addWidget(resetButton);
    ppsPlotWindow->resize(800, 500);
}

void GnssDisplay::resetPpsPlot()
{
    if (!ppsPlot) return;

    ppsPlotElapsedSeconds = 0;
    ppsPlot->graph(0)->data()->clear();
    ppsPlot->graph(1)->data()->clear();
    ppsPlot->xAxis->setRange(0, 10);
    ppsPlot->yAxis->setRange(-1, 1);
    ppsPlot->replot();
}

void GnssDisplay::updatePpsPlot()
{
    if (!config->getGnssPpsPlotEnabled() || !ppsPlot) return;

    ++ppsPlotElapsedSeconds;
    if (!ppsDataReceived || !ppsData.gpsReference.valid || !ppsData.galileoReference.valid) return;

    const double x = ppsPlotElapsedSeconds;
    ppsPlot->graph(0)->addData(x, -ppsData.gpsReference.currentNs / 1000.0);
    ppsPlot->graph(1)->addData(x, -ppsData.galileoReference.currentNs / 1000.0);
    ppsPlot->yAxis->rescale(true);
    ppsPlot->xAxis->setRange(qMax(0.0, x - 60.0), qMax(10.0, x), Qt::AlignRight);
    ppsPlot->replot(QCustomPlot::rpQueuedReplot);
}

void GnssDisplay::updSettings()
{
    if (!isClosing && config->getGnssDisplayWidget() && !wdg->isVisible()) {
        wdg->show();
        wdg->restoreGeometry(config->getGnssDisplayWindowState());
    }
    else if (!config->getGnssDisplayWidget() && wdg->isVisible()) wdg->close();
    updateReceiverVisibility();
    gnss1Name = config->getGnssUseInstrumentGnss() ? tr("Instrument GNSS") : config->getGnss1Name();
    gnss2Name = config->getGnss2Name();
    if (gnss1Name.isEmpty()) gnss1RightGroupBox->setTitle("GNSS receiver 1 - info/calculations");
    else gnss1RightGroupBox->setTitle(gnss1Name);
    if (gnss2Name.isEmpty()) gnss2RightGroupBox->setTitle("GNSS receiver 2 - info/calculations");
    else gnss2RightGroupBox->setTitle(gnss2Name);
    if (config->getGnssPpsPlotEnabled()) {
        setupPpsPlot();
        ppsPlotWindow->show();
        ppsPlotTimer->start();
    }
    else {
        ppsPlotTimer->stop();
        if (ppsPlotWindow) ppsPlotWindow->hide();
    }
    updatePpsText();
}

void GnssDisplay::updatePpsText()
{
    ppsGroupBox->setVisible(config->getGnssPpsDisplayEnabled());
    if (!config->getGnssPpsDisplayEnabled()) return;

    const qint64 ageMs = ppsDataReceived
                             ? qAbs(ppsData.receivedAtUtc.msecsTo(QDateTime::currentDateTimeUtc()))
                             : 0;
    const bool stale = !ppsDataReceived || ageMs > config->getGnssPpsStaleTimeoutSec() * 1000LL;
    if (!ppsAvailabilityKnown)
        ppsAvailability->setText(coloredText("unknown", "orange"));
    else if (!ppsOnline)
        ppsAvailability->setText(coloredText("offline", "red"));
    else if (stale)
        ppsAvailability->setText(coloredText("online, data stale", "orange"));
    else
        ppsAvailability->setText(coloredText(QString("online, data age %1 s").arg(ageMs / 1000.0, 0, 'f', 1), "green"));

    if (!ppsDataReceived) {
        const QString unavailable = coloredText("n/a", "red");
        ppsReference->setText(unavailable);
        ppsGps->setText(unavailable);
        ppsGalileo->setText(unavailable);
        ppsGpsReference->setText(unavailable);
        ppsGalileoReference->setText(unavailable);
        ppsGalileoGps->setText(unavailable);
        ppsJitter->setText(unavailable);
        ppsSamples->setText(unavailable);
        ppsBackend->setText(unavailable);
        ppsQualified->setText(unavailable);
        ppsError->setText(ppsLastError.isEmpty() ? unavailable : coloredText(ppsLastError, "red"));
        return;
    }

    ppsReference->setText(formatSource(ppsData.reference, false));
    ppsGps->setText(formatSource(ppsData.gps, true));
    ppsGalileo->setText(formatSource(ppsData.galileo, true));
    ppsGpsReference->setText(formatOffset(ppsData.gpsReference));
    ppsGalileoReference->setText(formatOffset(ppsData.galileoReference));
    ppsGalileoGps->setText(formatOffset(ppsData.galileoGps));
    ppsJitter->setText(QString("%1 / %2 us")
                           .arg(ppsData.gpsReference.stddevNs / 1000.0, 0, 'f', 3)
                           .arg(ppsData.galileoReference.stddevNs / 1000.0, 0, 'f', 3));
    ppsSamples->setText(QString("%1 / %2")
                            .arg(ppsData.gpsReference.samples)
                            .arg(ppsData.galileoReference.samples));
    ppsBackend->setText(ppsData.backend.toHtmlEscaped());
    ppsQualified->setText(ppsData.qualified ? coloredText("yes", "green") : coloredText("no", "orange"));

    QString status = ppsLastError;
    if (status.isEmpty()) status = ppsData.error;
    if (stale && status.isEmpty()) status = tr("Data is stale");
    ppsError->setText(status.isEmpty() ? coloredText("normal", "green") : coloredText(status, stale ? "orange" : "red"));
}

void GnssDisplay::updText()
{
    const bool useInstrumentGnss = config->getGnssUseInstrumentGnss();
    GnssData displayGnss1 = useInstrumentGnss ? instrumentGnss : gnss1;
    updateReceiverVisibility();
    updatePpsText();

    if (displayGnss1.posValid) {
        if (gnss1Name.isEmpty()) gnss1LeftGroupBox->setTitle("GNSS receiver 1 - position valid");
        else  gnss1LeftGroupBox->setTitle(gnss1Name + " - position valid");
        gnss1Latitude->setText("<span style='font-size:normal;'>" + QString::number(displayGnss1.latitude, 'f', 7) + "</span>");
        gnss1Longitude->setText("<span style='font-size:normal;'>" + QString::number(displayGnss1.longitude, 'f', 7) + "</span>");
        gnss1Altitude->setText("<span style='font-size:normal;'>" + QString::number(displayGnss1.altitude, 'f', 0) + "</span>");
        gnss1Time->setText("<span style='font-size:normal;'>" + displayGnss1.timestamp.toString("dd.MM.yy HH:mm:ss") + "</span>");
        gnss1Dop->setText("<span style='font-size:normal;'>" + QString::number(displayGnss1.hdop, 'f', 1) + "</span>");
        gnss1CNo->setText("<span style='font-size:normal;'>" + QString::number(displayGnss1.cno) + "</span>");
        gnss1Agc->setText("<span style='font-size:normal;'>" + QString::number(displayGnss1.agc) + " %</span>");
        gnss1Sats->setText("<span style='font-size:normal;'>" + QString::number(displayGnss1.satsTracked) + "</span>");

        gnss1PosValid->setText("<span style='font-size:normal;color:green;'>yes, tracking " + QString::number(displayGnss1.satsTracked) + " sats</span>");

        if (abs(displayGnss1.posOffset) < 10) gnss1PosOffset->setText("<span style='font-size:normal;color:green;'>Normal, &lt; 10 m</span>");
        else if (abs(displayGnss1.posOffset) < 50) gnss1PosOffset->setText("<span style='font-size:normal;color:orange;'>" + QString::number(abs(displayGnss1.posOffset), 'f', 0) + " m</span>");
        else if (abs(displayGnss1.posOffset) < 1000) gnss1PosOffset->setText("<span style='font-size:normal;color:red;'>" + QString::number(abs(displayGnss1.posOffset), 'f', 0) + " m</span>");
        else gnss1PosOffset->setText("<span style='font-size:normal;color:red;'>" + QString::number(abs(displayGnss1.posOffset / 1000), 'f', 0) + " km</span>");

        if (abs(displayGnss1.altOffset) < 10) gnss1AltOffset->setText("<span style='font-size:normal;color:green;'>Normal, &lt; 10 m</span>");
        else if (abs(displayGnss1.altOffset) < 50) gnss1AltOffset->setText("<span style='font-size:normal;color:orange;'>" + QString::number(abs(displayGnss1.altOffset), 'f', 0) + " m</span>");
        else if (abs(displayGnss1.altOffset) < 1000) gnss1AltOffset->setText("<span style='font-size:normal;color:red;'>" + QString::number(abs(displayGnss1.altOffset), 'f', 0) + " m</span>");
        else gnss1AltOffset->setText("<span style='font-size:normal;color:red;'>" + QString::number(abs(displayGnss1.altOffset / 1000), 'f', 0) + " km</span>");

        if (displayGnss1.timeOffset < 1000) gnss1TimeOffset->setText("<span style='font-size:normal;color:green;'>0 secs</span>");
        else if (displayGnss1.timeOffset < 5000) gnss1TimeOffset->setText("<span style='font-size:normal;color:orange;'>" + QString::number(displayGnss1.timeOffset / 1000) + " sec(s)</span>");
        else if (displayGnss1.timeOffset < 60000) gnss1TimeOffset->setText("<span style='font-size:normal;color:red;'>" + QString::number(displayGnss1.timeOffset / 1000) + " sec(s)</span>");
        else if (displayGnss1.timeOffset < 60 * 60000) gnss1TimeOffset->setText("<span style='font-size:normal;color:red;'>" + QString::number(displayGnss1.timeOffset / 60000) + " min(s)</span>");
        else if (displayGnss1.timeOffset < 60 * 60 * 60000) gnss1TimeOffset->setText("<span style='font-size:normal;color:red;'>" + QString::number(displayGnss1.timeOffset / 3600000) + " hour(s)</span>");
        else if (displayGnss1.timeOffset < 24UL * 60 * 60 * 60000) gnss1TimeOffset->setText("<span style='font-size:normal;color:red;'>" + QString::number(displayGnss1.timeOffset / 24UL * 3600000) + " hour(s)</span>");

        /*gnss1CwJamming->setText("<span style='font-size:normal;'>" + QString::number(gnss1.jammingIndicator) + "</span>");
        QString jam;
        if (gnss1.jammingState == JAMMINGSTATE::NOJAMMING) jam = "No jamming";
        else if (gnss1.jammingState == JAMMINGSTATE::WARNINGFIXOK) jam = "Warning, fix ok";
        else if (gnss1.jammingState == JAMMINGSTATE::CRITICALNOFIX) jam = "Critical, no fix";
        else jam = "Unknown";
        gnss1JammingState->setText(tr("<span style='font-size:normal;'>") + jam + tr("</span>"));*/

    }
    else {
        if (gnss1Name.isEmpty()) gnss1LeftGroupBox->setTitle("GNSS receiver 1 - position invalid");
        else  gnss1LeftGroupBox->setTitle(gnss1Name + " - position invalid");
        gnss1Latitude->setText("<span style='font-size:normal;'>n/a</span>");
        gnss1Longitude->setText("<span style='font-size:normal;'>n/a</span>");
        gnss1Altitude->setText("<span style='font-size:normal;'>n/a</span>");
        gnss1Time->setText("<span style='font-size:normal;'>n/a</span>");
        gnss1Dop->setText("<span style='font-size:normal;'>n/a</span>");
        gnss1CNo->setText("<span style='font-size:normal;'>n/a</span>");
        gnss1Sats->setText("<span style='font-size:normal;'>n/a</span>");

        gnss1PosValid->setText("<span style='font-size:normal;color:red;'>no</span>");
        gnss1Agc->setText("<span style='font-size:normal;'>" + QString::number(displayGnss1.agc) + " %</span>");

        gnss1PosOffset->setText(QString("<span style='font-size:normal;color:red;'>n/a</span>"));
        gnss1AltOffset->setText("<span style='font-size:normal;color:red;'>n/a</span>");
        gnss1TimeOffset->setText("<span style='font-size:normal;color:red;'>n/a</span>");
        /*gnss1CwJamming->setText("<span style='font-size:normal;'> n/a</span>");
        gnss1JammingState->setText("<span style='font-size:normal;'> n/a</span>");*/
    }

    if (gnss2.posValid) {
        if (gnss2Name.isEmpty()) gnss2LeftGroupBox->setTitle("GNSS receiver 2 - position valid");
        else  gnss2LeftGroupBox->setTitle(gnss2Name + " - position valid");
        gnss2Latitude->setText("<span style='font-size:normal;'>" + QString::number(gnss2.latitude, 'f', 7) + "</span>");
        gnss2Longitude->setText("<span style='font-size:normal;'>" + QString::number(gnss2.longitude, 'f', 7) + "</span>");
        gnss2Altitude->setText("<span style='font-size:normal;'>" + QString::number(gnss2.altitude, 'f', 0) + "</span>");
        gnss2Time->setText("<span style='font-size:normal;'>" + gnss2.timestamp.toString("dd.MM.yy HH:mm:ss") + "</span>");
        gnss2Dop->setText("<span style='font-size:normal;'>" + QString::number(gnss2.hdop, 'f', 1) + "</span>");
        gnss2CNo->setText("<span style='font-size:normal;'>" + QString::number(gnss2.cno) + "</span>");
        gnss2Agc->setText("<span style='font-size:normal;'>" + QString::number(gnss2.agc) + " %</span>");
        gnss2Sats->setText("<span style='font-size:normal;'>" + QString::number(gnss2.satsTracked) + "</span>");

        gnss2PosValid->setText("<span style='font-size:normal;color:green;'>yes, tracking " + QString::number(gnss2.satsTracked) + " sats</span>");

        if (abs(gnss2.posOffset) < 10) gnss2PosOffset->setText("<span style='font-size:normal;color:green;'>Normal, &lt; 10 m</span>");
        else if (abs(gnss2.posOffset) < 50) gnss2PosOffset->setText("<span style='font-size:normal;color:orange;'>" + QString::number(abs(gnss2.posOffset), 'f', 0) + " m</span>");
        else if (abs(gnss2.posOffset) < 1000) gnss2PosOffset->setText("<span style='font-size:normal;color:red;'>" + QString::number(abs(gnss2.posOffset), 'f', 0) + " m</span>");
        else gnss2PosOffset->setText("<span style='font-size:normal;color:red;'>" + QString::number(abs(gnss2.posOffset / 1000), 'f', 0) + " km</span>");

        if (abs(gnss2.altOffset) < 10) gnss2AltOffset->setText("<span style='font-size:normal;color:green;'>Normal, &lt; 10 m</span>");
        else if (abs(gnss2.altOffset) < 50) gnss2AltOffset->setText("<span style='font-size:normal;color:orange;'>" + QString::number(abs(gnss2.altOffset), 'f', 0) + " m</span>");
        else if (abs(gnss2.altOffset) < 1000) gnss2AltOffset->setText("<span style='font-size:normal;color:red;'>" + QString::number(abs(gnss2.altOffset), 'f', 0) + " m</span>");
        else gnss2AltOffset->setText("<span style='font-size:normal;color:red;'>" + QString::number(abs(gnss2.altOffset / 1000), 'f', 0) + " km</span>");

        if (gnss2.timeOffset < 1000) gnss2TimeOffset->setText("<span style='font-size:normal;color:green;'>0 secs</span>");
        else if (gnss2.timeOffset < 5000) gnss2TimeOffset->setText("<span style='font-size:normal;color:orange;'>" + QString::number(gnss2.timeOffset / 1000) + " sec(s)</span>");
        else if (gnss2.timeOffset < 60000) gnss2TimeOffset->setText("<span style='font-size:normal;color:red;'>" + QString::number(gnss2.timeOffset / 1000) + " sec(s)</span>");
        else if (gnss2.timeOffset < 60 * 60000) gnss2TimeOffset->setText("<span style='font-size:normal;color:red;'>" + QString::number(gnss2.timeOffset / 60000) + " min(s)</span>");
        else if (gnss2.timeOffset < 60 * 60 * 60000) gnss2TimeOffset->setText("<span style='font-size:normal;color:red;'>" + QString::number(gnss2.timeOffset / 3600000) + " hour(s)</span>");
        else if (gnss2.timeOffset < 24UL * 60 * 60 * 60000) gnss2TimeOffset->setText("<span style='font-size:normal;color:red;'>" + QString::number(gnss2.timeOffset / 24UL * 3600000) + " hour(s)</span>");
        /*gnss2CwJamming->setText("<span style='font-size:normal;'>" + QString::number(gnss2.jammingIndicator) + "</span>");
        QString jam;
        if (gnss2.jammingState == JAMMINGSTATE::NOJAMMING) jam = "No jamming";
        else if (gnss2.jammingState == JAMMINGSTATE::WARNINGFIXOK) jam = "Warning, fix ok";
        else if (gnss2.jammingState == JAMMINGSTATE::CRITICALNOFIX) jam = "Critical, no fix";
        else jam = "Unknown";
        gnss2JammingState->setText(tr("<span style='font-size:normal;'>") + jam + tr("</span>"));*/
    }
    else {
        if (gnss2Name.isEmpty()) gnss2LeftGroupBox->setTitle("GNSS receiver 1 - position invalid");
        else  gnss2LeftGroupBox->setTitle(gnss2Name + " - position invalid");
        gnss2Latitude->setText("<span style='font-size:normal;'>n/a</span>");
        gnss2Longitude->setText("<span style='font-size:normal;'>n/a</span>");
        gnss2Altitude->setText("<span style='font-size:normal;'>n/a</span>");
        gnss2Time->setText("<span style='font-size:normal;'>n/a</span>");
        gnss2Dop->setText("<span style='font-size:normal;'>n/a</span>");
        gnss2CNo->setText("<span style='font-size:normal;'>n/a</span>");
        gnss2Sats->setText("<span style='font-size:normal;'>n/a</span>");

        gnss2PosValid->setText("<span style='font-size:normal;color:red;'>no</span>");
        gnss2Agc->setText("<span style='font-size:normal;'>" + QString::number(gnss2.agc) + " %</span>");

        gnss2PosOffset->setText(QString("<span style='font-size:normal;color:red;'>n/a</span>"));
        gnss2AltOffset->setText("<span style='font-size:normal;color:red;'>n/a</span>");
        gnss2TimeOffset->setText("<span style='font-size:normal;color:red;'>n/a</span>");
        /*gnss2CwJamming->setText("<span style='font-size:normal;'> n/a</span>");
        gnss2JammingState->setText("<span style='font-size:normal;'> n/a</span>");*/
    }
}

void GnssDisplay::reqGnssData()
{
    if (config->getGnssUseInstrumentGnss()) {
        emit requestGnssData(3);
        return;
    }

    if (config->getGnssSerialPort1Activate()) emit requestGnssData(1);
    if (config->getGnssSerialPort2Activate()) emit requestGnssData(2);
}
