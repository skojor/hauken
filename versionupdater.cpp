#include "versionupdater.h"

VersionUpdater::VersionUpdater(QSharedPointer<Config> c)
{
    config = c;

    QFile file(config->getWorkFolder() + "/" + ConfigFile);
    if (file.exists())
        handleConfigUpdates();
}

void VersionUpdater::handleConfigUpdates()
{
    QSettings settings(config->getCurrentFilename(), QSettings::IniFormat);

    QFile file(config->getWorkFolder() + "/" + ConfigFile);
    if (!file.open(QIODevice::ReadOnly)) {
        qDebug() << "Found config update file, but cannot open it. Giving up";
    }
    else {
        QTextStream ts(&file);

        while (!ts.atEnd()) {
            QString key, value;
            ts >> key >> value;
            if (!key.isEmpty() && !value.isEmpty() && !settings.contains(key)) settings.setValue(key, value);
            qDebug() << "Adding config key/value:" << key << value;
        }
        file.close();
        file.remove();
    }
}
