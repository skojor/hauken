#ifndef DATASTREAMAUDIO_H
#define DATASTREAMAUDIO_H

#include <QObject>
#include <QDebug>
#include <QtEndian>
#include <QAudioSink>
#include "streamparserbase.h"

class DatastreamAudio : public StreamParserBase
{
    Q_OBJECT
public:
    explicit DatastreamAudio(QObject *parent = nullptr);
    bool readOptHeader(QDataStream &ds) { return m_optHeader.readData(ds, m_attrHeader.optHeaderLength);}
    bool checkHeaders();
    void reportAudioMode(const int mode);

signals:
    void audioDataReady(const QByteArray &);
    void audioModeChanged(int, int, QAudioFormat::SampleFormat);
    void headerDataChanged(quint64 freq, int bw, QString demodType);

private:
    void readData(QDataStream &ds);
    void checkForHeaderChanges();
    void checkOptHeader() {};

    int m_audioMode = 0;
    quint64 m_freq;
    int m_bw;
    QString m_demodType;
    AudioOptHeader m_optHeader;

};



#endif // DATASTREAMAUDIO_H
