#include "config.h"
#include "version.h"
#include "asciitranslator.h"


Config::Config(QObject *parent)
    : QObject(parent)
{
    basicSettings = new QSettings("Nkom", "Hauken"); //(findWorkFolderName() + "/hauken.conf");
    curFile = basicSettings->value("lastFile", "c:/Hauken/default.ini").toString();
    if (!basicSettings->value("lastFile").isValid()) basicSettings->setValue("lastFile", "c:/Hauken/default.ini");

    settings = new QSettings(curFile, QSettings::IniFormat);
}

void Config::newFileName(const QString file)
{
    curFile = file;
    basicSettings->setValue("lastFile", curFile);
    delete settings;
    settings = new QSettings(curFile, QSettings::IniFormat);
    settings->setValue("SW_VERSION", FULL_VERSION);
}

QByteArray Config::simpleEncr(QByteArray toEncrypt)
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
