#include "audiorecorder.h"

AudioRecorder::AudioRecorder(QObject *parent)
    : QObject{parent}
{
    m_lame = lame_init();

}

void AudioRecorder::receiveAudioData(const QByteArray &pcm)
{
    if (!m_lame || !m_file.isOpen() || !m_recorderEnabled) {
        //qDebug() << "mp3 booboo" << m_lame << m_file.isOpen();
        return;
    }

    int numSamples = pcm.size();
    if (m_format.sampleFormat() == QAudioFormat::Int16)
        numSamples = pcm.size() / 2;  // 16-bit

    else { // Need to convert to 16-bit signed int
        // TODO: WILL NOT WORK WITH 8-BIT AUDIO!
    }
    QByteArray mp3buf(pcm.size() * 2, 0);

    int bytesEncoded = 0;

    if (m_format.channelCount() == 2) {
        const short* samples = reinterpret_cast<const short*>(pcm.constData());

        bytesEncoded = lame_encode_buffer_interleaved(
            m_lame,
            const_cast<short*>(samples),
            numSamples / 2,
            reinterpret_cast<unsigned char*>(mp3buf.data()),
            mp3buf.size()
            );
    } else {
        const short* samples = reinterpret_cast<const short*>(pcm.constData());

        bytesEncoded = lame_encode_buffer(
            m_lame,
            const_cast<short*>(samples),
            nullptr,
            numSamples,
            reinterpret_cast<unsigned char*>(mp3buf.data()),
            mp3buf.size()
            );
    }

    if (bytesEncoded > 0)
        m_file.write(mp3buf.constData(), bytesEncoded);
}

void AudioRecorder::updFormat(const int samplerate, const int channels, const QAudioFormat::SampleFormat format)
{
    m_format.setSampleRate(samplerate);
    m_format.setChannelCount(channels);
    m_format.setSampleFormat(format);
    lame_set_in_samplerate(m_lame, samplerate);
    lame_set_num_channels(m_lame, channels);
    lame_set_brate(m_lame, 32);
    if (lame_init_params(m_lame) < 0) {
        //qDebug() << "Lame error" << lame_init_params(m_lame);
    }
}

void AudioRecorder::demodChanged(int freq, int bw, const QString &demodType)
{
    if (m_recorderEnabled) {
        if (m_file.isOpen()) m_file.close();
        m_file.setFileName(m_fileLocation + "/" +
                           QDateTime::currentDateTime().toString("yyyyMMdd") + "_" +
                           QString::number(freq) + "_" +
                           QString::number(bw) + "_" +
                           demodType + ".mp3");
        if (!m_file.open(QIODevice::Append))
            qDebug() << "Error opening" << m_file.fileName() << "for append";
    }
}
