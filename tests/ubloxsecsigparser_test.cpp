#include <QtTest>

#include "ubloxsecsigparser.h"

class UbloxSecSigParserTest : public QObject
{
    Q_OBJECT

private slots:
    void parsesStatesAndJammedFrequencies();
    void acceptsUnknownStatesWithoutFrequencies();
    void rejectsWrongMessageAndVersion();
    void rejectsInvalidStates();
    void rejectsTruncatedAndMismatchedPayloads();
    void rejectsInvalidChecksum();
};

namespace {
QByteArray secSigFrame(quint8 flags, const QList<QPair<quint32, bool>> &frequencies,
                       quint8 version = 2, int advertisedFrequencyCount = -1)
{
    const int count = advertisedFrequencyCount < 0 ? frequencies.size() : advertisedFrequencyCount;
    QByteArray payload(4 + frequencies.size() * 4, 0);
    payload[0] = static_cast<char>(version);
    payload[1] = static_cast<char>(flags);
    payload[3] = static_cast<char>(count);

    for (int index = 0; index < frequencies.size(); ++index) {
        const quint32 value = (frequencies.at(index).first & 0x00ffffff)
                              | (frequencies.at(index).second ? 0x01000000 : 0);
        const int offset = 4 + index * 4;
        for (int byte = 0; byte < 4; ++byte)
            payload[offset + byte] = static_cast<char>((value >> (byte * 8)) & 0xff);
    }

    QByteArray frame = QByteArray::fromHex("b5622709");
    frame.append(static_cast<char>(payload.size() & 0xff));
    frame.append(static_cast<char>((payload.size() >> 8) & 0xff));
    frame.append(payload);
    quint8 checksumA = 0;
    quint8 checksumB = 0;
    for (int index = 2; index < frame.size(); ++index) {
        checksumA = static_cast<quint8>(checksumA + static_cast<quint8>(frame.at(index)));
        checksumB = static_cast<quint8>(checksumB + checksumA);
    }
    frame.append(static_cast<char>(checksumA));
    frame.append(static_cast<char>(checksumB));
    return frame;
}
}

void UbloxSecSigParserTest::parsesStatesAndJammedFrequencies()
{
    UbloxSecSigData data;
    const QByteArray frame = secSigFrame(0x3d, {{1575420, true}, {1176450, false}});

    QVERIFY(UbloxSecSigParser::parse(frame, data));
    QVERIFY(data.jammingDetectionEnabled);
    QCOMPARE(data.jammingState, UbloxJammingState::WARNING);
    QVERIFY(data.spoofingDetectionEnabled);
    QCOMPARE(data.spoofingState, UbloxSpoofingState::AFFIRMED);
    QCOMPARE(data.frequencies.size(), 2);
    QCOMPARE(data.frequencies.at(0).centerFrequencyKHz, 1575420u);
    QVERIFY(data.frequencies.at(0).jammed);
    QCOMPARE(data.frequencies.at(1).centerFrequencyKHz, 1176450u);
    QVERIFY(!data.frequencies.at(1).jammed);
}

void UbloxSecSigParserTest::acceptsUnknownStatesWithoutFrequencies()
{
    UbloxSecSigData data;

    QVERIFY(UbloxSecSigParser::parse(secSigFrame(0, {}), data));
    QCOMPARE(data.jammingState, UbloxJammingState::UNKNOWN);
    QCOMPARE(data.spoofingState, UbloxSpoofingState::UNKNOWN);
    QVERIFY(data.frequencies.isEmpty());
}

void UbloxSecSigParserTest::rejectsWrongMessageAndVersion()
{
    UbloxSecSigData data;
    QByteArray wrongMessage = secSigFrame(0, {});
    wrongMessage[3] = 0x08;

    QVERIFY(!UbloxSecSigParser::parse(wrongMessage, data));
    QVERIFY(!UbloxSecSigParser::parse(secSigFrame(0, {}, 1), data));
}

void UbloxSecSigParserTest::rejectsInvalidStates()
{
    UbloxSecSigData data;

    QVERIFY(!UbloxSecSigParser::parse(secSigFrame(0x06, {}), data));
    QVERIFY(!UbloxSecSigParser::parse(secSigFrame(0x40, {}), data));
}

void UbloxSecSigParserTest::rejectsTruncatedAndMismatchedPayloads()
{
    UbloxSecSigData data;
    QByteArray truncated = secSigFrame(0, {{1575420, true}});
    truncated.chop(2);

    QVERIFY(!UbloxSecSigParser::parse(truncated, data));
    QVERIFY(!UbloxSecSigParser::parse(secSigFrame(0, {{1575420, true}}, 2, 2), data));
}

void UbloxSecSigParserTest::rejectsInvalidChecksum()
{
    UbloxSecSigData data;
    QByteArray frame = secSigFrame(0, {});
    frame[frame.size() - 1] ^= 0x01;

    QVERIFY(!UbloxSecSigParser::parse(frame, data));
}

QTEST_MAIN(UbloxSecSigParserTest)
#include "ubloxsecsigparser_test.moc"