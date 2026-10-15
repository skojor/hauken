#include <QtTest>
#include <QSignalSpy>
#include <QTemporaryDir>

#include "config.h"
#include "tracebuffer.h"
#include "waterfall.h"

class MeasurementReceiverProfileTest : public QObject
{
    Q_OBJECT

private slots:
    void profileOverlayRestoresOrdinarySettings();
    void profileBoundaryClearsAndGatesTraceHistory();
    void waterfallRestartClearsImageAndPendingTrace();
};

void MeasurementReceiverProfileTest::profileOverlayRestoresOrdinarySettings()
{
    QTemporaryDir tempDir;
    QVERIFY(tempDir.isValid());
    Config config(tempDir.filePath("profile.ini"));
    config.settings->setValue("instr/StartFreq", 1000000000ULL);
    config.settings->setValue("instr/StopFreq", 1100000000ULL);
    config.settings->setValue("instr/IpAddr", "192.0.2.10");

    MeasurementReceiverSettings gnssSettings = config.getGnssIncidentReceiverSettings();
    QCOMPARE(gnssSettings.startFreq, 1000000000ULL);
    QCOMPARE(gnssSettings.stopFreq, 1100000000ULL);
    gnssSettings.startFreq = 1500000000ULL;
    gnssSettings.stopFreq = 1600000000ULL;
    config.setGnssIncidentReceiverSettings(gnssSettings);
    config.setGnssIncidentReceiverSettingsEnabled(true);

    QVERIFY(config.activateGnssIncidentReceiverSettings());
    QVERIFY(config.isGnssIncidentReceiverSettingsActive());
    QCOMPARE(config.getInstrStartFreq(), 1500000000ULL);
    QCOMPARE(config.getInstrStopFreq(), 1600000000ULL);
    QCOMPARE(config.getInstrIpAddr(), QString("192.0.2.10"));
    QVERIFY(!config.activateGnssIncidentReceiverSettings());

    config.deactivateGnssIncidentReceiverSettings();
    QVERIFY(!config.isGnssIncidentReceiverSettingsActive());
    QCOMPARE(config.getInstrStartFreq(), 1000000000ULL);
    QCOMPARE(config.getInstrStopFreq(), 1100000000ULL);
}

void MeasurementReceiverProfileTest::profileBoundaryClearsAndGatesTraceHistory()
{
    QTemporaryDir tempDir;
    QVERIFY(tempDir.isValid());
    auto config = QSharedPointer<Config>::create(tempDir.filePath("trace.ini"));
    config->settings->setValue("workFolder", tempDir.path());
    TraceBuffer traceBuffer(config);
    const QVector<qint16> trace(config->getPlotResolution(), 20);

    traceBuffer.addTrace(trace);
    QCOMPARE(traceBuffer.retSecondsOfBuffer(120).data.size(), 1);

    traceBuffer.setAcceptingTraces(false);
    traceBuffer.invalidateForReceiverProfileChange();
    QCOMPARE(traceBuffer.retSecondsOfBuffer(120).data.size(), 0);
    traceBuffer.addTrace(trace);
    QCOMPARE(traceBuffer.retSecondsOfBuffer(120).data.size(), 0);

    traceBuffer.setAcceptingTraces(true);
    traceBuffer.addTrace(trace);
    QCOMPARE(traceBuffer.retSecondsOfBuffer(120).data.size(), 1);
}

void MeasurementReceiverProfileTest::waterfallRestartClearsImageAndPendingTrace()
{
    QTemporaryDir tempDir;
    QVERIFY(tempDir.isValid());
    auto config = QSharedPointer<Config>::create(tempDir.filePath("waterfall.ini"));
    Waterfall waterfall(config);
    waterfall.start();
    waterfall.updSize(QSize(4, 4));
    waterfall.setAutoYRange(0, 20);

    const QVector<double> trace{10, 10, 10, 10};
    waterfall.receiveTrace(trace);
    waterfall.receiveTrace(trace);
    waterfall.receiveTrace(trace);
    waterfall.updTimerCallback();

    QSignalSpy snapshotSpy(&waterfall, &Waterfall::snapshotReady);
    waterfall.requestSnapshot(1);
    QCOMPARE(snapshotSpy.size(), 1);
    const QImage populated = qvariant_cast<QImage>(snapshotSpy.takeFirst().at(1));
    QVERIFY(populated.pixelColor(0, 0).alpha() > 0);

    waterfall.restartPlot();
    waterfall.receiveTrace(trace);
    waterfall.updTimerCallback();
    waterfall.requestSnapshot(2);
    QCOMPARE(snapshotSpy.size(), 1);
    const QImage cleared = qvariant_cast<QImage>(snapshotSpy.takeFirst().at(1));
    QCOMPARE(cleared.pixelColor(0, 0).alpha(), 0);
}

QTEST_GUILESS_MAIN(MeasurementReceiverProfileTest)
#include "measurementreceiverprofile_test.moc"