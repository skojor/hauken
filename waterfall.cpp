#include "waterfall.h"

Waterfall::Waterfall(QSharedPointer<Config> c)
{
    config = c;
}

void Waterfall::start()
{
    pixmap = new QPixmap;
    updIntervalTimer = new QTimer;
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

void Waterfall::updTimerCallback()
{
    if (!traceCopy.isEmpty()) {
        mutex.lock(); // this needs exclusive access to containers and pixmap
        pixmap->scroll(0, 1, pixmap->rect());

        QImage image = pixmap->toImage();
        QColor color;

        int pixmapSize = pixmap->width();
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
}
