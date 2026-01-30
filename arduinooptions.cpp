#include "arduinooptions.h"

ArduinoOptions::ArduinoOptions(QSharedPointer<Config> c)
{
    config = c;
    auto mainLayout = new QFormLayout(this);

    setWindowTitle("Arduino configuration");

    mainLayout->addRow(cbOpt2);
    cbOpt2->setText("Enable Arduino option");
    cbOpt2->setToolTip("Will try to connect to Arduino at the given serial port");

    mainLayout->addRow(new QLabel("Serial port name"), comboOpt1);
    comboOpt1->addItems(getAvailablePorts());
    comboOpt1->setToolTip("Arduino serial port");

    mainLayout->addRow(new QLabel("Baudrate"), comboOpt2);
    comboOpt2->addItems(QStringList() << "1200" << "2400" << "4800" << "9600" << "19200" << "38400" << "57600" << "115200");
    comboOpt2->setToolTip("Arduino baudrate");

    mainLayout->addRow(cbOpt1);
    cbOpt1->setText("Arduino with temperature and RF relay control");
    cbOpt1->setToolTip("Enable only if analog temperature sensor and relay port is connected to the Arduino");

    mainLayout->addRow(cbOpt3);
    cbOpt3->setText("Arduino with temperature and humidity sensor (DHT20)");
    cbOpt3->setToolTip("Enable only if DHT20 sensor is connected to the Arduino");

    mainLayout->addRow(cbOpt4);
    cbOpt4->setText("Arduino with Watchdog relay function");
    cbOpt4->setToolTip("Enable only if Arduino is connected to a relay watchdog function (see also activate watchdog below");

    mainLayout->addRow(cbOpt5);
    cbOpt5->setText("Activate Arduino watchdog function");
    cbOpt5->setToolTip("This will activate the Arduino watchdog on program start, and periodically send a null string to the serial line to reset the watchdog.\n" \
                       "If the program crashes the watchdog will eventually switch the relay ON (thus disconnect the load on NC), and switch relay back OFF.\n");

    mainLayout->addRow(new QLabel("Watchdog ping address"), leOpt3);
    leOpt3->setToolTip("If set the watchdog will be reset only if the given address is pingable");

    mainLayout->addRow(new QLabel("Seconds between ping attempts"), sbOpt1);
    sbOpt1->setToolTip("How long to wait between each ping attempt");

   /* connect(btnBox, &QDialogButtonBox::accepted, this, &ArduinoOptions::saveCurrentSettings);
    connect(btnBox, &QDialogButtonBox::rejected, dialog, &QDialog::close);*/

    mainLayout->addRow(new QLabel("RF relay on text"), leOpt1);
    mainLayout->addRow(new QLabel("RF relay off text"), leOpt2);

    mainLayout->addRow(new QLabel("A restart is needed to activate any changes here"));
    mainLayout->addWidget(btnBox);

    comboOpt1->setCurrentIndex(comboOpt1->findText(config->getArduinoSerialName()));
    comboOpt2->setCurrentIndex(comboOpt2->findText(config->getArduinoBaudrate()));
    cbOpt1->setChecked(config->getArduinoReadTemperatureAndRelay());
    cbOpt2->setChecked(config->getArduinoEnable());
    cbOpt3->setChecked(config->getArduinoReadDHT20());
    cbOpt4->setChecked(config->getArduinoWatchdogRelay());
    leOpt1->setText(config->getArduinoRelayOnText());
    leOpt2->setText(config->getArduinoRelayOffText());

    cbOpt5->setEnabled(cbOpt4->isChecked());
    cbOpt5->setChecked(config->getArduinoActivateWatchdog());

    leOpt3->setText(config->getArduinoPingAddress());
    sbOpt1->setValue(config->getArduinoPingInterval());

    connect(cbOpt1, &QCheckBox::clicked, this, [this](bool b) {
        this->cbOpt3->setChecked(false);
        this->cbOpt4->setChecked(false);
        if (b) {
            this->cbOpt5->setChecked(false);
            this->cbOpt5->setDisabled(true);
        }

    });
    connect(cbOpt3, &QCheckBox::clicked, this, [this](bool b) {
        this->cbOpt1->setChecked(false);
        //this->cbOpt4->setChecked(false);
        if (b) {
            this->cbOpt5->setChecked(false);
            this->cbOpt5->setDisabled(true);
        }
    });
    connect(cbOpt4, &QCheckBox::clicked, this, [this](bool b) {
        this->cbOpt1->setChecked(false);
        //this->cbOpt3->setChecked(false);
        this->cbOpt5->setEnabled(b);
        //this->leOpt3->setEnabled(b);
        this->sbOpt1->setEnabled(b);
        if (!b && cbOpt5->isChecked()) {
            this->cbOpt5->setChecked(false);
            this->cbOpt5->setEnabled(false);
            //this->leOpt3->setEnabled(false);
            //this->sbOpt1->setEnabled(false);
        }
    });
}

QStringList ArduinoOptions::getAvailablePorts()
{
    QStringList list;
    for (auto &val : QSerialPortInfo::availablePorts())
        list.append(val.portName());
    return list;
}

void ArduinoOptions::start()
{

    dialog->exec();
}

void ArduinoOptions::saveCurrentSettings()
{
    config->setArduinoSerialName(comboOpt1->currentText());
    config->setArduinoBaudrate(comboOpt2->currentText());
    config->setArduinoReadTemperatureAndRelay(cbOpt1->isChecked());
    config->setArduinoEnable(cbOpt2->isChecked());
    config->setArduinoReadDHT20(cbOpt3->isChecked());
    config->setArduinoWatchdogRelay(cbOpt4->isChecked());
    config->setArduinoRelayOnText(leOpt1->text());
    config->setArduinoRelayOffText(leOpt2->text());
    config->setArduinoActivateWatchdog(cbOpt5->isChecked());
    config->setArduinoPingAddress(leOpt3->text());
    config->setArduinoPingInterval(sbOpt1->value());

    //dialog->close();
}
