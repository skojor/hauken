#include "iqplot.h"
#include "asciitranslator.h"
#include "fftw3.h"
#include <QRegularExpression>
#include <QtConcurrent>
#include <QFileInfo>
#include "opencv2/opencv.hpp"

IqPlot::IqPlot(QSharedPointer<Config> c)
{
    config = c;
}

void IqPlot::start()
{
    fillWindow();

    lastIqRequestTimer = new QTimer;
    timeoutTimer = new QTimer;

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

void IqPlot::getIqData(const QVector<complexInt16> iq16)
{
    iqMetadata.fromFile = false;
    if (!samplesNeeded)
        samplesNeeded = (int)(config->getIqLogTime() * samplerate);

    if (!listFreqs.isEmpty() && flagHeaderValidated) timeoutTimer->start(IQTRANSFERTIMEOUT_MS); // Restart timer as long as data is flowing and we have work to do
    qDebug() << flagHeaderValidated << throwFirstSamples << iq16.size() << iqSamples.size();
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
    if (iqMetadata.fromFile and config->getNewLogFolder()) dir = config->getLogFolder() + "/fromFile";

    QTextStream ts(&filename);

    if (!QDir().exists(dir))
        QDir().mkpath(dir);

    if (!iqMetadata.fromFile || filenameFromFile.isEmpty())
        filename = dir + "/" + config->incidentTimestamp().toString("yyyyMMddhhmmss_")
                   + AsciiTranslator::toAscii(config->getStationName()) + "_" + QString::number(frequency, 'f', 3) + "MHz_"
                   + QString::number(samplerate * 1e-6, 'f', 2) + "Msps_bw"
                   + QString::number(bandwidth * 1e-3, 'f', 0) + "kHz_"
                   + (config->getIqSaveAs16bit() ? "16bit" : "8bit");
    else
        filename = dir + "/" + filenameFromFile;

    if (!iqMetadata.fromFile && config->getIqSaveToFile() && iq16.size()) {
        (void)QtConcurrent::run(&IqPlot::saveIqData,
                                 this,
                                 iq16);
    }
    /*QVector<QImage> images = receiveIqDataWorker(iq16, 5e-4, true, 10, true);
    for (auto && img : images)
        img.save("c:/hauken/" + QString::number(QDateTime::currentMSecsSinceEpoch()) + ".png");

    emit imagesForClassification(images);*/

    /*if (config->getEmailAddIqPlot())
        createJpgWithInfo( receiveIqDataWorker(iq16, config->getIqFftPlotLength() * 1e-6, true, 1, false), config->getIqFftPlotLength() * 1e-6 );
    if (config->getEmailAddGif())
        createGif(images);
    if (config->getIqGenerateMovie())
        createMovie( receiveIqDataWorker(iq16, config->getIqFftPlotLength() * 1e-6, false, -1) );
*/
    iqdataReady(iq16, iqMetadata);
    emit fftdataReady( doFft(iq16), iqMetadata );
}

/*QVector<QImage> IqPlot::receiveIqDataWorker(const QVector<complexInt16> &iq, const double secondsToAnalyze, bool findMaximum, int nrOfImages, bool createGray)
{
    QVector<QImage> images;
    int samplesIterator = 0, imageCtr = 0;
    bool repeat = false;
    fftw_complex *in = (fftw_complex *) fftw_malloc(sizeof(fftw_complex) * fftSize);
    fftw_complex *out = (fftw_complex *) fftw_malloc(sizeof(fftw_complex) * fftSize);
    fftw_plan plan = fftw_plan_dft_1d(fftSize, in, out, FFTW_FORWARD, FFTW_MEASURE);

    if (findMaximum)
        samplesIterator = analyzeIqStart(iq) - (samplerate * (secondsToAnalyze / 4)); // Hopefully this is just before where sth interesting happens

    if (nrOfImages != 1)
        repeat = true;

    if (samplesIterator < 0)
        samplesIterator = 0;

    while (samplesIterator > 0
           && samplesIterator + (samplerate * secondsToAnalyze) + fftSize >= iq.size())
        samplesIterator--;

    int ySize = samplerate * secondsToAnalyze + samplesIterator;
    int samplesIteratorInc = 12 * secondsToAnalyze / 5e-4;

    qDebug() << "FFT plot debug: Samples inc." << (int) samplesIteratorInc << ". Iterator starts at"
             << samplesIterator << "and counts up to" << ySize << ". Total samples analyzed"
             << ySize - samplesIterator << bandwidth << samplerate << headerCenterFreq;

    if (!bandwidth) bandwidth = samplerate / 1.28; // Failsafe if header is not giving us bw data
    int removeSamples =  0.5 * ( samplerate - bandwidth ) / ( samplerate / fftSize );

    QVector<double> result(fftSize - removeSamples * 2);
    QVector<QVector<double>> iqFftResult( (secondsToAnalyze * samplerate - (fftSize - samplesIteratorInc)) / samplesIteratorInc );
    bool useDB = config->getIqUseDB();
    double normalize = 1.0 / fftSize;
    do {
        if ( iq.size() > ySize ) {
            int fftIterator = 0, resIterator;
            while ( samplesIterator <= ySize - fftSize) { // KBO note - 2129 lines to have exact 500 us plot
                for (int i = 0; i < fftSize; i++) {
                    in[i][0] = (iq[samplesIterator + i].real * window[i]);
                    in[i][1] = (iq[samplesIterator + i].imag * window[i]);
                }
                fftw_execute(plan); // FFT is done here

                resIterator = 0;
                for (int i = (fftSize / 2) + removeSamples; i < fftSize;
                     i++) { // Find magnitude, normalize, reorder and cut edges
                    double val = sqrt(out[i][0] * out[i][0] + out[i][1] * out[i][1]) * normalize;
                    if (val == 0) val = 1e-9; // log10(0) = -inf
                    if (useDB) val = 10 * log10(val);
                    result[resIterator++] = val;
                }

                for (int i = 0; i < (fftSize / 2) - removeSamples; i++) {
                    double val = sqrt(out[i][0] * out[i][0] + out[i][1] * out[i][1]) * normalize;
                    if (val == 0)
                        val = 1e-9;
                    if (useDB) val = 10 * log10(val);
                    result[resIterator++] = val;
                }

                if (fftIterator >= iqFftResult.size()) // FIXME - Calculated vector size is not always correct for all bw/samplerate combos
                    iqFftResult.append(result);
                else iqFftResult[fftIterator++] =result;
                samplesIterator += samplesIteratorInc;
            }
            //qDebug() << "sizes" << iqFftResult.first().size() << iqFftResult.size();

            ySize += samplerate * secondsToAnalyze - fftSize;
            samplesIterator -= samplesIteratorInc; // Go back x samples on repeated runs
            images.append(createIqPlot(iqFftResult, secondsToAnalyze, createGray));
            imageCtr++;

            if (samplesIterator + samplerate * secondsToAnalyze > iq.size()) repeat = false;
            if (nrOfImages != -1 && imageCtr == nrOfImages) repeat = false;
        } else {
            qDebug() << "Not enough samples to create I/Q plot";
            repeat = false;
        }
    } while(repeat);
    fftw_destroy_plan(plan);
    fftw_free(in);
    fftw_free(out);
    return images;
}*/

QVector<QVector<double> > IqPlot::doFft(const QVector<complexInt16> &iq, int samplesToAnalyze)
{
    // New 2.55: Do FFT and send result to PlotAndAnalyze class. Trying to make a system in the madness...
    int samplesIterator = 0;
    int samplesIteratorInc = 12;
    iqMetadata.fftSize = fftSize;
    iqMetadata.samplerate = samplerate;

    if (!bandwidth) bandwidth = samplerate / 1.28; // Failsafe if header is not giving us bw data
    int removeSamples =  0.5 * ( samplerate - bandwidth ) / ( samplerate / fftSize );

    fftw_complex *in = (fftw_complex *) fftw_malloc(sizeof(fftw_complex) * fftSize);
    fftw_complex *out = (fftw_complex *) fftw_malloc(sizeof(fftw_complex) * fftSize);
    fftw_plan plan = fftw_plan_dft_1d(fftSize, in, out, FFTW_FORWARD, FFTW_MEASURE);

    QVector<double> result(fftSize - removeSamples * 2);
    QVector<QVector<double>> iqFftResult( iq.size() / samplesIteratorInc );

    int resIterator, fftResultIterator = 0;

    while ( samplesIterator <= iq.size() - fftSize) {
        for (int i = 0; i < fftSize; i++) {
            in[i][0] = ( window[i] * iq[samplesIterator + i].real );
            in[i][1] = ( window[i] * iq[samplesIterator + i].imag );
        }
        fftw_execute(plan); // FFT is done here

        resIterator = 0;
        for (int i = (fftSize / 2) + removeSamples; i < fftSize;
             i++) { // Find magnitude, normalize, reorder and cut edges
            double val = sqrt(out[i][0] * out[i][0] + out[i][1] * out[i][1]);
            if (val == 0) val = 1e-10;
            val = 10 * log10(val);
            result[resIterator++] = val;
        }

        for (int i = 0; i < (fftSize / 2) - removeSamples; i++) {
            double val = sqrt(out[i][0] * out[i][0] + out[i][1] * out[i][1]);
            if (val == 0) val = 1e-10;
            val = 10 * log10(val);
            result[resIterator++] = val;
        }
        iqFftResult[fftResultIterator++] = result;
        samplesIterator += samplesIteratorInc;
    }
    fftw_destroy_plan(plan);
    fftw_free(in);
    fftw_free(out);
    return iqFftResult;
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

QImage IqPlot::createIqPlot(const QVector<QVector<double>> &iqFftResult,
                            const double secondsAnalyzed,
                            bool createGray)
{
    double min, max, avg;
    if (iqFftResult.size() > 0)
        findIqFftMinMaxAvg(iqFftResult, min, max, avg);
    else
        return QImage();

    //qDebug() << min << max << avg;
    if (config->getIqUseAvgForPlot()) min = avg; // Minimum from fft produces alot of "noise" in the plot, this works like a filter

    QImage image(iqFftResult.first().size(), iqFftResult.size(), QImage::Format_ARGB32);
    QColor imgColor;
    double percent;
    int x, y = 0;
    int alpha = 127;

    for (auto &&line : iqFftResult) {
        x = 0;
        for (auto &&val : line) {
            percent = (val - min) / (max - min);
            if (percent > 1)
                percent = 1;
            else if (percent < 0)
                percent = 0;
            if (!createGray) imgColor.setHsv(255 - (255 * percent), 255, alpha);
            else imgColor.setHsv(0, 0, 255 * percent);
            image.setPixel(x, y, imgColor.rgba());
            x++;
        }
        y++;
    }
    return image;
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

/*void IqPlot::saveImage(const QImage &image, const double secondsAnalyzed)
{
    if (!filename.isEmpty()) {
        image.save(filename + "_" + QString::number(secondsAnalyzed * 1e6) + "us.jpg", "JPG", 75);
        emit iqPlotReady(filename + "_" + QString::number(secondsAnalyzed * 1e6) + "us.jpg");
    } else {
        image.save(config->getLogFolder() + "/"
                       + QDateTime::currentDateTime().toString("yyMMddhhmmss") + "_"
                       + QString::number(secondsAnalyzed * 1e6) + "us.jpg",
                   "JPG",
                   75);
        emit iqPlotReady(config->getLogFolder() + "/"
                         + QDateTime::currentDateTime().toString("yyMMddhhmmss") + "_"
                         + QString::number(secondsAnalyzed * 1e6) + "us.jpg");
    }
}*/

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

/*quint64 IqPlot::analyzeIqStart(const QVector<complexInt16> &iq)
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
}*/

bool IqPlot::readAndAnalyzeFile(const QString fname)
{
    parseFilename(fname);
    QFileInfo info(fname);
    iqMetadata.filename = info.completeBaseName();

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
        qInfo() << "Reading IQ data from" << fname << "as" << (int16 ? "16 bit" : "8 bit") << "integers";
        iqMetadata.fromFile = true;

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
            else if (val.contains("bw", Qt::CaseInsensitive))
                bandwidth = val.split("kHz").first().toDouble() * 1e3;
        }
        qInfo() << "IQ file reader: Guessing frequency,sample rate and bandwidth from filename:"
                << ffmFrequency << samplerate << bandwidth;
        if (!file.contains("bw", Qt::CaseInsensitive))
            bandwidth = samplerate / 1.28;

    } else {
        qWarning() << "IQ filename not parsed, setting standard frequency/samplerate values";
        samplerate = 51.2e6;
        ffmFrequency = 0;
        bandwidth = samplerate  / 1.28;
    }
    iqMetadata.centerfreq = ffmFrequency * 1e6;
    iqMetadata.bandwidth = bandwidth;
    iqMetadata.samplerate = samplerate;
    iqMetadata.fromFile = true;
    iqMetadata.timestamp = 0; // Don't have any timestamp here
}

void IqPlot::updSettings()
{

}

void IqPlot::validateHeader(quint64 freq, quint64 bw, quint64 rate, quint64 timestamp)
{
    if (!listFreqs.isEmpty()
        && freq == (quint64)(listFreqs.first() * 1e6)
        && bw == (quint32)(config->getIqFftPlotBw() * 1e3))
    {
        flagHeaderValidated = true;
        samplerate = rate;
        bandwidth = bw;
        headerCenterFreq = freq;
        iqMetadata.centerfreq = freq;
        iqMetadata.samplerate = rate;
        iqMetadata.bandwidth = bw;
        iqMetadata.timestamp = timestamp;
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

    //qDebug() << throwFirstSamples;
    emit resetTimeoutTimer();

    if (listFreqs.size() > 1) {// we have more work to do
        throwFirstSamples = 2;// * samplerate / 1e7;
        if (!throwFirstSamples) throwFirstSamples = 1;
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

