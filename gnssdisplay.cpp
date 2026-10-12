#include "gnssdisplay.h"
#include "appclock.h"
#include <QPainter>
#include <QTextDocument>

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

QString formatAgc(const GnssData &data)
{
    if (data.agcBlocks.isEmpty())
        return QString("%1 (%2) %").arg(data.agc).arg(data.agcOffset);

    QStringList values;
    for (auto iterator = data.agcBlocks.cbegin(); iterator != data.agcBlocks.cend(); ++iterator) {
        const QString label = iterator.value().frequencyMapped
                                  ? QString("Block %1 - %2").arg(iterator.key()).arg(iterator.value().frequencyLabel)
                                  : QString("Block %1").arg(iterator.key());
        values.append(QString("%1: %2 (%3) %").arg(label).arg(iterator.value().agc).arg(iterator.value().agcOffset));
    }
    return values.join("<br>");
}

QString formatJammingIndicator(const GnssData &data)
{
    if (data.agcBlocks.isEmpty())
        return data.jammingIndicator >= 0 ? QString::number(data.jammingIndicator) + " %" : "n/a";

    QStringList values;
    for (auto iterator = data.agcBlocks.cbegin(); iterator != data.agcBlocks.cend(); ++iterator) {
        if (iterator.value().jammingIndicator < 0) continue; // not provided by this receiver/band (e.g. Septentrio)
        const QString label = iterator.value().frequencyMapped
                                  ? QString("Block %1 - %2").arg(iterator.key()).arg(iterator.value().frequencyLabel)
                                  : QString("Block %1").arg(iterator.key());
        values.append(QString("%1: %2 %").arg(label).arg(iterator.value().jammingIndicator));
    }
    return values.isEmpty() ? "n/a" : values.join("<br>");
}

