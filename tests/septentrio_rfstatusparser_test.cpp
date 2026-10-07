#include <QtTest>
#include <QtEndian>

#include "septentrio_rfstatusparser.h"

class SeptentrioRfStatusParserTest : public QObject
{
    Q_OBJECT

private slots:
    void parsesFlagsAndMultipleBands();
    void acceptsFlagsWithoutBandRecords();
    void rejectsWrongBlockId();
    void rejectsTruncatedBandRecords();
    void rejectsInvalidSubBlockLength();
};

namespace {
void writeLittleEndian16(QByteArray &data, int offset, quint16 value)
{
    qToLittleEndian(value, reinterpret_cast<uchar *>(data.data() + offset));
}

void writeLittleEndian32(QByteArray &data, int offset, quint32 value)
{
    qToLittleEndian(value, reinterpret_cast<uchar *>(data.data() + offset));
}

struct TestBand
{
    quint32 frequencyHz;
    quint16 bandwidthKHz;
    quint8 info;
    qint8 powerDbm;
};

QByteArray rfStatusBlock(const QList<TestBand> &bands, quint8 flags = 0,
                         quint8 subBlockLength = 8)
{
    const int length = 20 + bands.size() * subBlockLength;
    QByteArray block(length, 0);
    block[0] = '$';
    block[1] = static_cast<char>(0x40);
    writeLittleEndian16(block, 4, 4092);
    writeLittleEndian16(block, 6, static_cast<quint16>(length));
    block[14] = static_cast<char>(bands.size());
    block[15] = static_cast<char>(subBlockLength);
    block[16] = static_cast<char>(flags);

    for (int index = 0; index < bands.size(); ++index) {
        const int offset = 20 + index * subBlockLength;
        writeLittleEndian32(block, offset, bands.at(index).frequencyHz);
        writeLittleEndian16(block, offset + 4, bands.at(index).bandwidthKHz);
        block[offset + 6] = static_cast<char>(bands.at(index).info);
        block[offset + 7] = static_cast<char>(bands.at(index).powerDbm);
    }
    return block;
}
}

void SeptentrioRfStatusParserTest::parsesFlagsAndMultipleBands()
{
    SeptentrioRfStatus status;
    const QByteArray block = rfStatusBlock({
        {1575420000, 20, 0x02, -42},
        {1227600000, 10, 0x88, -31}
    }, 0x03);

    QVERIFY(SeptentrioRfStatusParser::parse(block, status));
    QVERIFY(status.received);
    QCOMPARE(status.flags, 0x03);
    QCOMPARE(status.bands.size(), 2);
    QCOMPARE(status.bands.at(0).frequencyHz, quint32(1575420000));
    QCOMPARE(status.bands.at(0).bandwidthKHz, quint16(20));
    QCOMPARE(status.bands.at(0).mode, quint8(2));
    QCOMPARE(status.bands.at(0).antennaId, quint8(0));
    QCOMPARE(status.bands.at(0).interferencePowerDbm, qint8(-42));
    QCOMPARE(status.bands.at(1).mode, quint8(8));
    QCOMPARE(status.bands.at(1).antennaId, quint8(2));
}

void SeptentrioRfStatusParserTest::acceptsFlagsWithoutBandRecords()
{
    SeptentrioRfStatus status;

    QVERIFY(SeptentrioRfStatusParser::parse(rfStatusBlock({}, 0x01), status));
    QVERIFY(status.received);
    QCOMPARE(status.flags, 0x01);
    QVERIFY(status.bands.isEmpty());
}

void SeptentrioRfStatusParserTest::rejectsWrongBlockId()
{
    QByteArray block = rfStatusBlock({});
    writeLittleEndian16(block, 4, 4014);
    SeptentrioRfStatus status;

    QVERIFY(!SeptentrioRfStatusParser::parse(block, status));
    QVERIFY(!status.received);
}

void SeptentrioRfStatusParserTest::rejectsTruncatedBandRecords()
{
    QByteArray block = rfStatusBlock({{1575420000, 20, 0x08, -42}});
    block.chop(1);
    writeLittleEndian16(block, 6, static_cast<quint16>(block.size()));
    SeptentrioRfStatus status;

    QVERIFY(!SeptentrioRfStatusParser::parse(block, status));
    QVERIFY(!status.received);
}

void SeptentrioRfStatusParserTest::rejectsInvalidSubBlockLength()
{
    QByteArray block = rfStatusBlock({});
    block[15] = 7;
    SeptentrioRfStatus status;

    QVERIFY(!SeptentrioRfStatusParser::parse(block, status));
    QVERIFY(!status.received);
}

QTEST_MAIN(SeptentrioRfStatusParserTest)
#include "septentrio_rfstatusparser_test.moc"