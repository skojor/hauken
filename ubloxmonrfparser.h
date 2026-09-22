#ifndef UBLOXMONRFPARSER_H
#define UBLOXMONRFPARSER_H

#include <QByteArray>
#include <QMap>
#include <QVector>

struct UbloxMonRfBlock
{
	int blockId = -1;
	int agcPercent = -1;
	int jammingIndicatorPercent = -1;
};

namespace UbloxMonRfParser {
bool parseAgcBlocks(const QByteArray &frame, QMap<int, int> &agcBlocks);
bool parseAgcBlocks(const QByteArray &frame, QVector<UbloxMonRfBlock> &agcBlocks);
}

#endif // UBLOXMONRFPARSER_H