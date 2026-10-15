#include <QtTest>

#include "emailattachmentordering.h"

class NotificationEmailOrderingTest : public QObject
{
    Q_OBJECT

private slots:
    void gnssTriggeredPlacesGnssImagesFirst();
    void measurementTriggeredPlacesGnssImagesLast();
    void otherTriggersPreserveImageOrder();
};

void NotificationEmailOrderingTest::gnssTriggeredPlacesGnssImagesFirst()
{
    const QStringList attachments = {
        "measurement-maxhold.jpg",
        "gnss_info_receiver-data.jpg",
        "gnssplot_receiver-a.jpg",
        "measurement-spectrogram.jpg",
        "gnss_spectrum_receiver-b_band_1.jpg",
        "gnss_spectrum_receiver-b_band_2.jpg",
        "waterfall.jpg"
    };

    const QStringList orderedAttachments = orderEmailAttachments(
        attachments, EmailImageOrder::GnssFirst,
        [](const QString &attachment) {
            if (attachment.startsWith("gnss_info_"))
                return EmailAttachmentGroup::GnssInfo;
            if (attachment.startsWith("gnssplot_") || attachment.startsWith("gnss_spectrum_"))
                return EmailAttachmentGroup::GnssPlot;
            return EmailAttachmentGroup::Other;
        });

    QCOMPARE(orderedAttachments, QStringList({
        "gnssplot_receiver-a.jpg",
        "gnss_spectrum_receiver-b_band_1.jpg",
        "gnss_spectrum_receiver-b_band_2.jpg",
        "gnss_info_receiver-data.jpg",
        "measurement-maxhold.jpg",
        "measurement-spectrogram.jpg",
        "waterfall.jpg"
    }));
}

void NotificationEmailOrderingTest::measurementTriggeredPlacesGnssImagesLast()
{
    const QStringList attachments = {
        "gnss_info_receiver-data.jpg",
        "gnssplot_receiver-a.jpg",
        "measurement-maxhold.jpg",
        "gnss_spectrum_receiver-b_band_1.jpg",
        "gnss_spectrum_receiver-b_band_2.jpg",
        "measurement-spectrogram.jpg",
        "waterfall.jpg"
    };

    const QStringList orderedAttachments = orderEmailAttachments(
        attachments, EmailImageOrder::GnssLast,
        [](const QString &attachment) {
            if (attachment.startsWith("gnss_info_"))
                return EmailAttachmentGroup::GnssInfo;
            if (attachment.startsWith("gnssplot_") || attachment.startsWith("gnss_spectrum_"))
                return EmailAttachmentGroup::GnssPlot;
            return EmailAttachmentGroup::Other;
        });

    QCOMPARE(orderedAttachments, QStringList({
        "measurement-maxhold.jpg",
        "measurement-spectrogram.jpg",
        "waterfall.jpg",
        "gnssplot_receiver-a.jpg",
        "gnss_spectrum_receiver-b_band_1.jpg",
        "gnss_spectrum_receiver-b_band_2.jpg",
        "gnss_info_receiver-data.jpg"
    }));
}

void NotificationEmailOrderingTest::otherTriggersPreserveImageOrder()
{
    const QStringList attachments = {
        "gnss_info_receiver-data.jpg",
        "measurement-maxhold.jpg",
        "gnssplot_receiver-a.jpg",
        "measurement-spectrogram.jpg",
        "gnss_spectrum_receiver-b_band_1.jpg",
        "gnss_spectrum_receiver-b_band_2.jpg"
    };

    const QStringList orderedAttachments = orderEmailAttachments(
        attachments, EmailImageOrder::Preserve,
        [](const QString &attachment) {
            if (attachment.startsWith("gnss_info_"))
                return EmailAttachmentGroup::GnssInfo;
            if (attachment.startsWith("gnssplot_") || attachment.startsWith("gnss_spectrum_"))
                return EmailAttachmentGroup::GnssPlot;
            return EmailAttachmentGroup::Other;
        });

    QCOMPARE(orderedAttachments, QStringList({
        "measurement-maxhold.jpg",
        "gnssplot_receiver-a.jpg",
        "measurement-spectrogram.jpg",
        "gnss_spectrum_receiver-b_band_1.jpg",
        "gnss_spectrum_receiver-b_band_2.jpg"
        "gnss_info_receiver-data.jpg"
    }));
}

QTEST_APPLESS_MAIN(NotificationEmailOrderingTest)
#include "notification_email_ordering_test.moc"