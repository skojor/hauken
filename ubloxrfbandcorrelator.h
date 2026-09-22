#ifndef UBLOXRFBANDCORRELATOR_H
#define UBLOXRFBANDCORRELATOR_H

#include <QMap>
#include <QString>

#include "ubloxmonrfparser.h"
#include "ubloxmonspanparser.h"

struct UbloxRfBandMetadata
{
    quint32 centerHz = 0;
    quint32 spanHz = 0;
    quint32 resolutionHz = 0;

    bool operator==(const UbloxRfBandMetadata &) const = default;
};

class UbloxRfBandCorrelator
{
public:
    void setReceiverIdentity(const QString &identity);
    void observeMonRf(const QVector<UbloxMonRfBlock> &blocks, qint64 timestampMs, quint64 sequence);
    void observeMonSpan(const QVector<UbloxMonSpanBlock> &blocks, qint64 timestampMs, quint64 sequence);
    void invalidate();

    bool isQualified() const { return qualified; }
    QMap<int, UbloxRfBandMetadata> mapping() const { return qualifiedMapping; }

private:
    void tryPair();
    void resetQualification();

    QString receiverIdentity;
    QVector<UbloxMonRfBlock> pendingRfBlocks;
    QVector<UbloxMonSpanBlock> pendingSpanBlocks;
    qint64 pendingRfTimestampMs = -1;
    qint64 pendingSpanTimestampMs = -1;
    quint64 pendingRfSequence = 0;
    quint64 pendingSpanSequence = 0;
    QMap<int, UbloxRfBandMetadata> candidateMapping;
    QMap<int, UbloxRfBandMetadata> qualifiedMapping;
    int consistentPairCount = 0;
    bool qualified = false;

    static constexpr qint64 maximumPairAgeMs = 250;
    static constexpr int requiredConsistentPairs = 3;
};

#endif // UBLOXRFBANDCORRELATOR_H