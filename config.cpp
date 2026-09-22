#include "config.h"
#include "version.h"
#include "asciitranslator.h"


Config::Config(QObject *parent)
    : QObject(parent)
{
    basicSettings = new QSettings("Nkom", "Hauken"); //(findWorkFolderName() + "/hauken.conf");

    const QString defaultWorkFolder = basicSettings->value("workFolder", findWorkFolderName()).toString();
    const QString defaultConfigFile = QDir(defaultWorkFolder).filePath("default.ini");

    curFile = basicSettings->value("lastFile", defaultConfigFile).toString();
    if (!basicSettings->value("lastFile").isValid()) basicSettings->setValue("lastFile", defaultConfigFile);

    settings = new QSettings(curFile, QSettings::IniFormat);
    migrateGnssProfilesIfNeeded();

    const QString configuredWorkFolder = settings->value("workFolder").toString();
    const QString oldDefaultConfigFile = QDir(findWorkFolderName()).filePath("default.ini");
    if (!configuredWorkFolder.isEmpty()
        && QFileInfo(curFile).absoluteFilePath().compare(QFileInfo(oldDefaultConfigFile).absoluteFilePath(),
                                                         Qt::CaseInsensitive) == 0
        && QDir::cleanPath(configuredWorkFolder).compare(QDir::cleanPath(findWorkFolderName()),
                                                         Qt::CaseInsensitive) != 0) {
        const QString configuredDefaultConfigFile = QDir(configuredWorkFolder).filePath("default.ini");
        QDir().mkpath(configuredWorkFolder);
        if (!QFileInfo::exists(configuredDefaultConfigFile)) {
            QFile::copy(curFile, configuredDefaultConfigFile);
        }
        newFileName(configuredDefaultConfigFile);
    }
}

void Config::newFileName(const QString file)
{
    curFile = file;
    basicSettings->setValue("lastFile", curFile);
    delete settings;
    settings = new QSettings(curFile, QSettings::IniFormat);
    settings->setValue("SW_VERSION", FULL_VERSION);
    migrateGnssProfilesIfNeeded();
}

QStringList Config::getMqttBrokerProfileIds() const
{
    return settings->value("mqtt/brokerProfileIds").toStringList();
}

QList<MqttBrokerProfile> Config::getMqttBrokerProfiles() const
{
    QList<MqttBrokerProfile> profiles;
    const QStringList ids = getMqttBrokerProfileIds();
    profiles.reserve(ids.size());
    for (const QString &id : ids) {
        const MqttBrokerProfile profile = getMqttBrokerProfile(id);
        if (!profile.id.isEmpty()) profiles.append(profile);
    }
    return profiles;
}

MqttBrokerProfile Config::getMqttBrokerProfile(const QString &id) const
{
    MqttBrokerProfile profile;
    if (id.isEmpty() || !getMqttBrokerProfileIds().contains(id)) return profile;

    const QString prefix = "mqtt/brokers/" + id + "/";
    profile.id = id;
    profile.name = settings->value(prefix + "name", id).toString().trimmed();
    profile.enabled = settings->value(prefix + "enabled", false).toBool();
    profile.server = settings->value(prefix + "server").toString().trimmed();
    profile.username = settings->value(prefix + "username").toString().trimmed();
    profile.password = simpleEncr(settings->value(prefix + "password").toByteArray());
    profile.port = settings->value(prefix + "port", 1883).toInt();
    profile.keepaliveTopic = settings->value(prefix + "keepaliveTopic").toString().trimmed();
    profile.subNames = settings->value(prefix + "subNames").toStringList();
    profile.subTopics = settings->value(prefix + "subTopics").toStringList();
    profile.subToIncidentlog = settings->value(prefix + "subToIncidentlog").toStringList();
    profile.primary = settings->value(prefix + "primary", false).toBool();
    return profile;
}

