#include "waterfall.h"

Waterfall::Waterfall(QSharedPointer<Config> c)
{
    config = c;
}

void Waterfall::start()
{
    pixmap = new QPixmap;
    updIntervalTimer = new QTimer;
    updIntervalTimer->setSingleShot(true);
    connect(updIntervalTimer, &QTimer::timeout, this, &Waterfall::updTimerCallback);
    updSettings();
}

void Waterfall::receiveTrace(const QVector<double> &trace)
{
    if (timeout) {
        timeout = false;
        mutex.lock();
        traceCopy = trace;
        mutex.unlock();
        if (!updIntervalTimer->isActive())
            updIntervalTimer->start(100); // first call
    }
}

void Waterfall::updTimerCallback()
{
    if (!traceCopy.isEmpty()) {
        timeout = true;
        mutex.lock(); // this needs exclusive access to containers and pixmap
        pixmap->scroll(0, 1, pixmap->rect());

        QImage image = pixmap->toImage();
        QColor color;

        int pixmapSize = pixmap->width();
        double ratio = (double) traceCopy.size() / (double) pixmapSize;
        double percent = 0;
        QElapsedTimer timer;
        timer.start();
        for (int x = 0; x < pixmapSize; x++) {
            percent = (traceCopy.at((int) (ratio * x)) - scaleMin)
                      / (scaleMax - scaleMin); // 0 - 1 range

            if (percent < 0)
                percent = 0;
            else if (percent > 1)
                percent = 1;

            if (colorset == COLORS::GREY)
                color.setHsv(180, 0, 255 - (255 * percent), 255);
            else if (colorset == COLORS::BLUE)
                color.setHsv(240, (255 * percent), 255, 255);
            else if (colorset == COLORS::RED)
                color.setHsv(0, (255 * percent), 255, 255);
            else
                //color.setHsv(190 - (190 * percent), 180, 255, 65);
                color.setHsv(255 - (255 * percent), 255, 240);

            image.setPixel(x, 0, color.rgba());
        }
        *pixmap = QPixmap::fromImage(image, Qt::NoFormatConversion);
        emit imageReady(pixmap);
        mutex.unlock(); // done with the exclusive work here
        double interval = 1000.0 / ((double) pixmap->height() / waterfallTime);
        updIntervalTimer->start((int) interval);
    }
}

void Waterfall::restartPlot()
{
    mutex.lock();
    *pixmap = QPixmap(pixmap->size());
    mutex.unlock();
}

void Waterfall::updSize(QRect s)
{
    mutex.lock();
    *pixmap = QPixmap(s.width(), s.height());
    mutex.unlock();
}

void Waterfall::updSettings()
{
    scaleMin = config->getPlotYMin() + 2; // +2 to clean up the screen when no signal is present
    scaleMax = config->getPlotYMax();
    if (startfreq != config->getInstrStartFreq() || stopfreq != config->getInstrStopFreq()
        || !resolution.contains(config->getInstrResolution())
        || !fftMode.contains(config->getInstrFftMode())) {
        startfreq = config->getInstrStartFreq();
        stopfreq = config->getInstrStopFreq();
        resolution = config->getInstrResolution();
        fftMode = config->getInstrFftMode();
        restartPlot();
    }
    if (waterfallTime != config->getWaterfallTime()) {
        waterfallTime = config->getWaterfallTime();
    }
    if (!mode.contains(config->getShowWaterfall())) {
        mode = config->getShowWaterfall();
        if (mode == "Grey")
            colorset = COLORS::GREY;
        else if (mode == "Red")
            colorset = COLORS::RED;
        else if (mode == "Blue")
            colorset = COLORS::BLUE;
        else if (mode == "Pride")
            colorset = COLORS::PRIDE;
    }
    secsToAnalyze = config->getIqFftPlotLength() / 1e6;
    samplerate = (double) config->getIqFftPlotBw() * 1.28
                 * 1e3; // TODO: Is this universal for all R&S instruments?
    fillWindow();
}

void Waterfall::receiveIqData(const QList<complexInt16> &iq16)
{
    filename = config->getLogFolder() + "/" + QDateTime::currentDateTime().toString("yyMMddhhmmss_")
               + config->getStationName() + "_" + QString::number(ffmFrequency, 'f', 0) + "MHz_"
               + QString::number(samplerate * 1e-6, 'f', 2) + "Msps_" + "8bit";
    if (!dataFromFile && config->getIqSaveToFile() && iq16.size())
        saveIqData(iq16);

    QFuture<void> f1000 = QtConcurrent::run(&Waterfall::receiveIqDataWorker,
                                            this,
                                            iq16,
                                            config->getIqFftPlotLength() / 1e6);
    dataFromFile = false;
    updSettings(); // Reread sample rate, frequencies, so on
}

