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

    int totalSamples16 = 0;
    const short *samples = nullptr;
    QVector<qint16> conv;  // QVector is better than QList for raw PCM

    if (m_format.sampleFormat() == QAudioFormat::Int16) {
        totalSamples16 = pcm.size() / 2; // bytes → 16-bit samples
        samples = reinterpret_cast<const short*>(pcm.constData());
    }
    else {
        // Convert 8→16 bit
        totalSamples16 = pcm.size();     // each 8-bit byte becomes 1 signed 16-bit sample
        conv.resize(totalSamples16);
        for (int i = 0; i < totalSamples16; i++)
            conv[i] = (qint16(pcm[i]) - 128) * 256;

        samples = conv.constData();
    }

    QByteArray mp3buf(totalSamples16 * 4, 0);
    int numFrames = totalSamples16 / m_format.channelCount();
    int bytesEncoded = 0;

    if (m_format.channelCount() == 2) {
        bytesEncoded = lame_encode_buffer_interleaved(
            m_lame,
            const_cast<short*>(samples),
            numFrames,
            reinterpret_cast<unsigned char*>(mp3buf.data()),
            mp3buf.size()
            );
    } else {
        bytesEncoded = lame_encode_buffer(
            m_lame,
            const_cast<short*>(samples),
            nullptr,
            totalSamples16,
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
    if (m_file.isOpen()) m_file.close();

    if (m_recorderEnabled) {
        m_file.setFileName(m_fileLocation + "/" +
                           QDateTime::currentDateTime().toString("yyyyMMdd") + "_" +
                           QString::number(freq) + "_" +
                           QString::number(bw) + "_" +
                           demodType + ".mp3");
        if (!m_file.open(QIODevice::Append))
            qDebug() << "Error opening" << m_file.fileName() << "for append";
    }
}