void Config::setMqttBrokerProfile(const MqttBrokerProfile &profile)
{
    const QString id = profile.id.trimmed();
    if (id.isEmpty()) return;

    QStringList ids = getMqttBrokerProfileIds();
    if (profile.primary) {
        for (const QString &existingId : ids) {
            if (existingId != id)
                settings->setValue("mqtt/brokers/" + existingId + "/primary", false);
        }
    }

    const QString prefix = "mqtt/brokers/" + id + "/";
    settings->setValue(prefix + "name", profile.name.trimmed());
    settings->setValue(prefix + "enabled", profile.enabled);
    settings->setValue(prefix + "server", profile.server.trimmed());
    settings->setValue(prefix + "username", profile.username.trimmed());
    settings->setValue(prefix + "password", simpleEncr(profile.password.simplified().toLocal8Bit()));
    settings->setValue(prefix + "port", profile.port);
    settings->setValue(prefix + "keepaliveTopic", profile.keepaliveTopic.trimmed());
    settings->setValue(prefix + "subNames", profile.subNames);
    settings->setValue(prefix + "subTopics", profile.subTopics);
    settings->setValue(prefix + "subToIncidentlog", profile.subToIncidentlog);
    settings->setValue(prefix + "primary", profile.primary);

    if (!ids.contains(id)) {
        ids.append(id);
        settings->setValue("mqtt/brokerProfileIds", ids);
    }
}

void Config::removeMqttBrokerProfile(const QString &id)
{
    QStringList ids = getMqttBrokerProfileIds();
    if (!ids.removeOne(id)) return;

    settings->remove("mqtt/brokers/" + id);
    settings->setValue("mqtt/brokerProfileIds", ids);
}

bool Config::hasMqttBrokerProfiles() const
{
    return !getMqttBrokerProfileIds().isEmpty();
}

QStringList Config::getGnssReceiverProfileIds() const
{
    return settings->value("gnss/receiverProfileIds").toStringList();
}

QList<GnssReceiverProfile> Config::getGnssReceiverProfiles() const
{
    QList<GnssReceiverProfile> profiles;
    const QStringList ids = getGnssReceiverProfileIds();
    profiles.reserve(ids.size());
    for (const QString &id : ids) {
        const GnssReceiverProfile profile = getGnssReceiverProfile(id);
        if (!profile.id.isEmpty()) profiles.append(profile);
    }
    return profiles;
}

GnssReceiverProfile Config::getGnssReceiverProfile(const QString &id) const
{
    GnssReceiverProfile profile;
    if (id.isEmpty() || !getGnssReceiverProfileIds().contains(id)) return profile;

    const QString prefix = "gnss/receivers/" + id + "/";
    profile.id = id;
    profile.name = settings->value(prefix + "name", id).toString().trimmed();
    profile.type = settings->value(prefix + "type", "Serial").toString() == "Instrument"
                       ? GnssReceiverProfile::Type::Instrument : GnssReceiverProfile::Type::Serial;
    profile.enabled = settings->value(prefix + "enabled", false).toBool();
    profile.portName = settings->value(prefix + "portName").toString().trimmed();
    profile.baudrate = settings->value(prefix + "baudrate", "4800").toString();
    profile.ublox = settings->value(prefix + "ublox", false).toBool();
    profile.logToFile = settings->value(prefix + "logToFile", false).toBool();
    profile.triggerRecording = settings->value(prefix + "triggerRecording", false).toBool();
    profile.nonRemovable = settings->value(prefix + "nonRemovable", false).toBool();
    return profile;
}

void Config::setGnssReceiverProfile(const GnssReceiverProfile &profile)
{
    const QString id = profile.id.trimmed();
    if (id.isEmpty()) return;

    const QString prefix = "gnss/receivers/" + id + "/";
    settings->setValue(prefix + "name", profile.name.trimmed());
    settings->setValue(prefix + "type", profile.type == GnssReceiverProfile::Type::Instrument ? "Instrument" : "Serial");
    settings->setValue(prefix + "enabled", profile.enabled);
    settings->setValue(prefix + "portName", profile.portName.trimmed());
    settings->setValue(prefix + "baudrate", profile.baudrate.trimmed());
    settings->setValue(prefix + "ublox", profile.ublox);
    settings->setValue(prefix + "logToFile", profile.logToFile);
    settings->setValue(prefix + "triggerRecording", profile.triggerRecording);
    settings->setValue(prefix + "nonRemovable", profile.nonRemovable);

    QStringList ids = getGnssReceiverProfileIds();
    if (!ids.contains(id)) {
        ids.append(id);
        settings->setValue("gnss/receiverProfileIds", ids);
    }
}

