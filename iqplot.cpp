#include "iqplot.h"
#include "asciitranslator.h"
#include "fftw3.h"
#include "appclock.h"
#include <QRegularExpression>
#include <QtConcurrent>
#include <QFileInfo>
#include <algorithm>
#include <cmath>
//#include "opencv2/opencv.hpp"

IqPlot::IqPlot(QSharedPointer<Config> c)
{
    config = c;
}

void IqPlot::start()
{
    lastIqRequestTimer = new QTimer(this);
    timeoutTimer = new QTimer(this);
    stopIqStreamTimer = new QTimer(this);

    timeoutTimer->setSingleShot(true);
    stopIqStreamTimer->setSingleShot(true);

    connect(timeoutTimer, &QTimer::timeout, this, [this]() {
        qWarning() << "I/Q transfer timed out."
                   << "samples" << iqSamples.size()
                   << "needed" << samplesNeeded
                   << "remainingFreqsMHz" << listFreqs
                   << "requestedEnd" << flagRequestedEndVifConnection
                   << "headerValidated" << flagHeaderValidated
                   << "metadataFreq" << m_iqMetadata.centerfreq
                   << "metadataBw" << m_iqMetadata.bandwidth
                   << "metadataRate" << m_iqMetadata.samplerate;
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

    /*connect(stopIqStreamTimer, &QTimer::timeout, this, [this] () {
        qDebug() << "STOP";
        receiverControl(); // Stops iq stream or changes ffm center and starts new after given time
    });*/
}

void IqPlot::end()
{
    // Timers are now owned by parent (this), will be deleted automatically
}

void IqPlot::getIqData(const QVector<complexInt16> &iq16)
{
    m_iqMetadata.fromFile = false;
    if (!samplesNeeded)
        samplesNeeded = (int)(config->getIqLogTime() * m_iqMetadata.samplerate);

    if (!listFreqs.isEmpty() && flagHeaderValidated) timeoutTimer->start(IQTRANSFERTIMEOUT_MS); // Restart timer as long as data is flowing and we have work to do
    if (flagHeaderValidated and !throwFirstSamples) {
        const qsizetype oldSize = iqSamples.size();
        iqSamples.resize(oldSize + iq16.size());
        std::copy(iq16.cbegin(), iq16.cend(), iqSamples.begin() + oldSize);
        //qDebug() << "Gathering I/Q cnt" << iqSamples.size();
    }
    else if (flagHeaderValidated and throwFirstSamples) {
        //qDebug() << "Throwing samples" << throwFirstSamples;
        throwFirstSamples--;

    }
    //qDebug() << flagHeaderValidated << throwFirstSamples << iq16.size() << iqSamples.size();

    //qDebug() << iqSamples.size() ;
    if (iqSamples.size() >= samplesNeeded and !listFreqs.isEmpty()) {
        m_iqMetadata.centerfreq = listFreqs.first() * 1e6;

        IqSamplesStruct iqs;
        iqs.metadata = m_iqMetadata;
        iqs.iq = iqSamples;

        iqSamplesVector.append(iqs);
        flagHeaderValidated = false;
        receiverControl(); // Change freq, or end datastream if we are done
        iqSamples.clear();
    }
}

void IqPlot::parseIqData(const QVector<complexInt16> &iq16, IqMetadata meta)
{
    m_iqMetadata = meta;
    secsToAnalyze = config->getIqFftPlotLength() / 1e6;
    if (!m_iqMetadata.samplerate) m_iqMetadata.samplerate = (double) config->getIqFftPlotBw() * 1.28
                     * 1e3;

    QString dir = config->incidentFolder();
    if (m_iqMetadata.fromFile and config->getNewLogFolder()) dir = config->getLogFolder() + "/fromFile";

    QTextStream ts(&filename);

    if (!QDir().exists(dir))
        QDir().mkpath(dir);

    if (!m_iqMetadata.fromFile || filenameFromFile.isEmpty())
        filename = dir + "/" + config->incidentTimestamp().toString("yyyyMMddhhmmss_")
                   + AsciiTranslator::toAscii(config->getStationName()) + "_" + QString::number(m_iqMetadata.centerfreq * 1e-6, 'f', 3) + "MHz_"
                   + QString::number(m_iqMetadata.samplerate * 1e-6, 'f', 2) + "Msps_bw"
                   + QString::number(m_iqMetadata.bandwidth * 1e-3, 'f', 0) + "kHz_"
                   + (config->getIqSaveAs16bit() ? "16bit" : "8bit");
    else
        filename = dir + "/" + filenameFromFile;

    if (!m_iqMetadata.fromFile && config->getIqSaveToFile() && iq16.size()) {
        (void)QtConcurrent::run(&IqPlot::saveIqData,
                                 this,
                                 iq16);
    }
    QVector<QVector<double>> fftData = doFft(iq16);
    iqdataReady(iq16, m_iqMetadata);
    emit fftdataReady(fftData, m_iqMetadata);
}

QVector<QVector<double> > IqPlot::doFft(const QVector<complexInt16> &iq, int samplesToAnalyze)
{
    // New 2.55: Do FFT and send result to PlotAndAnalyze class. Trying to make a system in the madness...
    int samplesIterator = 0;

    switch (m_iqMetadata.samplerate) {
    case 102400000:
        m_iqMetadata.fftSize = 128;
        m_iqMetadata.samplesInc = 24;
        break;
    case 160000000:
        m_iqMetadata.fftSize = 256;
        m_iqMetadata.samplesInc = 48;
        break;
    case 25600000:
        m_iqMetadata.fftSize = 32;
        m_iqMetadata.samplesInc = 6;
        break;
    case 12800000:
        m_iqMetadata.fftSize = 16;
        m_iqMetadata.samplesInc = 2;
        break;
    case 6400000:
        m_iqMetadata.fftSize = 16;
        m_iqMetadata.samplesInc = 2;
        break;
    default:
        m_iqMetadata.fftSize = 64;
        m_iqMetadata.samplesInc = 12;
    }
    fillWindow();

    if (!m_iqMetadata.bandwidth) m_iqMetadata.bandwidth = m_iqMetadata.samplerate / 1.28; // Failsafe if header is not giving us bw data
    int removeSamples =  0.5 * ( m_iqMetadata.samplerate - m_iqMetadata.bandwidth ) / ( m_iqMetadata.samplerate / m_iqMetadata.fftSize );

    fftw_complex *in = (fftw_complex *) fftw_malloc(sizeof(fftw_complex) * m_iqMetadata.fftSize);
    fftw_complex *out = (fftw_complex *) fftw_malloc(sizeof(fftw_complex) * m_iqMetadata.fftSize);
    fftw_plan plan = fftw_plan_dft_1d(m_iqMetadata.fftSize, in, out, FFTW_FORWARD, FFTW_MEASURE);

    QVector<double> result(m_iqMetadata.fftSize - removeSamples * 2);
    QVector<QVector<double>> iqFftResult;
    iqFftResult.reserve(iq.size() / m_iqMetadata.samplesInc);

    int resIterator, fftResultIterator = 0;

    while ( samplesIterator <= iq.size() - m_iqMetadata.fftSize) {
        for (int i = 0; i < m_iqMetadata.fftSize; i++) {
            in[i][0] = ( window[i] * iq[samplesIterator + i].real );
            in[i][1] = ( window[i] * iq[samplesIterator + i].imag );
        }
        fftw_execute(plan); // FFT is done here

        resIterator = 0;
        for (int i = (m_iqMetadata.fftSize / 2) + removeSamples; i < m_iqMetadata.fftSize;
             i++) { // Find magnitude, normalize, reorder and cut edges
            double val = sqrt(out[i][0] * out[i][0] + out[i][1] * out[i][1]);
            if (val == 0) val = 1e-10;
            val = 10 * log10(val);
            result[resIterator++] = val;
        }

        for (int i = 0; i < (m_iqMetadata.fftSize / 2) - removeSamples; i++) {
            double val = sqrt(out[i][0] * out[i][0] + out[i][1] * out[i][1]);
            if (val == 0) val = 1e-10;
            val = 10 * log10(val);
            result[resIterator++] = val;
        }
        iqFftResult.append(result);
        samplesIterator += m_iqMetadata.samplesInc;
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

void IqPlot::fillWindow()
{
    window.resize(m_iqMetadata.fftSize);
    if (config->getIqUseWindow()) {
        for (int i = 0; i < m_iqMetadata.fftSize; i++) {
            window[i] = 0.5 * (1 - cos(2 * M_PI * i / m_iqMetadata.fftSize)); // Hanning / Hann
        }
    } else {
        for (auto &&val : window)
            val = 1; // Effectively no window
    }
}

void IqPlot::requestIqData()
{
    requestIqDataImpl(false);
}

void IqPlot::requestManualIqData()
{
    requestIqDataImpl(true);
}

double IqPlot::fallbackIqCenterFrequency(bool manualRequest)
{
    if (!manualRequest && trigFrequency > 0 && std::isfinite(trigFrequency))
        return trigFrequency;

    const QStringList trigFrequencies = config->getTrigFrequencies();
    for (qsizetype i = 0; i + 1 < trigFrequencies.size(); i += 2) {
        bool startOk = false;
        bool stopOk = false;
        const double startMHz = trigFrequencies.at(i).toDouble(&startOk);
        const double stopMHz = trigFrequencies.at(i + 1).toDouble(&stopOk);
        if (startOk && stopOk && startMHz >= 0 && stopMHz > startMHz
            && std::isfinite(startMHz) && std::isfinite(stopMHz))
            return startMHz + (stopMHz - startMHz) / 2;
    }

    const quint64 pscanStart = config->getInstrStartFreq();
    const quint64 pscanStop = config->getInstrStopFreq();
    if (pscanStop > pscanStart)
        return (pscanStart + (pscanStop - pscanStart) / 2) * 1e-6;

    if (config->getInstrMode().contains("pscan", Qt::CaseInsensitive)
        && centerFrequency > 0 && std::isfinite(centerFrequency))
        return centerFrequency * 1e-6;

    return 0;
}

void IqPlot::requestIqDataImpl(bool manualRequest)
{
    emit reqIqCenterFrequency();
    lastIqRequestTimer->start(75e3); // Reset timer as long as incident is ongoing

    if ((config->getIqCreateFftPlot() || !replayFreqs.isEmpty()) && !flagOngoingAlarm) // New alarm signal received
    {
        flagOngoingAlarm = true;

        samplesNeeded = 0; //(int)(config->getIqLogTime() * (double)config->getIqFftPlotBw() * 1.28 * 1e3);
        requestedTrigFrequency = manualRequest ? 0 : trigFrequency;

        listFreqs.clear();
        if (!replayFreqs.isEmpty()) {
            listFreqs = replayFreqs;
        }
        else if (config->getIqRecordMultipleBands()) {
            for (double frequency : config->getIqMultibandCenterFreqs()) {
                if (frequency > 0 && std::isfinite(frequency))
                    listFreqs.append(frequency);
            }
        }

        if (!config->getIqRecordAllTrigArea() && !manualRequest
            && trigFrequency > 0 && std::isfinite(trigFrequency) && listFreqs.isEmpty())
            listFreqs.append(trigFrequency);

        if (config->getIqRecordAllTrigArea() && replayFreqs.isEmpty()) {
            const QStringList trigFrequencies = config->getTrigFrequencies();
            const double currentBandwidth = config->getIqFftPlotBw() * 1e3;
            for (qsizetype i = 0; i + 1 < trigFrequencies.size(); i += 2) {
                bool startOk = false;
                bool stopOk = false;
                const double start = trigFrequencies.at(i).toDouble(&startOk) * 1e6;
                const double stop = trigFrequencies.at(i + 1).toDouble(&stopOk) * 1e6;
                if (!startOk || !stopOk || !std::isfinite(start) || !std::isfinite(stop)
                    || start < 0 || stop <= start)
                    continue;

                const double range = stop - start;
                if (currentBandwidth > 0 && std::isfinite(currentBandwidth)
                    && range > currentBandwidth) {
                    double f = 1e6 * (int)((start + range / 2) / 1e6);
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
                else if (currentBandwidth > 0 && std::isfinite(currentBandwidth)) {
                    listFreqs.append((start + range / 2) / 1e6);
                }
            }
        }

        if (listFreqs.isEmpty()) {
            const double fallbackFrequency = fallbackIqCenterFrequency(manualRequest);
            if (fallbackFrequency > 0 && std::isfinite(fallbackFrequency))
                listFreqs.append(fallbackFrequency);
        }

        if (listFreqs.isEmpty()) {
            qWarning() << "Unable to request I/Q data: no valid trigger or pscan frequency is available";
            flagOngoingAlarm = false;
            return;
        }

        emit busyRecording(true);
        emit reqVifConnection();
        emit setFfmCenterFrequency(listFreqs.first());
        flagRequestedEndVifConnection = false;
        timeoutTimer->start(IQTRANSFERTIMEOUT_MS);
    }
}

bool IqPlot::readAndAnalyzeFile(const QString fname)
{
    parseFilename(fname);
    QFileInfo info(fname);
    m_iqMetadata.filename = info.completeBaseName();

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
        m_iqMetadata.fromFile = true;

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
                parseIqData(iq16, m_iqMetadata);
            }
        } else {
            QVector<complexInt8> iq8;
            iq8.resize(file.size() / 2);
            if (file.read((char *) iq8.data(), file.size()) == -1)
                qWarning() << "IQ8 read failed:" << file.errorString();
            else
                parseIqData(convertComplex8to16bit(iq8), m_iqMetadata);
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
                m_iqMetadata.centerfreq = val.split("MHz").first().toDouble() * 1e6;
            else if (val.contains("Msps", Qt::CaseInsensitive))
                m_iqMetadata.samplerate = val.split("Msps").first().toDouble() * 1e6;
            else if (val.contains("bw", Qt::CaseInsensitive))
                m_iqMetadata.bandwidth = val.split("kHz").first().toDouble() * 1e3;
        }
        qInfo() << "IQ file reader: Guessing frequency,sample rate and bandwidth from filename:"
                << m_iqMetadata.centerfreq << m_iqMetadata.samplerate << m_iqMetadata.bandwidth;
        if (!file.contains("bw", Qt::CaseInsensitive))
            m_iqMetadata.bandwidth = m_iqMetadata.samplerate / 1.28;

    } else {
        qWarning() << "IQ filename not parsed, setting standard frequency/samplerate values";
        m_iqMetadata.samplerate = 51.2e6;
        m_iqMetadata.centerfreq = 0;
        m_iqMetadata.bandwidth = m_iqMetadata.samplerate  / 1.28;
    }
    m_iqMetadata.fromFile = true;
    m_iqMetadata.timestamp = 0; // Don't have any timestamp here
    m_iqMetadata.trigFrequency = m_iqMetadata.centerfreq; // don't know this one either
}

void IqPlot::updSettings()
{

}

void IqPlot::validateHeader(quint64 freq, quint64 bw, quint64 rate, quint64 timestamp)
{
    const quint32 expectedBw = replayBandwidth ? replayBandwidth : (quint32)(config->getIqFftPlotBw() * 1e3);
    if (!listFreqs.isEmpty()
        && freq == (quint64)(listFreqs.first() * 1e6)
        && bw == expectedBw)
    {
        flagHeaderValidated = true;
        m_iqMetadata.samplerate = rate;
        m_iqMetadata.bandwidth = bw;
        m_iqMetadata.centerfreq = freq;
        m_iqMetadata.timestamp = AppClock::currentMSecsSinceEpoch() * 1e6; // Giving up timestamp from R&S for now, not following start of IQ transfer
        m_iqMetadata.trigFrequency = requestedTrigFrequency;
        if (replaySampleCounts.contains(freq))
            samplesNeeded = qMin((qint64)(config->getIqLogTime() * rate), replaySampleCounts.value(freq));
        else if (!samplesNeeded)
            samplesNeeded = (int)(config->getIqLogTime() * rate);
        const qsizetype requiredSamples = qsizetype(samplesNeeded);
        if (iqSamples.capacity() < requiredSamples)
            iqSamples.reserve(requiredSamples);
        qDebug() << "Timestamp set at" << QDateTime::fromMSecsSinceEpoch(1e-6 * m_iqMetadata.timestamp);
        //qDebug() << "validated at" << QDateTime::currentDateTime().toString("mm:ss:zzz") << ", samplerate:" << rate << freq << bw << timestamp;
    }
    else {
        flagHeaderValidated = false;
        if (!listFreqs.isEmpty()) {
            qDebug() << "I/Q header rejected"
                     << "expectedFreq" << quint64(listFreqs.first() * 1e6)
                     << "expectedBw" << expectedBw
                     << "gotFreq" << freq
                     << "gotBw" << bw
                     << "gotRate" << rate
                     << "pendingFreqsMHz" << listFreqs;
        }

    }
}

void IqPlot::receiverControl()
{
    flagHeaderValidated = false; // Assume future data to be invalid for now

    emit resetTimeoutTimer();

    if (listFreqs.size() > 1) {// we have more work to do
        throwFirstSamples = replayFreqs.isEmpty() ? 12 * m_iqMetadata.samplerate / 1e7 : 0; // replayed files hold no settling samples
        if (!throwFirstSamples && replayFreqs.isEmpty()) throwFirstSamples = 1;
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
        for (auto && iqStruct : iqSamplesVector) {
            parseIqData(iqStruct.iq, iqStruct.metadata); // NEW: Delay data work until all iq gathering is finished
        }
        iqSamplesVector.clear();
    }
}

void IqPlot::setReplayIqPlan(const QVector<double> &freqsMHz, const QVector<qint64> &sampleCounts, quint32 bandwidthHz)
{
    replayFreqs = freqsMHz;
    replayBandwidth = freqsMHz.isEmpty() ? 0 : bandwidthHz;
    replaySampleCounts.clear();
    for (int i = 0; i < freqsMHz.size() && i < sampleCounts.size(); i++)
        replaySampleCounts.insert((quint64)(freqsMHz[i] * 1e6), sampleCounts[i]);
    resetTimer();
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

