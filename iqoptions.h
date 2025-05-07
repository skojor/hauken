#ifndef IQOPTIONS_H
#define IQOPTIONS_H

#include <QObject>
#include "optionsbaseclass.h"

class IqOptions : public OptionsBaseClass
{
public:
    IqOptions(QSharedPointer<Config> c);

public slots:
    void start();
    void saveCurrentSettings();
};

#endif // IQOPTIONS_H
