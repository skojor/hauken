#include "versionupdater.h"

VersionUpdater::VersionUpdater(QSharedPointer<Config> c)
{
    config = c;

    QFile file(config->getWorkFolder() + "/" + ConfigFile);
    if (file.exists())
        handleConfigUpdates();

    // One time conversion from old MHz freq storage (< v.2.50.13)
    if (config->getInstrStartFreq() < 6e3)
        config->setInstrStartFreq(config->getInstrStartFreq() * 1e6);
    if (config->getInstrStopFreq() < 6e3)
        config->setInstrStopFreq(config->getInstrStopFreq() * 1e6);
    if (config->getInstrFfmCenterFreq() < 6e3)
        config->setInstrFfmCenterFreq(config->getInstrFfmCenterFreq() * 1e6);

    // OAuth update from old to new values
    if (config->getOAuth2AuthUrl().contains("oauth")) {
        config->setOAuth2AuthUrl(config->getOAuth2AuthUrl().split("/oauth").first());
    }
    if (config->getOAuth2Scope().contains("access_as_user")) {
        config->setOAuth2Scope(config->getOAuth2Scope().split("/access_as_user").first() + "/.default");
    }
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
            if (mode.contains("ifempty", Qt::CaseInsensitive) && !key.isEmpty() && !value.isEmpty()
                && !settings.value(key).isValid()) {
                settings.setValue(key, value);
                qInfo() << "Adding config key/value:" << key << value;
            } else if (mode.contains("overwrite", Qt::CaseInsensitive) && !key.isEmpty()
                       && !value.isEmpty()) {
                settings.setValue(key, value);
                qInfo() << "Overwriting config value:" << key << value;

            } else if (mode.contains("encr", Qt::CaseInsensitive) && !key.isEmpty()
                       && !value.isEmpty() && !settings.value(key).isValid()) {
                settings.setValue(key, config->simpleEncr(value.toLatin1()));
            }
        }
        file.close();
        file.remove();
    }
}
