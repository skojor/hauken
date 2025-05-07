#ifndef GENERALOPTIONS_H
#define GENERALOPTIONS_H

#include "optionsbaseclass.h"

class GeneralOptions : public OptionsBaseClass
{
    Q_OBJECT
public:
    GeneralOptions(QSharedPointer<Config> c);

public slots:
    void start();
    void saveCurrentSettings();
};

#endif // GENERALOPTIONS_H
