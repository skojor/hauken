#include "gnssoptions.h"

GnssOptions::GnssOptions(QSharedPointer<Config> c)
    : OptionsBaseClass{}
{
    config = c;
    setWindowTitle("GNSS receiver configuration");

    QGroupBox *gnss1GroupBox = new QGroupBox("First GNSS receiver");
    QFormLayout *gnss1Layout = new QFormLayout;

    gnss1Layout->addRow(cbOpt1);
    cbOpt1->setText("Use this GNSS device");
    cbOpt1->setToolTip("Check this box to activate the GNSS surveillance for this port");
    cbOpt1->setChecked(config->getGnssSerialPort1Activate());

    gnss1Layout->addRow(new QLabel("Serial port name"), comboOpt1);
    comboOpt1->addItems(getAvailablePorts());
    comboOpt1->setToolTip("GNSS device serial port");
    comboOpt1->setCurrentIndex(comboOpt1->findText(config->getGnssSerialPort1Name()));

    gnss1Layout->addRow(new QLabel("Baudrate"), comboOpt2);
    comboOpt2->addItems(QStringList() << "1200" << "2400" << "4800" << "9600" << "19200" << "38400" << "57600" << "115200");
    comboOpt2->setToolTip("GNSS device baudrate");
    comboOpt2->setCurrentIndex(comboOpt2->findText(config->getGnssSerialPort1Baudrate()));

    gnss1Layout->addRow(cbOpt3);
    cbOpt3->setText("Log NMEA to file");
    cbOpt3->setToolTip("When checked all NMEA sentences will be logged to a logfile.\nA new logfile is started every day");
    cbOpt3->setChecked(config->getGnssSerialPort1LogToFile());

    gnss1Layout->addRow(cbOpt4);
    cbOpt4->setText("Monitor AGC level");
    cbOpt4->setToolTip("When checked the AGC receiver level will be monitored for sudden changes.\nOnly usable together with uBlox M7/M8 receivers.");
    cbOpt4->setChecked(config->getGnssSerialPort1MonitorAgc());

    gnss1Layout->addRow((cbOpt9));
    cbOpt9->setText("Detected incidents triggers spectrum recording");
    cbOpt9->setToolTip("Any incidents detected will cause a recording to be started, and uploaded if configured");
    cbOpt9->setChecked(config->getGnssSerialPort1TriggerRecording());

    gnss1GroupBox->setLayout(gnss1Layout);

    // second setup

    QGroupBox *gnss2GroupBox = new QGroupBox("Second GNSS receiver");
    QFormLayout *gnss2Layout = new QFormLayout;

    gnss2Layout->addRow(cbOpt5);
    cbOpt5->setText("Use this GNSS device");
    cbOpt5->setToolTip("Check this box to activate the GNSS surveillance for this port");
    cbOpt5->setChecked(config->getGnssSerialPort2Activate());

    gnss2Layout->addRow(new QLabel("Serial port name"), comboOpt3);
    comboOpt3->addItems(getAvailablePorts());
    comboOpt3->setToolTip("GNSS device serial port");
    comboOpt3->setCurrentIndex(comboOpt3->findText(config->getGnssSerialPort2Name()));

    gnss2Layout->addRow(new QLabel("Baudrate"), comboOpt4);
    comboOpt4->addItems(QStringList() << "1200" << "2400" << "4800" << "9600" << "19200" << "38400" << "57600" << "115200");
    comboOpt4->setToolTip("GNSS device baudrate");
    comboOpt4->setCurrentIndex(comboOpt4->findText(config->getGnssSerialPort2Baudrate()));

    gnss2Layout->addRow(cbOpt7);
    cbOpt7->setText("Log NMEA to file");
    cbOpt7->setToolTip("When checked all NMEA sentences will be logged to a logfile.\nA new logfile is started every day");
    cbOpt7->setChecked(config->getGnssSerialPort2LogToFile());

    gnss2Layout->addRow(cbOpt8);
    cbOpt8->setText("Monitor AGC level");
    cbOpt8->setToolTip("When checked the AGC receiver level will be monitored for sudden changes.\nOnly usable together with uBlox M7/M8 receivers.");
    cbOpt8->setChecked(config->getGnssSerialPort2MonitorAgc());

    gnss2Layout->addRow((cbOpt10));
    cbOpt10->setText("Detected incidents triggers spectrum recording");
    cbOpt10->setToolTip("Any incidents detected will cause a recording to be started, and uploaded if configured");
    cbOpt10->setChecked(config->getGnssSerialPort2TriggerRecording());

    gnss2GroupBox->setLayout(gnss2Layout);

    mainLayout->addWidget(gnss1GroupBox);
    mainLayout->addWidget(gnss2GroupBox);

    connect(btnBox, &QDialogButtonBox::accepted, this, &GnssOptions::saveCurrentSettings);
    connect(btnBox, &QDialogButtonBox::rejected, dialog, &QDialog::close);

    mainLayout->addWidget(btnBox);

}

QStringList GnssOptions::getAvailablePorts()
{
    QStringList list;
    for (auto &val : QSerialPortInfo::availablePorts())
        list.append(val.portName());
    return list;
}
void GnssOptions::start()
{
    dialog->exec();
}

void GnssOptions::saveCurrentSettings()
{
    config->setGnssSerialPort1Activate(cbOpt1->isChecked());
    config->setGnssSerialPort1Baudrate(comboOpt2->currentText());
    config->setGnssSerialPort1LogToFile(cbOpt3->isChecked());
    config->setGnssSerialPort1MonitorAgc(cbOpt4->isChecked());
    config->setGnssSerialPort1Name(comboOpt1->currentText());
    config->setGnssSerialPort1TriggerRecording(cbOpt9->isChecked());

    config->setGnssSerialPort2Activate(cbOpt5->isChecked());
    config->setGnssSerialPort2Baudrate(comboOpt4->currentText());
    config->setGnssSerialPort2LogToFile(cbOpt7->isChecked());
    config->setGnssSerialPort2MonitorAgc(cbOpt8->isChecked());
    config->setGnssSerialPort2Name(comboOpt3->currentText());
    config->setGnssSerialPort2TriggerRecording(cbOpt10->isChecked());

    dialog->close();
}
