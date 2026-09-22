#include <QtTest>

#include "ubloxmonrfparser.h"

class UbloxMonRfParserTest : public QObject
{
    Q_OBJECT

private slots:
    void parsesEveryBlockById();
    void preservesBlockIdentityWhenReordered();
    void preservesPhysicalRecordOrder();
    void parsesJammingIndicatorPerBlock();
    void rejectsTruncatedPayload();
    void rejectsMismatchedBlockCount();
    void rejectsDuplicateBlockIds();
    void rejectsInvalidChecksum();
    void rejectsOutOfRangeAgc();
};

namespace {
QByteArray monRfFrame(const QList<QPair<int, int>> &blocks, int advertisedBlockCount = -1,
                      const QList<int> &jammingIndicators = {})
{
    constexpr int payloadHeaderSize = 4;
    constexpr int blockSize = 24;
    QByteArray payload(payloadHeaderSize + blocks.size() * blockSize, 0);
    payload[0] = 0;
    payload[1] = static_cast<char>(advertisedBlockCount < 0 ? blocks.size() : advertisedBlockCount);

    for (int index = 0; index < blocks.size(); ++index) {
        const int offset = payloadHeaderSize + index * blockSize;
        payload[offset] = static_cast<char>(blocks.at(index).first);
        const int agc = blocks.at(index).second;
        payload[offset + 14] = static_cast<char>(agc & 0xff);
        payload[offset + 15] = static_cast<char>((agc >> 8) & 0xff);
        if (index < jammingIndicators.size())
            payload[offset + 16] = static_cast<char>(jammingIndicators.at(index));
    }

    QByteArray frame = QByteArray::fromHex("b5620a38");
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

void UbloxMonRfParserTest::parsesEveryBlockById()
{
    QMap<int, int> blocks;

    QVERIFY(UbloxMonRfParser::parseAgcBlocks(monRfFrame({{0, 8191}, {2, 4095}, {5, 0}}), blocks));
    QCOMPARE(blocks.size(), 3);
    QCOMPARE(blocks.value(0), 100);
    QCOMPARE(blocks.value(2), 49);
    QCOMPARE(blocks.value(5), 0);
}

void UbloxMonRfParserTest::preservesBlockIdentityWhenReordered()
{
    QMap<int, int> blocks;

    QVERIFY(UbloxMonRfParser::parseAgcBlocks(monRfFrame({{5, 1000}, {0, 7000}, {2, 3000}}), blocks));
    QCOMPARE(blocks.value(0), 7000 * 100 / 8191);
    QCOMPARE(blocks.value(2), 3000 * 100 / 8191);
    QCOMPARE(blocks.value(5), 1000 * 100 / 8191);
}

void UbloxMonRfParserTest::preservesPhysicalRecordOrder()
{
    QVector<UbloxMonRfBlock> blocks;

    QVERIFY(UbloxMonRfParser::parseAgcBlocks(monRfFrame({{5, 1000}, {0, 7000}, {2, 3000}}), blocks));
    QCOMPARE(blocks.size(), 3);
    QCOMPARE(blocks.at(0).blockId, 5);
    QCOMPARE(blocks.at(1).blockId, 0);
    QCOMPARE(blocks.at(2).blockId, 2);
}

void UbloxMonRfParserTest::parsesJammingIndicatorPerBlock()
{
    QVector<UbloxMonRfBlock> blocks;

    QVERIFY(UbloxMonRfParser::parseAgcBlocks(
        monRfFrame({{5, 1000}, {0, 7000}, {2, 3000}}, -1, {0, 128, 255}), blocks));
    QCOMPARE(blocks.at(0).jammingIndicatorPercent, 0);
    QCOMPARE(blocks.at(1).jammingIndicatorPercent, 128 * 100 / 255);
    QCOMPARE(blocks.at(2).jammingIndicatorPercent, 100);
}

void UbloxMonRfParserTest::rejectsTruncatedPayload()
{
    QByteArray frame = monRfFrame({{0, 1000}, {1, 2000}});
    frame.chop(10);
    QMap<int, int> blocks = {{9, 42}};
    const QMap<int, int> expectedBlocks = {{9, 42}};

    QVERIFY(!UbloxMonRfParser::parseAgcBlocks(frame, blocks));
    QCOMPARE(blocks, expectedBlocks);
}

void UbloxMonRfParserTest::rejectsMismatchedBlockCount()
{
    QMap<int, int> blocks;

    QVERIFY(!UbloxMonRfParser::parseAgcBlocks(monRfFrame({{0, 1000}}, 2), blocks));
    QVERIFY(blocks.isEmpty());
}

void UbloxMonRfParserTest::rejectsDuplicateBlockIds()
{
    QMap<int, int> blocks;

    QVERIFY(!UbloxMonRfParser::parseAgcBlocks(monRfFrame({{1, 1000}, {1, 2000}}), blocks));
    QVERIFY(blocks.isEmpty());
}

void UbloxMonRfParserTest::rejectsInvalidChecksum()
{
    QByteArray frame = monRfFrame({{0, 1000}});
    frame[frame.size() - 1] ^= 0x01;
    QMap<int, int> blocks;

    QVERIFY(!UbloxMonRfParser::parseAgcBlocks(frame, blocks));
    QVERIFY(blocks.isEmpty());
}

void UbloxMonRfParserTest::rejectsOutOfRangeAgc()
{
    QMap<int, int> blocks;

    QVERIFY(!UbloxMonRfParser::parseAgcBlocks(monRfFrame({{0, 8192}}), blocks));
    QVERIFY(blocks.isEmpty());
}

QTEST_MAIN(UbloxMonRfParserTest)
#include "ubloxmonrfparser_test.moc"