#include "plotandanalyze.h"
#include "asciitranslator.h"
#include "gif.h"
#include "opencv2/opencv.hpp"
#include "version.h"
#include "appclock.h"
#include "fftw3.h"
#include <algorithm>
#include <complex>
#include <limits>
#include <QFileInfo>
#include <QPainter>

PlotAndAnalyze::PlotAndAnalyze(QSharedPointer<Config> c)
{
    m_config = c;
}

void PlotAndAnalyze::start()
{
    m_reqTraceplotTimer = new QTimer(this);
    m_incidentEndTimer = new QTimer(this);
    m_reqTraceplotTimer->setSingleShot(true);
    m_incidentEndTimer->setSingleShot(true);

    connect(m_reqTraceplotTimer, &QTimer::timeout, this, [this] () {
        requestRecordingArtifacts(m_recordingId);
    });

    connect(m_incidentEndTimer, &QTimer::timeout, this, [this] () {
        m_freqNotificationSentForIncident = false;
    });
}

void PlotAndAnalyze::end()
{
    // Timers are now owned by parent (this), will be deleted automatically
    delete m_reqTraceplotTimer;
}

void PlotAndAnalyze::traceIncidentStarted()
{
    if (m_incidentEndTimer && m_incidentEndTimer->isActive())
        m_incidentEndTimer->stop();
}

void PlotAndAnalyze::traceIncidentEnded()
{
    if (!m_incidentEndTimer)
        return;

    m_incidentEndTimer->start(m_config->getSdefRecordTime() * 60 * 1000);
}

void PlotAndAnalyze::receiveFftData(const QVector<QVector<double> > &fftVector, const IqMetadata &meta)
{
    // FFTVector contains FFT of all available IQ samples, with 12 samples overlap. Meta describes freq, samplerate and bw data from measurement
    if (fftVector.isEmpty()) {
        qWarning() << "PlotAndAnalyze: Received empty FFT vector data, aborting";
        return;
    }
    QVector<QImage> images;
    m_metadata = meta; // Work copy, stored in class
    const double halfBandwidthMHz = m_metadata.bandwidth * 0.5e-6;
    m_lastClassificationStartMHz = m_metadata.centerfreq * 1e-6 - halfBandwidthMHz;
    m_lastClassificationStopMHz = m_metadata.centerfreq * 1e-6 + halfBandwidthMHz;
    m_lastClassificationHasRfi = false;
    createFilename();
    findIqFftMinMaxAvg(fftVector, 0, 0, true);

    // This part looks at spectral density and estimates period time
    images = createImages(fftVector, 5e-3, m_metadata.maxLoc, 1, true);
    int lines = m_metadata.maxLoc + ( ((double)m_metadata.samplerate * 5e-3) - (m_metadata.fftSize - m_metadata.samplesInc) ) / m_metadata.samplesInc; // Look at 5 ms of data, or as much as available
    if (m_metadata.maxLoc + lines > fftVector.size()) lines = fftVector.size();
    calcPeriodAndDensity(fftVector, m_metadata.maxLoc, lines);

    // Classification images generated here.
    images = createImages(fftVector, 5e-4, m_metadata.maxLoc, 1, true);
    images += createImages(fftVector, 5e-4, m_metadata.maxLoc, 10, true);
    if (images.size() > 1) images.removeAt(1); // First and second img is the same if enough I/Q data available, remove

    /*for (auto && img : images) {
        QImage scl = img.scaled(256, 256, Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
        scl.save("c:/hauken/" + QString::number(QDateTime::currentMSecsSinceEpoch()) + ".jpg");
    }*/ //DEBUG INFO

    if (!images.isEmpty()) {
        emit imagesReadyForClassification(images, m_metadata); // Send copy of metadata to allow async/multi thread op
    }
    else {
        qWarning() << "Not enough I/Q data to generate plot(s)";
        return;
    }

    // Email plot with text/lines generated here
    images = createImages(fftVector, m_config->getIqFftPlotLength() * 1e-6, m_metadata.maxLoc, 1, false);
    if (!images.isEmpty())
        createJpgWithInfo(images.first(), m_config->getIqFftPlotLength() * 1e-6);
    else {
        qWarning() << "Not enough I/Q data to generate plot(s)";
        return;
    }

    // GIF creation here (if enabled)
    if (m_config->getEmailAddGif()) {
        images = createImages(fftVector, 5e-4, m_metadata.maxLoc, 50, false);
        if (!images.isEmpty()) {
            createGif(images);
        }
        else {
            qWarning() << "Not enough I/Q data to generate plot(s)";
            return;
        }
    }

    // Movie created here (if enabled)
    if (m_config->getIqGenerateMovie()) {
        images = createImages(fftVector, 5e-4, 0, 9999, false);
        if (!images.isEmpty())
            createMovie(images);
        else  {
            qWarning() << "Not enough I/Q data to generate plot(s)";
            return;
        }
    }
}

QVector<QImage> PlotAndAnalyze::createImages(const QVector<QVector<double>> &fftVector,
                                             double lenOfImage,
                                             int startAtPos,
                                             int nrOfImages,
                                             bool grayscale,
                                             bool increaseRange)
{
    QVector<QImage> imageVector;

    // This should be 2129 lines for 500 us long image with a 64 bin fft and 12 samples increase ctr.
    int linesPerImage = 0;
    if (m_metadata.samplesInc)
        linesPerImage =(( (m_metadata.samplerate * lenOfImage)
                          - (m_metadata.fftSize - m_metadata.samplesInc) )) / m_metadata.samplesInc;
    else return imageVector;

    if (startAtPos == m_metadata.maxLoc) { // go back a 1/4 image in time if maxLoc is chosen as starting point
        startAtPos -= linesPerImage / 4;
    }

    int lineIterator = startAtPos;
    if ( lineIterator + (nrOfImages * linesPerImage) > fftVector.size() ) {
        // Go back in time to create enough pics if not enough ffts
        lineIterator = fftVector.size() - (nrOfImages * linesPerImage);
    }
    if (lineIterator < 0) lineIterator = 0;
    m_metadata.imageStartAt = lineIterator;

    for (int imageCtr = 0; imageCtr < nrOfImages; imageCtr++) {
        findIqFftMinMaxAvg(fftVector, lineIterator, lineIterator + linesPerImage);       // Find min max avg for each image!
        double min = m_metadata.min;
        if (m_config->getIqUseAvgForPlot() || grayscale) min = m_metadata.avg;
        if (increaseRange)
            m_metadata.max += 10;

        QImage image(fftVector.first().size(), linesPerImage, QImage::Format_ARGB32);
        QColor imgColor;
        double percent;
        int x, y = 0;
        int alpha = 127;

        if (lineIterator + linesPerImage > fftVector.size()) break;

        for (int i = lineIterator; i < lineIterator + linesPerImage; i++) {
            x = 0;
            for (auto &&val : fftVector[i]) {
                percent = (val - min) / (m_metadata.max - min);
                if (percent > 1)
                    percent = 1;
                else if (percent < 0)
                    percent = 0;
                if (!grayscale) imgColor.setHsv(255 - (255 * percent), 255, alpha);
                else imgColor.setHsv(0, 0, 255 * percent);
                image.setPixel(x, y, imgColor.rgba());
                x++;
            }
            y++;
        }
        lineIterator += linesPerImage;
        imageVector.append(image);
    }
    //if (!imageVector.isEmpty()) qDebug() << "FFT plot" << imageVector.size() << imageVector.first().size() << m_metadata.maxLoc << m_metadata.imageStartAt << linesPerImage;
    return imageVector;
}

