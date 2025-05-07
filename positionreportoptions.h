#ifndef POSITIONREPORTOPTIONS_H
#define POSITIONREPORTOPTIONS_H

#include "config.h"
#include "optionsbaseclass.h"

class PositionReportOptions : public OptionsBaseClass
{
public:
    PositionReportOptions(QSharedPointer<Config> c);

public slots:
    void start();
    void saveCurrentSettings();
};

#endif // POSITIONREPORTOPTIONS_H
