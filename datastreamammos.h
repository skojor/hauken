#ifndef DATASTREAMAMMOS_H
#define DATASTREAMAMMOS_H

#include <QObject>
#include "streamparserbase.h"

class DatastreamAmmos : public StreamParserBase
{
    Q_OBJECT
public:
    explicit DatastreamAmmos(QObject *parent = nullptr);
    void invalidateHeader() { m_frequency = m_bandwidth = m_samplerate = m_sampleCtr = 0; }
    void parseAmmosData(const QByteArray &data);

signals:
    void ifDataReady(const QVector<complexInt16>);
    void headerChanged(quint64, quint32, quint32, quint64);

private:
    void readData(QDataStream &ds);
    bool readOptHeader(QDataStream &ds);
    bool checkHeaders();
    void checkOptHeader();

    int locateHeader(const QByteArray &buf) const;
    int locateInvertedHeader(const QByteArray &buf) const;
    void swapHeader();

    AmmosHeader m_header;
    QByteArray m_buffer;
    bool m_invertedFrame = false;
    quint64 m_frequency = 0;
    quint32 m_bandwidth = 0;
    quint32 m_samplerate = 0;
    quint64 m_sampleCtr = 0;
    int m_seqNr = 0;
};

#endif // DATASTREAMAMMOS_H
