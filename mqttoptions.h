#ifndef MQTTOPTIONS_H
#define MQTTOPTIONS_H

#include "config.h"
#include "optionsbaseclass.h"

class MqttOptions : public OptionsBaseClass
{
public:
    MqttOptions(QSharedPointer<Config> c);

public slots:
    void start();
    void saveCurrentSettings();

private:

};

#endif // MQTTOPTIONS_H
