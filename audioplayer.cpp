#include "audioplayer.h"

AudioPlayer::AudioPlayer(QObject *parent)
    : QObject{parent}
{
   // m_format.setSampleFormat(QAudioFormat::Unknown);
}

void AudioPlayer::playChunk(const QByteArray &pcm)
{
    if (!m_output || !m_playerActive || !m_formatSet)
        return;
    else m_output->write(pcm);
    /*if (written != pcm.size()) {
        // TODO: Error control
    }*/
}

void AudioPlayer::setFormat(int samplerate, int channels, QAudioFormat::SampleFormat format)
{
    if (samplerate == 0 && channels == 0 && format == QAudioFormat::Unknown) { // Mode 0, switch off
        if (m_audioSink) {
            m_audioSink->stop();
            m_audioSink->deleteLater();
        }
    }
    if (samplerate != 8000 and samplerate != 16000 and samplerate != 32000)
        qDebug() << "Sample rate not supported:" << samplerate;
    else if (channels < 1 or channels > 2)
        qDebug() << "Channels can only be 1 or 2!";
    else {
        m_formatSet = true;
        if (m_audioSink)
            m_audioSink->deleteLater();
        m_format.setSampleRate(samplerate);
        m_format.setChannelCount(channels);
        m_format.setSampleFormat(format);

        auto device = QMediaDevices::defaultAudioOutput();
        m_audioSink = new QAudioSink(device, m_format, this);
        m_output = m_audioSink->start();
    }
}

void AudioPlayer::playAudio(bool play)
{
    if (play) {
        m_playerActive = true;
    }
    else {
        m_playerActive = false;
    }

}

void AudioPlayer::setAudioDevice(int devIndex)
{
    QList<QAudioDevice> audioDevices = QMediaDevices::audioOutputs();
    if (audioDevices.size() >= devIndex + 1) {
        if (m_audioSink)
            m_audioSink->deleteLater();
        auto device = audioDevices.at(devIndex);
        m_audioSink = new QAudioSink(device, m_format, this);
        m_output = m_audioSink->start();
    }

}

void AudioPlayer::updSettings()
{

}
