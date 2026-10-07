#include <QtTest>

#include "gnssanalyzer.h"

class GnssAnalyzerTest : public QObject
{
    Q_OBJECT

private slots:
    void gnssIncidentGrace();
    void overlappingIncidents_data();
    void overlappingIncidents();
    void renewedIncidentRestartsGrace();
    void signalAveragesFreeze_data();
    void signalAveragesFreeze();
};

void GnssAnalyzerTest::gnssIncidentGrace()
{
    GnssIncidentState state;
    const auto now = GnssIncidentState::Clock::now();
    QVERIFY(!state.shouldPauseSignalAverages(now));
    state.setGnssIncidentActive(true, now);
    QVERIFY(state.shouldPauseSignalAverages(now + std::chrono::minutes(10)));
    state.setGnssIncidentActive(false, now);
    QVERIFY(state.shouldPauseSignalAverages(now + std::chrono::milliseconds(59999)));
    state.setGnssIncidentActive(false, now + std::chrono::seconds(30));
    QVERIFY(!state.shouldPauseSignalAverages(now + std::chrono::seconds(60)));
}

void GnssAnalyzerTest::overlappingIncidents_data()
{
    QTest::addColumn<bool>("gnssClearsFirst");
    QTest::newRow("GNSS-clears-first") << true;
    QTest::newRow("RF-clears-first") << false;
}

void GnssAnalyzerTest::overlappingIncidents()
{
    QFETCH(bool, gnssClearsFirst);
    GnssIncidentState state;
    const auto now = GnssIncidentState::Clock::now();
    state.setGnssIncidentActive(true, now);
    state.setTraceIncidentActive(true, now);
    if (gnssClearsFirst) state.setGnssIncidentActive(false, now);
    else state.setTraceIncidentActive(false, now);
    QVERIFY(state.shouldPauseSignalAverages(now + std::chrono::minutes(10)));

    const auto lastClear = now + std::chrono::minutes(10);
    if (gnssClearsFirst) state.setTraceIncidentActive(false, lastClear);
    else state.setGnssIncidentActive(false, lastClear);
    QVERIFY(state.shouldPauseSignalAverages(lastClear + std::chrono::milliseconds(59999)));
    QVERIFY(!state.shouldPauseSignalAverages(lastClear + std::chrono::seconds(60)));
}

void GnssAnalyzerTest::renewedIncidentRestartsGrace()
{
    GnssIncidentState state;
    const auto now = GnssIncidentState::Clock::now();
    state.setGnssIncidentActive(true, now);
    state.setGnssIncidentActive(false, now);
    state.setTraceIncidentActive(true, now + std::chrono::seconds(30));
    state.setTraceIncidentActive(false, now + std::chrono::seconds(40));
    QVERIFY(state.shouldPauseSignalAverages(now + std::chrono::seconds(99)));
    QVERIFY(!state.shouldPauseSignalAverages(now + std::chrono::seconds(100)));
}

void GnssAnalyzerTest::signalAveragesFreeze_data()
{
    QTest::addColumn<bool>("gnssIncident");
    QTest::newRow("GNSS") << true;
    QTest::newRow("RF") << false;
}

void GnssAnalyzerTest::signalAveragesFreeze()
{
    QFETCH(bool, gnssIncident);
    auto state = QSharedPointer<GnssIncidentState>::create();
    GnssAnalyzer analyzer({}, "receiver", state);
    GnssData data;
    data.cno = 40;
    data.agc = 50;
    data.agcBlocks[0].agc = 30;
    data.agcBlocks[1].agc = 70;
    const auto calculate = [&data](GnssAnalyzer &target) {
        return QMetaObject::invokeMethod(&target, "calcAvgs", Qt::DirectConnection,
                                         Q_ARG(GnssData &, data));
    };
    QVERIFY(calculate(analyzer));
    QCOMPARE(data.avgCno, 40);
    QCOMPARE(data.avgAgc, 50);
    QCOMPARE(data.agcBlocks[0].avgAgc, 30);
    QCOMPARE(data.agcBlocks[1].avgAgc, 70);

    if (gnssIncident) state->setGnssIncidentActive(true);
    else state->setTraceIncidentActive(true);
    data.cno = 10;
    data.agc = 100;
    data.agcBlocks[0].agc = 90;
    data.agcBlocks[1].agc = 10;
    QVERIFY(calculate(analyzer));

    if (gnssIncident) state->setGnssIncidentActive(false);
    else state->setTraceIncidentActive(false);
    GnssAnalyzer addedDuringGrace({}, "new-receiver", state);
    QVERIFY(calculate(addedDuringGrace));
    QCOMPARE(data.avgCno, 40);
    QCOMPARE(data.avgAgc, 50);
    QCOMPARE(data.agcBlocks[0].avgAgc, 30);
    QCOMPARE(data.agcBlocks[1].avgAgc, 70);
    QCOMPARE(data.avgCnoArray.size(), 1);
    QCOMPARE(data.avgAgcArray.size(), 1);
    QCOMPARE(data.agcBlocks[0].avgAgcArray.size(), 1);
    QCOMPARE(data.agcBlocks[1].avgAgcArray.size(), 1);

    const auto past = GnssIncidentState::Clock::now() - std::chrono::seconds(61);
    state->setGnssIncidentActive(true, past);
    state->setGnssIncidentActive(false, past);
    QVERIFY(calculate(analyzer));
    QCOMPARE(data.avgCno, 25);
    QCOMPARE(data.avgAgc, 75);
    QCOMPARE(data.agcBlocks[0].avgAgc, 60);
    QCOMPARE(data.agcBlocks[1].avgAgc, 40);
}

QTEST_GUILESS_MAIN(GnssAnalyzerTest)
#include "gnssanalyzer_test.moc"