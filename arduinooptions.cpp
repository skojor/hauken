#include "arduinooptions.h"

ArduinoOptions::ArduinoOptions(QSharedPointer<Config> c)
{
    config = c;
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
    cbOpt1->setText("Read temperature from device");
    cbOpt1->setToolTip("Enable only if temperature sensor is connected");
    connect(btnBox, &QDialogButtonBox::accepted, this, &ArduinoOptions::saveCurrentSettings);
    connect(btnBox, &QDialogButtonBox::rejected, dialog, &QDialog::close);

    mainLayout->addRow(new QLabel("Relay on text"), leOpt1);
    mainLayout->addRow(new QLabel("Relay off text"), leOpt2);

    mainLayout->addRow(new QLabel("A restart is needed to activate any changes here"));
    mainLayout->addWidget(btnBox);
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
    comboOpt1->setCurrentIndex(comboOpt1->findText(config->getArduinoSerialName()));
    comboOpt2->setCurrentIndex(comboOpt2->findText(config->getArduinoBaudrate()));
    cbOpt1->setChecked(config->getArduinoReadTemperature());
    cbOpt2->setChecked(config->getArduinoEnable());
    leOpt1->setText(config->getArduinoRelayOnText());
    leOpt2->setText(config->getArduinoRelayOffText());
    dialog->exec();
}

void ArduinoOptions::saveCurrentSettings()
{
    config->setArduinoSerialName(comboOpt1->currentText());
    config->setArduinoBaudrate(comboOpt2->currentText());
    config->setArduinoReadTemperature(cbOpt1->isChecked());
    config->setArduinoEnable(cbOpt2->isChecked());
    config->setArduinoRelayOnText(leOpt1->text());
    config->setArduinoRelayOffText(leOpt2->text());
    dialog->close();
}