void PlotAndAnalyze::findIqFftMinMaxAvg(const QVector<QVector<double> > &iqFftResult, int from, int to, bool findMaxLoc)
{
    m_metadata.min = 99e9;
    m_metadata.max = -99e9;
    m_metadata.avg = 0;

    if (!from && !to) { // Whole range, find maxLoc!
        to = iqFftResult.size();
    }

    if (from >= to || to > iqFftResult.size())
        return;

    for (int i = from; i < to; i++) {
        for (const auto &val : iqFftResult[i]) {
            if (val < m_metadata.min) m_metadata.min = val;
            else if (val > m_metadata.max) {
                m_metadata.max = val;
                if (findMaxLoc) m_metadata.maxLoc = i;
            }
            m_metadata.avg += val;
        }
    }
    m_metadata.avg /= ( to - from ) * iqFftResult.first().size();
}

double PlotAndAnalyze::calculateSpectralStructure(const QImage &image) // Not in use, unreliable
{
    cv::Mat bgra(image.height(), image.width(), CV_8UC4,
                 const_cast<uchar*>(image.constBits()),
                 image.bytesPerLine());

    cv::Mat gray;
    cv::cvtColor(bgra, gray, cv::COLOR_BGRA2GRAY);

    cv::Mat floatImg;
    gray.convertTo(floatImg, CV_32F);

    cv::Mat sorted;
    floatImg.reshape(1,1).copyTo(sorted);
    cv::sort(sorted, sorted, cv::SORT_ASCENDING);

    float percentile90 =
        sorted.at<float>(sorted.cols * 0.95);

    cv::Mat signalMask = floatImg > percentile90;

    cv::Mat gradX, gradY;
    cv::Sobel(gray, gradX, CV_32F, 1, 0, 3);
    cv::Sobel(gray, gradY, CV_32F, 0, 1, 3);

    cv::Mat mag;
    cv::magnitude(gradX, gradY, mag);

    cv::Scalar mean, stddev;
    cv::meanStdDev(mag, mean, stddev, signalMask);

    double threshold = mean[0] + stddev[0];

    cv::Mat edgeMask = mag > threshold;
    edgeMask &= signalMask;

    double active = cv::countNonZero(edgeMask);
    double total  = cv::countNonZero(signalMask);

    if(total == 0)
        return 0;

    return active / total;
}

void PlotAndAnalyze::receiveClassification(cv::Mat allResults, QStringList classes, IqMetadata meta)
{
    //QTimer::singleShot(5000, this, &PlotAndAnalyze::reqMaxholdData); // TEST, REMOVE
    m_metadata = meta; // To ensure metadata follows correct samples in case of async/multi thread
    QVector<float> rfiResults(allResults.cols, 0);
    QVector<int> imgsWithRfi, imgsWithoutRfi;
    QString text;
    QTextStream ts(&text);
    bool flagReport = false, foundClassifications = false;
    bool mostLikelyChirpRadar = false;
    bool mostLikelyWbJammer = false;
    bool mostLikelyPbJammer = false;
    bool mostLikelyJammer = false;
    bool prnFound = false;
    bool sweepAndPulse = false;
    bool nbSweep = false;
    bool pulseAndWb = false;
    int imgsWithJammerInRow = 0, imgsWithJammerTotal = 0;
    int imgsWithPrnTotal = 0;
    bool prevImgWasJammer = true;
    bool classificationHasRfi = false;

    // Look for images with classification other than !rfi
    for (int row = 0; row < allResults.rows; row++) {
        cv::Mat output = allResults.row(row);
        std::vector<float> probs;
        output.reshape(1, 1).copyTo(probs);
        for (int classRes = 0; classRes < probs.size(); classRes++) {
            if (classes[classRes].contains("rfi") and probs[classRes] >= 0.5) {
                if (probs[classRes] > 0.5)
                    classificationHasRfi = true;
                imgsWithRfi.append(row);
                break;
            }
            else if (classes[classRes].contains("rfi") and probs[classRes] < 0.5) {
                imgsWithoutRfi.append(row);
                break;
            }
        }
    }

    const double halfBandwidthMHz = m_metadata.bandwidth * 0.5e-6;
    m_lastClassificationStartMHz = m_metadata.centerfreq * 1e-6 - halfBandwidthMHz;
    m_lastClassificationStopMHz = m_metadata.centerfreq * 1e-6 + halfBandwidthMHz;
    m_lastClassificationHasRfi = classificationHasRfi;

    ts << "FFM center " << QString::number(m_metadata.centerfreq * 1e-6, 'f', 1) << " MHz. ";

    if (imgsWithRfi.isEmpty()) { // If any images contains RFI, continue. If not, false alarm
        ts << "Classification: No RFI present. ";

        for (auto && row : imgsWithoutRfi) { // Go through every result without rfiPresent to find what is in each
            cv::Mat output = allResults.row(row);
            std::vector<float> probs;
            output.reshape(1, 1).copyTo(probs);
            for (int i = 0; i < probs.size(); i++) {
                if (probs[i] > 0.5) //
                    rfiResults[i]++;
            }
        }
        for (int i = 0; i < rfiResults.size() - 1; i++) {
            if (rfiResults[i]) { // > 0
                ts << classes[i] << " (" << rfiResults[i] << " of " << allResults.rows << " images). ";
            }
        }
    }

    else {
        for (auto && row : imgsWithRfi) { // Go through every result with rfiPresent to find what is in each
            bool sweep = false, pulse = false, wb = false, pb = false, nb = false, wbPbHigherThanNb = false;
            cv::Mat output = allResults.row(row);
            std::vector<float> probs;
            output.reshape(1, 1).copyTo(probs);

            for (int i = 0; i < probs.size(); i++) {
                if (probs[i] > 0.5) {//
                    rfiResults[i]++;
                    if (classes[i].contains("sweep", Qt::CaseInsensitive)) sweep = true;
                    else if (classes[i].contains("fullband", Qt::CaseInsensitive)) wb = true;
                    else if (classes[i].contains("partialband", Qt::CaseInsensitive)) pb = true;
                    else if (classes[i].contains("nb", Qt::CaseInsensitive)) nb = true;
                    else if (classes[i].contains("pulse", Qt::CaseInsensitive)) pulse = true;
                    else if (classes[i].contains("prn", Qt::CaseInsensitive)) prnFound = true;
                }
            }
            if (sweep and pulse and (wb or pb)) mostLikelyChirpRadar = true;
            else if (sweep and pb and !pulse) mostLikelyPbJammer = true;
            else if (sweep and wb and !pulse) mostLikelyWbJammer = true;
            else if (sweep and !pulse) mostLikelyJammer = true;
            else if (sweep and pulse) sweepAndPulse = true;
            else if (sweep and nb) nbSweep = true;
            else if (pulse and wb) pulseAndWb = true;

            if (mostLikelyJammer or mostLikelyPbJammer or mostLikelyWbJammer) {
                imgsWithJammerTotal++;
                if (prevImgWasJammer) {
                    imgsWithJammerInRow++;
                }
            }
            else
                prevImgWasJammer = false;

            if (prnFound)
                imgsWithPrnTotal++;
        }
        if (imgsWithJammerTotal)
            qDebug() << "Jammer counters in row/total/prn" << imgsWithJammerInRow << imgsWithJammerTotal << imgsWithPrnTotal;

        if (withinL1(m_metadata.trigFrequency)) { // TODO need to check bw of the whole signal. Can be done with tracedata in other func.
            ts << "Signal within L1 band (BW 1.023 MHz). ";
            flagReport = true;
        }
        else if (withinL2(m_metadata.trigFrequency)) {
            ts << "Signal within L2 band. ";
        }

        if (prnFound) {
            flagReport = true;
            ts << "Classified as a PRN modulated signal. ";
        }

        else if (mostLikelyChirpRadar) {
            ts << "Classified as a chirp radar signal. ";
            if (shouldIncludePeriodEstimate() && m_metadata.periodTime > 0)
                ts << "Period estimate: " << (int)(m_metadata.periodTime * 1e6) << " μs. ";
            if (withinL2(m_metadata.trigFrequency))
                ts << "Within L2 band. ";
        }

        else if (mostLikelyWbJammer and (imgsWithJammerInRow > 2 or imgsWithJammerTotal > 5)) {
            flagReport = true;
            ts << "Classified as a wide band jammer. ";
            if (shouldIncludePeriodEstimate() && m_metadata.periodTime > 0)
                ts << "Period estimate: " << (int)(m_metadata.periodTime * 1e6) << " μs. ";
        }

        else if (mostLikelyPbJammer and (imgsWithJammerInRow > 2 or imgsWithJammerTotal > 5)) {
            flagReport = true;
            ts << "Classified as a partial band jammer. ";
            if (shouldIncludePeriodEstimate() && m_metadata.periodTime > 0)
                ts << "Period estimate: " << (int)(m_metadata.periodTime * 1e6) << " μs. ";
        }

        else if (mostLikelyJammer and (imgsWithJammerInRow > 2 or imgsWithJammerTotal > 5)) {
            flagReport = true;
            ts << "Could be a jammer. ";
            if (shouldIncludePeriodEstimate() && m_metadata.periodTime > 0)
                ts << "Period estimate: " << (int)(m_metadata.periodTime * 1e6) << " μs. ";
        }

        else if ((mostLikelyJammer or mostLikelyPbJammer or mostLikelyWbJammer)
                 and (imgsWithJammerInRow < 3 or imgsWithJammerTotal < 6)) {
            ts << "Classification inconclusive. ";
        }

        else if (sweepAndPulse) {
            ts << "Sweep and pulse signal. ";
            if (shouldIncludePeriodEstimate() && m_metadata.periodTime > 0)
                ts << "Period estimate: " << (int)(m_metadata.periodTime * 1e6) << " μs. ";
        }

        else if (nbSweep) {
            flagReport = true;
            ts << "Narrowband sweep signal. ";
            if (shouldIncludePeriodEstimate() && m_metadata.periodTime > 0)
                ts << "Period estimate: " << (int)(m_metadata.periodTime * 1e6) << " μs. ";
        }

        else if (pulseAndWb) {
            ts << "Wideband pulse. ";
            if (shouldIncludePeriodEstimate() && m_metadata.periodTime > 0)
                ts << "Period estimate: " << (int)(m_metadata.periodTime * 1e6) << " μs. ";
            if (withinAirRadar(m_metadata.trigFrequency))
                ts << "Could be air radar at 1274 MHz. ";
        }

        else {
            ts << "Classification: ";
            for (int i = 0; i < rfiResults.size() - 1; i++) {
                if (rfiResults[i] > 0)
                    ts << classes[i] << ": " << rfiResults[i] << " of " << allResults.rows << " images. ";
            }
        }

        if (m_collectNotificationArtifacts && flagReport) emit reportIntentional(text);

        emit analyzerResult(text, 100);
    }

    const bool classificationAddedToPlot = attachClassificationToPendingPlot(text);

    if (m_collectNotificationArtifacts) {
        if (m_metadata.fromFile || classificationAddedToPlot)
            emit toIncidentLog(NOTIFY::TYPE::AIDONTNOTIFY, "", text);
        else
            emit toIncidentLog(NOTIFY::TYPE::AI, "", text);
    }

    //if (!m_metadata.fromFile)
    writeMetaToDisk(allResults, classes);
}


