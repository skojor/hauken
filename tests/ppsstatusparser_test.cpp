#include <QtTest>
#include "ppsstatusparser.h"

class PpsStatusParserTest : public QObject
{
    Q_OBJECT

private slots:
    void parsesCompleteStatus();
    void rejectsMissingSource();
    void rejectsUnsupportedSchema();
    void parsesAvailability();
};

namespace {
QByteArray validStatus()
{
    return R"json({
        "schema_version": 1,
        "sequence": 42,
        "timestamp": "2026-08-20T12:34:56.123Z",
        "backend": "linuxpps",
        "qualified": true,
        "error": "",
        "sources": {
            "reference": {"valid": true, "edge_timestamp_ns": 1700000000000000123, "age_ns": 1000000, "interval_ns": 1000000000, "reason": ""},
            "gps": {"valid": true, "edge_timestamp_ns": 1700000000000001123, "age_ns": 1100000, "interval_ns": 1000000000, "reason": "", "metadata": {"valid": true, "fix_mode": 3, "satellites": 12}},
            "galileo": {"valid": true, "edge_timestamp_ns": 1700000000000002123, "age_ns": 1200000, "interval_ns": 1000000000, "reason": "", "metadata": {"valid": true, "fix_mode": 3, "satellites": 9}}
        },
        "offsets": {
            "gps_reference": {"valid": true, "current_ns": 1000, "mean_ns": 999.5, "stddev_ns": 20.5, "min_ns": 900, "max_ns": 1100, "samples": 40},
            "galileo_reference": {"valid": true, "current_ns": 2000, "mean_ns": 1998.5, "stddev_ns": 21.5, "min_ns": 1900, "max_ns": 2100, "samples": 39},
            "galileo_gps": {"valid": true, "current_ns": 1000, "mean_ns": 999.0, "stddev_ns": 15.0, "min_ns": 950, "max_ns": 1050, "samples": 39}
        }
    })json";
}
}

void PpsStatusParserTest::parsesCompleteStatus()
{
    PpsData data;
    QString error;

    QVERIFY2(PpsStatusParser::parseStatus(validStatus(), data, error), qPrintable(error));
    QCOMPARE(data.schemaVersion, 1);
    QCOMPARE(data.sequence, quint64(42));
    QCOMPARE(data.reference.edgeTimestampNs, qint64(1700000000000000123));
    QCOMPARE(data.gpsReference.currentNs, qint64(1000));
    QCOMPARE(data.galileoReference.currentNs, qint64(2000));
    QCOMPARE(data.gps.satellites, 12);
    QVERIFY(data.qualified);
    QVERIFY(data.receivedAtUtc.isValid());
}

void PpsStatusParserTest::rejectsMissingSource()
{
    QByteArray payload = validStatus();
    payload.replace("\"galileo\": {", "\"missing_galileo\": {");
    PpsData data;
    QString error;

    QVERIFY(!PpsStatusParser::parseStatus(payload, data, error));
    QVERIFY(error.contains("galileo"));
}

void PpsStatusParserTest::rejectsUnsupportedSchema()
{
    QByteArray payload = validStatus();
    payload.replace("\"schema_version\": 1", "\"schema_version\": 2");
    PpsData data;
    QString error;

    QVERIFY(!PpsStatusParser::parseStatus(payload, data, error));
    QCOMPARE(error, QString("Unsupported PPS schema version"));
}

void PpsStatusParserTest::parsesAvailability()
{
    bool online = false;
    QString error;
    QVERIFY(PpsStatusParser::parseAvailability("online", online, error));
    QVERIFY(online);
    QVERIFY(PpsStatusParser::parseAvailability("offline\n", online, error));
    QVERIFY(!online);
    QVERIFY(!PpsStatusParser::parseAvailability("unknown", online, error));
}

QTEST_MAIN(PpsStatusParserTest)
#include "ppsstatusparser_test.moc"