#include "plotandanalyze.h"
#include "asciitranslator.h"
#include "gif.h"
#include "opencv2/opencv.hpp"
#include "version.h"
#include <complex>

PlotAndAnalyze::PlotAndAnalyze(QSharedPointer<Config> c)
{
    m_config = c;
}

void PlotAndAnalyze::start()
{
    m_reqTracedataTimer = new QTimer;
    m_sendPlotsTimer = new QTimer;
    m_reqTraceplotTimer = new QTimer;
    m_reqTracedataTimer->setSingleShot(true);
    m_sendPlotsTimer->setSingleShot(true);
    m_reqTraceplotTimer->setSingleShot(true);

    connect(m_reqTracedataTimer, &QTimer::timeout, this, [this] () {
        emit reqTracedata();
    });

    connect(m_sendPlotsTimer, &QTimer::timeout, this, [this] () {
        for (int i = 0; i < m_plotsToSend.size(); i++) {
            emit imageReady(m_plotsToSend[i], m_plotsDescription[i]);
        }
        m_plotsToSend.clear();
        m_plotsDescription.clear();
    });

    connect(m_reqTraceplotTimer, &QTimer::timeout, this, [this] () {
        emit reqTracePlot();
    });
}

void PlotAndAnalyze::end()
{
    delete m_reqTraceplotTimer;
    delete m_sendPlotsTimer;
    delete m_reqTracedataTimer;
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

    if (!images.isEmpty()) {
        emit imagesReadyForClassification(images, m_metadata); // Send copy of metadata to allow async/multi thread op
        for (int i=0; i<images.size(); i++) {
            QImage img = images[i].scaled(256, 256, Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
            img.save("c:/hauken/test" + QString::number(i) + ".png");
        }
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
            double delta = (double)m_metadata.imageStartAt * (1.0 / m_metadata.samplerate) * (double)m_metadata.samplesInc;
            quint64 startingAt = m_metadata.timestamp * 1e-6 + delta * 1e3;

            m_plotsDescription.append("FFT animation from "
                                      + QDateTime::fromMSecsSinceEpoch(startingAt).toString("hh:mm:ss.zzz")
                                      + ", 500 us per image, total time covered "
                                      + QString::number(500 * images.size()) + " us");

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
    m_metadata = meta; // To ensure metadata follows correct samples in case of async/multi thread
    QVector<float> rfiResults(allResults.cols, 0);
    QVector<int> imgsWithRfi, imgsWithoutRfi;
    QString text;
    QTextStream ts(&text);
    bool flagReport = false, foundClassifications = false;

    // Look for images with classification other than !rfi
    for (int row = 0; row < allResults.rows; row++) {
        cv::Mat output = allResults.row(row);
        std::vector<float> probs;
        output.reshape(1, 1).copyTo(probs);
        for (int classRes = 0; classRes < probs.size(); classRes++) {
            if (classes[classRes].contains("rfi") and probs[classRes] >= 0.9) {
                imgsWithRfi.append(row);
                break;
            }
            else if (classes[classRes].contains("rfi") and probs[classRes] < 0.9) {
                imgsWithoutRfi.append(row);
                break;
            }
        }
    }

    ts << "Center frequency " << QString::number(m_metadata.centerfreq * 1e-6, 'f', 1) << " MHz. ";

    if (imgsWithRfi.isEmpty()) { // If any images contains RFI, continue. If not, false alarm
        ts << "Classification: No RFI present. ";

        for (auto && row : imgsWithoutRfi) { // Go through every result to find what is in each
            cv::Mat output = allResults.row(row);
            std::vector<float> probs;
            output.reshape(1, 1).copyTo(probs);
            for (int i = 0; i < probs.size(); i++) {
                if (probs[i] > 0.9) //
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
        for (auto && row : imgsWithRfi) { // Go through every result to find what is in each
            cv::Mat output = allResults.row(row);
            std::vector<float> probs;
            output.reshape(1, 1).copyTo(probs);
            for (int i = 0; i < probs.size(); i++) {
                if (probs[i] > 0.9) //
                    rfiResults[i]++;
            }
        }

        if (m_metadata.trigFrequency) { // Second criteria, is this within +- 0.5 MHz of L1 center? TODO - check other bands?
            if (abs(m_metadata.trigFrequency - GPSL1) < 0.5e6) {
                ts << "Signal within L1 center frequency: " << m_metadata.trigFrequency * 1e-6 << " MHz. ";
                flagReport = true;
            }
            /*else {
                ts << "Signal outside L1 center frequency. ";
            }*/
        }

        for (int i = 0; i < rfiResults.size() - 1; i++) {
            if (rfiResults[i]) { // > 0
                foundClassifications = true;
                if (classes[i].contains("sweep", Qt::CaseInsensitive)) { // Sweep classification, check freq center and PRF
                    ts << "Classification: Sweep ("
                       << rfiResults[i] << " of "
                       << allResults.rows << " images), PRF/period estimate ";
                    if (m_metadata.periodTime > 0) {
                        ts << QString::number(1.0 / m_metadata.periodTime, 'f', 0)
                        << " / "
                        << QString::number(1e6 * m_metadata.periodTime, 'f', 1)  << " µs. ";
                    }
                    else {
                        ts << "inconclusive. ";
                    }
                    if (m_metadata.periodTime > 1e-4 && abs(m_metadata.trigFrequency - GPSL2) < 10e6)
                        ts << "Looks like a radar sweep within L2 band. ";
                    else if (abs(m_metadata.trigFrequency - 1.274e9) < 2e6) { // Radar hack
                        ts << "Freq. within air radar freq. (1274 MHz)";
                    }
                    else if (m_metadata.periodTime > 1e-4) {
                        ts << "Looks like a radar sweep. ";
                    }
                    else if (m_metadata.periodTime <= 0 && abs(m_metadata.trigFrequency - GPSL2) < 10e6) {
                        ts << "Within L2 band, suggests it is a radar sweep. ";
                    }
                    else if (m_metadata.periodTime > 0 && m_metadata.periodTime <= 1e-4) {
                        ts << "PRF suggests it could be a jammer. ";
                        if (m_metadata.spectral > 0.15) {
                            ts << "Spectral density " << QString::number(m_metadata.spectral, 'f', 2) << " indicates the same. ";
                        }
                        else {
                            ts << "Spectral density low. ";
                        }
                        flagReport = true;
                    }

                }
                else if (classes[i].contains("prn", Qt::CaseInsensitive)) {
                    ts << "Classification: PRN signal ("
                       << rfiResults[i] << " of "
                       << allResults.rows << " images). ";
                    ;
                    flagReport = true;
                }
                else if (classes[i].contains("pulse", Qt::CaseInsensitive)) {
                    ts << "Classification: Pulsed signal ("
                       << rfiResults[i] << " of "
                       << allResults.rows << " images). ";
                    ;
                    if (!flagReport && m_metadata.periodTime > 0) // Info not added by sweep already
                        ts << "Repetition rate/period estimate: "
                           << QString::number(1.0 / m_metadata.periodTime, 'f', 0)
                           << " / "
                           << QString::number(1e6 * m_metadata.periodTime, 'f', 1)  << " µs. ";
                }
                else if (classes[i].contains("nb", Qt::CaseInsensitive)) {
                    ts << "Classification: Narrowband ("
                       << rfiResults[i] << " of "
                       << allResults.rows << " images). ";
                }
                else if (classes[i].contains("partial", Qt::CaseInsensitive)) {
                    ts << "Classification: Partial band ("
                       << rfiResults[i] << " of "
                       << allResults.rows << " images). ";
                }
                else if (classes[i].contains("full", Qt::CaseInsensitive)) {
                    ts << "Classification: Full band ("
                       << rfiResults[i] << " of "
                       << allResults.rows << " images). ";
                }
            }
        }
        if (!foundClassifications)
            ts << "Classification inconclusive. ";

        if (flagReport) emit reportIntentional(text);

        emit analyzerResult(text, 100);
    }

    if (m_metadata.fromFile)
        emit toIncidentLog(NOTIFY::TYPE::AIDONTNOTIFY, "", text);
    else
        emit toIncidentLog(NOTIFY::TYPE::AI, "", text);

    if (!m_metadata.fromFile) writeMetaToDisk(allResults, classes);
}

void PlotAndAnalyze::createGif(QVector<QImage> &images)
{
    if (images.size() > 0) {
        const int width = 256, height = 256;

        GifWriter gifWriter;
        int delay = 40; // ms

        if (!GifBegin(&gifWriter, qPrintable(m_metadata.filename + ".gif"), width, height, delay)) {
            qWarning() << "Cannot write to gif file:" << m_metadata.filename + ".gif";
            return;
        }
        for (auto &&img : images) {
            QImage converted = img.convertToFormat(QImage::Format_RGBA8888);
            converted = converted.scaled(width, height, Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
            if (!converted.isNull())
                GifWriteFrame(&gifWriter, converted.bits(), converted.width(), converted.height(), delay);
        }
        GifEnd(&gifWriter);
        if (m_config->getEmailAddGif() and !m_metadata.fromFile)
            m_plotsToSend.append(m_metadata.filename + ".gif");
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
    if (m_config->getEmailAddIqPlot() and !m_metadata.fromFile) {
        double delta = (double)m_metadata.maxLoc * (1.0 / m_metadata.samplerate) * (double)m_metadata.samplesInc;
        quint64 startingAt = m_metadata.timestamp * 1e-6 + delta * 1e3;

        m_plotsToSend.prepend(m_metadata.filename + "_info.jpg");
        m_plotsDescription.prepend("Single plot, "
                                   + QString::number(secondsAnalyzed * 1e6)
                                   + " us long. Max level "
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

void PlotAndAnalyze::receiveTracedata(TraceDataStruct traceData, QCustomPlot *plot)
{
    if (traceData.data.size() > 20) {
        int max, min, avg;
        findTracedataMinMaxAvg(traceData.data, min, max, avg);
        //qDebug() << "Trace debug data" << min << max << avg << "range" << max - min;
        if (max - min < 400) max += 400 - (max - min);
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
        image = image.scaled(768, 512);

        m_mutex.lock();
        plot->layer("triggerLayer")->setVisible(false);
        plot->layer("liveGraph")->setVisible(false);
        bool flagOverlayOn = plot->layer("overlayLayer")->visible();
        plot->layer("overlayLayer")->setVisible(false);
        plot->yAxis->setVisible(false);
        plot->replot();

        plot->axisRect()->setBackground(QPixmap::fromImage(image), true);
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
}

void PlotAndAnalyze::recordingState()
{
    if (!m_config->getIqCreateFftPlot())
        createFilename();

    int traceplotTimeout = (m_config->getPlotMaxholdTime() * 1e3) - 5e3;
    int tracedataTimeout = 3e4;

    m_reqTracedataTimer->start(tracedataTimeout);
    m_reqTraceplotTimer->start(traceplotTimeout);

    if (traceplotTimeout > tracedataTimeout) m_sendPlotsTimer->start(traceplotTimeout + 10e3);
    else m_sendPlotsTimer->start(tracedataTimeout + 10e3);

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

void PlotAndAnalyze::receivePlot(QPixmap *pix, QDateTime timestamp)
{
    QPixmap img = pix->scaled(768, 400, Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
    img.save(m_metadata.filename + "_trace_maxhold.jpg");
    m_plotsToSend.prepend(m_metadata.filename + "_trace_maxhold.jpg");
    m_plotsDescription.prepend("Maxhold and current signal levels at " +
                               timestamp.toString("hh:mm:ss, ") +
                               "maxhold time " +
                               QString::number(m_config->getPlotMaxholdTime()) +
                               " seconds"
                               );

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
    from -= 1500; // Rewind a little from maxLoc
    if (from < 0) from = 0;

    QVector<int> firstBinAboveSquelchArray;
    double squelch = m_metadata.max;
    double timePerLine = (1.0 / m_metadata.samplerate) * m_metadata.samplesInc;
    int avgFirstBin = 0, prevBinValue;

    for (int i = 0; i < 200; i++) { // Try to locate optimal squelch level
        squelch -= 0.2;
        firstBinAboveSquelchArray.clear();
        for (int row = from; row < to; row++) { // Locate first bin above squelch
            int firstBin = 0;
            for (int bin = 0; bin < data[row].size(); bin++) {
                if (data[row][bin] > squelch) {
                    firstBinAboveSquelchArray.append(bin);
                    break;
                }
            }
        }
        prevBinValue = avgFirstBin;
        avgFirstBin = 0;
        for (auto && val : firstBinAboveSquelchArray) avgFirstBin += val;
        double avg = 0.5 + ((double)avgFirstBin / firstBinAboveSquelchArray.size());
        avgFirstBin = avg;

        //qDebug() << "Bin locator: Found" << firstBinAboveSquelchArray.size() << "first bins qualifying, avg" << avgFirstBin << ", squelch" << squelch << "stddev" << stddev(firstBinAboveSquelchArray) << firstBinAboveSquelchArray;
        if (stddev(firstBinAboveSquelchArray) > 5) {
            if (i) {
                squelch += 0.2;
                avgFirstBin = prevBinValue;
            }
            qDebug() << "PRF: Using squelch level" << squelch << "first bin" << avgFirstBin;
            break;
        }
    }

    QVector<double> timeBetweenLinesAboveSquelch;
    bool skipFirstValue = true;
    int binsAboveSquelch = 0;
    int linesBetweenTops = 11;

    for (int row = from; row < to; row++) {
        if (data[row][avgFirstBin] > squelch) {
            binsAboveSquelch++;
            if (linesBetweenTops > 10) {
                //qDebug() << "top at" << row << "time betw tops" << 1e6 * (double)linesBetweenTops * timePerLine << "us";
                if (!skipFirstValue) timeBetweenLinesAboveSquelch.append((double)linesBetweenTops * timePerLine);
                else
                    skipFirstValue = false;
            }
            linesBetweenTops = 0;
        }
        else
            linesBetweenTops++;
    }
    double avgPeriod = 0;
    if (!timeBetweenLinesAboveSquelch.isEmpty()) {
        for (auto && val : timeBetweenLinesAboveSquelch) avgPeriod += val;
        if (!timeBetweenLinesAboveSquelch.isEmpty()) avgPeriod /=  timeBetweenLinesAboveSquelch.size();
    }
    m_metadata.periodTime = avgPeriod;
    if (m_metadata.periodTime < 6e-6) m_metadata.periodTime = 0; // Assume super low period time means wrong measurement

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

void PlotAndAnalyze::writeMetaToDisk(cv::Mat results, QStringList classes)
{
    QJsonObject root;
    QJsonObject metadata;
    QJsonArray classification;

    root["program"] = "Hauken";
    root["version"] = QString(PROJECT_VERSION);
    root["fileCreated"] = QDateTime::currentDateTimeUtc().toString(Qt::ISODateWithMs);

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
        for (auto && val : probs)
            array.append(val);
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
