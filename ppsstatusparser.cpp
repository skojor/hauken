#include "ppsstatusparser.h"

#include <QJsonDocument>
#include <QJsonObject>

namespace {
bool requireBool(const QJsonObject &object, const QString &key, bool &value, QString &error)
{
    if (!object.value(key).isBool()) {
        error = QString("'%1' must be a boolean").arg(key);
        return false;
    }
    value = object.value(key).toBool();
    return true;
}

bool requireInteger(const QJsonObject &object, const QString &key, qint64 &value, QString &error)
{
    const QJsonValue jsonValue = object.value(key);
    if (!jsonValue.isDouble()) {
        error = QString("'%1' must be an integer").arg(key);
        return false;
    }
    value = jsonValue.toInteger();
    if (QJsonValue(value) != jsonValue) {
        error = QString("'%1' must be an integer").arg(key);
        return false;
    }
    return true;
}

bool parseSource(const QJsonObject &object, PpsSourceData &source, QString &error)
{
    if (!requireBool(object, "valid", source.valid, error)
        || !requireInteger(object, "edge_timestamp_ns", source.edgeTimestampNs, error)
        || !requireInteger(object, "age_ns", source.ageNs, error)
        || !requireInteger(object, "interval_ns", source.intervalNs, error))
        return false;

    source.reason = object.value("reason").toString();
    const QJsonValue metadataValue = object.value("metadata");
    if (metadataValue.isUndefined()) return true;
    if (!metadataValue.isObject()) {
        error = "'metadata' must be an object";
        return false;
    }

    const QJsonObject metadata = metadataValue.toObject();
    if (!requireBool(metadata, "valid", source.metadataValid, error)) return false;
    qint64 fixMode = 0;
    qint64 satellites = 0;
    if (!requireInteger(metadata, "fix_mode", fixMode, error)
        || !requireInteger(metadata, "satellites", satellites, error))
        return false;
    source.fixMode = static_cast<int>(fixMode);
    source.satellites = static_cast<int>(satellites);
    return true;
}

bool parseOffset(const QJsonObject &object, PpsOffsetData &offset, QString &error)
{
    if (!requireBool(object, "valid", offset.valid, error)
        || !requireInteger(object, "current_ns", offset.currentNs, error)
        || !requireInteger(object, "min_ns", offset.minimumNs, error)
        || !requireInteger(object, "max_ns", offset.maximumNs, error))
        return false;

    if (!object.value("mean_ns").isDouble() || !object.value("stddev_ns").isDouble()) {
        error = "'mean_ns' and 'stddev_ns' must be numbers";
        return false;
    }
    offset.meanNs = object.value("mean_ns").toDouble();
    offset.stddevNs = object.value("stddev_ns").toDouble();

    qint64 samples = 0;
    if (!requireInteger(object, "samples", samples, error) || samples < 0) {
        if (error.isEmpty()) error = "'samples' cannot be negative";
        return false;
    }
    offset.samples = static_cast<quint64>(samples);
    return true;
}

bool requireObject(const QJsonObject &parent, const QString &key, QJsonObject &object, QString &error)
{
    if (!parent.value(key).isObject()) {
        error = QString("'%1' must be an object").arg(key);
        return false;
    }
    object = parent.value(key).toObject();
    return true;
}
}

bool PpsStatusParser::parseStatus(const QByteArray &payload, PpsData &data, QString &error)
{
    QJsonParseError parseError;
    const QJsonDocument document = QJsonDocument::fromJson(payload, &parseError);
    if (parseError.error != QJsonParseError::NoError || !document.isObject()) {
        error = "Invalid PPS status JSON: " + parseError.errorString();
        return false;
    }

    const QJsonObject root = document.object();
    qint64 schemaVersion = 0;
    qint64 sequence = 0;
    if (!requireInteger(root, "schema_version", schemaVersion, error) || schemaVersion != 1) {
        if (error.isEmpty()) error = "Unsupported PPS schema version";
        return false;
    }
    if (!requireInteger(root, "sequence", sequence, error) || sequence < 0) return false;
    if (!root.value("timestamp").isString() || !root.value("backend").isString()) {
        error = "'timestamp' and 'backend' must be strings";
        return false;
    }

    PpsData parsed;
    parsed.schemaVersion = static_cast<int>(schemaVersion);
    parsed.sequence = static_cast<quint64>(sequence);
    parsed.generatedAtUtc = QDateTime::fromString(root.value("timestamp").toString(), Qt::ISODateWithMs);
    if (!parsed.generatedAtUtc.isValid())
        parsed.generatedAtUtc = QDateTime::fromString(root.value("timestamp").toString(), Qt::ISODate);
    if (!parsed.generatedAtUtc.isValid()) {
        error = "'timestamp' is not a valid ISO-8601 timestamp";
        return false;
    }
    parsed.generatedAtUtc = parsed.generatedAtUtc.toUTC();
    parsed.receivedAtUtc = QDateTime::currentDateTimeUtc();
    parsed.backend = root.value("backend").toString();
    parsed.error = root.value("error").toString();
    if (!requireBool(root, "qualified", parsed.qualified, error)) return false;

    QJsonObject sources;
    QJsonObject offsets;
    QJsonObject reference;
    QJsonObject gps;
    QJsonObject galileo;
    QJsonObject gpsReference;
    QJsonObject galileoReference;
    QJsonObject galileoGps;
    if (!requireObject(root, "sources", sources, error)
        || !requireObject(root, "offsets", offsets, error)
        || !requireObject(sources, "reference", reference, error)
        || !requireObject(sources, "gps", gps, error)
        || !requireObject(sources, "galileo", galileo, error)
        || !requireObject(offsets, "gps_reference", gpsReference, error)
        || !requireObject(offsets, "galileo_reference", galileoReference, error)
        || !requireObject(offsets, "galileo_gps", galileoGps, error))
        return false;

    if (!parseSource(reference, parsed.reference, error)
        || !parseSource(gps, parsed.gps, error)
        || !parseSource(galileo, parsed.galileo, error)
        || !parseOffset(gpsReference, parsed.gpsReference, error)
        || !parseOffset(galileoReference, parsed.galileoReference, error)
        || !parseOffset(galileoGps, parsed.galileoGps, error))
        return false;

    data = parsed;
    error.clear();
    return true;
}

bool PpsStatusParser::parseAvailability(const QByteArray &payload, bool &online, QString &error)
{
    const QByteArray value = payload.trimmed().toLower();
    if (value == "online") {
        online = true;
        error.clear();
        return true;
    }
    if (value == "offline") {
        online = false;
        error.clear();
        return true;
    }
    error = "PPS availability must be 'online' or 'offline'";
    return false;
}