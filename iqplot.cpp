#include "iqplot.h"
#include "asciitranslator.h"
#include "fftw3.h"

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
                 * 1e3; // TODO: Is this universal for all R&S instruments?
    fillWindow();

    if (foldernameDateTime.secsTo(QDateTime::currentDateTime()) > 40) // Don't change folder/filename timestamp too often
        foldernameDateTime = QDateTime::currentDateTime();

    QString dir = config->getLogFolder();
    if (config->getNewLogFolder()) { // create new folder for incident
        dir = config->getLogFolder() + "/" + foldernameDateTime.toString("yyyyMMdd_hhmmss_") + config->getStationName();
        if (!QDir().exists(dir))
            QDir().mkpath(dir);
    }

    filename = dir + "/" + foldernameDateTime.toString("yyyyMMddhhmmss_")
               + AsciiTranslator::toAscii(config->getStationName()) + "_" + QString::number(frequency, 'f', 3) + "MHz_"
               + QString::number(samplerate * 1e-6, 'f', 2) + "Msps_" +
               (config->getIqSaveAs16bit() ? "16bit" : "8bit");

    if (!dataFromFile && config->getIqSaveToFile() && iq16.size()) {
        (void)QtConcurrent::run(&IqPlot::saveIqData,
                          this,
                          iq16);
    }

    QFuture<void> f1000 = QtConcurrent::run(&IqPlot::receiveIqDataWorker,
                                            this,
                                            iq16,
                                            config->getIqFftPlotLength() / 1e6);
}

void IqPlot::receiveIqDataWorker(const QVector<complexInt16> iq, const double secondsToAnalyze)
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
    /*qDebug() << "FFT plot debug: Samples inc." << (int) samplesIteratorInc << ". Iterator starts at"
             << samplesIterator << "and counts up to" << ySize << ". Total samples analyzed"
             << ySize - samplesIterator;*/

    if ((int) samplesIteratorInc == 0)
        samplesIteratorInc = 1;

    double secondsPerLine = secPerSample * samplesIteratorInc;
    fftw_complex in[newFftSize], out[newFftSize];
    fftw_plan plan = fftw_plan_dft_1d(newFftSize, in, out, FFTW_FORWARD, FFTW_ESTIMATE);
    int removeSamples
        = 84.0 * (double) newFftSize
          / 1024.0; // Removing samples to have just above the original sample width visible in the plot

    QVector<double> result;
    QVector<QVector<double>> iqFftResult;

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
        //qInfo() << "Plot worker finished";
    } else {
        qWarning() << "Not enough samples to create IQ FFT plot, giving up";
    }
    emit workerDone();
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
                          const double secondsPerLine)
{
    double min, max, avg;
    findIqFftMinMaxAvg(iqFftResult, min, max, avg);
    //qDebug() << min << max << avg;
    if (config->getIqUseAvgForPlot()) min = avg; // Minimum from fft produces alot of "noise" in the plot, this works like a filter

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

void IqPlot::addText(QImage *image, const double secondsAnalyzed, const double secondsPerLine)
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

    if (config->getIqCreateFftPlot()
        && ( !lastIqRequestTimer.isValid() || lastIqRequestTimer.elapsed() > 120e3 ))
    {
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
        lastIqRequestTimer.restart();
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

void IqPlot::validateHeader(qint64 freq, qint64 bw, qint64 rate)
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

    qDebug() << throwFirstSamples;
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
