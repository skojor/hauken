#include "config.h"

Config::Config(QObject *parent)
    : QObject(parent)
{
    QByteArray test = "Here is an example of a Random number generator which will pick a value";
    QByteArray res = simpleEncr(test);
    qDebug() << "obsc. test" << res << simpleEncr(res);
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
