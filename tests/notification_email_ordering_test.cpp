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
        "gnssplot_receiver-a.jpg",
        "measurement-spectrogram.jpg",
        "gnss_spectrum_receiver-b.jpg",
        "waterfall.jpg"
    };

    const QStringList orderedAttachments = orderEmailAttachments(
        attachments, EmailImageOrder::GnssFirst,
        [](const QString &attachment) {
            return attachment.startsWith("gnssplot_") || attachment.startsWith("gnss_spectrum_");
        });

    QCOMPARE(orderedAttachments, QStringList({
        "gnssplot_receiver-a.jpg",
        "gnss_spectrum_receiver-b.jpg",
        "measurement-maxhold.jpg",
        "measurement-spectrogram.jpg",
        "waterfall.jpg"
    }));
}

void NotificationEmailOrderingTest::measurementTriggeredPlacesGnssImagesLast()
{
    const QStringList attachments = {
        "gnssplot_receiver-a.jpg",
        "measurement-maxhold.jpg",
        "gnss_spectrum_receiver-b.jpg",
        "measurement-spectrogram.jpg",
        "waterfall.jpg"
    };

    const QStringList orderedAttachments = orderEmailAttachments(
        attachments, EmailImageOrder::GnssLast,
        [](const QString &attachment) {
            return attachment.startsWith("gnssplot_") || attachment.startsWith("gnss_spectrum_");
        });

    QCOMPARE(orderedAttachments, QStringList({
        "measurement-maxhold.jpg",
        "measurement-spectrogram.jpg",
        "waterfall.jpg",
        "gnssplot_receiver-a.jpg",
        "gnss_spectrum_receiver-b.jpg"
    }));
}

void NotificationEmailOrderingTest::otherTriggersPreserveImageOrder()
{
    const QStringList attachments = {
        "measurement-maxhold.jpg",
        "gnssplot_receiver-a.jpg",
        "measurement-spectrogram.jpg",
        "gnss_spectrum_receiver-b.jpg"
    };

    const QStringList orderedAttachments = orderEmailAttachments(
        attachments, EmailImageOrder::Preserve,
        [](const QString &attachment) {
            return attachment.startsWith("gnssplot_") || attachment.startsWith("gnss_spectrum_");
        });

    QCOMPARE(orderedAttachments, attachments);
}

QTEST_APPLESS_MAIN(NotificationEmailOrderingTest)
#include "notification_email_ordering_test.moc"