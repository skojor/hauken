#ifndef SDEFOPTIONS_H
#define SDEFOPTIONS_H

#include "optionsbaseclass.h"

class SdefOptions : public OptionsBaseClass
{
public:
    SdefOptions(QSharedPointer<Config> c);

signals:

public slots:
    void start();
    void saveCurrentSettings();
};

#endif // SDEFOPTIONS_H
