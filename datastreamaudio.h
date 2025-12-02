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
    void reportAudioMode(const int mode);

signals:
    void audioDataReady(const QByteArray &);
    void audioModeChanged(int, int, QAudioFormat::SampleFormat);
    void headerDataChanged(int freq, int bw, QString demodType);

private slots:
    bool readHeaders(QDataStream &ds);
    bool checkHeaders();
    void readAudioData(QDataStream &ds);
    void checkForHeaderChanges();

private:
    Eb200Header m_eb200Header;
    UdpDatagramAttribute m_attrHeader;
    AudioOptHeader m_audioOptHeader;
    int m_audioMode = 0;
    quint64 m_freq;
    int m_bw;
    QString m_demodType;
};



#endif // DATASTREAMAUDIO_H
