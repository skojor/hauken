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

QString Config::simpleEncr(QString toEncrypt)
{
    QChar key = '$';
    QString output = toEncrypt;

    for (int i = 0; i < toEncrypt.size(); i++)
        output[i] = uchar(toEncrypt.at(i).unicode() ^ key.unicode());

    return output;
}
