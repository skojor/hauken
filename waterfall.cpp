#include "waterfall.h"

Waterfall::Waterfall(QSharedPointer<Config> c)
{
    config = c;
}

void Waterfall::start()
{
    updIntervalTimer = new QTimer;
    updIntervalTimer->setSingleShot(true);
    connect(updIntervalTimer, &QTimer::timeout, this, &Waterfall::updTimerCallback);

/*    testDraw = new QTimer;
    connect(testDraw, &QTimer::timeout, this, [this] {
        QVector<double> tmp;
        for (int i=0; i<config->getPlotResolution(); i++) {
           tmp.append(double((rand()) % 100 - 30) / 10.0);
        }
        this->receiveTrace(tmp);
        if (!updIntervalTimer->isActive()) this->updIntervalTimer->start();
    });
    testDraw->start(200);*/
}

void Waterfall::receiveTrace(const QVector<double> &trace)
{
    traceCopy = trace;
    if (!updIntervalTimer->isActive()) updIntervalTimer->start(100); // first call
}

void Waterfall::updTimerCallback()
{
    pixmap->scroll(0, 1, pixmap->rect());
    QPainter painter(pixmap);
    QColor color;

    int traceSize = traceCopy.size();
    int pixmapSize = pixmap->width();
    double iterator = 0;
    double ratio = (double)traceSize / (double)pixmapSize;
    QPen pen;
    painter.setPen(pen);
    pen.setWidth(1);

    for (int x=0; x<pixmapSize; x++) {
        double percent = (traceCopy.at((int)iterator) - scaleMin) / (scaleMax - scaleMin); // 0 - 1 range
        iterator += ratio;

        if (percent < 0) percent = 0;
        else if (percent > 1) percent = 1;
        if (!greyscale)
            color.setHsv(190 - (190 * percent), 180, 255, 65);
        else
            color.setHsv(180, 0, 255 - (255 * percent), 255);
        //qDebug() << percent << traceCopy.at((int)iterator) << scaleMin << scaleMax << 190 - (190 * percent);

        pen.setColor(color);
        //pen.setColor(Qt::red);
        painter.drawPoint(x, 0);
    }
    emit imageReady(pixmap);
    updIntervalTimer->start((int)(1000.0 / ((double)pixmap->height() / waterfallTime)));
}

void Waterfall::restartPlot()
{
    QSize size(640, 480);
    if (pixmap != nullptr) {
        size = pixmap->size();
        delete pixmap;
    }
    pixmap = new QPixmap(size);
}

void Waterfall::updSize(QRect s)
{
    QSize size(s.width(), s.height());
    if (pixmap != nullptr)
        delete pixmap;
    pixmap = new QPixmap(size);
}

void Waterfall::updSettings()
{
    scaleMin = config->getPlotYMin();
    scaleMax = config->getPlotYMax();
    if (startfreq != config->getInstrStartFreq() || stopfreq != config->getInstrStopFreq()
            || !resolution.contains(config->getInstrResolution()) || !fftMode.contains(config->getInstrFftMode())) {
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
            greyscale = true;
        else if (mode == "Pride")
            greyscale = false;
    }
}
