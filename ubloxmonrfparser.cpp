#include "ubloxmonrfparser.h"

#include <QtEndian>

bool UbloxMonRfParser::parseAgcBlocks(const QByteArray &frame, QVector<UbloxMonRfBlock> &agcBlocks)
{
    constexpr int payloadOffset = 6;
    constexpr int payloadHeaderSize = 4;
    constexpr int rfBlockSize = 24;
    constexpr int agcOffsetInBlock = 14;
    constexpr int jammingIndicatorOffsetInBlock = 16;

    if (frame.size() < payloadOffset + payloadHeaderSize + 2
        || static_cast<quint8>(frame.at(0)) != 0xb5
        || static_cast<quint8>(frame.at(1)) != 0x62
        || static_cast<quint8>(frame.at(2)) != 0x0a
        || static_cast<quint8>(frame.at(3)) != 0x38) {
        return false;
    }

    quint8 checksumA = 0;
    quint8 checksumB = 0;
    for (int index = 2; index < frame.size() - 2; ++index) {
        checksumA += static_cast<quint8>(frame.at(index));
        checksumB += checksumA;
    }
    if (checksumA != static_cast<quint8>(frame.at(frame.size() - 2))
        || checksumB != static_cast<quint8>(frame.at(frame.size() - 1))) {
        return false;
    }

    const quint16 payloadSize = qFromLittleEndian<quint16>(
        reinterpret_cast<const uchar*>(frame.constData() + 4));
    const int blockCount = static_cast<quint8>(frame.at(payloadOffset + 1));
    if (blockCount == 0 || payloadSize != payloadHeaderSize + blockCount * rfBlockSize
        || frame.size() != payloadOffset + payloadSize + 2) {
        return false;
    }

    QVector<UbloxMonRfBlock> parsedBlocks;
    QMap<int, bool> parsedBlockIds;
    for (int blockIndex = 0; blockIndex < blockCount; ++blockIndex) {
        const int blockOffset = payloadOffset + payloadHeaderSize + blockIndex * rfBlockSize;
        const int blockId = static_cast<quint8>(frame.at(blockOffset));
        const int agc = qFromLittleEndian<quint16>(
            reinterpret_cast<const uchar*>(frame.constData() + blockOffset + agcOffsetInBlock));
        const int jammingIndicator = static_cast<quint8>(
            frame.at(blockOffset + jammingIndicatorOffsetInBlock));
        if (agc > 8191 || parsedBlockIds.contains(blockId)) return false;

        parsedBlockIds.insert(blockId, true);
        parsedBlocks.append({blockId, agc * 100 / 8191, jammingIndicator * 100 / 255});
    }

    agcBlocks = parsedBlocks;
    return true;
}

bool UbloxMonRfParser::parseAgcBlocks(const QByteArray &frame, QMap<int, int> &agcBlocks)
{
    QVector<UbloxMonRfBlock> orderedBlocks;
    if (!parseAgcBlocks(frame, orderedBlocks)) return false;

    QMap<int, int> parsedBlocks;
    for (const UbloxMonRfBlock &block : orderedBlocks)
        parsedBlocks.insert(block.blockId, block.agcPercent);
    agcBlocks = parsedBlocks;
    return true;
}