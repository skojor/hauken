#ifndef AUDIOPLAYER_H
#define AUDIOPLAYER_H

#include <QObject>
#include <QAudioSink>
#include <QMediaDevices>
#include <QIODevice>
#include <QDebug>

/*
 * Class to play audio through standard audio device.
 * Format set by signal/slots from the source.
 * Data input as serialized PCM data in a QByteArray.
 */

class AudioPlayer : public QObject
{
    Q_OBJECT
public:
    explicit AudioPlayer(QObject *parent = nullptr);
    void playChunk(const QByteArray &pcm);
    void updSettings();
    void setFormat(int samplerate, int channels, QAudioFormat::SampleFormat format);
    void setAudioDevice(int devIndex);
    void playAudio(bool);

private:
    QAudioSink *m_audioSink = nullptr;
    QIODevice *m_output = nullptr;
    QAudioFormat m_format;
    bool m_playerActive = true;
    bool m_formatSet = false;
};

#endif // AUDIOPLAYER_H
