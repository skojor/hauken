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
    fillWindow();
    updSettings();
}

void Waterfall::receiveTrace(const QVector<double> &trace)
{
    if (timeout) {
        timeout = false;
        mutex.lock();
        traceCopy = trace;
        mutex.unlock();
        if (!updIntervalTimer->isActive()) updIntervalTimer->start(100); // first call
    }
}

void Waterfall::updTimerCallback()
{
    if (!traceCopy.isEmpty()) {
        timeout = true;
        mutex.lock(); // this needs exclusive access to containers and pixmap pointer.
        pixmap->scroll(0, 1, pixmap->rect());
        QPainter painter(pixmap);
        QColor color;
        QPen pen;
        pen.setWidth(1);

        int traceSize = traceCopy.size();
        int pixmapSize = pixmap->width();
        double ratio = (double)traceSize / (double)pixmapSize;
        double percent = 0;

        for (int x = 0; x < pixmapSize; x++) {
            percent = (traceCopy.at((int)(ratio * x)) - scaleMin) / (scaleMax - scaleMin); // 0 - 1 range

            if (percent < 0) percent = 0;
            else if (percent > 1) percent = 1;

            if (colorset == COLORS::GREY)
                color.setHsv(180, 0, 255 - (255 * percent), 255);
            else if (colorset == COLORS::BLUE)
                color.setHsv(240, (255 * percent), 255, 255);
            else if (colorset == COLORS::RED)
                color.setHsv(0, (255 * percent), 255, 255);
            else
                color.setHsv(190 - (190 * percent), 180, 255, 65);

            painter.setPen(pen);
            pen.setColor(color);
            painter.drawPoint(x, 0);

        }

        emit imageReady(pixmap);
        mutex.unlock(); // done with the exclusive work here
        double interval = 1000.0 / ((double)pixmap->height() / waterfallTime);
        updIntervalTimer->start((int)interval);
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
            colorset = COLORS::GREY;
        else if (mode == "Red")
            colorset = COLORS::RED;
        else if (mode == "Blue")
            colorset = COLORS::BLUE;
        else if (mode == "Pride")
            colorset = COLORS::PRIDE;
        /*else
            colorset = COLORS::OFF;*/
    }
    secsToAnalyze = config->getInstrFftPlotLength() / 1e6;
    samplerate = (double)config->getInstrFftPlotBw() * 1.28 * 1e3; // TODO: Is this universal for all R&S instruments?
}

void Waterfall::receiveIqData(QList<qint16> cmpI, QList<qint16> cmpQ)
{
    qDebug() << "Rec IQ bytes:" << cmpI.size() << cmpQ.size();
    int samplesIterator = 50; // Skip first 50 samples because ... we can?
    fftw_complex in[FFT_SIZE], out[FFT_SIZE];
    fftw_plan plan = fftw_plan_dft_1d(FFT_SIZE, in, out, FFTW_FORWARD, FFTW_ESTIMATE);
    QList<double> result;
    int ySize = (samplerate * secsToAnalyze) + samplesIterator; // This should give us x ms of samples to work with
    double secPerSample = 1.0 / samplerate;
    int samplesIteratorInc = ySize / 1024;
    secsPerLine = secPerSample * samplesIteratorInc;
    if (cmpI.size() > ySize && cmpQ.size() > ySize) {
        while (samplesIterator < ySize) {
            for (int i = 0; i < FFT_SIZE; i++) {
                in[i][0] = cmpI[samplesIterator + i] * window[i];
                in[i][1] = cmpQ[samplesIterator + i] * window[i];
            }
            fftw_execute(plan);
            result.clear();

            for (int i = FFT_SIZE / 2; i < FFT_SIZE; i++) { // Find magnitude, normalize and reorder
                result.append( sqrt(out[i][0] * out[i][0] + out[i][1] * out[i][1]) * (1.0 / FFT_SIZE) );
            }
            for (int i = 0; i < FFT_SIZE / 2; i++) {
                result.append( sqrt(out[i][0] * out[i][0] + out[i][1] * out[i][1]) * (1.0 / FFT_SIZE) );
            }

            storeIqTrace(result);

            samplesIterator += samplesIteratorInc;
        }
        createIqPlot();
    }
    else {
        qDebug() << "Not enough samples to create IQ FFT plot, giving up";
    }

}

