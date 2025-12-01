#ifndef AUDIOOPTIONS_H
#define AUDIOOPTIONS_H

#include <QSharedPointer>
#include <QMediaDevices>
#include <QAudioDevice>
#include "config.h"
#include "optionsbaseclass.h"

class AudioOptions : public OptionsBaseClass
{
    Q_OBJECT
public:
    AudioOptions(QSharedPointer<Config> c);
    void start();
    void saveCurrentSettings();
    void getDemodBwList(const QStringList &list);
    void getDemodTypeList(const QStringList &list);

signals:
    void askForDemodBwList();
    void askForDemodTypeList();
    void audioMode(int);
    void demodType(QString);
    void demodBw(int);
    void audioDevice(int);
    void activateAudio(bool);
    void squelch(bool);
    void squelchLevel(int);

private:
    QSharedPointer<Config> config;
    QList<QAudioDevice> updMediaDevicesList();
    QStringList updAudioFormatList();
    void reportAudioMode();
    void reportDemodType();
    void reportDemodBw();
    void reportPlayback();
    void reportSquelch();
};

#endif // AUDIOOPTIONS_H
