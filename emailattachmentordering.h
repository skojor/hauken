#ifndef EMAILATTACHMENTORDERING_H
#define EMAILATTACHMENTORDERING_H

#include <QList>

enum class EmailImageOrder
{
    Preserve,
    GnssFirst,
    GnssLast
};

template <typename T, typename IsGnssAttachment>
QList<T> orderEmailAttachments(const QList<T> &attachments,
                               EmailImageOrder order,
                               IsGnssAttachment isGnssAttachment)
{
    if (order == EmailImageOrder::Preserve)
        return attachments;

    QList<T> gnssAttachments;
    QList<T> otherAttachments;
    for (const T &attachment : attachments) {
        if (isGnssAttachment(attachment))
            gnssAttachments.append(attachment);
        else
            otherAttachments.append(attachment);
    }

    if (order == EmailImageOrder::GnssFirst) {
        gnssAttachments.append(otherAttachments);
        return gnssAttachments;
    }

    otherAttachments.append(gnssAttachments);
    return otherAttachments;
}

#endif // EMAILATTACHMENTORDERING_H