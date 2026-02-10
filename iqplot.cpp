#include "iqplot.h"
#include "asciitranslator.h"
#include "fftw3.h"
#include "gif.h"
#include <QRegularExpression>
#include <QtConcurrent>
//#include "gif.h"
#include "opencv2/opencv.hpp"

IqPlot::IqPlot(QSharedPointer<Config> c)
{
    config = c;
    fillWindow();

    timeoutTimer->setSingleShot(true);

    connect(timeoutTimer, &QTimer::timeout, this, [this]() {
        qWarning() << "I/Q transfer timed out." << iqSamples.size() << flagRequestedEndVifConnection << flagHeaderValidated;
        emit endVifConnection();
        emit busyRecording(false);
        iqSamples.clear();
        flagRequestedEndVifConnection = true;
        flagHeaderValidated = false;
    });

    lastIqRequestTimer->setSingleShot(true);
    connect(lastIqRequestTimer, &QTimer::timeout, this, [this] () {
        flagOngoingAlarm = false;
    });
}

void IqPlot::getIqData(const QVector<complexInt16> &iq16)
{
    dataFromFile = false;
   if (!samplesNeeded)
        samplesNeeded = (int)(config->getIqLogTime() * samplerate);

    if (!listFreqs.isEmpty() && flagHeaderValidated) timeoutTimer->start(IQTRANSFERTIMEOUT_MS); // Restart timer as long as data is flowing and we have work to do
    //qDebug() << flagHeaderValidated << throwFirstSamples << iq16.size() << iqSamples.size();
    if (flagHeaderValidated and !throwFirstSamples) iqSamples += iq16;
    else if (flagHeaderValidated and throwFirstSamples)
        throwFirstSamples--;

    //qDebug() << iqSamples.size();
    if (iqSamples.size() >= samplesNeeded and !listFreqs.isEmpty()) {
        ffmFrequency = listFreqs.first(); // Silly, but blame the coder
        parseIqData(iqSamples, listFreqs.first());
        receiverControl(); // Change freq, or end datastream if we are done
        iqSamples.clear();
    }
}

void IqPlot::parseIqData(const QVector<complexInt16> &iq16, const double frequency)
{
    secsToAnalyze = config->getIqFftPlotLength() / 1e6;
    if (!samplerate) samplerate = (double) config->getIqFftPlotBw() * 1.28
                     * 1e3;
    fillWindow();

    QString dir = config->incidentFolder();
    QTextStream ts(&filename);

    if (!QDir().exists(dir))
        QDir().mkpath(dir);

    filename = dir + "/" + config->incidentTimestamp().toString("yyyyMMddhhmmss_")
               + AsciiTranslator::toAscii(config->getStationName()) + "_" + QString::number(frequency, 'f', 3) + "MHz_"
               + QString::number(samplerate * 1e-6, 'f', 2) + "Msps_bw"
               + QString::number(config->getIqFftPlotBw(), 'f', 0) + "kHz_"
               + (config->getIqSaveAs16bit() ? "16bit" : "8bit");

    if (!dataFromFile && config->getIqSaveToFile() && iq16.size()) {
        (void)QtConcurrent::run(&IqPlot::saveIqData,
                                 this,
                                 iq16);
    }
    (void)QtConcurrent::run(&IqPlot::createPlotsDetached,
                             this,
                             iq16);
}

void IqPlot::createPlotsDetached(const QVector<complexInt16> &iq)
{
    receiveIqDataWorker(iq, 5e-4, false, false);
    receiveIqDataWorker(iq, config->getIqFftPlotLength() / 1e6, true, false);
    receiveIqDataWorker(iq, config->getIqFftPlotLength() / 1e6, false, true);
}

