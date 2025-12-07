#ifndef STREAMPARSERBASE_H
#define STREAMPARSERBASE_H

#include <QObject>
#include "typedefs.h"

class StreamParserBase : public QObject
{
    Q_OBJECT
public:
    explicit StreamParserBase(QObject *parent = nullptr);
    virtual void readData(QDataStream &ds) = 0;
    virtual bool checkHeaders() = 0;
    virtual void checkOptHeader() = 0;
    virtual bool readOptHeader(QDataStream &ds) = 0;

    void parseData(const QByteArray &data);

    Eb200Header m_eb200Header;
    AttrHeaderCombined m_attrHeader;

signals:
    void frequencyChanged(double, double);
    void resolutionChanged(double);

};

#endif // STREAMPARSERBASE_H
