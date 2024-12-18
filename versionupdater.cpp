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
            QString mode, key, value;
            ts >> mode >> key >> value;
            if (mode.contains("ifempty", Qt::CaseInsensitive) && !key.isEmpty() && !value.isEmpty() && !settings.value(key).isNull()) {
                settings.setValue(key, value);
                qInfo() << "Adding config key/value:" << key << value;
            }
            else if (mode.contains("overwrite", Qt::CaseInsensitive) && !key.isEmpty() && !value.isEmpty()) {
                settings.setValue(key, value);
                qInfo() << "Overwriting config value:" << key << value;
            }
        }
        file.close();
        file.remove();
    }
}