void IqPlot::receiveIqDataWorker(const QVector<complexInt16> &iq, const double secondsToAnalyze, bool addInfo, bool createGif)
{
    int samplesIterator = 0;
    bool repeat = false;
    fftw_complex *in = (fftw_complex *) fftw_malloc(sizeof(fftw_complex) * fftSize);
    fftw_complex *out = (fftw_complex *) fftw_malloc(sizeof(fftw_complex) * fftSize);
    fftw_plan plan = fftw_plan_dft_1d(fftSize, in, out, FFTW_FORWARD, FFTW_ESTIMATE);

    if (!createGif) // Start at beginning if this should be a gif
        samplesIterator = analyzeIqStart(iq) - (samplerate * (secondsToAnalyze / 4)); // Hopefully this is just before where sth interesting happens
    else
        repeat = true;

    if (samplesIterator < 0)
        samplesIterator = 0;

    while (samplesIterator > 0
           && samplesIterator + (samplerate * secondsToAnalyze) + fftSize >= iq.size())
        samplesIterator--;

    int ySize = samplerate * secondsToAnalyze + samplesIterator;

    double secPerSample = 1.0 / samplerate;
    int samplesIteratorInc = 12; //;(double) (samplerate * secondsToAnalyze) / (double) imageYSize;
    /*qDebug() << "FFT plot debug: Samples inc." << (int) samplesIteratorInc << ". Iterator starts at"
             << samplesIterator << "and counts up to" << ySize << ". Total samples analyzed"
             << ySize - samplesIterator;*/
    int removeSamples = 7; // TODO, variable BW/Msps
    QVector<double> result;
    QVector<QVector<double>> iqFftResult;

    do {
        if (iq.size() > ySize) {
            bool useDB = config->getIqUseDB();
            if (!addInfo) useDB = true;

            while (samplesIterator + fftSize < ySize ) {
                for (int i = 0; i < fftSize; i++) {
                    in[i][0] = (iq[samplesIterator + i].real * window[i]);
                    in[i][1] = (iq[samplesIterator + i].imag * window[i]);
                }
                fftw_execute(plan); // FFT is done here

                for (int i = (fftSize / 2) + removeSamples; i < fftSize;
                     i++) { // Find magnitude, normalize, reorder and cut edges
                    double val = sqrt(out[i][0] * out[i][0] + out[i][1] * out[i][1])
                                 * (1.0 / fftSize);
                    if (val == 0)
                        val = 1e-9; // log10(0) = -inf
                    if (useDB)
                        result.append(10 * log10(val));
                    else
                        result.append(val);
                }
                for (int i = 0; i < (fftSize / 2) - removeSamples; i++) {
                    double val = sqrt(out[i][0] * out[i][0] + out[i][1] * out[i][1])
                    * (1.0 / fftSize);
                    if (val == 0)
                        val = 1e-9; // log10(0) = -inf
                    if (useDB)
                        result.append(10 * log10(val));
                    else
                        result.append(val);
                }

                iqFftResult.append(result);
                result.clear();

                samplesIterator += samplesIteratorInc;
            }
            ySize += samplerate * secondsToAnalyze;
            createIqPlot(iqFftResult, secondsToAnalyze, secPerSample * fftSize, addInfo, createGif);
            iqFftResult.clear();

            if (samplesIterator + samplerate * secondsToAnalyze > iq.size()) repeat = false;
        } else {
            qDebug() << "Not enough samples to create I/Q plot";
            repeat = false;
        }
    } while(repeat);

    fftw_destroy_plan(plan);
    fftw_free(in);
    fftw_free(out);

    createAnimation(QImage(), true); // End file creation
}

void IqPlot::saveIqData(const QVector<complexInt16> &iq16)
{
    QFile file(filename + ".iq");
    if (!file.open(QIODevice::WriteOnly))
        qWarning() << "Could not open" << filename + ".iq"
                   << "for writing IQ data, aborting";
    else {
        if (config->getIqSaveAs16bit())
            file.write((const char *) iq16.constData(), iq16.size() * 4);
        else
            file.write((const char *) convertComplex16to8bit(iq16).constData(), iq16.size() * 2);
        file.close();
    }
}

