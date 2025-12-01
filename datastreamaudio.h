#ifndef DATASTREAMAUDIO_H
#define DATASTREAMAUDIO_H

#include <QObject>
#include <QDebug>
#include <QtEndian>
#include <QAudioSink>
#include "typedefs.h"

class PcmPlayer;

class DatastreamAudio : public QObject
{
    Q_OBJECT
public:
    explicit DatastreamAudio(QObject *parent = nullptr);

public slots:
    void parseAudioData(const QByteArray &buf);

signals:
    void audioDataReady(const QByteArray &);
    void audioModeChanged(int, int, QAudioFormat::SampleFormat);

private slots:
    bool readHeaders(QDataStream &ds);
    bool checkHeaders();
    void readAudioData(QDataStream &ds);
    void reportAudioMode();

private:
    Eb200Header m_eb200Header;
    UdpDatagramAttribute m_attrHeader;
    AudioOptHeader m_audioOptHeader;
    int audioMode = 0;
};



#endif // DATASTREAMAUDIO_H
