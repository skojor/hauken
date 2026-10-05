#include "ubloxrfbandcorrelator.h"

#include <QtGlobal>

void UbloxRfBandCorrelator::setReceiverIdentity(const QString &identity)
{
    if (receiverIdentity == identity) return;

    receiverIdentity = identity;
    pendingRfBlocks.clear();
    pendingSpanBlocks.clear();
    pendingRfTimestampMs = -1;
    pendingSpanTimestampMs = -1;
    resetQualification();
}

void UbloxRfBandCorrelator::observeMonRf(const QVector<UbloxMonRfBlock> &blocks, qint64 timestampMs,
                                        quint64 sequence)
{
    if (!pendingRfBlocks.isEmpty()) resetQualification();
    pendingRfBlocks = blocks;
    pendingRfTimestampMs = timestampMs;
    pendingRfSequence = sequence;
    tryPair();
}

void UbloxRfBandCorrelator::observeMonSpan(const QVector<UbloxMonSpanBlock> &blocks, qint64 timestampMs,
                                          quint64 sequence)
{
    if (!pendingSpanBlocks.isEmpty()) resetQualification();
    pendingSpanBlocks = blocks;
    pendingSpanTimestampMs = timestampMs;
    pendingSpanSequence = sequence;
    tryPair();
}

void UbloxRfBandCorrelator::invalidate()
{
    pendingRfBlocks.clear();
    pendingSpanBlocks.clear();
    pendingRfTimestampMs = -1;
    pendingSpanTimestampMs = -1;
    pendingRfSequence = 0;
    pendingSpanSequence = 0;
    resetQualification();
}

void UbloxRfBandCorrelator::tryPair()
{
    if (pendingRfTimestampMs < 0 || pendingSpanTimestampMs < 0) return;

    if (qAbs(pendingRfTimestampMs - pendingSpanTimestampMs) > maximumPairAgeMs
        || qAbs(static_cast<qint64>(pendingRfSequence) - static_cast<qint64>(pendingSpanSequence)) != 1) {
        invalidate();
        return;
    }

    QMap<int, UbloxRfBandMetadata> observedMapping;
    if (!pendingRfBlocks.isEmpty() && pendingRfBlocks.size() == pendingSpanBlocks.size()) {
        for (int index = 0; index < pendingRfBlocks.size(); ++index) {
            const int blockId = pendingRfBlocks.at(index).blockId;
            const UbloxMonSpanBlock &span = pendingSpanBlocks.at(index);
            if (blockId < 0 || observedMapping.contains(blockId)) {
                observedMapping.clear();
                break;
            }
            observedMapping.insert(blockId, {span.centerHz, span.spanHz, span.resolutionHz});
        }
    }

    pendingRfBlocks.clear();
    pendingSpanBlocks.clear();
    pendingRfTimestampMs = -1;
    pendingSpanTimestampMs = -1;
    pendingRfSequence = 0;
    pendingSpanSequence = 0;

    if (observedMapping.isEmpty()) {
        resetQualification();
        return;
    }

    if (candidateMapping == observedMapping) {
        ++consistentPairCount;
    }
    else {
        resetQualification();
        candidateMapping = observedMapping;
        consistentPairCount = 1;
    }

    if (consistentPairCount >= requiredConsistentPairs) {
        qualifiedMapping = candidateMapping;
        qualified = true;
    }
}

void UbloxRfBandCorrelator::resetQualification()
{
    candidateMapping.clear();
    qualifiedMapping.clear();
    consistentPairCount = 0;
    qualified = false;
}