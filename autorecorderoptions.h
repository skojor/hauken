#ifndef AUTORECORDEROPTIONS_H
#define AUTORECORDEROPTIONS_H

#include "optionsbaseclass.h"
#include "config.h"

class AutoRecorderOptions : public OptionsBaseClass
{
public:
    AutoRecorderOptions(QSharedPointer<Config> c);
public slots:
    void start();
    void saveCurrentSettings();

private slots:

};

#endif // AUTORECORDEROPTIONS_H
