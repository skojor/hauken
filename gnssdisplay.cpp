#include "gnssdisplay.h"

GnssDisplay::GnssDisplay(QObject *parent)
{
    this->setParent(parent);

    wdg->setAttribute(Qt::WA_QuitOnClose);
    //wdg->setWindowFlag(Qt::WindowStaysOnTopHint);

    connect(updateGnssDataTimer, &QTimer::timeout, this, &GnssDisplay::reqGnssData);
}

void GnssDisplay::start()
{
    setupWidget();
    updText();
    updateGnssDataTimer->start(250);
}

void GnssDisplay::close()
{
    isClosing = true;   // don't reopen widget after shutdown

    qDebug() << "Closing Gnss Display widget...";
    setGnssDisplayWindowState(wdg->saveGeometry());
    wdg->close();

}

void GnssDisplay::updGnssData(GnssData g, int id)
{
    if (id == 1) gnss1 = g;
    else gnss2 = g;
    updText();
}

void GnssDisplay::setupWidget()
{
    wdg->resize(640, 480);
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
    gnss1LeftLayout->addRow("AGC", gnss1Agc);
    gnss1LeftLayout->addRow("Sats tracked", gnss1Sats);
    //gnss1LeftLayout->setFormAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    gnss1LeftGroupBox->setLayout(gnss1LeftLayout);

    gnss1RightGroupBox->setTitle("Info/calculations");
    QFormLayout *gnss1RightLayout = new QFormLayout;
    gnss1RightLayout->addRow("Position offset", gnss1PosOffset);
    gnss1RightLayout->addRow("Altitude offset", gnss1AltOffset);
    gnss1RightLayout->addRow("Time offset", gnss1TimeOffset);
    gnss1RightLayout->addRow("CW jamming indicator", gnss1CwJamming);
    gnss1RightLayout->addRow("Jamming state", gnss1JammingState);
    gnss1RightGroupBox->setLayout(gnss1RightLayout);

    gnss2LeftGroupBox->setTitle("GNSS receiver 2");
    QFormLayout *gnss2LeftLayout = new QFormLayout;
    gnss2LeftLayout->addRow("Latitude", gnss2Latitude);
    gnss2LeftLayout->addRow("Longitude", gnss2Longitude);
    gnss2LeftLayout->addRow("Altitude", gnss2Altitude);
    gnss2LeftLayout->addRow("GNSS time", gnss2Time);
    gnss2LeftLayout->addRow("DOP", gnss2Dop);
    gnss2LeftLayout->addRow("C/No", gnss2CNo);
    gnss2LeftLayout->addRow("AGC", gnss2Agc);
    gnss2LeftLayout->addRow("Sats tracked", gnss2Sats);
    //gnss1LeftLayout->setFormAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    gnss2LeftGroupBox->setLayout(gnss2LeftLayout);

    gnss2RightGroupBox->setTitle("Info/calculations");
    QFormLayout *gnss2RightLayout = new QFormLayout;
    gnss2RightLayout->addRow("Position offset", gnss2PosOffset);
    gnss2RightLayout->addRow("Altitude offset", gnss2AltOffset);
    gnss2RightLayout->addRow("Time offset", gnss2TimeOffset);
    gnss2RightLayout->addRow("CW jamming indicator", gnss2CwJamming);
    gnss2RightLayout->addRow("Jamming state", gnss2JammingState);
    gnss2RightGroupBox->setLayout(gnss2RightLayout);

    mainLayout->addWidget(gnss1LeftGroupBox, 0, 0);
    mainLayout->addWidget(gnss1RightGroupBox, 0, 1);

    mainLayout->addWidget(gnss2LeftGroupBox, 1, 0);
    mainLayout->addWidget(gnss2RightGroupBox, 1, 1);
    wdg->setLayout(mainLayout);
    wdg->restoreGeometry(getGnssDisplayWindowState());
    wdg->adjustSize();
}

void GnssDisplay::updSettings()
{
    if (!isClosing && getGnssDisplayWidget() && !wdg->isVisible()) {
        wdg->show();
    }
    else if (!getGnssDisplayWidget() && wdg->isVisible()) wdg->close();
    gnss1Name = getGnss1Name();
    gnss2Name = getGnss2Name();
    if (gnss1Name.isEmpty()) gnss1RightGroupBox->setTitle("GNSS receiver 1 - info/calculations");
    else gnss1RightGroupBox->setTitle(gnss1Name + " - info/calculations");
    if (gnss2Name.isEmpty()) gnss2RightGroupBox->setTitle("GNSS receiver 2 - info/calculations");
    else gnss2RightGroupBox->setTitle(gnss2Name + " - info/calculations");
}