void IqPlot::createIqPlot(const QVector<QVector<double>> &iqFftResult,
                          const double secondsAnalyzed,
                          const double secondsPerLine,
                          bool addInfo , bool createGif)
{
    double min, max, avg;
    if (iqFftResult.size() > 0)
        findIqFftMinMaxAvg(iqFftResult, min, max, avg);
    else
        return;

    //qDebug() << min << max << avg;
    if (config->getIqUseAvgForPlot() || !addInfo) min = avg; // Minimum from fft produces alot of "noise" in the plot, this works like a filter

    QImage *image = new QImage(QSize(iqFftResult.first().size(), iqFftResult.size()), QImage::Format_ARGB32);
    QColor imgColor;
    double percent;
    int x, y = 0;
    int alpha = 127;
    //if (!addInfo) alpha = 255;

    for (auto &&line : iqFftResult) {
        x = 0;
        for (auto &&val : line) {
            percent = (val - min) / (max - min);
            if (percent > 1)
                percent = 1;
            else if (percent < 0)
                percent = 0;
            if (addInfo) imgColor.setHsv(255 - (255 * percent), 255, alpha);
            else imgColor.setHsv(0, 0, 255 * percent);
            image->setPixel(x, y, imgColor.rgba());
            x++;
        }
        y++;
    }

    if (addInfo || createGif) {
        if (!createGif) {
            *image = image->scaled(512, 512, Qt::IgnoreAspectRatio, Qt::SmoothTransformation); // To better fit text #FIXME
            double secondsPerLine = secondsAnalyzed / 512.0;
            addLines(image, secondsAnalyzed, secondsPerLine);
            addText(image, secondsAnalyzed, secondsPerLine);
        }
        if (!createGif) saveImage(image, secondsAnalyzed);
        else createAnimation(*image);
    }
    else if (!addInfo) {
        emit rawPlotReady(*image); // For AI classification (rgb -> gray all ch)
        QImage imgscaled = image->scaled(256, 256, Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
        //saveImage(&imgscaled, secondsAnalyzed);
    }
    delete image;
}

void IqPlot::findIqFftMinMaxAvg(const QVector<QVector<double> > &iqFftResult,
                                double &min,
                                double &max,
                                double &avg)
{
    min = 99e9;
    max = -99e9;
    avg = 0;

    for (const auto &line : iqFftResult) {
        for (const auto &val : line) {
            if (val < min)
                min = val;
            else if (val > max)
                max = val;
            avg += val;
        }
    }
    avg /= iqFftResult.size() * iqFftResult.first().size();
}

void IqPlot::fillWindow()
{
    window.resize(fftSize);
    if (config->getIqUseWindow()) {
        for (int i = 0; i < fftSize; i++) {
            window[i] = 0.5 * (1 - cos(2 * M_PI * i / fftSize)); // Hanning / Hann
        }
    } else {
        for (auto &&val : window)
            val = 1; // Effectively no window
    }
}

void IqPlot::addLines(QImage *image, const double secondsAnalyzed, const double secondsPerLine)
{
    QPen pen;
    QPainter painter(image);
    pen.setWidth(1);
    pen.setStyle(Qt::DotLine);
    pen.setColor(QColor(255, 255, 255, 90));
    painter.setPen(pen);
    int height = image->size().height();
    int width = image->size().width();
    int xMinus = 25;

    painter.drawLine(width / 2, 5, width / 2, height);
    //painter.drawLine(xMinus, 5, xMinus, height);
    //painter.drawLine(width - xMinus, 5, width - xMinus, height);

    int lineDistance = (secondsAnalyzed / 5) / secondsPerLine;

    for (int y = 0; y < height; y++) {
        if (y % lineDistance == 0 && y > 0)
            painter.drawLine(0, y, width, y);
    }
    painter.end();
}

void IqPlot::addText(QImage *image, const double secondsAnalyzed, const double secondsPerLine)
{
    QPen pen;
    QPainter painter(image);
    pen.setWidth(10);
    pen.setColor(Qt::white);
    int fontSize = 7, yMinus = 15;

    painter.setFont(QFont("Arial", fontSize));
    painter.setPen(pen);
    QString strFr = QString::number(ffmFrequency, 'f', 0);
    strFr.remove(QRegularExpression("0+$"));
    strFr.remove(QRegularExpression("[\\.,]$")); // Don't show more decimals than needed
    QString strUpper = QString::number(samplerate / 1.28 / 2e6, 'f', 3);
    strUpper.remove(QRegularExpression("0+$"));
    strUpper.remove(QRegularExpression("[\\.,]$"));
    QString strLower = QString::number(-samplerate / 1.28 / 2e6, 'f', 3);
    strLower.remove(QRegularExpression("0+$"));
    strLower.remove(QRegularExpression("[\\.,]$"));

    painter.drawText((image->size().width() / 2) - 30,
                     yMinus,
                     strFr + " MHz");
    painter.drawText(2, yMinus, strLower + " MHz");
    painter.drawText(image->size().width() - 40,
                     yMinus,
                     "+ " + strUpper + " MHz");

    int lineDistance = (secondsAnalyzed / 5) / secondsPerLine;
    int microsecCtr = 1e6 * secondsAnalyzed / 5;
    for (int y = 0; y < image->size().height(); y++) {
        if (y % lineDistance == 0 && y > 0) {
            painter.drawText(0, y, QString::number(microsecCtr) + " µs");
            microsecCtr += 1e6 * secondsAnalyzed / 10;
        }
    }
    painter.end();
}

void IqPlot::saveImage(const QImage *image, const double secondsAnalyzed)
{
    if (!filename.isEmpty()) {
        image->save(filename + "_" + QString::number(secondsAnalyzed * 1e6) + "us.jpg", "JPG", 75);
        emit iqPlotReady(filename + "_" + QString::number(secondsAnalyzed * 1e6) + "us.jpg");
    } else {
        image->save(config->getLogFolder() + "/"
                        + QDateTime::currentDateTime().toString("yyMMddhhmmss") + "_"
                        + QString::number(secondsAnalyzed * 1e6) + "us.jpg",
                    "JPG",
                    75);
        emit iqPlotReady(config->getLogFolder() + "/"
                         + QDateTime::currentDateTime().toString("yyMMddhhmmss") + "_"
                         + QString::number(secondsAnalyzed * 1e6) + "us.jpg");
    }
}

void IqPlot::requestIqData()
{
    emit reqIqCenterFrequency();
    lastIqRequestTimer->start(120e3); // Reset timer as long as incident is ongoing

    if (config->getIqCreateFftPlot() && !flagOngoingAlarm) // New alarm signal received
    {
        flagOngoingAlarm = true;

        samplesNeeded = 0; //(int)(config->getIqLogTime() * (double)config->getIqFftPlotBw() * 1.28 * 1e3);

        listFreqs.clear();
        if (config->getIqRecordMultipleBands()) {
            listFreqs = config->getIqMultibandCenterFreqs();
        }

        if (!config->getIqRecordAllTrigArea() && trigFrequency > 0 && listFreqs.isEmpty())
            listFreqs.append(trigFrequency);

        if (!config->getIqRecordAllTrigArea() && listFreqs.isEmpty()) { // Still empty list? Probably due to manual triggered recording. Use previously req. center freq from meas.device
            if ((int)centerFrequency != 0) listFreqs.append(centerFrequency / 1e6);
        }

        if (config->getIqRecordAllTrigArea() || listFreqs.isEmpty()) {
            QStringList stringListTrigFreqs = config->getTrigFrequencies();
            if (stringListTrigFreqs.isEmpty() || stringListTrigFreqs.first() == "0") { // What to do here? Shouldn't happen, set center to FFM center for now
                listFreqs.append(1e-6 * config->getInstrFfmCenterFreq());
            }
            else {
                for (int i=0; i < stringListTrigFreqs.size(); i++) {
                    double start = stringListTrigFreqs[i].toDouble() * 1e6;
                    double stop = stringListTrigFreqs[++i].toDouble() * 1e6;
                    double range = stop - start;
                    double currentBandwidth = config->getIqFftPlotBw() * 1e3;

                    if (range > currentBandwidth) {

                        double f = 1e6 * (int)((start + range / 2) / 1e6); // round to nearest MHz of center
                        do {
                            listFreqs.append(f / 1e6);
                            f -= currentBandwidth;
                        } while (f > start - currentBandwidth);

                        f = 1e6 * (int)((start + range / 2) / 1e6);
                        do {
                            f += currentBandwidth;
                            listFreqs.append(f / 1e6);
                        } while (f < stop);

                    }
                    else {
                        listFreqs.append((int)((start + range / 2) / 1e6));
                    }
                }
            }
        }
        emit busyRecording(true);
        emit setFfmCenterFrequency(listFreqs.first());
        emit reqVifConnection();
        flagRequestedEndVifConnection = false;
        timeoutTimer->start(IQTRANSFERTIMEOUT_MS);
    }
}

quint64 IqPlot::analyzeIqStart(const QVector<complexInt16> &iq)
{
    qint16 max = -32768;
    quint64 locMax = 0;
    quint64 iter = 0;
    for (const auto &val : iq) {
        int mag = sqrt(val.real * val.real + val.imag * val.imag);
        if (mag > max) {
            max = mag;
            locMax = iter;
        }
        iter++;
    }
    return locMax;
}

bool IqPlot::readAndAnalyzeFile(const QString fname)
{
    parseFilename(fname);

    bool int16;
    if (fname.contains("16bit", Qt::CaseInsensitive))
        int16 = true;
    else
        int16 = false;

    QFile file(fname);
    if (!file.open(QIODevice::ReadOnly)) {
        qWarning() << "IQ plot: Cannot open file" << fname << ", giving up";
        return false;
    } else {
        qInfo() << "Reading IQ data from" << fname << "as" << (int16 ? "16 bit" : "8 bit")
        << "integers";
        dataFromFile = true;

        if (int16) {
            QVector<complexInt16> iq16;
            iq16.resize(file.size() / 4);
            if (file.read((char *) iq16.data(), file.size()) == -1)
                qWarning() << "IQ16 read failed:" << file.errorString();
            else {
                for (auto &val : iq16) { // readRaw makes a mess out of byte order. Reorder manually
                    val.real = qToBigEndian(val.real);
                    val.imag = qToBigEndian(val.imag);
                }
                parseIqData(iq16, ffmFrequency);
            }
        } else {
            QVector<complexInt8> iq8;
            iq8.resize(file.size() / 2);
            if (file.read((char *) iq8.data(), file.size()) == -1)
                qWarning() << "IQ8 read failed:" << file.errorString();
            else
                parseIqData(convertComplex8to16bit(iq8), ffmFrequency);
        }
        file.close();
        return true;
    }
}

const QVector<complexInt8> IqPlot::convertComplex16to8bit(const QVector<complexInt16> &input)
{
    QVector<complexInt8> output;
    qint16 max;
    findIqMaxValue(input, max);
    double factor;
    if (max <= 127)
        factor = 1.0;
    else
        factor = 127.0
                 / max; // Scaling max int16 value down to int8. Is this the correct way to do it?? TODO
    for (const auto &val : input)
        output.append(complexInt8{(qint8) (factor * val.real), (qint8) (factor * val.imag)});

    return output;
}

const QVector<complexInt16> IqPlot::convertComplex8to16bit(const QVector<complexInt8> &input)
{
    QVector<complexInt16> output;
    for (const auto &val : input)
        output.append(complexInt16{static_cast<qint16>(val.real * 128), static_cast<qint16>(val.imag * 128)});

    return output;
}

void IqPlot::findIqMaxValue(const QVector<complexInt16> &input, qint16 &max)
{
    max = -32768;
    for (const auto &val : input) {
        if (abs(val.real) > max)
            max = abs(val.real);
        if (abs(val.imag) > max)
            max = abs(val.imag);
    }
}

void IqPlot::parseFilename(const QString file)
{
    const QStringList split = file.split('_');
    if (split.size() >= 5) {
        for (const auto &val : split) {
            if (val.contains("MHz", Qt::CaseInsensitive))
                ffmFrequency = val.split("MHz").first().toDouble();
            else if (val.contains("Msps", Qt::CaseInsensitive))
                samplerate = val.split("Msps").first().toDouble() * 1e6;
        }
        qInfo() << "IQ file reader: Guessing frequency and sample rate from filename:"
                << ffmFrequency << samplerate;
    } else {
        qWarning() << "IQ filename not parsed, setting standard frequency/samplerate values";
        samplerate = 51.2e6;
        ffmFrequency = 0;
    }
}

void IqPlot::updSettings()
{

}

void IqPlot::validateHeader(quint64 freq, quint64 bw, quint64 rate)
{
    if (!listFreqs.isEmpty()
        && freq == (quint64)(listFreqs.first() * 1e6)
        && bw == (quint32)(config->getIqFftPlotBw() * 1e3))
    {
        flagHeaderValidated = true;
        samplerate = rate;
        //qDebug() << "validated at" << QDateTime::currentDateTime().toString("mm:ss:zzz") << ", samplerate:" << samplerate;
    }
    else {
        flagHeaderValidated = false;
        //qDebug() << "not validated at" << QDateTime::currentDateTime().toString("mm:ss:zzz") << freq << bw << samplerate;

    }
}

void IqPlot::receiverControl()
{
    flagHeaderValidated = false; // Assume future data to be invalid for now
    throwFirstSamples = 4 * samplerate / 1e7;
    if (!throwFirstSamples) throwFirstSamples = 1;

    //qDebug() << throwFirstSamples;
    emit resetTimeoutTimer();

    if (listFreqs.size() > 1) {// we have more work to do
        timeoutTimer->start(IQTRANSFERTIMEOUT_MS); // restart timer for new freq
        listFreqs.removeFirst();
        //emit headerValidated(false);
        emit setFfmCenterFrequency(listFreqs.first());
    }
    else { // We are done, stop I/Q stream
        timeoutTimer->stop();
        listFreqs.clear();
        if (!flagRequestedEndVifConnection) emit endVifConnection();
        flagRequestedEndVifConnection = true;
        emit busyRecording(false);
    }
}

void IqPlot::readFolder(const QString &folder)
{
    QDir selFolder(folder);
    QStringList filelist = selFolder.entryList(QStringList() << "*.iq", QDir::Files);
    qDebug() << "I/Q ReadFolder: Found these files:" << filelist;

    foreach (QString filename, filelist) {
        readAndAnalyzeFile(folder + "/" + filename);
        QThread::sleep(15);
    }
}

void IqPlot::createAnimation(const QImage &image, bool lastImage)
{
    if (!lastImage && !image.isNull()) {
        imageVector.append(image.scaled(256, 256, Qt::IgnoreAspectRatio, Qt::SmoothTransformation));
    }

    if (lastImage && imageVector.size() > 0) {

        QString filename;
        filename = config->incidentFolder() + "/" + config->incidentTimestamp().toString("yyyyMMddhhmmss_")
                   + AsciiTranslator::toAscii(config->getStationName()) + "_"
                   + QString::number(ffmFrequency, 'f', 3) + "MHz_"
                   + QString::number(samplerate * 1e-6, 'f', 2) + "Msps_bw"
                   + QString::number(config->getIqFftPlotBw(), 'f', 0) + "kHz_"
                   + (config->getIqSaveAs16bit() ? "16bit" : "8bit");

        GifWriter gifWriter;
        int delay = 20; // ms

        if (!GifBegin(&gifWriter, qPrintable(filename + ".gif"), imageVector[0].width(), imageVector[0].height(), delay)) {
            qWarning() << "Cannot write to gif file";
            return;
        }
        int iter = 0;
        for (auto &&img : imageVector) {
            QImage converted = img.convertToFormat(QImage::Format_RGBA8888);
            if (!converted.isNull())
                GifWriteFrame(&gifWriter, converted.bits(), converted.width(), converted.height(), delay);
            if (++iter > 50) break; // To keep gifs small sized
        }
        GifEnd(&gifWriter);
        emit iqPlotReady(filename + ".gif");

        int frameWidth = 256;
        int frameHeight = 256;
        int fps = 25;
        cv::VideoWriter videoWriter;
        int fourcc = cv::VideoWriter::fourcc('m', 'p', '4', 'v');
        videoWriter.open(qPrintable(filename + ".mp4"), fourcc, fps, cv::Size(frameWidth, frameHeight));
        if (!videoWriter.isOpened()) {
            qDebug() << "Could not open video file for write:" << filename + ".mp4";
            return;
        }
        for (auto &&image : imageVector) {
            cv::Mat frame(image.height(), image.width(), CV_8UC4,
                             const_cast<uchar*>(image.constBits()),
                             image.bytesPerLine());
            videoWriter.write(frame);
        }
        videoWriter.release();
        qDebug() << "Finished generating video, saved as" << filename + ".mp4";

        imageVector.clear();
    }
}
