#include <QtTest>
#include "measurementdeviceavailability.h"

class MeasurementDeviceAvailabilityTest : public QObject
{
    Q_OBJECT

private slots:
    void recognizesCompleteChannelListing();
    void rejectsIncompleteChannelListing();
    void requiresTwoConsecutiveClearSweeps();
    void occupiedAndInconclusiveSweepsResetConfirmation();
};

void MeasurementDeviceAvailabilityTest::recognizesCompleteChannelListing()
{
    const QByteArray reply("\n\n\n\n\n\n\n\n\n\n");

    QVERIFY(MeasurementDeviceAvailability::hasCompleteReply(reply));
}

void MeasurementDeviceAvailabilityTest::rejectsIncompleteChannelListing()
{
    const QByteArray reply("\n\n\n\n\n\n\n\n\n");

    QVERIFY(!MeasurementDeviceAvailability::hasCompleteReply(reply));
    QVERIFY(!MeasurementDeviceAvailability::hasCompleteReply(QByteArray()));
}

void MeasurementDeviceAvailabilityTest::requiresTwoConsecutiveClearSweeps()
{
    MeasurementDeviceAvailability availability;

    QVERIFY(!availability.recordSweep(MeasurementDeviceAvailability::Result::Clear));
    QCOMPARE(availability.consecutiveClearSweeps(), 1);
    QVERIFY(availability.recordSweep(MeasurementDeviceAvailability::Result::Clear));
}

void MeasurementDeviceAvailabilityTest::occupiedAndInconclusiveSweepsResetConfirmation()
{
    MeasurementDeviceAvailability availability;

    QVERIFY(!availability.recordSweep(MeasurementDeviceAvailability::Result::Clear));
    QVERIFY(!availability.recordSweep(MeasurementDeviceAvailability::Result::Occupied));
    QCOMPARE(availability.consecutiveClearSweeps(), 0);
    QVERIFY(!availability.recordSweep(MeasurementDeviceAvailability::Result::Clear));
    QVERIFY(!availability.recordSweep(MeasurementDeviceAvailability::Result::Inconclusive));
    QCOMPARE(availability.consecutiveClearSweeps(), 0);
}

QTEST_MAIN(MeasurementDeviceAvailabilityTest)
#include "measurementdeviceavailability_test.moc"