void Config::removeGnssReceiverProfile(const QString &id)
{
    QStringList ids = getGnssReceiverProfileIds();
    if (!ids.removeOne(id)) return;

    settings->remove("gnss/receivers/" + id);
    settings->setValue("gnss/receiverProfileIds", ids);
}

bool Config::hasGnssReceiverProfiles() const
{
    return !getGnssReceiverProfileIds().isEmpty();
}

void Config::migrateGnssProfilesIfNeeded()
{
    if (settings->value("gnss/migratedToProfiles", false).toBool()) return;
    settings->setValue("gnss/migratedToProfiles", true);
    if (hasGnssReceiverProfiles()) return; // user already has profiles, e.g. from a shared config file

    GnssReceiverProfile receiver1;
    receiver1.id = "legacy-1";
    receiver1.name = settings->value("gnss/gnss1Name", "GNSS 1").toString().trimmed();
    if (receiver1.name.isEmpty()) receiver1.name = "GNSS 1";
    receiver1.type = GnssReceiverProfile::Type::Serial;
    receiver1.enabled = settings->value("gnss/SerialPort1activate", false).toBool();
    receiver1.portName = settings->value("gnss/SerialPort1Name").toString().trimmed();
    receiver1.baudrate = settings->value("gnss/SerialPort1Baudrate", "4800").toString();
    receiver1.ublox = settings->value("gnss/SerialPort1MonitorAgc", false).toBool();
    receiver1.logToFile = settings->value("gnss/SerialPort1LogToFile", false).toBool();
    receiver1.triggerRecording = settings->value("gnss/SerialPort1TriggerRecording", false).toBool();
    setGnssReceiverProfile(receiver1);

    GnssReceiverProfile receiver2;
    receiver2.id = "legacy-2";
    receiver2.name = settings->value("gnss/gnss2Name", "GNSS 2").toString().trimmed();
    if (receiver2.name.isEmpty()) receiver2.name = "GNSS 2";
    receiver2.type = GnssReceiverProfile::Type::Serial;
    receiver2.enabled = settings->value("gnss/SerialPort2activate", false).toBool();
    receiver2.portName = settings->value("gnss/SerialPort2Name").toString().trimmed();
    receiver2.baudrate = settings->value("gnss/SerialPort2Baudrate", "4800").toString();
    receiver2.ublox = settings->value("gnss/SerialPort2MonitorAgc", false).toBool();
    receiver2.logToFile = settings->value("gnss/SerialPort2LogToFile", false).toBool();
    receiver2.triggerRecording = settings->value("gnss/SerialPort2TriggerRecording", false).toBool();
    setGnssReceiverProfile(receiver2);

    GnssReceiverProfile instrument;
    instrument.id = "instrument";
    instrument.name = "Instrument GNSS";
    instrument.type = GnssReceiverProfile::Type::Instrument;
    instrument.enabled = settings->value("gnss/UseInstrumentGnss", false).toBool();
    instrument.triggerRecording = settings->value("gnss/InstrumentGnssTriggerRecording", false).toBool();
    instrument.nonRemovable = true;
    setGnssReceiverProfile(instrument);

    // Remap legacy free-text position-source selections to the new profile ids
    const QString oldSdefSource = settings->value("sdef/GpsSource").toString();
    if (oldSdefSource == "InstrumentGnss") settings->setValue("sdef/GpsSource", "instrument");
    else if (oldSdefSource.contains("1")) settings->setValue("sdef/GpsSource", "legacy-1");
    else if (oldSdefSource.contains("2")) settings->setValue("sdef/GpsSource", "legacy-2");

    const QString oldPosReportSource = settings->value("posReport/source").toString();
    if (oldPosReportSource.isEmpty() || oldPosReportSource.compare("InstrumentGNSS", Qt::CaseInsensitive) == 0)
        settings->setValue("posReport/source", "instrument");
    else if (oldPosReportSource.contains("1")) settings->setValue("posReport/source", "legacy-1");
    else if (oldPosReportSource.contains("2")) settings->setValue("posReport/source", "legacy-2");
    else if (oldPosReportSource.compare("Manual position", Qt::CaseInsensitive) == 0)
        settings->setValue("posReport/source", "Manual");
}

QString Config::getWorkFolder()
{
    const QString folder = settings->value("workFolder",
                                           basicSettings->value("workFolder", findWorkFolderName())).toString();
    if (!folder.isEmpty()) {
        QDir().mkpath(folder);
    }
    return folder;
}

