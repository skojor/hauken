#include "ubloxsecsigparser.h"

#include <QtEndian>

namespace {
bool hasValidChecksum(const QByteArray &frame)
{
    quint8 checksumA = 0;
    quint8 checksumB = 0;
    for (int index = 2; index < frame.size() - 2; ++index) {
        checksumA = static_cast<quint8>(checksumA + static_cast<quint8>(frame.at(index)));
        checksumB = static_cast<quint8>(checksumB + checksumA);
    }
    return checksumA == static_cast<quint8>(frame.at(frame.size() - 2))
           && checksumB == static_cast<quint8>(frame.at(frame.size() - 1));
}
}

bool UbloxSecSigParser::parse(const QByteArray &frame, UbloxSecSigData &data)
{
    if (frame.size() < 12
        || static_cast<quint8>(frame.at(0)) != 0xb5
        || static_cast<quint8>(frame.at(1)) != 0x62
        || static_cast<quint8>(frame.at(2)) != 0x27
        || static_cast<quint8>(frame.at(3)) != 0x09
        || !hasValidChecksum(frame)) {
        return false;
    }

    const quint16 payloadLength = qFromLittleEndian<quint16>(
        reinterpret_cast<const uchar *>(frame.constData() + 4));
    const quint8 version = static_cast<quint8>(frame.at(6));
    const quint8 flags = static_cast<quint8>(frame.at(7));
    const quint8 frequencyCount = static_cast<quint8>(frame.at(9));
    const quint8 jammingState = (flags >> 1) & 0x03;
    const quint8 spoofingState = (flags >> 4) & 0x07;
    const int expectedPayloadLength = 4 + frequencyCount * 4;

    if (version != 2
        || payloadLength != expectedPayloadLength
        || frame.size() != payloadLength + 8
        || jammingState > static_cast<quint8>(UbloxJammingState::WARNING)
        || spoofingState > static_cast<quint8>(UbloxSpoofingState::AFFIRMED)) {
        return false;
    }

    UbloxSecSigData parsed;
    parsed.jammingDetectionEnabled = flags & 0x01;
    parsed.jammingState = static_cast<UbloxJammingState>(jammingState);
    parsed.spoofingDetectionEnabled = flags & 0x08;
    parsed.spoofingState = static_cast<UbloxSpoofingState>(spoofingState);
    parsed.frequencies.reserve(frequencyCount);

    for (int index = 0; index < frequencyCount; ++index) {
        const int offset = 10 + index * 4;
        const quint32 frequencyAndState = qFromLittleEndian<quint32>(
            reinterpret_cast<const uchar *>(frame.constData() + offset));
        parsed.frequencies.append({frequencyAndState & 0x00ffffff,
                                   static_cast<bool>(frequencyAndState & 0x01000000)});
    }

    data = parsed;
    return true;
}