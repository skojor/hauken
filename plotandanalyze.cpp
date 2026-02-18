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
    reqTracedataTimer = new QTimer;
    sendPlotsTimer = new QTimer;
    reqTracedataTimer->setSingleShot(true);
    sendPlotsTimer->setSingleShot(true);

    connect(reqTracedataTimer, &QTimer::timeout, this, [this] () {
        emit reqTracedata();
        sendPlotsTimer->start(3000); // Send plots in 3 seconds, wether last plot is ready or not
    });
    connect(sendPlotsTimer, &QTimer::timeout, this, [this] () {
        for (int i = 0; i < plotsToSend.size(); i++) {
            emit imageReady(plotsToSend[i], plotsDescription[i]);
            qDebug() << plotsToSend[i] << plotsDescription[i];
        }
        plotsToSend.clear();
        plotsDescription.clear();
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
            double delta = (double)m_metadata.maxLoc * (1.0 / m_metadata.samplerate) * (double)m_metadata.samplesInc;
            quint64 startingAt = m_metadata.timestamp * 1e-6 + delta * 1e3;
            //qDebug() << "sure?" << m_metadata.timestamp << m_metadata.maxLoc << QDateTime::fromMSecsSinceEpoch(startingAt).toString("hh:mm:ss.zzz") << QDateTime::fromMSecsSinceEpoch(m_metadata.timestamp * 1e-6).toString("hh:mm:ss.zzz");

            plotsDescription.append("FFT animation from "
                                    + QDateTime::fromMSecsSinceEpoch(startingAt).toString("hh:mm:ss.zzz")
                                    + ", 500 us per image, total time covered 25 ms");

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

    // This should be 2129 lines for 500 us long image with a 12 samples increase ctr.
    int linesPerImage = 0;
    if (m_metadata.samplesInc)
        linesPerImage =(( (m_metadata.samplerate * lenOfImage) - (m_metadata.fftSize - m_metadata.samplesInc) )) / m_metadata.samplesInc;
    else return imageVector;

    // Minimum from fft produces alot of "noise" in the plot, this works like a filter. Always on for grayscale (used for classification)
    if (m_config->getIqUseAvgForPlot() || grayscale) m_metadata.min = m_metadata.avg;
    int lineIterator = startAtPos;

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
        ts << "Trigger frequency " << m_metadata.trigFrequency << " MHz. ";

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
            plotsToSend.append(m_metadata.filename + ".gif");
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

        plotsToSend.prepend(m_metadata.filename + "_info.jpg");
        plotsDescription.prepend("Single plot, "
                                 + QString::number(secondsAnalyzed * 1e6)
                                 + " us long. Max level "
                                 + " at timestamp "
                                 + QDateTime::fromMSecsSinceEpoch(startingAt).toString("hh:mm:ss.zzz. ")
                                 + "IQ recording started at " + QDateTime::fromMSecsSinceEpoch(m_metadata.timestamp).toString("hh:mm:ss.zzz"));
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
    if (m_metadata.timestamp)
        painter.drawText((image.width() / 2) - 25, image.height() - 5, QDateTime::fromMSecsSinceEpoch(m_metadata.timestamp * 1e-6).toString("hh:mm:ss.zzz"));
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
        plotsToSend.append(m_metadata.filename + "_const.jpg");
        plotsDescription.append("I/Q constellation diagram, 25600 samples plotted from point of maximum signal level");
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

void PlotAndAnalyze::receiveTracedata(QVector<QVector<qint16>> data, QCustomPlot *plot)
{
    if (data.size() > 20) {
        int max, min, avg;
        findTracedataMinMaxAvg(data, min, max, avg);
        QImage image(data.first().size(), data.size(), QImage::Format_ARGB32);
        QColor imgColor;
        double percent;
        int x, y = 0;

        for (int i = 0; i < data.size(); i++) {
            x = 0;
            for (auto &&val : data[i]) {
                if (max - min) percent = (double)(val - min) / ((double)max - min);
                else percent = 0;
                if (percent > 1)
                    percent = 1;
                else if (percent < 0)
                    percent = 0;
                imgColor.setHsv(0, 0, 255 - (255 * percent));
                image.setPixel(x, y, imgColor.rgba());
                x++;
            }
            y++;
        }
        image = image.scaled(768, 512, Qt::IgnoreAspectRatio, Qt::SmoothTransformation);

        mutex.lock();
        plot->layer("triggerLayer")->setVisible(false);
        plot->layer("liveGraph")->setVisible(false);
        plot->replot();

        plot->axisRect()->setBackground(QPixmap::fromImage(image), true);
        plot->saveJpg(m_metadata.filename + "_spectrogram.jpg", 768, 512);

        plot->layer("triggerLayer")->setVisible(true);
        plot->layer("liveGraph")->setVisible(true);
        plot->replot();

        mutex.unlock();

        plotsToSend.prepend(m_metadata.filename + "_spectrogram.jpg");
        plotsDescription.prepend("Spectrogram of " + QString::number(data.size()) + " trace lines");
    }
}

void PlotAndAnalyze::recordingState()
{
    reqTracedataTimer->start(10 + (m_config->getNotifyTruncateTime() * 1e3));
}

void PlotAndAnalyze::findTracedataMinMaxAvg(const QVector<QVector<qint16>> &data, int &min, int &max, int &avg)
{
    min = 32767;
    max = -32767;
    quint64 sum = 0;

    for (auto && line : data) {
        for (auto && val : line) {
            if (val < min) min = val;
            else if (val > max) max = val;
            sum += val;
        }
    }
    avg = sum / ( data.size() * data.first().size() );
}
