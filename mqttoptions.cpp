#include "mqttoptions.h"

MqttOptions::MqttOptions(QSharedPointer<Config> c)
{
    config = c;
    setWindowTitle("MQTT configuration");


    connect(btnBox, &QDialogButtonBox::accepted, this, &MqttOptions::saveCurrentSettings);
    connect(btnBox, &QDialogButtonBox::rejected, dialog, &QDialog::close);


}

void MqttOptions::start()
{
    /*comboOpt1->setCurrentIndex(comboOpt1->findText(config->getArduinoSerialName()));
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
*/
    dialog->exec();
}

void MqttOptions::saveCurrentSettings()
{
    /*config->setArduinoSerialName(comboOpt1->currentText());
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
*/
    dialog->close();
}
