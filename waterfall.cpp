#include "waterfall.h"

#include <cstring>

Waterfall::Waterfall(QSharedPointer<Config> c)
{
    config = c;
}

void Waterfall::start()
{
    updIntervalTimer = new QTimer(this);
    //updIntervalTimer->setSingleShot(true);
    connect(updIntervalTimer, &QTimer::timeout, this, &Waterfall::updTimerCallback);
    updSettings();
}

void Waterfall::receiveTrace(const QVector<double> &trace)
{
    if (!initial) {
        mutex.lock();
        traceCopy = trace;
        mutex.unlock();
        if (!updIntervalTimer->isActive()) {
            updIntervalTimer->start(100); // first call
        }
    }
    else {
        initial--;
    }
}

void Waterfall::requestSnapshot(quint64 recordingId)
{
    QImage snapshot;
    {
        QMutexLocker locker(&mutex);
        snapshot = waterfallImage.copy();
    }
    emit snapshotReady(recordingId, snapshot);
}

void Waterfall::setAutoYRange(double lower, double upper)
{
    scaleMin = lower;
    scaleMax = upper;
    autoRangeReceived = true;
}

void Waterfall::updTimerCallback()
{
    if (!traceCopy.isEmpty() && !waterfallImage.isNull()
        && waterfallImage.width() > 0 && waterfallImage.height() > 0) {
        mutex.lock();
        if (waterfallImage.height() > 1) {
            const size_t rowBytes = static_cast<size_t>(waterfallImage.bytesPerLine());
            std::memmove(waterfallImage.bits() + rowBytes, waterfallImage.constBits(),
                         rowBytes * static_cast<size_t>(waterfallImage.height() - 1));
        }

        QColor color;

        int pixmapSize = waterfallImage.width();
        double ratio = (double) traceCopy.size() / (double) pixmapSize;
        double percent = 0;

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

            waterfallImage.setPixel(x, 0, color.rgba());
        }
        emit imageReady(waterfallImage);
        mutex.unlock(); // done with the exclusive work here
        double interval = 1000.0 / ((double) waterfallImage.height() / waterfallTime);
        updIntervalTimer->start((int) interval);
    }
}

void Waterfall::restartPlot()
{
    mutex.lock();
    if (!waterfallImage.isNull()) waterfallImage.fill(Qt::transparent);
    traceCopy.clear();
    initial = 2;
    if (updIntervalTimer) updIntervalTimer->stop();
    mutex.unlock();
}

void Waterfall::updSize(QSize size)
{
    mutex.lock();
    if (waterfallImage.isNull()) {
        waterfallImage = QImage(size, QImage::Format_ARGB32);
        waterfallImage.fill(Qt::transparent);
    }
    else if (waterfallImage.size() != size) {
        waterfallImage = waterfallImage.scaled(size, Qt::IgnoreAspectRatio, Qt::FastTransformation);
    }
    mutex.unlock();
}

void Waterfall::updSettings()
{
    if (!config->getPlotAutoYScale() || !autoRangeReceived) {
        scaleMin = config->getPlotYMin() + 2; // +2 to clean up the screen when no signal is present
        scaleMax = config->getPlotYMax();
    }
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
}
