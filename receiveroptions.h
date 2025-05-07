#ifndef RECEIVEROPTIONS_H
#define RECEIVEROPTIONS_H

#include "optionsbaseclass.h"

class ReceiverOptions : public OptionsBaseClass
{
    Q_OBJECT
public:
    ReceiverOptions(QSharedPointer<Config> c);

signals:

public slots:
    void start();
    void saveCurrentSettings();
};

#endif // RECEIVEROPTIONS_H
