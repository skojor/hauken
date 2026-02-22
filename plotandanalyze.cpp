#include "plotandanalyze.h"
#include "asciitranslator.h"
#include "gif.h"
#include "opencv2/opencv.hpp"
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

void PlotAndAnalyze::receiveFftData(const QVector<QVector<double> > &fftVector, const IqMetadata &meta)
{
    // FFTVector contains FFT of all available IQ samples, with 12 samples overlap. Meta describes freq, samplerate and bw data from measurement
    if (fftVector.isEmpty()) {
        qWarning() << "PlotAndAnalyze: Received empty FFT vector data, aborting";
        return;
    }
    m_metadata = meta; // Work copy, stored in class
    createFilename();
    findIqFftMinMaxAvg(fftVector);

    qDebug() << "maxloc debug" << m_metadata.maxLoc << fftVector.size();
    // This part looks at spectral density
    QVector<QImage> images = createImages(fftVector, 2e-3, m_metadata.maxLoc, 1, true);
    QImage img;
    if (!images.isEmpty()) {
        QImage img = images.first();//.scaled(256, 256, Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
        m_metadata.spectral = calculateSpectralStructure(img);
    }
    else {
        qWarning() << "Not enough I/Q data to generate plot(s)";
        return;
    }

    // Classification images generated here.
    images = createImages(fftVector, 5e-4, meta.maxLoc, 10, true);
    if (!images.isEmpty()) {
        emit imagesReadyForClassification(images);
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

QVector<QImage> PlotAndAnalyze::createImages(const QVector<QVector<double>> &fftVector, double lenOfImage, int startAtPos, int nrOfImages, bool grayscale)
{
    QVector<QImage> imageVector;

    // This should be 2129 lines for 500 us long image with a 64 bin fft and 12 samples increase ctr.
    int linesPerImage = 0;
    if (m_metadata.samplesInc)
        linesPerImage =(( (m_metadata.samplerate * lenOfImage) - (m_metadata.fftSize - m_metadata.samplesInc) )) / m_metadata.samplesInc;
    else return imageVector;

    if (startAtPos == m_metadata.maxLoc) { // go back a 1/4 image in time if maxLoc is chosen as starting point
        startAtPos -= linesPerImage / 4;
    }

    // Minimum from fft produces alot of "noise" in the plot, this works like a filter. Always on for grayscale (used for classification)
    if (m_config->getIqUseAvgForPlot() || grayscale) m_metadata.min = m_metadata.avg;
    int lineIterator = startAtPos;
    if ( lineIterator + (nrOfImages * linesPerImage) > fftVector.size() ) {
        // Go back in time to create enough pics if not enough ffts
        lineIterator = fftVector.size() - (nrOfImages * linesPerImage);
    }
    if (lineIterator < 0) lineIterator = 0;
    m_metadata.imageStartAt = lineIterator;

    for (int imageCtr = 0; imageCtr < nrOfImages; imageCtr++) {
        QImage image(fftVector.first().size(), linesPerImage, QImage::Format_ARGB32);
        QColor imgColor;
        double percent;
        int x, y = 0;
        int alpha = 127;

        if (lineIterator + linesPerImage > fftVector.size()) break;

        for (int i = lineIterator; i < lineIterator + linesPerImage; i++) {
            x = 0;
            for (auto &&val : fftVector[i]) {
                percent = (val - m_metadata.min) / (m_metadata.max - m_metadata.min);
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
    qDebug() << "img debugging" << imageVector.size() << imageVector.first().size() << m_metadata.maxLoc << m_metadata.imageStartAt << linesPerImage;
    return imageVector;
}

void PlotAndAnalyze::findIqFftMinMaxAvg(const QVector<QVector<double> > &iqFftResult)
{
    m_metadata.min = 99e9;
    m_metadata.max = -99e9;
    m_metadata.avg = 0;
    m_metadata.maxLoc = 0;

    for (int i = 0; i < iqFftResult.size(); i++) {
        for (const auto &val : iqFftResult[i]) {
            if (val < m_metadata.min) m_metadata.min = val;
            else if (val > m_metadata.max) {
                m_metadata.max = val;
                m_metadata.maxLoc = i;
            }
            m_metadata.avg += val;
        }
    }
    m_metadata.avg /= iqFftResult.size() * iqFftResult.first().size();
    //m_metadata.maxLoc -= 533; // Go back 125 us. NO! BAD CODE
    //if (m_metadata.maxLoc < 0) m_metadata.maxLoc = 0;
}

double PlotAndAnalyze::calculateSpectralStructure(const QImage &image)
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

void PlotAndAnalyze::receiveClassification(int classId, double confid, QStringList classes)
{
    QString text;
    QTextStream ts(&text);
    ts << "Classification: " << classes[classId] << " [confidence " << (int)confid << " %]. ";
    if (m_metadata.trigFrequency)
        ts << "Trigger frequency (max)" << m_metadata.trigFrequency << " MHz. ";

    if (classes[classId].contains("sweep", Qt::CaseInsensitive)) {
        ts << "Spectral density " << QString::number(m_metadata.spectral, 'f', 2);
        if (m_metadata.spectral > 0.15) {
            // Sweep jammer detected, report to notification class
            ts << ", most likely a jammer.";
            emit reportIntentional(text);
        }
        else {
            ts << ", most likely a radar.";
            if (m_metadata.trigFrequency > 1210 and
                m_metadata.trigFrequency < 1300) {

            }
        }
    }
    else if (classes[classId].contains("prn", Qt::CaseInsensitive))
        emit reportIntentional(text);

    else if (classes[classId].contains("cw", Qt::CaseInsensitive)) { // TODO expand this function with more logic, for now (test) checking just center L1
        if (abs(m_metadata.trigFrequency * 1e6 - 1.57542e9) < 0.5e6) { // TODO check this, is 1 MHz around center L1 critical?
            ts << " Within L1 center frequency.";
            emit reportIntentional(text);
        }
    }

    if (m_metadata.fromFile)
        emit toIncidentLog(NOTIFY::TYPE::AIDONTNOTIFY, "", text);
    else
        emit toIncidentLog(NOTIFY::TYPE::AI, "", text);

    emit analyzerResult(classes[classId], confid);
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
        if (m_config->getEmailAddGif()) // and !m_metadata.fromFile)
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
    if (m_config->getEmailAddIqPlot()) { // and !m_metadata.fromFile) {
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
    else {
        m_metadata.filename = dir + "/" + m_config->incidentTimestamp().toString("yyyyMMddhhmmss_")
        + AsciiTranslator::toAscii(m_config->getStationName()) + "_" + QString::number(m_metadata.centerfreq * 1e-6, 'f', 3) + "MHz_"
            + QString::number(m_metadata.samplerate * 1e-6, 'f', 2) + "Msps_bw"
            + QString::number(m_metadata.bandwidth * 1e-3, 'f', 0) + "kHz_"
            + QDateTime::fromMSecsSinceEpoch(m_metadata.timestamp * 1e-6).toString("hhmmss.zzz_")
            + (m_config->getIqSaveAs16bit() ? "16bit" : "8bit");
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
    qDebug() << startPlotAt << m_iq16.size() << maxVal << xAxis.size();
    plot->graph(0)->setData(xAxis, yAxis);
    //plot->rescaleAxes();
    plot->replot();
    plot->saveJpg(m_metadata.filename + "_const.jpg", 512, 512);

    if (m_config->getEmailAddIqPlot()) { // and !m_metadata.fromFile) {
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
