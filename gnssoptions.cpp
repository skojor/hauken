#include "gnssoptions.h"

GnssOptions::GnssOptions(QSharedPointer<Config> c)
    : OptionsBaseClass{}
{
    config = c;
    setWindowTitle("GNSS receiver configuration");

    comboOpt1->setEditable(true);
    comboOpt2->setEditable(true);
    comboOpt3->setEditable(true);
    comboOpt4->setEditable(true);

    QGroupBox *gnss1GroupBox = new QGroupBox("First GNSS receiver");
    QFormLayout *gnss1Layout = new QFormLayout;

    gnss1Layout->addRow(cbOpt1);
    cbOpt1->setText("Use this GNSS device");
    cbOpt1->setToolTip("Check this box to activate the GNSS surveillance for this port");

    gnss1Layout->addRow(new QLabel("Serial port name or IP address"), comboOpt1);
    comboOpt1->addItems(getAvailablePorts());
    if (!config->getGnssSerialPort1Name().isEmpty()) comboOpt1->addItem(config->getGnssSerialPort1Name());
    comboOpt1->setToolTip("GNSS device serial port, or TCP/IP server sending raw NMEA and/or uBlox binary data");

    gnss1Layout->addRow(new QLabel("Baudrate or port number"), comboOpt2);
    comboOpt2->addItems(QStringList() << "1200" << "2400" << "4800" << "9600" << "19200" << "38400" << "57600" << "115200" << config->getGnssSerialPort1Baudrate());
    comboOpt2->setToolTip("GNSS device baudrate, or TCP port number if IP address is used above");

    gnss1Layout->addRow(cbOpt3);
    cbOpt3->setText("Log NMEA to file");
    cbOpt3->setToolTip("When checked all NMEA sentences will be logged to a logfile.\nA new logfile is started every day");

    gnss1Layout->addRow(cbOpt4);
    cbOpt4->setText("Set up as uBlox M8/M9");
    cbOpt4->setToolTip("If checked the GPS will be configured as a uBlox receiver with enhanced binary protocol. \n"\
                       "Will enable several cool features, for now AGC and jamming indicator levels.\n"\
                       "Of course this will only work if the connected GNSS device is a uBlox. Use at your own risk!");

    gnss1Layout->addRow((cbOpt9));
    cbOpt9->setText("Detected incidents triggers spectrum recording");
    cbOpt9->setToolTip("Any incidents detected will cause a recording to be started, and uploaded if configured");

    gnss1GroupBox->setLayout(gnss1Layout);

    // second setup

    QGroupBox *gnss2GroupBox = new QGroupBox("Second GNSS receiver");
    QFormLayout *gnss2Layout = new QFormLayout;

    gnss2Layout->addRow(cbOpt5);
    cbOpt5->setText("Use this GNSS device");
    cbOpt5->setToolTip("Check this box to activate the GNSS surveillance for this port");

    gnss2Layout->addRow(new QLabel("Serial port name or IP address"), comboOpt3);
    comboOpt3->addItems(getAvailablePorts());
    if (!config->getGnssSerialPort2Name().isEmpty()) comboOpt3->addItem(config->getGnssSerialPort2Name());
    comboOpt3->setToolTip("GNSS device serial port, or TCP/IP server sending raw NMEA and/or uBlox binary data");

    gnss2Layout->addRow(new QLabel("Baudrate or port number"), comboOpt4);
    comboOpt4->addItems(QStringList() << "1200" << "2400" << "4800" << "9600" << "19200" << "38400" << "57600" << "115200" << config->getGnssSerialPort2Baudrate());
    comboOpt4->setToolTip("GNSS device baudrate, or TCP port number if IP address is used above");

    gnss2Layout->addRow(cbOpt7);
    cbOpt7->setText("Log NMEA to file");
    cbOpt7->setToolTip("When checked all NMEA sentences will be logged to a logfile.\nA new logfile is started every day");

    gnss2Layout->addRow(cbOpt8);
    cbOpt8->setText("Set up as uBlox M8/M9");
    cbOpt8->setToolTip("If checked the GPS will be configured as a uBlox receiver with enhanced binary protocol. \n"\
                       "Will enable several cool features, for now AGC and jamming indicator levels.\n"\
                       "Of course this will only work if the connected GNSS device is a uBlox. Use at your own risk!");

    gnss2Layout->addRow((cbOpt10));
    cbOpt10->setText("Detected incidents triggers spectrum recording");
    cbOpt10->setToolTip("Any incidents detected will cause a recording to be started, and uploaded if configured");

    gnss2GroupBox->setLayout(gnss2Layout);

    QGroupBox *gnss3GroupBox = new QGroupBox("Use instrument GNSS receiver");
    QFormLayout *gnss3Layout = new QFormLayout;
    gnss3GroupBox->setLayout(gnss3Layout);

    gnss3Layout->addRow(cbOpt11);
    cbOpt11->setText("Use instrumentGNSS to validate position/time offset");
    cbOpt11->setToolTip("Read position and time data from supported instruments and compare with position/time offsets.\n" \
                        "Currently supported instruments are R&S PR100, PR200, EM100, EM200, EB500, and Ettus E310");
    gnss3Layout->addRow((cbOpt12));
    cbOpt12->setText("Detected incidents triggers spectrum recording");
    cbOpt12->setToolTip("Any incidents detected will cause a recording to be started, and uploaded if configured");

    QGroupBox *gnss4GroupBox = new QGroupBox("GNSS data display");
    QFormLayout *gnss4Layout = new QFormLayout;
    gnss4GroupBox->setLayout(gnss4Layout);
    gnss4Layout->addRow(cbOpt13);
    cbOpt13->setText("Show GNSS data in a separate window");
    cbOpt13->setToolTip("Enabling this option opens a new window showing vital GNSS information for one or both GNSS receivers");

    cbOpt14->setText("GNSS messages to incident log");
    cbOpt14->setToolTip("Enable this option to show GNSS messages in the incident log");
    gnss4Layout->addRow(cbOpt14);

    gnss4Layout->addRow(new QLabel(tr("Incident filter time (sec)")), sbOpt1);
    sbOpt1->setToolTip("Any incidents will be filtered until the set filter time has passed. If incident is gone before the set time it will not appear in the log.");
    sbOpt1->setRange(1, 86400);

    gnss4Layout->addRow(tr("GNSS 1 name"), leOpt1);
    leOpt1->setToolTip(tr("Name will be displayed on separate window, in title"));

    gnss4Layout->addRow(tr("GNSS 2 name"), leOpt2);
    leOpt2->setToolTip(tr("Name will be displayed on separate window, in title"));

    auto mainLayout = new QFormLayout(this);

    mainLayout->addWidget(gnss1GroupBox);
    mainLayout->addWidget(gnss2GroupBox);
    mainLayout->addWidget(gnss3GroupBox);
    mainLayout->addWidget(gnss4GroupBox);

    mainLayout->addWidget(new QLabel("<span style='font-size:normal;color:red;'>Restart the program after changing serial ports!</span>"));

    /*connect(btnBox, &QDialogButtonBox::accepted, this, &GnssOptions::saveCurrentSettings);
    connect(btnBox, &QDialogButtonBox::rejected, dialog, &QDialog::close);

    mainLayout->addWidget(btnBox);*/
    cbOpt1->setChecked(config->getGnssSerialPort1Activate());
    comboOpt1->setCurrentIndex(comboOpt1->findText(config->getGnssSerialPort1Name()));
    comboOpt2->setCurrentIndex(comboOpt2->findText(config->getGnssSerialPort1Baudrate()));
    cbOpt3->setChecked(config->getGnssSerialPort1LogToFile());
    cbOpt4->setChecked(config->getGnssSerialPort1MonitorAgc());
    cbOpt9->setChecked(config->getGnssSerialPort1TriggerRecording());
    cbOpt5->setChecked(config->getGnssSerialPort2Activate());
    comboOpt3->setCurrentIndex(comboOpt3->findText(config->getGnssSerialPort2Name()));
    comboOpt4->setCurrentIndex(comboOpt4->findText(config->getGnssSerialPort2Baudrate()));
    cbOpt7->setChecked(config->getGnssSerialPort2LogToFile());
    cbOpt8->setChecked(config->getGnssSerialPort2MonitorAgc());
    cbOpt10->setChecked(config->getGnssSerialPort2TriggerRecording());
    cbOpt11->setChecked(config->getGnssUseInstrumentGnss());
    cbOpt12->setChecked(config->getGnssInstrumentGnssTriggerRecording());
    cbOpt13->setChecked(config->getGnssDisplayWidget());
    leOpt1->setText(config->getGnss1Name());
    leOpt2->setText(config->getGnss2Name());
    cbOpt14->setChecked(config->getGnssShowNotifications());
    sbOpt1->setValue(config->getGnssTimeFilter());
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
    //dialog->exec();
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
    config->setGnssUseInstrumentGnss(cbOpt11->isChecked());
    config->setGnssInstrumentGnssTriggerRecording(cbOpt12->isChecked());
    config->setGnssDisplayWidget(cbOpt13->isChecked());
    config->setGnss1Name(leOpt1->text());
    config->setGnss2Name(leOpt2->text());
    config->setGnssShowNotifications(cbOpt14->isChecked());
    config->setGnssTimeFilter(sbOpt1->value());

    //dialog->close();
}