void Waterfall::receiveIqDataWorker(const QList<complexInt16> iq, const double secondsToAnalyze)
{
    const int fftSize = 64;
    const int newFftSize = fftSize * 16;

    //int imageYSize = samplerate * secondsToAnalyze;//fftSize * 16 * 2;

    int samplesIterator = analyzeIqStart(iq)
                          - (samplerate
                             * (secondsToAnalyze
                                / 4)); // Hopefully this is just before where sth interesting happens
    if (samplesIterator < 0)
        samplesIterator = 0;

    while (samplesIterator > 0
           && samplesIterator + (samplerate * secondsToAnalyze) + newFftSize >= iq.size())
        samplesIterator--;

    int ySize = samplerate * secondsToAnalyze + samplesIterator;

    double secPerSample = 1.0 / samplerate;
    int samplesIteratorInc = (double) (samplerate * secondsToAnalyze) / (double) imageYSize;
    qDebug() << "FFT plot debug: Samples inc." << (int) samplesIteratorInc << ". Iterator starts at"
             << samplesIterator << "and counts up to" << ySize << ". Total samples analyzed"
             << ySize - samplesIterator;

    if ((int) samplesIteratorInc == 0)
        samplesIteratorInc = 1;

    double secondsPerLine = secPerSample * samplesIteratorInc;
    fftw_complex in[newFftSize], out[newFftSize];
    fftw_plan plan = fftw_plan_dft_1d(newFftSize, in, out, FFTW_FORWARD, FFTW_ESTIMATE);
    int removeSamples
        = 84.0 * (double) newFftSize
          / 1024.0; // Removing samples to have just above the original sample width visible in the plot

    QList<double> result;
    QList<QList<double>> iqFftResult;

    for (int i = 0; i < newFftSize;
         i++) { // Set all input values to 0 initially, used for zero padding
        in[i][0] = 0;
        in[i][1] = 0;
    }

    if (iq.size() > ySize) {
        bool useDB = config->getIqUseDB();

        while (samplesIterator < ySize) {
            for (int i = (newFftSize / 2) - (fftSize / 2), j = 0;
                 i < (newFftSize / 2) + (fftSize / 2);
                 i++, j++) {
                in[i][0] = (iq[samplesIterator + i].real * window[j]);
                in[i][1] = (iq[samplesIterator + i].imag * window[j]);
            }
            fftw_execute(plan); // FFT is done here

            for (int i = (newFftSize / 2) + removeSamples; i < newFftSize;
                 i++) { // Find magnitude, normalize, reorder and cut edges
                double val = sqrt(out[i][0] * out[i][0] + out[i][1] * out[i][1])
                             * (1.0 / newFftSize);
                if (val == 0)
                    val = 1e-9; // log10(0) = -inf
                if (useDB)
                    result.append(10 * log10(val));
                else
                    result.append(val);
            }
            for (int i = 0; i < (newFftSize / 2) - removeSamples; i++) {
                double val = sqrt(out[i][0] * out[i][0] + out[i][1] * out[i][1])
                             * (1.0 / newFftSize);
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
        fftw_destroy_plan(plan);

        createIqPlot(iqFftResult, secondsToAnalyze, secondsPerLine);
        qInfo() << "Plot worker finished";
    } else {
        qWarning() << "Not enough samples to create IQ FFT plot, giving up";
    }
}

void Waterfall::saveIqData(const QList<complexInt16> &iq16)
{
    QFile file(filename + ".iq");
    if (!file.open(QIODevice::WriteOnly))
        qWarning() << "Could not open" << filename + ".iq"
                   << "for writing IQ data, aborting";
    else {
        file.write((const char *) convertComplex16to8bit(iq16).constData(), iq16.size() * 2);
        file.close();
    }
}

void Waterfall::createIqPlot(const QList<QList<double>> &iqFftResult,
                             const double secondsAnalyzed,
                             const double secondsPerLine)
{
    double min, max, avg;
    findIqFftMinMaxAvg(iqFftResult, min, max, avg);
    qDebug() << min << max << avg;
    min = avg; // Minimum from fft produces alot of "noise" in the plot, this works like a filter

    QImage image(QSize(iqFftResult.first().size(), iqFftResult.size()), QImage::Format_ARGB32);
    QColor imgColor;
    double percent = 0;
    int x, y = 0;

    for (auto &&line : iqFftResult) {
        x = 0;
        for (auto &&val : line) {
            percent = (val - min) / (max - min);
            if (percent > 1)
                percent = 1;
            else if (percent < 0)
                percent = 0;
            imgColor.setHsv(255 - (255 * percent), 255, 127);
            image.setPixel(x, y, imgColor.rgba());
            x++;
        }
        y++;
    }
    addLines(&image, secondsAnalyzed, secondsPerLine);
    addText(&image, secondsAnalyzed, secondsPerLine);
    saveImage(&image, secondsAnalyzed);
}

void Waterfall::findIqFftMinMaxAvg(const QList<QList<double>> &iqFftResult,
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

void Waterfall::fillWindow()
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

void Waterfall::addLines(QImage *image, const double secondsAnalyzed, const double secondsPerLine)
{
    QPen pen;
    QPainter painter(image);
    pen.setWidth(1);
    pen.setStyle(Qt::DotLine);
    pen.setColor(Qt::white);
    painter.setPen(pen);
    int height = image->size().height();
    int width = image->size().width();
    int xMinus = 25;

    painter.drawLine(width / 2, 5, width / 2, height);
    painter.drawLine(xMinus, 5, xMinus, height);
    painter.drawLine(width - xMinus, 5, width - xMinus, height);

    int lineDistance = (secondsAnalyzed / 10) / secondsPerLine;

    for (int y = 0; y < height; y++) {
        if (y % lineDistance == 0 && y > 0)
            painter.drawLine(0, y, width, y);
    }
    painter.end();
}

void Waterfall::addText(QImage *image, const double secondsAnalyzed, const double secondsPerLine)
{
    QPen pen;
    QPainter painter(image);
    pen.setWidth(10);
    pen.setColor(Qt::white);
    int fontSize = 14, xMinus = 60, yMinus = 25;

    painter.setFont(QFont("Arial", fontSize));
    painter.setPen(pen);
    painter.drawText((image->size().width() / 2) - xMinus + 15,
                     yMinus,
                     QString::number(ffmFrequency) + " MHz");
    painter.drawText(0, yMinus, QString::number(-samplerate / 1.28 / 2e6, 'f', 1) + " MHz");
    painter.drawText((image->size().width()) - (xMinus + 40),
                     yMinus,
                     "+ " + QString::number(samplerate / 1.28 / 2e6, 'f', 1) + " MHz");

    int lineDistance = (secondsAnalyzed / 10) / secondsPerLine;
    int microsecCtr = 1e6 * secondsAnalyzed / 10;
    for (int y = 0; y < image->size().height(); y++) {
        if (y % lineDistance == 0 && y > 0) {
            painter.drawText(0, y, QString::number(microsecCtr) + " µs");
            microsecCtr += 1e6 * secondsAnalyzed / 10;
        }
    }
    painter.end();
}

void Waterfall::saveImage(const QImage *image, const double secondsAnalyzed)
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

void Waterfall::requestIqData()
{
    if (!lastIqRequestTimer.isValid() || lastIqRequestTimer.elapsed() > 120e3) {
        int samplesNeeded
            = samplerate
              * config->getIqLogTime(); // DL 0.x seconds of IQ data. This way we have sth to find intermittent signals inside
        emit requestIq(samplesNeeded);
        lastIqRequestTimer.restart();
    }
}

int Waterfall::analyzeIqStart(const QList<complexInt16> &iq)
{
    qint16 max = -32768;
    int locMax = 0;
    int iter = 0;
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

bool Waterfall::readAndAnalyzeFile(const QString fname)
{
    parseFilename(fname);

    bool int16;
    if (fname.contains("8bit", Qt::CaseInsensitive))
        int16 = false;
    else
        int16 = true;

    QFile file(fname);
    if (!file.open(QIODevice::ReadOnly)) {
        qWarning() << "IQ plot: Cannot open file" << fname << ", giving up";
        return false;
    } else {
        qInfo() << "Reading IQ data from" << fname << "as" << (int16 ? "16 bit" : "8 bit")
                << "integers";
        dataFromFile = true;

        if (int16) {
            QList<complexInt16> iq16;
            iq16.resize(file.size() / 4);
            if (file.read((char *) iq16.data(), file.size()) == -1)
                qWarning() << "IQ16 read failed:" << file.errorString();
            else {
                for (auto &val : iq16) { // readRaw makes a mess out of byte order. Reorder manually
                    val.real = qToBigEndian(val.real);
                    val.imag = qToBigEndian(val.imag);
                }
                receiveIqData(iq16);
            }
        } else {
            QList<complexInt8> iq8;
            iq8.resize(file.size() / 2);
            if (file.read((char *) iq8.data(), file.size()) == -1)
                qWarning() << "IQ8 read failed:" << file.errorString();
            else
                receiveIqData(convertComplex8to16bit(iq8));
        }
        file.close();
        return true;
    }
}

const QList<complexInt8> Waterfall::convertComplex16to8bit(const QList<complexInt16> &input)
{
    QList<complexInt8> output;
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

const QList<complexInt16> Waterfall::convertComplex8to16bit(const QList<complexInt8> &input)
{
    QList<complexInt16> output;
    for (const auto &val : input)
        output.append(complexInt16{val.real * 128, val.imag * 128});

    return output;
}

void Waterfall::findIqMaxValue(const QList<complexInt16> &input, qint16 &max)
{
    max = -32768;
    for (const auto &val : input) {
        if (abs(val.real) > max)
            max = abs(val.real);
        if (abs(val.imag) > max)
            max = abs(val.imag);
    }
}

void Waterfall::parseFilename(const QString file)
{
    QStringList split = file.split('_');
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
