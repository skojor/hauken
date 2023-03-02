#include "positionreportoptions.h"

PositionReportOptions::PositionReportOptions(QSharedPointer<Config> c)
{
    config = c;
    setWindowTitle("Position report options");

    mainLayout->addRow(cbOpt1);
    cbOpt1->setText("Enable periodic position reports via HTTP/POST");
    cbOpt1->setToolTip("Enable to send periodic id/timestamp/position/other reports as a HTTP POST json packet.");

    mainLayout->addRow(new QLabel("GNSS receiver to use for reports"), comboOpt1);
    comboOpt1->addItem("InstrumentGNSS");
    comboOpt1->addItem("GNSS receiver 1");
    comboOpt1->addItem("GNSS receiver 2");
    comboOpt1->setToolTip("Which GNSS receiver should be used for the position data");

    mainLayout->addRow(new QLabel("Receiver http(s) address"), leOpt1);
    leOpt1->setToolTip("The receiving address for the json report");

    mainLayout->addRow(new QLabel("Send interval (in seconds)"), sbOpt1);
    sbOpt1->setToolTip("How often to send position reports");
    sbOpt1->setRange(1, 9999);

    mainLayout->addRow(cbOpt2);
    cbOpt2->setText("Include position");
    cbOpt2->setToolTip("Include position as latitude/longitude in the report");

    mainLayout->addRow(cbOpt3);
    cbOpt3->setText("Include speed and course over ground");
    cbOpt3->setToolTip("Include SOG/COG from GNSS");

    mainLayout->addRow(cbOpt4);
    cbOpt4->setText("Include GNSS stats");
    cbOpt4->setToolTip("Include sats tracked and DOP in the report");


    connect(btnBox, &QDialogButtonBox::accepted, this, &PositionReportOptions::saveCurrentSettings);
    connect(btnBox, &QDialogButtonBox::rejected, dialog, &QDialog::close);
    mainLayout->addWidget(btnBox);
}

void PositionReportOptions::start()
{
    cbOpt1->setChecked(config->getPosReportActivated());
    comboOpt1->setCurrentIndex(comboOpt1->findText(config->getPosReportSource()));
    leOpt1->setText(config->getPosReportUrl());
    sbOpt1->setValue(config->getPosReportSendInterval());
    cbOpt2->setChecked(config->getPosReportAddPos());
    cbOpt3->setChecked(config->getPosReportAddSogCog());
    cbOpt4->setChecked(config->getPosReportAddGnssStats());

    dialog->exec();
}

void PositionReportOptions::saveCurrentSettings()
{
    config->setPosReportActivated(cbOpt1->isChecked());
    config->setPosReportSource(comboOpt1->currentText());
    config->setPosReportUrl(leOpt1->text());
    config->setPosReportSendInterval(sbOpt1->value());
    config->setPosReportAddPos(cbOpt2->isChecked());
    config->setPosReportAddSogCog(cbOpt3->isChecked());
    config->setPosreportAddGnssStats(cbOpt4->isChecked());

    dialog->close();
}