void GnssDisplay::updText()
{
    if (gnss1.posValid) {
        if (gnss1Name.isEmpty()) gnss1LeftGroupBox->setTitle("GNSS receiver 1 - position valid");
        else  gnss1LeftGroupBox->setTitle(gnss1Name + " - position valid");
        gnss1Latitude->setText("<span style='font-size:x-large;'>" + QString::number(gnss1.latitude, 'f', 7) + "</span>");
        gnss1Longitude->setText("<span style='font-size:x-large;'>" + QString::number(gnss1.longitude, 'f', 7) + "</span>");
        gnss1Altitude->setText("<span style='font-size:x-large;'>" + QString::number(gnss1.altitude, 'f', 0) + "</span>");
        gnss1Time->setText("<span style='font-size:x-large;'>" + gnss1.timestamp.toString("dd.MM.yy HH:mm:ss") + "</span>");
        gnss1Dop->setText("<span style='font-size:x-large;'>" + QString::number(gnss1.hdop, 'f', 1) + "</span>");
        gnss1CNo->setText("<span style='font-size:x-large;'>" + QString::number(gnss1.cno) + "</span>");
        gnss1Agc->setText("<span style='font-size:x-large;'>" + QString::number(gnss1.agc) + "</span>");
        gnss1Sats->setText("<span style='font-size:x-large;'>" + QString::number(gnss1.satsTracked) + "</span>");

        if (abs(gnss1.posOffset) < 9999) gnss1PosOffset->setText("<span style='font-size:x-large;'>" + QString::number(abs(gnss1.posOffset), 'f', 0) + " m</span>");
        else gnss1PosOffset->setText("<span style='font-size:x-large;'>> 10 km</span>");
        if (abs(gnss1.altOffset) < 9999) gnss1AltOffset->setText("<span style='font-size:x-large;'>" + QString::number(abs(gnss1.altOffset), 'f', 0) + " m</span>");
        else gnss1AltOffset->setText("<span style='font-size:x-large;'>> 10 km</span>");
        if (gnss1.timeOffset < 9999) gnss1TimeOffset->setText("<span style='font-size:x-large;'>" + QString::number(gnss1.timeOffset) + " ms</span>");
        else gnss1TimeOffset->setText("<span style='font-size:x-large;'>> 10 s</span>");
        gnss1CwJamming->setText("<span style='font-size:x-large;'>" + QString::number(gnss1.jammingIndicator) + "</span>");
        QString jam;
        if (gnss1.jammingState == JAMMINGSTATE::NOJAMMING) jam = "No jamming";
        else if (gnss1.jammingState == JAMMINGSTATE::WARNINGFIXOK) jam = "Warning, fix ok";
        else if (gnss1.jammingState == JAMMINGSTATE::CRITICALNOFIX) jam = "Critical, no fix";
        else jam = "Unknown";
        gnss1JammingState->setText(tr("<span style='font-size:x-large;'>") + jam + tr("</span>"));

    }
    else {
        if (gnss1Name.isEmpty()) gnss1LeftGroupBox->setTitle("GNSS receiver 1 - position invalid");
        else  gnss1LeftGroupBox->setTitle(gnss1Name + " - position invalid");
        gnss1Latitude->setText("<span style='font-size:x-large;'>n/a</span>");
        gnss1Longitude->setText("<span style='font-size:x-large;'>n/a</span>");
        gnss1Altitude->setText("<span style='font-size:x-large;'>n/a</span>");
        gnss1Time->setText("<span style='font-size:x-large;'>n/a</span>");
        gnss1Dop->setText("<span style='font-size:x-large;'>n/a</span>");
        gnss1CNo->setText("<span style='font-size:x-large;'>n/a</span>");
        gnss1Agc->setText("<span style='font-size:x-large;'>n/a</span>");
        gnss1Sats->setText("<span style='font-size:x-large;'>n/a</span>");

        gnss1PosOffset->setText(QString("<span style='font-size:x-large;'> n/a m</span>"));
        gnss1AltOffset->setText("<span style='font-size:x-large;'> n/a m</span>");
        gnss1TimeOffset->setText("<span style='font-size:x-large;'> n/a ms</span>");
        gnss1CwJamming->setText("<span style='font-size:x-large;'> n/a</span>");
        gnss1JammingState->setText("<span style='font-size:x-large;'> n/a</span>");
    }

    if (gnss2.posValid) {
        if (gnss2Name.isEmpty()) gnss2LeftGroupBox->setTitle("GNSS receiver 2 - position valid");
        else  gnss2LeftGroupBox->setTitle(gnss2Name + " - position valid");
        gnss2Latitude->setText("<span style='font-size:x-large;'>" + QString::number(gnss2.latitude, 'f', 7) + "</span>");
        gnss2Longitude->setText("<span style='font-size:x-large;'>" + QString::number(gnss2.longitude, 'f', 7) + "</span>");
        gnss2Altitude->setText("<span style='font-size:x-large;'>" + QString::number(gnss2.altitude, 'f', 0) + "</span>");
        gnss2Time->setText("<span style='font-size:x-large;'>" + gnss2.timestamp.toString("dd.MM.yy HH:mm:ss") + "</span>");
        gnss2Dop->setText("<span style='font-size:x-large;'>" + QString::number(gnss2.hdop, 'f', 1) + "</span>");
        gnss2CNo->setText("<span style='font-size:x-large;'>" + QString::number(gnss2.cno) + "</span>");
        gnss2Agc->setText("<span style='font-size:x-large;'>" + QString::number(gnss2.agc) + "</span>");
        gnss2Sats->setText("<span style='font-size:x-large;'>" + QString::number(gnss2.satsTracked) + "</span>");

        if (abs(gnss2.posOffset) < 9999) gnss2PosOffset->setText("<span style='font-size:x-large;'>" + QString::number(abs(gnss2.posOffset), 'f', 0) + " m</span>");
        else gnss2PosOffset->setText("<span style='font-size:x-large;'>> 10 km</span>");
        if (abs(gnss2.altOffset) < 9999) gnss2AltOffset->setText("<span style='font-size:x-large;'>" + QString::number(abs(gnss2.altOffset), 'f', 0) + " m</span>");
        else gnss2AltOffset->setText("<span style='font-size:x-large;'>> 10 km</span>");
        if (gnss2.timeOffset < 9999) gnss2TimeOffset->setText("<span style='font-size:x-large;'>" + QString::number(gnss2.timeOffset) + " ms</span>");
        else gnss2TimeOffset->setText("<span style='font-size:x-large;'>> 10 s</span>");
        gnss2CwJamming->setText("<span style='font-size:x-large;'>" + QString::number(gnss2.jammingIndicator) + "</span>");
        QString jam;
        if (gnss2.jammingState == JAMMINGSTATE::NOJAMMING) jam = "No jamming";
        else if (gnss2.jammingState == JAMMINGSTATE::WARNINGFIXOK) jam = "Warning, fix ok";
        else if (gnss2.jammingState == JAMMINGSTATE::CRITICALNOFIX) jam = "Critical, no fix";
        else jam = "Unknown";
        gnss2JammingState->setText(tr("<span style='font-size:x-large;'>") + jam + tr("</span>"));
    }
    else {
        if (gnss2Name.isEmpty()) gnss2LeftGroupBox->setTitle("GNSS receiver 1 - position invalid");
        else  gnss2LeftGroupBox->setTitle(gnss2Name + " - position invalid");
        gnss2Latitude->setText("<span style='font-size:x-large;'>n/a</span>");
        gnss2Longitude->setText("<span style='font-size:x-large;'>n/a</span>");
        gnss2Altitude->setText("<span style='font-size:x-large;'>n/a</span>");
        gnss2Time->setText("<span style='font-size:x-large;'>n/a</span>");
        gnss2Dop->setText("<span style='font-size:x-large;'>n/a</span>");
        gnss2CNo->setText("<span style='font-size:x-large;'>n/a</span>");
        gnss2Agc->setText("<span style='font-size:x-large;'>n/a</span>");
        gnss2Sats->setText("<span style='font-size:x-large;'>n/a</span>");

        gnss2PosOffset->setText(QString("<span style='font-size:x-large;'> n/a m</span>"));
        gnss2AltOffset->setText("<span style='font-size:x-large;'> n/a m</span>");
        gnss2TimeOffset->setText("<span style='font-size:x-large;'> n/a ms</span>");
        gnss2CwJamming->setText("<span style='font-size:x-large;'> n/a</span>");
        gnss2JammingState->setText("<span style='font-size:x-large;'> n/a</span>");
    }
}

void GnssDisplay::reqGnssData()
{
    emit requestGnssData(1);
    emit requestGnssData(2);
}
