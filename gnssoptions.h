#ifndef GNSSOPTIONS_H
#define GNSSOPTIONS_H

#include <QSerialPortInfo>
#include "config.h"
#include "optionsbaseclass.h"

class GnssOptions : public OptionsBaseClass
{
public:
    GnssOptions(QSharedPointer<Config> c);

public slots:
    void start();
    void saveCurrentSettings();

private slots:
    QStringList getAvailablePorts();
};

#endif // GNSSOPTIONS_H