bool PlotAndAnalyze::shouldIncludePeriodEstimate() const
{
    const double centerFrequency = static_cast<double>(m_metadata.centerfreq);
    return qAbs(centerFrequency - GPSL1) < 1e3 || qAbs(centerFrequency - GPSL2) < 1e3;
}

bool PlotAndAnalyze::classificationHasRfiForRange(double startMHz, double stopMHz) const
{
    if (!m_lastClassificationHasRfi)
        return false;

    return m_lastClassificationStartMHz <= stopMHz && m_lastClassificationStopMHz >= startMHz;
}

bool PlotAndAnalyze::attachClassificationToPendingPlot(const QString &text)
{
    if (text.isEmpty() || m_metadata.fromFile)
        return false;

    const QString infoPlot = m_metadata.filename + "_info.jpg";
    int fallbackIndex = -1;

    for (int i = 0; i < m_plotsToSend.size() && i < m_plotsDescription.size(); i++) {
        if (m_plotsToSend[i] == infoPlot) {
            m_plotsDescription[i].prepend(text + "<br>");
            return true;
        }
        if (fallbackIndex < 0 && m_plotsToSend[i].startsWith(m_metadata.filename))
            fallbackIndex = i;
    }

    if (fallbackIndex >= 0 && fallbackIndex < m_plotsDescription.size()) {
        m_plotsDescription[fallbackIndex].prepend(text + "<br>");
        return true;
    }

    return false;
}

