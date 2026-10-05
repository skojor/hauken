#include <QtTest>

#include "ubloxmonspanparser.h"

class UbloxMonSpanParserTest : public QObject
{
    Q_OBJECT

private slots:
    void parsesOrderedFrequencyMetadata();
    void rejectsMismatchedCount();
    void rejectsUnknownVersion();
    void rejectsInvalidResolution();
    void rejectsInvalidChecksum();
};

namespace {
void appendU4(QByteArray &data, quint32 value)
{
    data.append(static_cast<char>(value & 0xff));
    data.append(static_cast<char>((value >> 8) & 0xff));
    data.append(static_cast<char>((value >> 16) & 0xff));
    data.append(static_cast<char>((value >> 24) & 0xff));
}

QByteArray monSpanFrame(const QList<quint32> &centers, int advertisedCount = -1,
                        quint32 resolution = 500000)
{
    QByteArray payload(4, 0);
    payload[1] = static_cast<char>(advertisedCount < 0 ? centers.size() : advertisedCount);
    for (int index = 0; index < centers.size(); ++index) {
        QByteArray spectrum(256, static_cast<char>(index + 1));
        payload.append(spectrum);
        appendU4(payload, 128000000);
        appendU4(payload, resolution);
        appendU4(payload, centers.at(index));
        payload.append(static_cast<char>(10 + index));
        payload.append(QByteArray(3, 0));
    }

    QByteArray frame = QByteArray::fromHex("b5620a31");
    frame.append(static_cast<char>(payload.size() & 0xff));
    frame.append(static_cast<char>((payload.size() >> 8) & 0xff));
    frame.append(payload);
    quint8 checksumA = 0;
    quint8 checksumB = 0;
    for (int index = 2; index < frame.size(); ++index) {
        checksumA += static_cast<quint8>(frame.at(index));
        checksumB += checksumA;
    }
    frame.append(static_cast<char>(checksumA));
    frame.append(static_cast<char>(checksumB));
    return frame;
}
}

void UbloxMonSpanParserTest::parsesOrderedFrequencyMetadata()
{
    QVector<UbloxMonSpanBlock> blocks;

    QVERIFY(UbloxMonSpanParser::parse(
        monSpanFrame({1586000000, 1191937500, 1254906250}), blocks));
    QCOMPARE(blocks.size(), 3);
    QCOMPARE(blocks.at(0).centerHz, quint32(1586000000));
    QCOMPARE(blocks.at(1).centerHz, quint32(1191937500));
    QCOMPARE(blocks.at(2).centerHz, quint32(1254906250));
    QCOMPARE(blocks.at(0).spanHz, quint32(128000000));
    QCOMPARE(blocks.at(0).resolutionHz, quint32(500000));
    QCOMPARE(blocks.at(2).pga, quint8(12));
    QCOMPARE(blocks.at(1).spectrum.size(), 256);
}

void UbloxMonSpanParserTest::rejectsMismatchedCount()
{
    QVector<UbloxMonSpanBlock> blocks;
    QVERIFY(!UbloxMonSpanParser::parse(monSpanFrame({1586000000}, 2), blocks));
    QVERIFY(blocks.isEmpty());
}

void UbloxMonSpanParserTest::rejectsUnknownVersion()
{
    QByteArray frame = monSpanFrame({1586000000});
    frame[6] = 1;
    quint8 checksumA = 0;
    quint8 checksumB = 0;
    for (int index = 2; index < frame.size() - 2; ++index) {
        checksumA += static_cast<quint8>(frame.at(index));
        checksumB += checksumA;
    }
    frame[frame.size() - 2] = static_cast<char>(checksumA);
    frame[frame.size() - 1] = static_cast<char>(checksumB);
    QVector<UbloxMonSpanBlock> blocks;
    QVERIFY(!UbloxMonSpanParser::parse(frame, blocks));
}

void UbloxMonSpanParserTest::rejectsInvalidResolution()
{
    QVector<UbloxMonSpanBlock> blocks;
    QVERIFY(!UbloxMonSpanParser::parse(monSpanFrame({1586000000}, -1, 0), blocks));
    QVERIFY(blocks.isEmpty());
}

void UbloxMonSpanParserTest::rejectsInvalidChecksum()
{
    QByteArray frame = monSpanFrame({1586000000});
    frame[frame.size() - 1] ^= 0x01;
    QVector<UbloxMonSpanBlock> blocks;
    QVERIFY(!UbloxMonSpanParser::parse(frame, blocks));
    QVERIFY(blocks.isEmpty());
}

QTEST_MAIN(UbloxMonSpanParserTest)
#include "ubloxmonspanparser_test.moc"