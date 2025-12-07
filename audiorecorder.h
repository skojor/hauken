#ifndef AUDIORECORDER_H
#define AUDIORECORDER_H

#include <QObject>
#include <QDebug>
#include "lame.h"
#include <QFile>
#include <QAudioFormat>
#include <QTimer>
#include <QDateTime>

class AudioRecorder : public QObject
{
    Q_OBJECT
public:
    explicit AudioRecorder(QObject *parent = nullptr);
    void receiveAudioData(const QByteArray &pcm);
    void updFormat(const int samplerate, const int channels, const QAudioFormat::SampleFormat format);
    void enableRecorder(bool b) { m_recorderEnabled = b; if (m_file.isOpen()) m_file.close();}
    void demodChanged(int freq, int bw, const QString &demodType);
    void setFileLocation(QString s) { m_fileLocation = s;}

signals:

private:
    QFile m_file;
    lame_t m_lame = nullptr;
    QAudioFormat m_format;
    bool m_recorderEnabled = false;
    QString m_fileLocation;
};

#endif // AUDIORECORDER_H