void PlotAndAnalyze::createGif(QVector<QImage> &images)
{
    if (images.size() > 0) {
        const int width = 192, height = 192;
        const int sourceDelay = 40; // ms
        const int maxFrames = 20;
        const int bitDepth = 5;
        const qint64 maxInlineGifBytes = 300 * 1024;
        const int frameStep = qMax(1, (images.size() + maxFrames - 1) / maxFrames);

        GifWriter gifWriter;
        int delay = sourceDelay * frameStep;
        const QString gifFilename = m_metadata.filename + ".gif";

        if (!GifBegin(&gifWriter, qPrintable(gifFilename), width, height, delay, bitDepth)) {
            qWarning() << "Cannot write to gif file:" << gifFilename;
            return;
        }
        for (int i = 0; i < images.size(); i += frameStep) {
            QImage converted = images[i].convertToFormat(QImage::Format_RGBA8888);
            converted = converted.scaled(width, height, Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
            if (!converted.isNull())
                GifWriteFrame(&gifWriter, converted.bits(), converted.width(), converted.height(), delay, bitDepth);
        }
        GifEnd(&gifWriter);
        if (m_config->getEmailAddGif() and !m_metadata.fromFile) {
            double delta = (double)m_metadata.imageStartAt * (1.0 / m_metadata.samplerate) * (double)m_metadata.samplesInc;
            quint64 startingAt = m_metadata.timestamp * 1e-6 + delta * 1e3;
            QString fileToSend = gifFilename;
            QString description = QString::number(m_metadata.centerfreq * 1e-6)
                                  + " MHz. Compressed FFT animation from "
                                  + QDateTime::fromMSecsSinceEpoch(startingAt).toString("hh:mm:ss.zzz")
                                  + ", 500 us per source image, total time covered "
                                  + QString::number(500 * images.size()) + " us";

            if (QFileInfo(gifFilename).size() > maxInlineGifBytes) {
                const int thumbWidth = 128, thumbHeight = 128, columns = 5;
                const int frameCount = (images.size() + frameStep - 1) / frameStep;
                const int rows = (frameCount + columns - 1) / columns;
                QImage sheet(columns * thumbWidth, rows * thumbHeight, QImage::Format_RGB888);
                sheet.fill(Qt::black);

                QPainter painter(&sheet);
                for (int sourceIndex = 0, frameIndex = 0; sourceIndex < images.size(); sourceIndex += frameStep, frameIndex++) {
                    const QRect target((frameIndex % columns) * thumbWidth,
                                       (frameIndex / columns) * thumbHeight,
                                       thumbWidth,
                                       thumbHeight);
                    painter.drawImage(target, images[sourceIndex].scaled(thumbWidth, thumbHeight, Qt::IgnoreAspectRatio, Qt::SmoothTransformation));
                }
                painter.end();

                const QString sheetFilename = m_metadata.filename + "_frames.jpg";
                if (sheet.save(sheetFilename, "jpg", 70)) {
                    fileToSend = sheetFilename;
                    description = QString::number(m_metadata.centerfreq * 1e-6)
                                  + " MHz. Compressed FFT frame overview from "
                                  + QDateTime::fromMSecsSinceEpoch(startingAt).toString("hh:mm:ss.zzz")
                                  + ", 500 us per source image, total time covered "
                                  + QString::number(500 * images.size()) + " us";
                }
            }

            m_plotsToSend.append(fileToSend);
            m_plotsDescription.append(description);
        }
    }
}

void PlotAndAnalyze::createMovie(QVector<QImage> &images)
{
    const int width = 256, height = 256;
    int fps = 15;
    cv::VideoWriter videoWriter;
    int fourcc = cv::VideoWriter::fourcc('m', 'p', '4', 'v');
    videoWriter.open(qPrintable(m_metadata.filename + "_movie.mp4"), fourcc, fps, cv::Size(width, height));
    if (!videoWriter.isOpened()) {
        qDebug() << "Could not open video file for write:" << m_metadata.filename + "_movie.mp4";
        return;
    }
    for (auto &&img : images) {
        img = img.scaled(width, height, Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
        cv::Mat frame(img.height(), img.width(), CV_8UC4,
                      const_cast<uchar*>(img.constBits()),
                      img.bytesPerLine());
        videoWriter.write(frame);
    }
    videoWriter.release();
    //qDebug() << "Finished generating video, saved as" << m_metadata.filename + _movie.mp4";
}

void PlotAndAnalyze::createJpgWithInfo(QImage &image, const double secondsAnalyzed)
{
    if (image.isNull()) {
        qDebug() << "No image for info/save, aborting";
        return;
    }
    image = image.scaled(512, 512, Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
    addLines(image, secondsAnalyzed);
    addText(image, secondsAnalyzed);
    image.save(m_metadata.filename + "_info.jpg");
    emit iqPlotImageReady(m_metadata.filename + "_info.jpg", m_metadata.centerfreq);
    if (m_config->getEmailAddIqPlot() and !m_metadata.fromFile) {
        double delta = (double)m_metadata.maxLoc * (1.0 / m_metadata.samplerate) * (double)m_metadata.samplesInc;
        quint64 startingAt = m_metadata.timestamp * 1e-6 + delta * 1e3;

        m_plotsToSend.prepend(m_metadata.filename + "_info.jpg");
        m_plotsDescription.prepend(//QString::number(m_metadata.centerfreq * 1e-6)
                                   QString::number(secondsAnalyzed * 1e6)
                                   + " μs plot. Max level "
                                   + " at timestamp "
                                   + QDateTime::fromMSecsSinceEpoch(startingAt).toString("hh:mm:ss.zzz. ")
                                   + "IQ recording started at "
                                   + QDateTime::fromMSecsSinceEpoch(m_metadata.timestamp * 1e-6).toString("hh:mm:ss.zzz"));
    }
}

void PlotAndAnalyze::createFilename()
{
    QString dir = m_config->incidentFolder();

    if (m_metadata.fromFile)
        m_metadata.filename = m_config->getLogFolder() + "/fromFile/" + m_metadata.filename;
    else if (m_metadata.centerfreq) {
        m_metadata.filename = dir + "/" + m_config->incidentTimestamp().toString("yyyyMMddhhmmss_")
        + AsciiTranslator::toAscii(m_config->getStationName()) + "_" + QString::number(m_metadata.centerfreq * 1e-6, 'f', 3) + "MHz_"
            + QString::number(m_metadata.samplerate * 1e-6, 'f', 2) + "Msps_bw"
            + QString::number(m_metadata.bandwidth * 1e-3, 'f', 0) + "kHz_"
            + QDateTime::fromMSecsSinceEpoch(m_metadata.timestamp * 1e-6).toString("hhmmss.zzz_")
            + (m_config->getIqSaveAs16bit() ? "16bit" : "8bit");
    }
    else {
        m_metadata.filename = dir + "/" + m_config->incidentTimestamp().toString("yyyyMMddhhmmss_") +
                              AsciiTranslator::toAscii(m_config->getStationName()) + "_" +
                              QString::number(m_startfreq * 1e-3, 'f', 0) +
                              QString::number(m_stopfreq * 1e-3, 'f', 0);
    }
}

void PlotAndAnalyze::addLines(QImage &image, const double secondsAnalyzed)
{
    QPen pen;
    QPainter painter(&image);
    pen.setWidth(1);
    pen.setStyle(Qt::DotLine);
    pen.setColor(QColor(255, 255, 255, 90));
    painter.setPen(pen);
    int height = image.size().height();
    int width = image.size().width();
    int xMinus = 25;

    painter.drawLine(width / 2, 5, width / 2, height);
    int lineDistance = (secondsAnalyzed / 5.0) / ( secondsAnalyzed / (double)image.height() );

    for (int y = 0; y < height; y++) {
        if (y % lineDistance == 0 && y > 0)
            painter.drawLine(0, y, width, y);
    }
    painter.end();
}

void PlotAndAnalyze::addText(QImage &image, const double secondsAnalyzed)
{
    QPen pen;
    QPainter painter(&image);
    pen.setWidth(10);
    pen.setColor(Qt::white);
    int fontSize = 7, yMinus = 15;

    painter.setFont(QFont("Arial", fontSize));
    painter.setPen(pen);
    QString strFr = QString::number(m_metadata.centerfreq * 1e-6, 'f', 3);
    strFr.remove(QRegularExpression("0+$"));
    strFr.remove(QRegularExpression("[\\.,]$")); // Don't show more decimals than needed
    QString strUpper = QString::number(m_metadata.bandwidth / 2e6, 'f', 3);
    strUpper.remove(QRegularExpression("0+$"));
    strUpper.remove(QRegularExpression("[\\.,]$"));
    QString strLower = QString::number(-1 * (m_metadata.bandwidth / 2e6), 'f', 3);
    strLower.remove(QRegularExpression("0+$"));
    strLower.remove(QRegularExpression("[\\.,]$"));

    painter.drawText((image.width() / 2) - 20,
                     yMinus,
                     strFr + " MHz");
    painter.drawText(2, yMinus, strLower + " MHz");
    painter.drawText(image.width() - 40,
                     yMinus,
                     "+ " + strUpper + " MHz");

    int lineDistance = (secondsAnalyzed / 5.0) / ( secondsAnalyzed / (double)image.height() );
    int microsecCtr = 1e6 * secondsAnalyzed / 5;
    for (int y = 0; y < image.height(); y++) {
        if (y % lineDistance == 0 && y > 0) {
            painter.drawText(0, y, QString::number(microsecCtr) + " µs");
            microsecCtr += 1e6 * secondsAnalyzed / 5;
        }
    }
    if (m_metadata.timestamp) {
        double delta = (double)m_metadata.maxLoc *
                       (1.0 / m_metadata.samplerate) *
                       (double)m_metadata.samplesInc;
        quint64 startingAt = secondsAnalyzed * 1e3 +
                             m_metadata.timestamp * 1e-6 + delta * 1e3;

        painter.drawText((image.width() / 2) - 25,
                         image.height() - 5,
                         QDateTime::fromMSecsSinceEpoch(startingAt).toString("hh:mm:ss.zzz"));
    }
    painter.end();
}

void PlotAndAnalyze::createIqDiagram()
{
    int startPlotAt = 0, plotThisManyVals = 25600;
    float maxVal = findMaxMagnitudeAndPosition(m_iq16, startPlotAt); // Starting plot at max magnitude, plotting next plotThisManyVals vals (or as many as is left)

    QCustomPlot *plot = new QCustomPlot;
    plot->addGraph();
    plot->graph(0)->setPen(QPen(Qt::white));
    plot->graph(0)->setLineStyle(QCPGraph::lsNone);
    plot->graph(0)->setScatterStyle(QCPScatterStyle(QCPScatterStyle::ssCircle, 1));
    plot->setFixedSize(512, 512);
    plot->setAutoAddPlottableToLegend(true);
    plot->setBackground(Qt::black);
    plot->xAxis->grid()->setPen(Qt::NoPen);
    plot->xAxis->grid()->setZeroLinePen(QPen(Qt::gray));
    plot->yAxis->grid()->setPen(Qt::NoPen);
    plot->yAxis->grid()->setZeroLinePen(QPen(Qt::gray));
    plot->xAxis->setBasePen(Qt::NoPen);
    plot->yAxis->setBasePen(Qt::NoPen);
    plot->xAxis->setTickLabels(false);
    plot->yAxis->setTickLabels(false);
    plot->xAxis->setTicks(false);
    plot->yAxis->setTicks(false);
    plot->xAxis->setRange(-2e4, 2e4);
    plot->yAxis->setRange(-2e4, 2e4);

    QVector<double> xAxis, yAxis;
    for (int i = startPlotAt; i < startPlotAt + plotThisManyVals; i++) {
        if (i < m_iq16.size()) {
            xAxis.append(m_iq16[i].real);
            yAxis.append(m_iq16[i].imag);
        }
        else break;
    }
    //qDebug() << startPlotAt << m_iq16.size() << maxVal << xAxis.size();
    plot->graph(0)->setData(xAxis, yAxis);
    //plot->rescaleAxes();
    plot->replot();
    plot->saveJpg(m_metadata.filename + "_const.jpg", 512, 512);

    if (m_config->getEmailAddIqPlot() and !m_metadata.fromFile) {
        m_plotsToSend.append(m_metadata.filename + "_const.jpg");
        m_plotsDescription.append("I/Q constellation diagram, 25600 samples plotted from point of maximum signal level");
    }
    delete plot;
}

float PlotAndAnalyze::findMaxMagnitudeAndPosition(const QVector<complexInt16> &iq, int &pos)
{
    float maxVal = -32767;
    int iter = 0;
    for (auto && val : iq) {
        std::complex<float> complexVal(val.real, val.imag);
        if (std::abs(complexVal) > maxVal) {
            maxVal = std::abs(complexVal);
            pos = iter;
        }
        iter++;
    }
    return maxVal;
}

void PlotAndAnalyze::receiveTracedata(TraceDataStruct traceData, QCustomPlot *plot, QVector<qint16> avglevel)
{
    processTraceData(traceData, plot, avglevel);
}

void PlotAndAnalyze::receiveTracedata(quint64 recordingId, TraceDataStruct traceData, QCustomPlot *plot, QVector<qint16> avglevel)
{
    if (!m_captureRequested || recordingId != m_recordingId || m_traceDataReceived)
        return;

    processTraceData(traceData, plot, avglevel);
    m_traceDataReceived = true;
    completeRecordingArtifactsIfReady();
}

void PlotAndAnalyze::processTraceData(TraceDataStruct traceData, QCustomPlot *plot, QVector<qint16> avglevel)
{
    if (traceData.data.size() > 20) {
        int max, min, avg;
        findTracedataMinMaxAvg(traceData.data, min, max, avg);
        qDebug() << "Trace debug data" << min << max << avg;

        if (plot) { // Use scale of plot window if available
            max = plot->yAxis->range().upper * 10;
            min = (plot->yAxis->range().lower + 2) * 10;
        }
        else
            min += 20;

        QImage image(traceData.data.first().size(),
                     traceData.data.size(),
                     QImage::Format_ARGB32);

        const int height = traceData.data.size();
        const int width  = traceData.data.first().size();

        const double range = (max != min) ? double(max - min) : 1.0;
        const double invRange = 1.0 / range;

        for (int srcY = height - 1, y = 0; srcY > 0; --srcY, ++y)
        {
            QRgb* line = reinterpret_cast<QRgb*>(image.scanLine(y));
            const auto& row = traceData.data[srcY];

            for (int x = 0; x < width; ++x)
            {
                double p = (row[x] - min) * invRange;
                //p = pow(p, 0.85);
                if (p < 0.0) p = 0.0;
                else if (p > 1.0) p = 1.0;
                int t = 255 - (p * 255);
                line[x] = qRgba(t, t, t, 255);
            }
        }

        if (plot) {
            m_mutex.lock();
            plot->layer("triggerLayer")->setVisible(false);
            plot->layer("liveGraph")->setVisible(false);
            bool flagOverlayOn = plot->layer("overlayLayer")->visible();
            plot->layer("overlayLayer")->setVisible(false);
            plot->yAxis->setVisible(false);
            plot->replot();

            plot->axisRect()->setBackground(QPixmap::fromImage(image), true, Qt::IgnoreAspectRatio);
            plot->saveJpg(m_metadata.filename + "_spectrogram.jpg", 768, 400);

            plot->axisRect()->setBackground(QPixmap());
            plot->layer("triggerLayer")->setVisible(true);
            plot->layer("liveGraph")->setVisible(true);
            plot->layer("overlayLayer")->setVisible(flagOverlayOn);
            plot->yAxis->setVisible(true);
            plot->replot();

            m_mutex.unlock();

            m_plotsToSend.append(m_metadata.filename + "_spectrogram.jpg");
            m_plotsDescription.append("Spectrogram from " +
                                      traceData.timestamp.last().toString("hh:mm:ss") +
                                      " to " +
                                      traceData.timestamp.first().toString("hh:mm:ss"));
        }
        else {
            image.save(m_config->getLogFolder() + "/file_spectrogram.jpg");
        }
    }
}

void PlotAndAnalyze::recordingContextStarted(quint64 recordingId, const QDateTime &, const QString &, bool includeAnalysisArtifacts)
{
    m_recordingId = recordingId;
    m_collectNotificationArtifacts = includeAnalysisArtifacts;
    m_captureRequested = false;
    m_tracePlotReceived = false;
    m_waterfallReceived = false;
    m_traceDataReceived = false;
    m_gnssSpectrumSnapshotsReceived = false;
    m_plotsToSend.clear();
    m_plotsDescription.clear();

    if (!includeAnalysisArtifacts)
        return;

    if (!m_config->getIqCreateFftPlot())
        createFilename();

    int traceplotTimeout = (m_config->getPlotMaxholdTime() * 1e3) - 5e3;
    if (traceplotTimeout > 45e3) traceplotTimeout = 45e3;
    m_reqTraceplotTimer->start(traceplotTimeout);
}

void PlotAndAnalyze::recordingContextEnded(quint64 recordingId)
{
    if (recordingId == m_recordingId && m_collectNotificationArtifacts && !m_captureRequested)
        requestRecordingArtifacts(recordingId);
}

void PlotAndAnalyze::requestRecordingArtifacts(quint64 recordingId)
{
    if (!m_collectNotificationArtifacts || recordingId != m_recordingId || m_captureRequested)
        return;

    m_captureRequested = true;
    m_reqTraceplotTimer->stop();
    emit reqTracedata(recordingId);
    emit reqMaxholdData();
    emit reqTracePlot(recordingId);
    emit reqWaterfallSnapshot(recordingId);
    emit reqGnssBacklogs(recordingId);
    emit reqGnssSpectrumSnapshots(recordingId);
}

void PlotAndAnalyze::receiveGnssSpectrumSnapshotsReady(quint64 recordingId)
{
    if (!m_captureRequested || recordingId != m_recordingId || m_gnssSpectrumSnapshotsReceived)
        return;

    m_gnssSpectrumSnapshotsReceived = true;
    completeRecordingArtifactsIfReady();
}

void PlotAndAnalyze::completeRecordingArtifactsIfReady()
{
    if (!m_captureRequested || !m_tracePlotReceived || !m_waterfallReceived || !m_traceDataReceived ||
        !m_gnssSpectrumSnapshotsReceived)
        return;

    for (int i = 0; i < m_plotsToSend.size(); ++i)
        emit recordingArtifactReady(m_recordingId, m_plotsToSend[i], m_plotsDescription[i]);

    m_plotsToSend.clear();
    m_plotsDescription.clear();
    m_collectNotificationArtifacts = false;
    emit recordingPlotsReady(m_recordingId);
}

void PlotAndAnalyze::findTracedataMinMaxAvg(const QVector<QVector<qint16>> &data, int &min, int &max, int &avg)
{
    min = 32767;
    max = -32767;
    qint64 sum = 0;

    for (auto && line : data) {
        for (auto && val : line) {
            if (val < min) min = val;
            else if (val > max) max = val;
            sum += val;
        }
    }
    avg = sum / ( data.size() * data.first().size() );
}

void PlotAndAnalyze::receivePlot(quint64 recordingId, QPixmap *pix, QDateTime timestamp)
{
    if (!m_captureRequested || recordingId != m_recordingId || m_tracePlotReceived)
        return;

    m_tracePlotReceived = true;
    if (!pix || pix->isNull()) {
        completeRecordingArtifactsIfReady();
        return;
    }

    QPixmap img = pix->scaled(768, 400, Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
    const QString filename = m_metadata.filename + "_trace_maxhold.jpg";
    if (img.save(filename)) {
        m_plotsToSend.prepend(filename);
        m_plotsDescription.prepend("Maxhold and current signal levels at " +
                               timestamp.toString("hh:mm:ss, ") +
                               "maxhold time " +
                               QString::number(m_config->getPlotMaxholdTime()) +
                               " seconds"
                               );
    }
    completeRecordingArtifactsIfReady();
}

void PlotAndAnalyze::receiveWaterfallSnapshot(quint64 recordingId, const QImage &image)
{
    if (!m_captureRequested || recordingId != m_recordingId || m_waterfallReceived)
        return;

    m_waterfallReceived = true;
    if (!image.isNull())
        emit recordingImageReady(recordingId, image, "Waterfall at incident capture time");
    completeRecordingArtifactsIfReady();

}

void PlotAndAnalyze::updFrequencies(quint64 a, quint64 b)
{
    m_startfreq = a; m_stopfreq = b;

}

double mean(const QVector<int>& v)
{
    if (v.isEmpty()) return 0.0;

    double sum = 0.0;
    for (double x : v)
        sum += x;

    return sum / v.size();
}

double stddev(const QVector<int>& v)
{
    if (v.isEmpty()) return 0.0;

    double m = mean(v);

    double accum = 0.0;
    for (double x : v)
        accum += (x - m) * (x - m);

    return std::sqrt(accum / v.size());
}

void PlotAndAnalyze::calcPeriodAndDensity(const QVector<QVector<double>> &data, int from, int to)
{
    if (from >= to || from < 0 || to < 0 || to > data.size() || from > data.size()) return;
    m_metadata.periodTime = estimateSweepPeriodFromIq(from, to);
    from -= 1500; // Rewind a little from maxLoc
    if (from < 0) from = 0;

    double squelch = m_metadata.max;
    int binsAboveSquelch = 0;

    binsAboveSquelch = 0;
    squelch = m_metadata.max - 1;

    for (int row = from; row < to; row++) { // Here calc. spectral density
        for (auto && val : data[row]) {
            if (val > squelch) {
                binsAboveSquelch++;
            }
        }
    }
    m_metadata.spectral = 1e3 * (double)binsAboveSquelch / ((to - from) * data.first().size());
    //qDebug() << "PRF period/spec" << m_metadata.periodTime << m_metadata.spectral;
}

double PlotAndAnalyze::estimateSweepPeriodFromIq(int from, int to) const
{
    const auto finish = [this](double period, const QString &reason) {
        qDebug() << "IQ sweep period estimate:" << period * 1e6 << "us;" << reason
                 << "center frequency:" << m_iqMetadata.centerfreq
                 << "sample rate:" << m_iqMetadata.samplerate
                 << "IQ samples:" << m_iq16.size();
        return period;
    };

    if (m_iq16.isEmpty() || m_iqMetadata.samplerate == 0 || m_iqMetadata.samplesInc <= 0
        || m_iqMetadata.fftSize < 8 || m_iqMetadata.samplerate != m_metadata.samplerate
        || m_iqMetadata.centerfreq != m_metadata.centerfreq
        || m_iqMetadata.timestamp != m_metadata.timestamp) {
        return finish(0, "invalid or mismatched IQ capture");
    }

    const int windowSize = qMin(16, m_iqMetadata.fftSize);
    const int hopSize = m_iqMetadata.samplesInc;
    const int firstRow = qMax(0, from - 1500);
    const int lastRow = qMin(to, static_cast<int>(m_iq16.size() / hopSize));
    const qint64 firstSample = static_cast<qint64>(firstRow) * hopSize;
    const qint64 lastSample = qMin(static_cast<qint64>(lastRow) * hopSize,
                                   static_cast<qint64>(m_iq16.size()));
    if (lastSample - firstSample < windowSize)
        return finish(0, "insufficient samples for one analysis window");

    const int frameCount = 1 + static_cast<int>((lastSample - firstSample - windowSize) / hopSize);
    if (frameCount < 12)
        return finish(0, "too few analysis frames");

    QVector<double> spectra(frameCount * windowSize);
    double *fftInput = static_cast<double *>(fftw_malloc(sizeof(double) * windowSize * 2));
    fftw_complex *fftOutput = fftw_alloc_complex(windowSize);
    if (!fftInput || !fftOutput) {
        fftw_free(fftInput);
        fftw_free(fftOutput);
        return finish(0, "FFT buffer allocation failed");
    }

    fftw_plan fftPlan = fftw_plan_dft_1d(windowSize,
                                         reinterpret_cast<fftw_complex *>(fftInput),
                                         fftOutput, FFTW_FORWARD, FFTW_ESTIMATE);
    if (!fftPlan) {
        fftw_free(fftInput);
        fftw_free(fftOutput);
        return finish(0, "FFT plan creation failed");
    }

    for (int frame = 0; frame < frameCount; ++frame) {
        const qint64 frameStart = firstSample + static_cast<qint64>(frame) * hopSize;
        for (int sample = 0; sample < windowSize; ++sample) {
            const double window = 0.5 * (1.0 - std::cos(2.0 * M_PI * sample / (windowSize - 1)));
            const complexInt16 &iq = m_iq16[static_cast<qsizetype>(frameStart + sample)];
            fftInput[2 * sample] = window * iq.real;
            fftInput[2 * sample + 1] = window * iq.imag;
        }
        fftw_execute(fftPlan);
        for (int bin = 0; bin < windowSize; ++bin) {
            const double real = fftOutput[bin][0];
            const double imag = fftOutput[bin][1];
            spectra[frame * windowSize + bin] = real * real + imag * imag;
        }
    }

    fftw_destroy_plan(fftPlan);
    fftw_free(fftInput);
    fftw_free(fftOutput);

    int correlationSize = 1;
    while (correlationSize < 2 * frameCount)
        correlationSize <<= 1;

    double *correlationInput = static_cast<double *>(fftw_malloc(sizeof(double) * correlationSize));
    double *correlationOutput = static_cast<double *>(fftw_malloc(sizeof(double) * correlationSize));
    fftw_complex *correlationSpectrum = fftw_alloc_complex(correlationSize / 2 + 1);
    if (!correlationInput || !correlationOutput || !correlationSpectrum) {
        fftw_free(correlationInput);
        fftw_free(correlationOutput);
        fftw_free(correlationSpectrum);
        return finish(0, "autocorrelation buffer allocation failed");
    }

    fftw_plan forwardPlan = fftw_plan_dft_r2c_1d(correlationSize, correlationInput,
                                                  correlationSpectrum, FFTW_ESTIMATE);
    fftw_plan inversePlan = fftw_plan_dft_c2r_1d(correlationSize, correlationSpectrum,
                                                  correlationOutput, FFTW_ESTIMATE);
    if (!forwardPlan || !inversePlan) {
        if (forwardPlan) fftw_destroy_plan(forwardPlan);
        if (inversePlan) fftw_destroy_plan(inversePlan);
        fftw_free(correlationInput);
        fftw_free(correlationOutput);
        fftw_free(correlationSpectrum);
        return finish(0, "autocorrelation plan creation failed");
    }

    QVector<double> autocorrelation(frameCount, 0.0);
    for (int bin = 0; bin < windowSize; ++bin) {
        double meanPower = 0;
        for (int frame = 0; frame < frameCount; ++frame)
            meanPower += spectra[frame * windowSize + bin];
        meanPower /= frameCount;

        for (int frame = 0; frame < frameCount; ++frame)
            correlationInput[frame] = spectra[frame * windowSize + bin] - meanPower;
        std::fill(correlationInput + frameCount, correlationInput + correlationSize, 0.0);

        fftw_execute(forwardPlan);
        for (int i = 0; i <= correlationSize / 2; ++i) {
            const double real = correlationSpectrum[i][0];
            const double imag = correlationSpectrum[i][1];
            correlationSpectrum[i][0] = real * real + imag * imag;
            correlationSpectrum[i][1] = 0;
        }
        fftw_execute(inversePlan);
        for (int lag = 0; lag < frameCount; ++lag)
            autocorrelation[lag] += correlationOutput[lag] / correlationSize;
    }

    fftw_destroy_plan(forwardPlan);
    fftw_destroy_plan(inversePlan);
    fftw_free(correlationInput);
    fftw_free(correlationOutput);
    fftw_free(correlationSpectrum);

    const double correlationAtZero = autocorrelation[0];
    if (correlationAtZero <= 0)
        return finish(0, "no spectral variation to correlate");
    for (double &value : autocorrelation)
        value /= correlationAtZero;

    const int minLag = 2;
    const int maxLag = frameCount / 3;
    if (maxLag <= minLag + 1)
        return finish(0, "capture is too short for a reliable period");

    QVector<double> background;
    background.reserve(maxLag - minLag + 1);
    for (int lag = minLag; lag <= maxLag; ++lag)
        background.append(autocorrelation[lag]);
    std::sort(background.begin(), background.end());
    const double median = background[background.size() / 2];
    QVector<double> deviations;
    deviations.reserve(background.size());
    for (double value : background)
        deviations.append(qAbs(value - median));
    std::sort(deviations.begin(), deviations.end());
    const double mad = deviations[deviations.size() / 2];
    const double minimumPeak = qMax(0.12, median + 6.0 * 1.4826 * mad);

    QVector<int> peakLags;
    for (int lag = minLag + 1; lag < maxLag; ++lag) {
        const double value = autocorrelation[lag];
        if (value >= autocorrelation[lag - 1] && value >= autocorrelation[lag + 1]
            && value > minimumPeak)
            peakLags.append(lag);
    }
    if (peakLags.isEmpty())
        return finish(0, "no significant periodic peak");

    int bestLag = 0;
    int bestHarmonicCount = 0;
    double bestPeak = 0;
    QVector<int> supportedHarmonics;
    for (int candidateLag : peakLags) {
        QVector<int> harmonics{1};
        for (int harmonic = 2; harmonic <= 8; ++harmonic) {
            const int expectedLag = candidateLag * harmonic;
            if (expectedLag >= maxLag)
                break;

            const int tolerance = qMax(1, qCeil(expectedLag * 0.025));
            const auto matchingPeak = std::lower_bound(peakLags.cbegin(), peakLags.cend(),
                                                       expectedLag - tolerance);
            if (matchingPeak != peakLags.cend() && *matchingPeak <= expectedLag + tolerance)
                harmonics.append(harmonic);
        }

        if (harmonics.size() > bestHarmonicCount
            || (harmonics.size() == bestHarmonicCount && candidateLag < bestLag)) {
            bestLag = candidateLag;
            bestHarmonicCount = harmonics.size();
            bestPeak = autocorrelation[candidateLag];
            supportedHarmonics = harmonics;
        }
    }
    if (bestHarmonicCount < 4)
        return finish(0, "no peak has sufficient harmonic support");

    const auto interpolatedCorrelation = [&autocorrelation, frameCount](double lag) {
        if (lag < 0 || lag >= frameCount - 1)
            return 0.0;
        const int lowerLag = static_cast<int>(lag);
        const double fraction = lag - lowerLag;
        return autocorrelation[lowerLag] * (1.0 - fraction)
               + autocorrelation[lowerLag + 1] * fraction;
    };

    double refinedLag = bestLag;
    double bestLadderScore = -std::numeric_limits<double>::infinity();
    for (int step = -50; step <= 50; ++step) {
        const double candidateLag = bestLag + step * 0.01;
        double score = 0;
        for (int harmonic : supportedHarmonics)
            score += interpolatedCorrelation(candidateLag * harmonic) / std::sqrt(harmonic);
        if (score > bestLadderScore) {
            bestLadderScore = score;
            refinedLag = candidateLag;
        }
    }

    const double period = refinedLag * hopSize / m_iqMetadata.samplerate;
    const QString peakDetails = QStringLiteral("fundamental peak; lag %1 frames; correlation %2; threshold %3; harmonics %4; analyzed frames %5")
                                    .arg(refinedLag, 0, 'f', 2)
                                    .arg(bestPeak, 0, 'f', 3)
                                    .arg(minimumPeak, 0, 'f', 3)
                                    .arg(bestHarmonicCount)
                                    .arg(frameCount);
    return finish(period, peakDetail}

void PlotAndAnalyze::writeMetaToDisk(cv::Mat results, QStringList classes)
{
    QJsonObject root;
    QJsonObject metadata;
    QJsonArray classification;

    root["program"] = "Hauken";
    root["version"] = QString(PROJECT_VERSION);
    root["fileCreated"] = AppClock::nowUtc().toString(Qt::ISODateWithMs);

    metadata["ffmCenterFreq"]  = QString::number(m_metadata.centerfreq);
    metadata["ffmBandwidth"]   = QString::number(m_metadata.bandwidth);
    metadata["samplerate"]     = QString::number(m_metadata.samplerate);
    metadata["sampleIncrement"]= QString::number(m_metadata.samplesInc);
    metadata["fftBinSize"]     = QString::number(m_metadata.fftSize);
    metadata["maxValue"]       = QString::number(m_metadata.max);
    metadata["minValue"]       = QString::number(m_metadata.min);
    metadata["avgValue"]       = QString::number(m_metadata.avg);
    metadata["trigFrequency"]  = QString::number(m_metadata.trigFrequency * 1e6, 'f', 0);
    metadata["period"]         = QString::number(m_metadata.periodTime);
    metadata["spectral"]       = QString::number(m_metadata.spectral);
    metadata["iqFirstTimestamp"] = QString::number(m_metadata.timestamp);
    metadata["startFreq"]      = QString::number(m_startfreq);
    metadata["stopFreq"]       = QString::number(m_stopfreq);
    metadata["resolution"]     = QString::number(m_resolution);

    for (int row = 0; row < results.rows; row++) {
        QJsonArray array;
        cv::Mat output = results.row(row);
        std::vector<float> probs;
        output.reshape(1, 1).copyTo(probs);
        QString res;
        for (int i = 0; i < probs.size(); i++)
            array.append(classes[i] + ": " + QString::number(probs[i], 'f', 2));
        classification.append(array);
    }

    root["metadata"] = metadata;
    root["classification"] = classification;

    QJsonDocument doc(root);

    if (!m_metadata.filename.isEmpty()) {
        QFile file(m_metadata.filename + ".json");
        if (!file.open(QIODevice::WriteOnly)) {
            qWarning() << "Couldn't write metadata to file" << file.fileName();
            return;
        }
        file.write(doc.toJson(QJsonDocument::Indented));
        file.close();
    }
}

void PlotAndAnalyze::readFile(QString filename)
{
    QFile file (filename);
    if (!file.open(QIODevice::ReadOnly)) {
        qWarning() << "PlotAndAnalyze: Could not open" << filename << "for reading";
        return;
    }
    QTextStream ts(&file);
    TraceDataStruct traceData;
    bool isMobile = false;

    while (!ts.atEnd()) {
        QString line = file.readLine();
        if (line.contains("MobileData")) isMobile = true;
        QStringList split = line.split(',');
        if (split.size() > 15) {
            QDateTime dt = QDateTime::fromString(split[0], "yyyy-MM-dd hh:mm:ss.zzz");
            if (dt.isValid()) {
                traceData.timestamp.append(dt);
                split.removeFirst();
                if (isMobile) {
                    split.remove(0, 2); // Remove pos data, don't need it
                }
                QVector<qint16> data;
                for (auto && val : split) {
                    data.append(val.toInt() * 10);
                }
                traceData.data.append(data);
            }
        }
    }
    if (!traceData.data.isEmpty())
        receiveTracedata(traceData);
    file.close();
}

void PlotAndAnalyze::findFreqsAboveAvgLevel(const QVector<double> maxholdData,
                                            const QVector<double> avgData,
                                            double startfreq,
                                            double stopfreq)
{
    if (maxholdData.size() != avgData.size()) {
        qDebug() << "Trace data and average level data mismatch, something very wrong here";
        return;
    }

    if (maxholdData.size() < 2 || stopfreq <= startfreq) {
        qDebug() << "Trace data frequency span is invalid, cannot calculate signal bandwidth"
                 << maxholdData.size() << startfreq << stopfreq;
        return;
    }

    struct SignalSpan {
        double startMHz = 0;
        double stopMHz = 0;
    };

    const double configuredTrigLevel = m_config->getInstrTrigLevel();
    const double triglevel = configuredTrigLevel < 5.0 ? configuredTrigLevel + 3.0 : configuredTrigLevel;
    const double res = ((stopfreq - startfreq) / (maxholdData.size() - 1));
    const double l1StartMHz = (GPSL1 - 512e3) * 1e-6;
    const double l1StopMHz = (GPSL1 + 512e3) * 1e-6;
    const bool triggerAtL1Center = qAbs(static_cast<double>(m_metadata.trigFrequency) - GPSL1) <= 5e3;
    const double trigFrequencyMHz = m_metadata.trigFrequency > 1e6
            ? m_metadata.trigFrequency * 1e-6
            : m_metadata.trigFrequency;

    QVector<SignalSpan> signalSpans;
    int startIndex = -1;

    auto appendSignalSpan = [&](int endIndex) {
        if (startIndex < 0 || endIndex < startIndex)
            return;

        SignalSpan span;
        span.startMHz = qMax(startfreq, startfreq + res * startIndex - res / 2.0);
        span.stopMHz = qMin(stopfreq, startfreq + res * endIndex + res / 2.0);

        if (span.stopMHz > span.startMHz)
            signalSpans.append(span);

        startIndex = -1;
    };

    for (int i = 0; i < maxholdData.size(); i++) {
        const bool aboveTrigLevel = maxholdData.at(i) > avgData.at(i) + triglevel;

        if (aboveTrigLevel) {
            if (startIndex < 0)
                startIndex = i;
        }
        else {
            appendSignalSpan(i - 1);
        }
    }
    appendSignalSpan(maxholdData.size() - 1);

    if (signalSpans.isEmpty()) {
        qDebug() << "No maxhold signal levels found above average + trigger level";
        return;
    }

    int selectedIndex = -1;
    double widestSpanMHz = -1;

    for (int i = 0; i < signalSpans.size(); i++) {
        const SignalSpan &span = signalSpans.at(i);
        const double widthMHz = span.stopMHz - span.startMHz;

        if (trigFrequencyMHz >= span.startMHz && trigFrequencyMHz <= span.stopMHz) {
            selectedIndex = i;
            break;
        }

        if (widthMHz > widestSpanMHz) {
            widestSpanMHz = widthMHz;
            selectedIndex = i;
        }
    }

    const SignalSpan &selectedSpan = signalSpans.at(selectedIndex);
    const double signalWidthMHz = selectedSpan.stopMHz - selectedSpan.startMHz;
    const double centerMHz = (selectedSpan.startMHz + selectedSpan.stopMHz) / 2.0;
    bool signalOverlapsL1 = false;
    for (const SignalSpan &span : signalSpans) {
        if (span.startMHz <= l1StopMHz && span.stopMHz >= l1StartMHz) {
            signalOverlapsL1 = true;
            break;
        }
    }

    QString text;
    QTextStream ts(&text);
    ts << "Maxhold signal bandwidth estimate: "
       << QString::number(signalWidthMHz, 'f', 3) << " MHz"
       << " at center frequency " << QString::number(centerMHz, 'f', 6) << " MHz"
       << " (" << QString::number(selectedSpan.startMHz, 'f', 6)
       << "-" << QString::number(selectedSpan.stopMHz, 'f', 6) << " MHz).";

    if (signalSpans.size() > 1)
        ts << " " << signalSpans.size() << " separate signals above threshold were found; reporting the "
           << (trigFrequencyMHz >= selectedSpan.startMHz && trigFrequencyMHz <= selectedSpan.stopMHz
               ? "one containing the trigger frequency"
               : "widest one")
           << ".";

    if (signalOverlapsL1)
        ts << " Signal overlaps L1 center frequency +/- 512 kHz.";

    if (!m_collectNotificationArtifacts || m_freqNotificationSentForIncident)
        return;

    m_freqNotificationSentForIncident = true;
    emit toIncidentLog(NOTIFY::TYPE::AI, "", text);

    if (signalOverlapsL1 && triggerAtL1Center &&
        classificationHasRfiForRange(l1StartMHz, l1StopMHz))
        emit reportIntentional("Signal within L1 band, could be harmful");
}
