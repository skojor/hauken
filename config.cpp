#include "config.h"

Config::Config(QObject *parent)
    : QObject(parent)
{

}

void Config::newFileName(const QString file)
{
    curFile = file;
    basicSettings->setValue("lastFile", curFile);
    delete settings;
    settings = new QSettings(curFile, QSettings::IniFormat);
    settings->setValue("SW_VERSION", SW_VERSION);
}