void Config::setWorkFolder(QString s)
{
    settings->setValue("workFolder", s);
    basicSettings->setValue("workFolder", s);
    if (!s.isEmpty()) {
        QDir().mkpath(s);
    }
}

QString Config::getLogFolder()
{
    const QString folder = settings->value("logFolder", QDir(getWorkFolder()).filePath("logs")).toString();
    if (!folder.isEmpty()) {
        QDir().mkpath(folder);
    }
    return folder;
}

void Config::setLogFolder(QString s)
{
    settings->setValue("logFolder", s);
    if (!s.isEmpty()) {
        QDir().mkpath(s);
    }
}

QByteArray Config::simpleEncr(QByteArray toEncrypt) const
{
    QByteArray key = "6NrqMXHFqBv3QBm0cZjo/0PAzsIbam+hhsWI7PLkT4Wt5biPOXMis2qh7eEw6dksSnu1XwNaIza4vLw+vm7lhnp+aNyZPrVqQcMDKRTyq1rXg1ZRzXEtjCxESRx7KcRbi24t+GXgcnNQB706JEqxMvCukyia+cK7VCGSdIYskDb6U/jqeb+QxnD5s1g6CeYHswbWpEuwCMVSZDk1vkSBZHE4oHTnjiwdb3bMvVzPW9KKQ75WGwU7trqNyLMtZS7JrVrxFX2LHov2qh7eEw6dksSnu1XwNaIza4vLw+vm7lhnp+aNyZPrVqQcMDKRTyq1rXg1ZRzXEtjCxESRx7KcRbi24t+GXgcnNQB706JEqxMvCukyia+cK7VCGSdIYskDb6U/jqeb+QxnD5s1g6CeYHswbWpEuwCMVSZDk1vkSBZHE4oHTnjiwdb3bMvVzPW9KKQ75WGwU7trqNyLMtZS7JrVr";
    QByteArray output = toEncrypt;
    for (int i = 0; i < output.size(); i++)
        output[i] = uchar(toEncrypt.at(i) ^ key.at(output.size() + (2^6) - i));
    return output;
}

QString Config::findWorkFolderName()
{
    if (QSysInfo::kernelType().contains("win")) {
        /*QFileInfo checkDir("D:/");
        if (checkDir.exists() && checkDir.isWritable()) return "D:/Hauken";
        else*/ return "C:/Hauken";
    }
    else
        return QString(QStandardPaths::writableLocation(QStandardPaths::HomeLocation) + "/Hauken");
}

QList<double> Config::getIqMultibandCenterFreqs()
{
    QList<double> iqGnssBands;

    QFile file(getWorkFolder() + "/IqGnssBands.csv");
    if (file.exists()) {
        file.open(QIODevice::ReadOnly);
        while (!file.atEnd()) {
            QString line = file.readLine();
            QStringList split = line.split(',');
            if (!line.contains("#")) {
                for (auto && val : split) {
                    bool ok = false;
                    double dbl = val.toDouble(&ok);
                    if (ok) iqGnssBands.append(dbl);
                }
            }
        }
    }
    file.close();

    if (iqGnssBands.size() > 0)
        qDebug() << "IQ GNSS bands successfully read from file \"IqGnssBands.csv\", found" << iqGnssBands.size() << "freqs";
    else
        qDebug() << "No IQ GNSS bands defined";

    return iqGnssBands;
}

void Config::incidentStarted()
{
    if (!flagIncident) { // First call to inc. started
        flagIncident = true;
        incidentDateTime = QDateTime::currentDateTime();
        //qDebug() << "Suggesting inc. file datestamp" << incidentDateTime.toString();
    }
}

void Config::incidentEnded()
{
    incidentDateTime = QDateTime(); // Invalid
    flagIncident = false;
}

QString Config::incidentFolder()
{
    if (!incidentDateTime.isValid()) {
        incidentDateTime = QDateTime::currentDateTime();
        flagIncident = true;
    }
    if (getNewLogFolder()) return getLogFolder() + "/" + incidentDateTime.toString("yyyyMMdd_hhmmss_") + AsciiTranslator::toAscii(getStationName());
    else return getLogFolder() + "/";

    return QString();
}
