#include "config.h"

Config::Config(QObject *parent)
    : QObject(parent)
{
    basicSettings = new QSettings(); //(findWorkFolderName() + "/hauken.conf");
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
    settings->setValue("SW_VERSION", SW_VERSION);
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
