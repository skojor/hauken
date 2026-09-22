#include "ubloxmonspanparser.h"

#include <QtEndian>

bool UbloxMonSpanParser::parse(const QByteArray &frame, QVector<UbloxMonSpanBlock> &blocks)
{
    constexpr int payloadOffset = 6;
    constexpr int payloadHeaderSize = 4;
    constexpr int spectrumSize = 256;
    constexpr int blockSize = 272;

    if (frame.size() < payloadOffset + payloadHeaderSize + 2
        || static_cast<quint8>(frame.at(0)) != 0xb5
        || static_cast<quint8>(frame.at(1)) != 0x62
        || static_cast<quint8>(frame.at(2)) != 0x0a
        || static_cast<quint8>(frame.at(3)) != 0x31) {
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
    const int version = static_cast<quint8>(frame.at(payloadOffset));
    const int blockCount = static_cast<quint8>(frame.at(payloadOffset + 1));
    if (version != 0 || blockCount == 0 || payloadSize != payloadHeaderSize + blockCount * blockSize
        || frame.size() != payloadOffset + payloadSize + 2) {
        return false;
    }

    QVector<UbloxMonSpanBlock> parsedBlocks;
    parsedBlocks.reserve(blockCount);
    for (int blockIndex = 0; blockIndex < blockCount; ++blockIndex) {
        const int blockOffset = payloadOffset + payloadHeaderSize + blockIndex * blockSize;
        UbloxMonSpanBlock block;
        block.spectrum = frame.mid(blockOffset, spectrumSize);
        block.spanHz = qFromLittleEndian<quint32>(
            reinterpret_cast<const uchar*>(frame.constData() + blockOffset + 256));
        block.resolutionHz = qFromLittleEndian<quint32>(
            reinterpret_cast<const uchar*>(frame.constData() + blockOffset + 260));
        block.centerHz = qFromLittleEndian<quint32>(
            reinterpret_cast<const uchar*>(frame.constData() + blockOffset + 264));
        block.pga = static_cast<quint8>(frame.at(blockOffset + 268));
        if (block.spanHz == 0 || block.resolutionHz == 0 || block.centerHz == 0
            || block.spanHz / block.resolutionHz != spectrumSize) {
            return false;
        }
        parsedBlocks.append(block);
    }

    blocks = parsedBlocks;
    return true;
}