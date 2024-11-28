#ifndef OAUTHOPTIONS_H
#define OAUTHOPTIONS_H

#include "config.h"
#include "optionsbaseclass.h"

class OAuthOptions : public OptionsBaseClass
{
public:
    OAuthOptions(QSharedPointer<Config> c);

public slots:
    void start();
    void saveCurrentSettings();

};

#endif // OAUTHOPTIONS_H
