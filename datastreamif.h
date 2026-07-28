#ifndef DATASTREAMIF_H
#define DATASTREAMIF_H

#include <QObject>
#include <QDebug>
#include <QtEndian>
#include "streamparserbase.h"

class DatastreamIf : public StreamParserBase
{
    Q_OBJECT
public:
    explicit DatastreamIf(QObject *parent = nullptr);
    void invalidateHeader();
    void parseIfData(const QByteArray &data);

signals:
    void ifDataReady(const QVector<complexInt16> &);
    void headerChanged(quint64, quint32, quint32, quint64);

private:
    void readData(QDataStream &ds);
    bool readOptHeader(QDataStream &ds) { return m_optHeader.readData(ds, m_attrHeader.optHeaderLength);}
    bool checkHeaders();
    void checkOptHeader();
    int locateHeader(const QByteArray &buf) const;
    bool readHeaders(const char *data, qsizetype size);
    bool readOptHeader(const char *data, qsizetype size);
    void readFrame(const char *data, qsizetype size);

    IfOptHeader m_optHeader;
    QByteArray m_buffer;
    quint64 m_frequency = 0;
    quint32 m_bandwidth = 0;
    quint32 m_samplerate = 0;
    quint64 m_sampleCtr = 0;
    int m_seqNr = 0;
    int m_byteCtr = 0;

};

#endif // DATASTREAMIF_H