QString formatInterferenceSpoofing(const GnssData &data)
{
    const SeptentrioRfStatus &status = data.septentrioRfStatus;
    if (data.septentrioReceiver && !status.received)
        return "RFStatus unavailable";
    if (data.septentrioReceiver
        && (!data.septentrioRfStatusTimestamp.isValid()
            || data.septentrioRfStatusTimestamp.msecsTo(AppClock::nowUtc()) > 5000)) {
        return "RFStatus stale";
    }
    if (!data.septentrioReceiver && data.ubloxSecSigReceived) {
        if (!data.ubloxSecSigTimestamp.isValid()
            || data.ubloxSecSigTimestamp.msecsTo(AppClock::nowUtc()) > 5000) {
            return "Signal security status stale";
        }

        const UbloxSecSigData &security = data.ubloxSecSig;
        QString jammingText;
        if (!security.jammingDetectionEnabled) jammingText = "disabled";
        else {
            switch (security.jammingState) {
            case UbloxJammingState::UNKNOWN: jammingText = "unknown"; break;
            case UbloxJammingState::NOJAMMING: jammingText = "no jamming indicated"; break;
            case UbloxJammingState::WARNING: jammingText = coloredText("jamming indicated", "red"); break;
            }
        }

        QStringList jammedFrequencies;
        for (const UbloxSecSigFrequency &frequency : security.frequencies) {
            if (frequency.jammed)
                jammedFrequencies.append(QString::number(frequency.centerFrequencyKHz / 1000.0, 'f', 3));
        }
        if (!jammedFrequencies.isEmpty())
            jammingText += QString(" (%1 MHz)").arg(jammedFrequencies.join(", "));

        QString spoofingText;
        if (!security.spoofingDetectionEnabled) spoofingText = "disabled";
        else {
            switch (security.spoofingState) {
            case UbloxSpoofingState::UNKNOWN: spoofingText = "unknown"; break;
            case UbloxSpoofingState::NOSPOOFING: spoofingText = "no spoofing indicated"; break;
            case UbloxSpoofingState::INDICATED: spoofingText = coloredText("spoofing indicated", "red"); break;
            case UbloxSpoofingState::AFFIRMED: spoofingText = coloredText("spoofing affirmed", "red"); break;
            }
        }
        return QString("Jamming: %1<br>Spoofing: %2").arg(jammingText, spoofingText);
    }
    if (!data.septentrioReceiver)
        return QString("Jamming indicator: %1<br>Spoofing: n/a").arg(formatJammingIndicator(data));

    QStringList interference;
    for (const SeptentrioRfBandStatus &band : status.bands) {
        QString mode;
        switch (band.mode) {
        case 1: mode = "suppressed by manual notch"; break;
        case 2: mode = "detected and canceled"; break;
        case 8: mode = "detected, not mitigated"; break;
        default: mode = QString("unknown mode %1").arg(band.mode); break;
        }

        QString antenna = QString("antenna %1").arg(band.antennaId);
        if (band.antennaId == 0) antenna = "main antenna";
        else if (band.antennaId == 1) antenna = "Aux1 antenna";
        else if (band.antennaId == 2) antenna = "Aux2 antenna";

        QString value = QString("%1 MHz, %2 kHz, %3: %4")
                            .arg(band.frequencyHz / 1e6, 0, 'f', 3)
                            .arg(band.bandwidthKHz)
                            .arg(antenna, mode);
        if (band.interferencePowerDbm != 0 && band.mode != 1)
            value += QString(" (%1 dBm)").arg(band.interferencePowerDbm);
        interference.append(value);
    }

    QStringList spoofingWarnings;
    if (status.flags & 0x01) spoofingWarnings.append("GNSS signal authenticity warning");
    if (status.flags & 0x02) spoofingWarnings.append("non-authentic navigation message detected");

    return QString("Interference: %1<br>Spoofing: %2")
        .arg(interference.isEmpty() ? "none reported" : interference.join("<br>"),
             spoofingWarnings.isEmpty() ? "none reported" : spoofingWarnings.join("; "));
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

void GnssDisplay::updGnssData(GnssData g, QString id)
{
    GnssReceiverCard *card = cards.value(id);
    if (!card) return;

    card->data = g;
    updText();
}

QImage GnssDisplay::emailInfoImage() const
{
    QString html = "<html><body><h2>GNSS receiver information</h2>";
    bool hasReceiver = false;
    const auto appendRow = [&html](const QString &label, const QLabel *value) {
        html += "<tr><th align='left'>" + label.toHtmlEscaped() + "</th><td>" + value->text() + "</td></tr>";
    };

    for (const GnssReceiverProfile &profile : config->getGnssReceiverProfiles()) {
        const GnssReceiverCard *card = cards.value(profile.id);
        if (!profile.enabled || !card)
            continue;

        hasReceiver = true;
        const QString name = profile.name.isEmpty() ? profile.id : profile.name;
        html += "<h3>" + name.toHtmlEscaped() + "</h3><table width='100%' cellspacing='0' cellpadding='4' border='1'>";
        appendRow("Latitude", card->latitude);
        appendRow("Longitude", card->longitude);
        appendRow("Altitude", card->altitude);
        appendRow("GNSS time", card->time);
        appendRow("DOP", card->dop);
        appendRow("C/No", card->cno);
        appendRow("Sats tracked", card->sats);
        appendRow("Position valid", card->posValid);
        appendRow("Position offset", card->posOffset);
        appendRow("Altitude offset", card->altOffset);
        appendRow("Time offset", card->timeOffset);
        appendRow("AGC", card->agc);
        appendRow("Interference / spoofing", card->cwJamming);
        html += "</table>";
    }

    if (!hasReceiver)
        return {};

    html += "</body></html>";
    QTextDocument document;
    document.setDefaultFont(wdg->font());
    document.setHtml(html);
    document.setTextWidth(1200);

    const QSize imageSize = document.size().toSize() + QSize(16, 16);
    QImage image(imageSize, QImage::Format_ARGB32_Premultiplied);
    image.fill(Qt::white);
    QPainter painter(&image);
    document.drawContents(&painter, QRectF(8, 8, imageSize.width() - 16, imageSize.height() - 16));
    return image;
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
    auto outerLayout = new QVBoxLayout;
    outerLayout->addLayout(receiversLayout);

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
    outerLayout->addWidget(ppsGroupBox);

    wdg->setLayout(outerLayout);
    rebuildReceiverCards();
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

void GnssDisplay::rebuildReceiverCards()
{
    const QList<GnssReceiverProfile> profiles = config->getGnssReceiverProfiles();
    QStringList desiredIds;
    for (const GnssReceiverProfile &profile : profiles)
        if (profile.enabled) desiredIds.append(profile.id);

    // Remove cards for receivers that were deleted or disabled
    for (auto it = cards.begin(); it != cards.end();) {
        if (desiredIds.contains(it.key())) {
            ++it;
            continue;
        }
        receiversLayout->removeWidget(it.value()->leftGroupBox);
        receiversLayout->removeWidget(it.value()->rightGroupBox);
        it.value()->leftGroupBox->deleteLater();
        it.value()->rightGroupBox->deleteLater();
        delete it.value();
        it = cards.erase(it);
    }

    // Create cards for newly enabled receivers
    for (const GnssReceiverProfile &profile : profiles) {
        if (!profile.enabled || cards.contains(profile.id)) continue;

        GnssReceiverCard *card = new GnssReceiverCard;
        QFormLayout *leftLayout = new QFormLayout;
        leftLayout->addRow("Latitude", card->latitude);
        leftLayout->addRow("Longitude", card->longitude);
        leftLayout->addRow("Altitude", card->altitude);
        leftLayout->addRow("GNSS time", card->time);
        leftLayout->addRow("DOP", card->dop);
        leftLayout->addRow("C/No", card->cno);
        leftLayout->addRow("Sats tracked", card->sats);
        card->leftGroupBox->setLayout(leftLayout);

        QFormLayout *rightLayout = new QFormLayout;
        rightLayout->addRow("Position valid", card->posValid);
        rightLayout->addRow("Position offset", card->posOffset);
        rightLayout->addRow("Altitude offset", card->altOffset);
        rightLayout->addRow("Time offset", card->timeOffset);
        rightLayout->addRow("AGC", card->agc);
        rightLayout->addRow("Interference / spoofing", card->cwJamming);
        card->rightGroupBox->setLayout(rightLayout);

        cards.insert(profile.id, card);
    }

    // Reposition every current card in configured order, wrapping after 2 receivers per row
    int index = 0;
    for (const GnssReceiverProfile &profile : profiles) {
        GnssReceiverCard *card = cards.value(profile.id);
        if (!card) continue;

        card->leftGroupBox->setTitle(profile.name);
        card->rightGroupBox->setTitle(profile.name + " - info/calculations");

        const int row = index / 2;
        const int col = (index % 2) * 2;
        receiversLayout->addWidget(card->leftGroupBox, row, col);
        receiversLayout->addWidget(card->rightGroupBox, row, col + 1);
        ++index;
    }
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
    auto dateTimeTicker = QSharedPointer<QCPAxisTickerDateTime>::create();
    dateTimeTicker->setDateTimeFormat("hh:mm:ss");
    dateTimeTicker->setDateTimeSpec(Qt::UTC);
    dateTimeTicker->setTickCount(7);
    ppsPlot->xAxis->setTicker(dateTimeTicker);
    ppsPlot->xAxis->setLabel(tr("UTC time"));
    ppsPlot->yAxis->setLabel(tr("Difference (us)"));
    const double now = QCPAxisTickerDateTime::dateTimeToKey(QDateTime::currentDateTimeUtc());
    ppsPlot->xAxis->setRange(now - 60.0, now);
    ppsPlot->yAxis->setRange(-50.0, 50.0);

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

    ppsPlot->graph(0)->data()->clear();
    ppsPlot->graph(1)->data()->clear();
    const double now = QCPAxisTickerDateTime::dateTimeToKey(QDateTime::currentDateTimeUtc());
    ppsPlotStartTime = 0;
    ppsPlot->xAxis->setRange(now, now + 10.0);
    ppsPlot->yAxis->setRange(-50.0, 50.0);
    ppsPlot->replot();
}

void GnssDisplay::updatePpsPlot()
{
    if (!config->getGnssPpsPlotEnabled() || !ppsPlot) return;

    if (!ppsDataReceived || !ppsData.gpsReference.valid || !ppsData.galileoReference.valid) return;

    const double x = QCPAxisTickerDateTime::dateTimeToKey(QDateTime::currentDateTimeUtc());
    if (ppsPlotStartTime == 0) ppsPlotStartTime = x;
    ppsPlot->graph(0)->addData(x, -ppsData.gpsReference.currentNs / 1000.0);
    ppsPlot->graph(1)->addData(x, -ppsData.galileoReference.currentNs / 1000.0);
    ppsPlot->yAxis->rescale(true);
    const QCPRange yRange = ppsPlot->yAxis->range();
    ppsPlot->yAxis->setRange(qMin(yRange.lower, -50.0), qMax(yRange.upper, 50.0));
    ppsPlot->xAxis->setRange(ppsPlotStartTime, qMax(ppsPlotStartTime + 10.0, x));
    ppsPlot->replot(QCustomPlot::rpQueuedReplot);
}

void GnssDisplay::updSettings()
{
    if (!isClosing && config->getGnssDisplayWidget() && !wdg->isVisible()) {
        wdg->show();
        wdg->restoreGeometry(config->getGnssDisplayWindowState());
    }
    else if (!config->getGnssDisplayWidget() && wdg->isVisible()) wdg->close();
    rebuildReceiverCards();
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

void GnssDisplay::updateCardText(const QString &id, GnssReceiverCard *card)
{
    const GnssReceiverProfile profile = config->getGnssReceiverProfile(id);
    const QString name = profile.name.isEmpty() ? id : profile.name;
    const GnssData &data = card->data;

    if (data.posValid) {
        card->leftGroupBox->setTitle(name + " - position valid");
        card->latitude->setText("<span style='font-size:normal;'>" + QString::number(data.latitude, 'f', 7) + "</span>");
        card->longitude->setText("<span style='font-size:normal;'>" + QString::number(data.longitude, 'f', 7) + "</span>");
        card->altitude->setText("<span style='font-size:normal;'>" + QString::number(data.altitude, 'f', 0) + "</span>");
        card->time->setText("<span style='font-size:normal;'>" + data.timestamp.toString("dd.MM.yy HH:mm:ss") + "</span>");
        card->dop->setText("<span style='font-size:normal;'>" + QString::number(data.hdop, 'f', 1) + "</span>");
        card->cno->setText("<span style='font-size:normal;'>" + QString::number(data.cno) + "</span>");
        card->agc->setText("<span style='font-size:normal;'>" + formatAgc(data) + "</span>");
        card->cwJamming->setText("<span style='font-size:normal;'>" + formatInterferenceSpoofing(data) + "</span>");
        card->sats->setText("<span style='font-size:normal;'>" + QString::number(data.satsTracked) + "</span>");

        card->posValid->setText("<span style='font-size:normal;color:green;'>yes, tracking " + QString::number(data.satsTracked) + " sats</span>");

        if (abs(data.posOffset) < 10) card->posOffset->setText("<span style='font-size:normal;color:green;'>Normal, &lt; 10 m</span>");
        else if (abs(data.posOffset) < 50) card->posOffset->setText("<span style='font-size:normal;color:orange;'>" + QString::number(abs(data.posOffset), 'f', 0) + " m</span>");
        else if (abs(data.posOffset) < 1000) card->posOffset->setText("<span style='font-size:normal;color:red;'>" + QString::number(abs(data.posOffset), 'f', 0) + " m</span>");
        else card->posOffset->setText("<span style='font-size:normal;color:red;'>" + QString::number(abs(data.posOffset / 1000), 'f', 0) + " km</span>");

        if (abs(data.altOffset) < 10) card->altOffset->setText("<span style='font-size:normal;color:green;'>Normal, &lt; 10 m</span>");
        else if (abs(data.altOffset) < 50) card->altOffset->setText("<span style='font-size:normal;color:orange;'>" + QString::number(abs(data.altOffset), 'f', 0) + " m</span>");
        else if (abs(data.altOffset) < 1000) card->altOffset->setText("<span style='font-size:normal;color:red;'>" + QString::number(abs(data.altOffset), 'f', 0) + " m</span>");
        else card->altOffset->setText("<span style='font-size:normal;color:red;'>" + QString::number(abs(data.altOffset / 1000), 'f', 0) + " km</span>");

        const qint64 timeOffsetMagnitude = qAbs(data.timeOffset);
        const QString timeOffsetSign = data.timeOffset < 0 ? "-" : "";
        if (timeOffsetMagnitude < 1000) {
            const QString value = timeOffsetMagnitude == 0
                                      ? "0 secs"
                                      : timeOffsetSign + QString::number(timeOffsetMagnitude) + " ms";
            card->timeOffset->setText("<span style='font-size:normal;color:green;'>" + value + "</span>");
        }
        else if (timeOffsetMagnitude < 5000) card->timeOffset->setText("<span style='font-size:normal;color:orange;'>" + timeOffsetSign + QString::number(timeOffsetMagnitude / 1000) + " sec(s)</span>");
        else if (timeOffsetMagnitude < 60000) card->timeOffset->setText("<span style='font-size:normal;color:red;'>" + timeOffsetSign + QString::number(timeOffsetMagnitude / 1000) + " sec(s)</span>");
        else if (timeOffsetMagnitude < 60 * 60000) card->timeOffset->setText("<span style='font-size:normal;color:red;'>" + timeOffsetSign + QString::number(timeOffsetMagnitude / 60000) + " min(s)</span>");
        else if (timeOffsetMagnitude < 60 * 60 * 60000) card->timeOffset->setText("<span style='font-size:normal;color:red;'>" + timeOffsetSign + QString::number(timeOffsetMagnitude / 3600000) + " hour(s)</span>");
        else card->timeOffset->setText("<span style='font-size:normal;color:red;'>" + timeOffsetSign + QString::number(timeOffsetMagnitude / (24 * 60 * 60 * 1000)) + " day(s)</span>");
    }
    else {
        card->leftGroupBox->setTitle(name + " - position invalid");
        card->latitude->setText("<span style='font-size:normal;'>n/a</span>");
        card->longitude->setText("<span style='font-size:normal;'>n/a</span>");
        card->altitude->setText("<span style='font-size:normal;'>n/a</span>");
        card->time->setText("<span style='font-size:normal;'>n/a</span>");
        card->dop->setText("<span style='font-size:normal;'>n/a</span>");
        card->cno->setText("<span style='font-size:normal;'>n/a</span>");
        card->sats->setText("<span style='font-size:normal;'>n/a</span>");

        card->posValid->setText("<span style='font-size:normal;color:red;'>no</span>");
        card->agc->setText("<span style='font-size:normal;'>" + formatAgc(data) + "</span>");
        card->cwJamming->setText("<span style='font-size:normal;'>" + formatInterferenceSpoofing(data) + "</span>");

        card->posOffset->setText("<span style='font-size:normal;color:red;'>n/a</span>");
        card->altOffset->setText("<span style='font-size:normal;color:red;'>n/a</span>");
        card->timeOffset->setText("<span style='font-size:normal;color:red;'>n/a</span>");
    }
}

void GnssDisplay::updText()
{
    updatePpsText();
    for (auto it = cards.cbegin(); it != cards.cend(); ++it)
        updateCardText(it.key(), it.value());
}

void GnssDisplay::reqGnssData()
{
    for (auto it = cards.cbegin(); it != cards.cend(); ++it)
        emit requestGnssData(it.key());
}