void Waterfall::storeIqTrace(QList<double> res)
{
    iqFftResult.append(res);
    //qDebug() << "Res" << iqFftResult.size();
}

void Waterfall::createIqPlot()
{
    double min, max;
    findIqFftMinMax(min, max);

    QPixmap pixmap(QSize(FFT_SIZE, iqFftResult.size()));
    QPainter painter(&pixmap);
    QColor color;
    QPen pen;
    pen.setWidth(1);

    double percent = 0;
    int x = 0, y = 0;

    for (auto && line : iqFftResult) {
        x = 0;
        for (auto && val : line) {
            percent = ((double)val - min) / (max - min);
            percent *= 2.5;
            if (percent > 1) percent = 1;
            else if (percent < 0) percent = 0;
            //color.setHsv(180, 0, 255 - (255 * percent), 255);
            color.setHsv(255 - (255 * percent), 255, 127);
            painter.setPen(pen);
            pen.setColor(color);
            painter.drawPoint(x++, y);
        }
        y++;
    }
    painter.end();
    addLines(&pixmap);
    addText(&pixmap);
    saveImage(&pixmap);
    iqFftResult.clear();
}

void Waterfall::findIqFftMinMax(double &min, double &max)
{
    min = 99e9;
    max = -99e9;

    for (auto && line : iqFftResult) {
        for (auto && val : line) {
            if (val < min) min = val;
            else if (val > max) max = val;
        }
    }
}

void Waterfall::fillWindow()
{
    double a0, a1, a2, a3, f;
    window.resize(FFT_SIZE);

    a0 = 0.35875, a1 = 0.48829, a2 = 0.14128, a3 = 0.01168, f; // Blackman-Harris
    for (int i = 0; i < FFT_SIZE; i++) {
        f = 2 * M_PI * i / (FFT_SIZE - 1);
        window[i] = a0 - a1 * cos(f) + a2 * cos(2*f) - a3 * cos(3*f);
    }
}

void Waterfall::addLines(QPixmap *pixmap)
{
    QPen pen;
    QPainter painter(pixmap);
    pen.setWidth(1);
    pen.setStyle(Qt::DotLine);
    pen.setColor(Qt::white);
    painter.setPen(pen);
    painter.drawLine(pixmap->size().width() / 2, 25, pixmap->size().width() / 2, pixmap->size().height());
    painter.drawLine(25, 25, 25, pixmap->size().height());
    painter.drawLine(pixmap->size().width() - 25, 25, pixmap->size().width() - 25, pixmap->size().height());

    int lineDistance = (secsToAnalyze / 10) / secsPerLine;

    for (int y = 0; y < pixmap->size().height(); y ++) {
        if (y % lineDistance == 0 && y > 0)
            painter.drawLine(0, y, pixmap->size().width(), y);
    }
    painter.end();

}

void Waterfall::addText(QPixmap *pixmap)
{
    QPen pen;
    QPainter painter(pixmap);

    pen.setWidth(10);
    pen.setColor(Qt::white);

    painter.setFont(QFont("Arial", 14));
    painter.setPen(pen);
    painter.drawText((pixmap->size().width() / 2) - 30, 25, QString::number(ffmFrequency) + " MHz");
    painter.drawText(0, 25, QString::number((int)(-samplerate / 2e6)) + " MHz");
    painter.drawText((pixmap->size().width()) - 90, 25, "+ " + QString::number((int)(samplerate / 2e6)) + " MHz");

    int lineDistance = (secsToAnalyze / 10) / secsPerLine;
    int microsecCtr = 1e6 * secsToAnalyze / 10;
    for (int y = 0; y < pixmap->size().height(); y ++) {
        if (y % lineDistance == 0 && y > 0) {
            painter.drawText(0, y, QString::number(microsecCtr) + " µs");
            microsecCtr += 1e6 * secsToAnalyze / 10;
        }
    }

    painter.end();
}

void Waterfall::saveImage(QPixmap *pixmap)
{
    pixmap->save("c:/hauken/out.jpg");
}
