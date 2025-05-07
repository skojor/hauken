#ifndef ARDUINOOPTIONS_H
#define ARDUINOOPTIONS_H

#include "config.h"
#include "optionsbaseclass.h"
#include <QSerialPortInfo>

class ArduinoOptions : public OptionsBaseClass
{
public:
    ArduinoOptions(QSharedPointer<Config> c);

public slots:
    void start();
    void saveCurrentSettings();

private slots:
    QStringList getAvailablePorts();
};

#endif // ARDUINOOPTIONS_H
