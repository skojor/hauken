#include <QtTest>

#include "ubloxrfbandcorrelator.h"

class UbloxRfBandCorrelatorTest : public QObject
{
    Q_OBJECT

private slots:
    void qualifiesStableMapping();
    void mapsByPhysicalOrder();
    void rejectsStalePairs();
    void rejectsNonConsecutivePairs();
    void invalidatesOnFrequencyChange();
    void resetsOnReceiverIdentityChange();
};

namespace {
QVector<UbloxMonRfBlock> rfBlocks(std::initializer_list<int> ids)
{
    QVector<UbloxMonRfBlock> blocks;
    for (int id : ids) blocks.append({id, 50});
    return blocks;
}

QVector<UbloxMonSpanBlock> spanBlocks(std::initializer_list<quint32> centers)
{
    QVector<UbloxMonSpanBlock> blocks;
    for (quint32 center : centers) {
        UbloxMonSpanBlock block;
        block.centerHz = center;
        block.spanHz = 128000000;
        block.resolutionHz = 500000;
        blocks.append(block);
    }
    return blocks;
}

void observePair(UbloxRfBandCorrelator &correlator, qint64 timestamp,
                 const QVector<UbloxMonRfBlock> &rf,
                 const QVector<UbloxMonSpanBlock> &span)
{
    const quint64 sequence = static_cast<quint64>(timestamp);
    correlator.observeMonRf(rf, timestamp, sequence);
    correlator.observeMonSpan(span, timestamp + 1, sequence + 1);
}
}

void UbloxRfBandCorrelatorTest::qualifiesStableMapping()
{
    UbloxRfBandCorrelator correlator;
    correlator.setReceiverIdentity("X20P/HPG 2.02");
    const auto rf = rfBlocks({0, 1, 2});
    const auto span = spanBlocks({1586000000, 1191937500, 1254906250});

    observePair(correlator, 1000, rf, span);
    observePair(correlator, 2000, rf, span);
    QVERIFY(!correlator.isQualified());
    observePair(correlator, 3000, rf, span);

    QVERIFY(correlator.isQualified());
    QCOMPARE(correlator.mapping().value(0).centerHz, quint32(1586000000));
    QCOMPARE(correlator.mapping().value(1).centerHz, quint32(1191937500));
    QCOMPARE(correlator.mapping().value(2).centerHz, quint32(1254906250));
}

void UbloxRfBandCorrelatorTest::mapsByPhysicalOrder()
{
    UbloxRfBandCorrelator correlator;
    const auto rf = rfBlocks({2, 0, 1});
    const auto span = spanBlocks({1254906250, 1586000000, 1191937500});
    observePair(correlator, 1000, rf, span);
    observePair(correlator, 2000, rf, span);
    observePair(correlator, 3000, rf, span);

    QVERIFY(correlator.isQualified());
    QCOMPARE(correlator.mapping().value(0).centerHz, quint32(1586000000));
}

void UbloxRfBandCorrelatorTest::rejectsStalePairs()
{
    UbloxRfBandCorrelator correlator;
    correlator.observeMonRf(rfBlocks({0}), 1000, 1);
    correlator.observeMonSpan(spanBlocks({1586000000}), 1300, 2);
    QVERIFY(!correlator.isQualified());

    correlator.observeMonRf(rfBlocks({0}), 1301, 3);
    QVERIFY(!correlator.isQualified());
}

void UbloxRfBandCorrelatorTest::rejectsNonConsecutivePairs()
{
    UbloxRfBandCorrelator correlator;
    const auto rf = rfBlocks({0});
    const auto span = spanBlocks({1586000000});

    correlator.observeMonRf(rf, 1000, 1);
    correlator.observeMonSpan(span, 1001, 3);
    QVERIFY(!correlator.isQualified());

    observePair(correlator, 2000, rf, span);
    observePair(correlator, 3000, rf, span);
    QVERIFY(!correlator.isQualified());
    observePair(correlator, 4000, rf, span);
    QVERIFY(correlator.isQualified());
}

void UbloxRfBandCorrelatorTest::invalidatesOnFrequencyChange()
{
    UbloxRfBandCorrelator correlator;
    const auto rf = rfBlocks({0});
    const auto span = spanBlocks({1586000000});
    observePair(correlator, 1000, rf, span);
    observePair(correlator, 2000, rf, span);
    observePair(correlator, 3000, rf, span);
    QVERIFY(correlator.isQualified());

    observePair(correlator, 4000, rf, spanBlocks({1575000000}));
    QVERIFY(!correlator.isQualified());
    QVERIFY(correlator.mapping().isEmpty());
}

void UbloxRfBandCorrelatorTest::resetsOnReceiverIdentityChange()
{
    UbloxRfBandCorrelator correlator;
    const auto rf = rfBlocks({0});
    const auto span = spanBlocks({1586000000});
    observePair(correlator, 1000, rf, span);
    observePair(correlator, 2000, rf, span);
    observePair(correlator, 3000, rf, span);
    QVERIFY(correlator.isQualified());

    correlator.setReceiverIdentity("X20P/HPG 2.03");
    QVERIFY(!correlator.isQualified());
    QVERIFY(correlator.mapping().isEmpty());
}

QTEST_MAIN(UbloxRfBandCorrelatorTest)
#include "ubloxrfbandcorrelator_test.moc"