#ifndef EMAILATTACHMENTORDERING_H
#define EMAILATTACHMENTORDERING_H

#include <QList>

enum class EmailImageOrder
{
    Preserve,
    GnssFirst,
    GnssLast
};

enum class EmailAttachmentGroup
{
    Other,
    GnssPlot,
    GnssInfo
};

template <typename T, typename AttachmentGroup>
QList<T> orderEmailAttachments(const QList<T> &attachments,
                               EmailImageOrder order,
                               AttachmentGroup attachmentGroup)
{
    QList<T> gnssPlots;
    QList<T> gnssInfo;
    QList<T> otherAttachments;
    QList<T> originalOrderWithoutInfo;
    int lastGnssPlotIndex = -1;
    for (const T &attachment : attachments) {
        const EmailAttachmentGroup group = attachmentGroup(attachment);
        switch (group) {
        case EmailAttachmentGroup::GnssPlot:
            gnssPlots.append(attachment);
            lastGnssPlotIndex = originalOrderWithoutInfo.size();
            originalOrderWithoutInfo.append(attachment);
            break;
        case EmailAttachmentGroup::GnssInfo:
            gnssInfo.append(attachment);
            break;
        case EmailAttachmentGroup::Other:
            otherAttachments.append(attachment);
            originalOrderWithoutInfo.append(attachment);
            break;
        }
    }

    if (order == EmailImageOrder::Preserve) {
        if (lastGnssPlotIndex < 0)
            lastGnssPlotIndex = originalOrderWithoutInfo.size() - 1;
        for (const T &attachment : gnssInfo)
            originalOrderWithoutInfo.insert(++lastGnssPlotIndex, attachment);
        return originalOrderWithoutInfo;
    }

    QList<T> orderedAttachments;
    orderedAttachments.reserve(attachments.size());
    if (order == EmailImageOrder::GnssFirst) {
        orderedAttachments.append(gnssPlots);
        orderedAttachments.append(gnssInfo);
        orderedAttachments.append(otherAttachments);
    }
    else {
        orderedAttachments.append(otherAttachments);
        orderedAttachments.append(gnssPlots);
        orderedAttachments.append(gnssInfo);
    }

    return orderedAttachments;
}

#endif // EMAILATTACHMENTORDERING_H