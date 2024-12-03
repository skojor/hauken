#ifndef VERSIONUPDATER_H
#define VERSIONUPDATER_H

#include <QObject>
#include <QFile>
#include "optionsbaseclass.h"

#define ConfigFile "ConfigUpdates.cfg"

/*
 * Class to handle TODO's when version updates.
 * Looks for files copied in from installer.
 */

class VersionUpdater : public QObject
{
    Q_OBJECT
public:
    VersionUpdater(QSharedPointer<Config> c);

private:
    void handleConfigUpdates();
    QSharedPointer<Config> config;
};

#endif // VERSIONUPDATER_H
