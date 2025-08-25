#ifndef EMAILOPTIONS_H
#define EMAILOPTIONS_H

#include "config.h"
#include "optionsbaseclass.h"

class EmailOptions : public OptionsBaseClass
{
public:
    EmailOptions(QSharedPointer<Config> c);

public slots:
    void start();
    void saveCurrentSettings();

private slots:

};

#endif // EMAILOPTIONS_H
