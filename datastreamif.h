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

signals:
    void ifDataReady(const QList<qint16> &);

private:
    void readData(QDataStream &ds);
    bool readOptHeader(QDataStream &ds) { return m_ifOptHeader.readData(ds);}
    bool checkHeaders();
    void checkOptHeader() {}

    IfOptHeader m_ifOptHeader;
};

#endif // DATASTREAMIF_H
