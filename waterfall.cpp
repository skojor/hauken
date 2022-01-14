#include "waterfall.h"

Waterfall::Waterfall(QSharedPointer<Config> c)
{
    config = c;
}

void Waterfall::start()
{
    testDraw = new QTimer;
    connect(testDraw, &QTimer::timeout, this, [this]
    {
        QVector<double> test;
        for (int i=0; i<config->getPlotResolution(); i++)
            test.append(-2.5 + sin(i));
        receiveTrace(test);
    });
    //testDraw->start(2000);
    updIntervalTimer = new QTimer;
    updIntervalTimer->setSingleShot(true);
    connect(updIntervalTimer, &QTimer::timeout, this, &Waterfall::updTimerCallback);
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

    for (int x=0; x<pixmapSize; x++) {
        double percent = (traceCopy.at((int)iterator) - scaleMin) / (scaleMax - scaleMin); // 0 - 1 range
        if (percent < 0) percent = 0;
        else if (percent > 1) percent = 1;

        iterator += ratio;
        int hue = 240 - (240 * percent);
        color.setHsv(hue, 180, 255, 65);

        QPen pen;
        pen.setColor(color);
        pen.setWidth(1);
        painter.setPen(pen);
        painter.drawPoint(x, 0);
    }
    emit imageReady(pixmap);
    double interval = 1000.0 / ((double)pixmap->height() / 120.0);
    updIntervalTimer->start((int)interval);
}

void Waterfall::redraw()
{

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
    redraw();
}

void Waterfall::updSettings()
{
    scaleMin = config->getPlotYMin();
    scaleMax = config->getPlotYMax();
    if (startfreq != config->getInstrStartFreq() || stopfreq != config->getInstrStopFreq() || !resolution.contains(config->getInstrResolution()) || !fftMode.contains(config->getInstrFftMode())) {
        startfreq = config->getInstrStartFreq();
        stopfreq = config->getInstrStopFreq();
        resolution = config->getInstrResolution();
        fftMode = config->getInstrFftMode();
        restartPlot();
    }
}
