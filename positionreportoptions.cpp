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
    comboOpt1->addItem("Manual position");
    comboOpt1->setToolTip("Which position source should be used for the position data");

    mainLayout->addRow(new QLabel("Receiver http(s) address"), leOpt1);
    leOpt1->setToolTip("The receiving address for the json report");

    mainLayout->addRow(new QLabel("Station ID"), leOpt2);
    leOpt2->setToolTip("The name used as identity in the position report");

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

    mainLayout->addRow(cbOpt5);
    cbOpt5->setText("Include current job stats");
    cbOpt5->setToolTip("Include uptime, connection and current band in the report");

    mainLayout->addRow(cbOpt6);
    cbOpt6->setText("Include temperature and humidity");
    cbOpt6->setToolTip("Include sensor temperature and humidity data from Arduino if available.");

    mainLayout->addRow(cbOpt7);
    cbOpt7->setText("Include webswitch and MQTT data");
    cbOpt7->setToolTip("Include Webswitch and MQTT sensor data (configured in own options menu). " \
                       "\nOnly sent if data is valid.");

    connect(btnBox, &QDialogButtonBox::accepted, this, &PositionReportOptions::saveCurrentSettings);
    connect(btnBox, &QDialogButtonBox::rejected, dialog, &QDialog::close);
    mainLayout->addWidget(btnBox);
}

void PositionReportOptions::start()
{
    cbOpt1->setChecked(config->getPosReportActivated());
    comboOpt1->setCurrentIndex(comboOpt1->findText(config->getPosReportSource()));
    leOpt1->setText(config->getPosReportUrl());
    leOpt2->setText(config->getPosReportId());
    sbOpt1->setValue(config->getPosReportSendInterval());
    cbOpt2->setChecked(config->getPosReportAddPos());
    cbOpt3->setChecked(config->getPosReportAddSogCog());
    cbOpt4->setChecked(config->getPosReportAddGnssStats());
    cbOpt5->setChecked(config->getPosReportAddConnStats());
    cbOpt6->setChecked(config->getPosReportAddSensorData());
    cbOpt7->setChecked(config->getPosReportAddMqttData());

    dialog->exec();
}

void PositionReportOptions::saveCurrentSettings()
{
    config->setPosReportActivated(cbOpt1->isChecked());
    config->setPosReportSource(comboOpt1->currentText());
    config->setPosReportUrl(leOpt1->text());
    config->setPosReportId(leOpt2->text());
    config->setPosReportSendInterval(sbOpt1->value());
    config->setPosReportAddPos(cbOpt2->isChecked());
    config->setPosReportAddSogCog(cbOpt3->isChecked());
    config->setPosreportAddGnssStats(cbOpt4->isChecked());
    config->setPosreportAddConnStats(cbOpt5->isChecked());
    config->setPosreportAddSensorData(cbOpt6->isChecked());
    config->setPosreportAddMqttData(cbOpt7->isChecked());

    dialog->close();
}
