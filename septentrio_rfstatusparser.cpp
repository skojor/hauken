#include "septentrio_rfstatusparser.h"

#include <QtEndian>

bool SeptentrioRfStatusParser::parse(const QByteArray &block, SeptentrioRfStatus &status)
{
    constexpr int blockHeaderSize = 20;
    constexpr int bandSubBlockSize = 8;
    constexpr int bandCountOffset = 14;
    constexpr int subBlockLengthOffset = 15;
    constexpr int flagsOffset = 16;

    if (block.size() < blockHeaderSize
        || block.at(0) != '$' || static_cast<quint8>(block.at(1)) != 0x40) {
        return false;
    }

    const auto *bytes = reinterpret_cast<const uchar *>(block.constData());
    const quint16 blockId = qFromLittleEndian<quint16>(bytes + 4) & 0x1fff;
    const quint16 blockLength = qFromLittleEndian<quint16>(bytes + 6);
    if (blockId != 4092 || blockLength != block.size()) return false;

    const quint8 bandCount = static_cast<quint8>(block.at(bandCountOffset));
    const quint8 subBlockLength = static_cast<quint8>(block.at(subBlockLengthOffset));
    if (subBlockLength < bandSubBlockSize
        || blockHeaderSize + bandCount * subBlockLength > block.size()) {
        return false;
    }

    SeptentrioRfStatus parsedStatus;
    parsedStatus.received = true;
    parsedStatus.flags = static_cast<quint8>(block.at(flagsOffset));
    parsedStatus.bands.reserve(bandCount);

    for (int index = 0; index < bandCount; ++index) {
        const int offset = blockHeaderSize + index * subBlockLength;
        const quint8 info = static_cast<quint8>(block.at(offset + 6));
        parsedStatus.bands.append({
            qFromLittleEndian<quint32>(bytes + offset),
            qFromLittleEndian<quint16>(bytes + offset + 4),
            static_cast<quint8>(info & 0x0f),
            static_cast<quint8>(info >> 6),
            static_cast<qint8>(block.at(offset + 7))
        });
    }

    status = parsedStatus;
    return true;
}