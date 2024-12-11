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

    fftSize = 64;
    imageYSize = fftSize * 16;

    fillWindow();
    //delete in, out;

    // Zero-padding/test

}

void Waterfall::receiveIqData(QList<qint16> cmpI, QList<qint16> cmpQ)
{
    int samplesIterator = analyzeIqStart(cmpI, cmpQ) - 1000; // Hopefully this is just before where sth interesting happens
    if (samplesIterator < 0) samplesIterator = 0;

    QList<double> result;

    while (samplesIterator > 0 && samplesIterator + (samplerate * secsToAnalyze) > cmpI.size())
        samplesIterator --;

    int ySize = samplerate * secsToAnalyze + samplesIterator;

    double secPerSample = 1.0 / samplerate;
    double samplesIteratorInc = (double)(samplerate * secsToAnalyze) / (double)imageYSize;
    qDebug() << "samples inc:" << samplesIteratorInc << "iterator starts at" << samplesIterator << "ysize" << ySize << "total samples analyzed" << ySize - samplesIterator;
    if ((int)samplesIteratorInc == 0) samplesIteratorInc = 1;

    secsPerLine = secPerSample * samplesIteratorInc;
    int newFftSize = fftSize * 16;
    in = new fftw_complex[newFftSize];
    out = new fftw_complex[newFftSize];
    fftw_plan plan = fftw_plan_dft_1d(newFftSize, in, out, FFTW_FORWARD, FFTW_MEASURE);

    for (int i = 0; i < newFftSize; i++) {
        for (int j = 0; j < 4; j++) {
            in[i][0] = 0;
            in[i][1] = 0;
        }
    }

    int removeSamples = 112.0 * (double)newFftSize / 1024.0;
    //result.resize(newFftSize);

    if (cmpI.size() > ySize && cmpQ.size() > ySize) {
        while (samplesIterator < ySize) {
            for (int i = (newFftSize / 2) - (fftSize / 2), j = 0; i < (newFftSize / 2) + (fftSize / 2); i++, j++) {
                in[i][0] = cmpI[samplesIterator + i];// * window[j];
                in[i][1] = cmpQ[samplesIterator + i];// * window[j];
            }

            fftw_execute(plan);

            for (int i = (newFftSize / 2) + removeSamples; i < newFftSize; i++) { // Find magnitude, normalize, reorder and cut edges
                result.append( sqrt(out[i][0] * out[i][0] + out[i][1] * out[i][1]) * (1.0 / newFftSize) );
            }
            for (int i = 0; i < (newFftSize / 2) - removeSamples; i++) {
                result.append( sqrt(out[i][0] * out[i][0] + out[i][1] * out[i][1]) * (1.0 / newFftSize) );
            }

            storeIqTrace(result);
            result.clear();

            samplesIterator += (int)samplesIteratorInc;
            QApplication::processEvents();
        }
        //qDebug() << "size" << iqFftResult.size() << iqFftResult.last().size();
        fftw_destroy_plan(plan);
        createIqPlot();

        if (!filename.isEmpty()) { // store IQ to file
            QFile file(filename + ".iq");
            if (!file.open(QIODevice::WriteOnly))
                qDebug() << "Could not open" << filename + ".iq" << "for writing IQ data, aborting";
            else {
                QDataStream ds(&file);
                for (int i = 0; i < cmpI.size(); i++)
                    ds << cmpI[i] << cmpQ[i];
                file.close();
            }
        }
        else {
            QFile file(config->getLogFolder() + "/" + QDateTime::currentDateTime().toString("yyMMddhhmmss") + "_iqRawInt16");
            if (!file.open(QIODevice::WriteOnly))
                qDebug() << "Could not open" << filename + ".iq" << "for writing IQ data, aborting";
            else {
                QDataStream ds(&file);
                for (int i = 0; i < cmpI.size(); i++)
                    ds << cmpI[i] << cmpQ[i];
                file.close();
            }
        }
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
    //if (max < 10) max = 10; // in case of no signal
    int hSize = iqFftResult.first().size();

    QPixmap pixmap(QSize(hSize, iqFftResult.size()));
    //qDebug() << pixmap.size();
    QPainter painter(&pixmap);
    QColor color;
    QPen pen;
    pen.setWidth(1);

    double percent = 0;
    int x, y = 0;

    for (auto && line : iqFftResult) {
        x = 0;
        for (auto && val : line) {
            percent = ((double)val - min) / (max - min);
            percent *= 2;
            if (percent > 1) percent = 1;
            else if (percent < 0) percent = 0;
            //color.setHsv(180, 0, 255 - (255 * percent), 255);
            color.setHsv(255 - (255 * percent), 255, 127);
            painter.setPen(pen);
            pen.setColor(color);
            painter.drawPoint(x, y);
            x++;
        }
        y++;
        QApplication::processEvents();
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
    window.resize(fftSize);

    double a0, a1, a2, a3, a4, f;
    a0 = 1, a1 = 1.93, a2 = 1.29, a3 = 0.388, a4 = 0.028, f;
    for (int i = 0; i < fftSize; i++) {
        f = 2 * M_PI * i / (fftSize - 1);
        window[i] = a0 - a1 * cos(f) + a2 * cos(2*f) - a3 * cos(3*f) + a4 * cos(4*f);   /// Flat-top window
    }

    /*a0 = 0.35875, a1 = 0.48829, a2 = 0.14128, a3 = 0.01168, f; // Blackman-Harris
    for (int i = 0; i < fftSize; i++) {
        f = 2 * M_PI * i / (fftSize - 1);
        window[i] = a0 - a1 * cos(f) + a2 * cos(2*f) - a3 * cos(3*f);
    }*/
    /*for (int i = 0; i < fftSize; i++) {
        window[i] = 0.5 * (1 - cos(2 * M_PI * i / fftSize));          // Hanning / Hann
    }*/
    /*for (int i = 0; i < fftSize; i++) {
        window[i] = 0.54 - 0.46 * cos(2*M_PI * i / (fftSize -1));
    }*/
}

void Waterfall::addLines(QPixmap *pixmap)
{
    QPen pen;
    QPainter painter(pixmap);
    pen.setWidth(1);
    pen.setStyle(Qt::DotLine);
    pen.setColor(Qt::white);
    painter.setPen(pen);
    int height = pixmap->size().height();
    int width = pixmap->size().width();
    int xMinus = 25;
    if (fftSize == 256) {
        xMinus = 1;
    }
    else if (fftSize == 512) {
        xMinus = 5;
    }
    else if (fftSize == 1024) {
        xMinus = 10;
    }
    painter.drawLine(width / 2, 5, width / 2, height);
    painter.drawLine(xMinus, 5, xMinus, height);
    painter.drawLine(width - xMinus, 5, width - xMinus, height);

    int lineDistance = (secsToAnalyze / 10) / secsPerLine;

    for (int y = 0; y < height; y ++) {
        if (y % lineDistance == 0 && y > 0)
            painter.drawLine(0, y, width, y);
    }
    painter.end();

}

void Waterfall::addText(QPixmap *pixmap)
{
    QPen pen;
    QPainter painter(pixmap);

    pen.setWidth(10);
    pen.setColor(Qt::white);
    int fontSize = 14, xMinus = 60, yMinus = 25;

    if (fftSize == 256) {
        fontSize = 5;
        xMinus = 0;
        yMinus = 8;
    }
    else if (fftSize == 512) {
        fontSize = 8;
        xMinus = 15;
        yMinus = 10;
    }
    else if (fftSize == 1024) {
        fontSize = 10;
        xMinus = 30;
        yMinus = 12;
    }

    painter.setFont(QFont("Arial", fontSize));
    painter.setPen(pen);
    painter.drawText((pixmap->size().width() / 2) - xMinus - 10, yMinus, QString::number(ffmFrequency) + " MHz");
    painter.drawText(0, yMinus, QString::number((int)(-samplerate / 1.28 / 2e6)) + " MHz");
    painter.drawText((pixmap->size().width()) - (xMinus + 30), yMinus, "+ " + QString::number((int)(samplerate / 1.28 / 2e6)) + " MHz");

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
    if (!filename.isEmpty()) {
        pixmap->save(filename + "_plot.jpg");
        emit iqPlotReady(filename + "_plot.jpg");
    }
    else {
        pixmap->save(config->getLogFolder() + "/" + QDateTime::currentDateTime().toString("yyMMddhhmmss") + "_plot.jpg");
        emit iqPlotReady(config->getLogFolder() + "/noname_plot.jpg");
    }

}

void Waterfall::requestIqData()
{
    if (!lastIqRequestTimer.isValid() || lastIqRequestTimer.elapsed() > 900e3) {
        int samplesNeeded = samplerate * 0.5; // DL 0.x seconds of IQ data. This way we have sth to find intermittent signals inside
        emit requestIq(samplesNeeded);
        lastIqRequestTimer.restart();
    }
}

int Waterfall::analyzeIqStart(const QList<qint16> cmpI, const QList<qint16> cmpQ)
{
    qint16 max = -32700;
    int locMax = -1;
    for (int i = 0; i < cmpI.size(); i++) {
        int val = sqrt(cmpI[i] * cmpI[i] + cmpQ[i] * cmpQ[i]);
        if (val > max) {
            max = val;
            locMax = i;
        }
    }
    return locMax;
}

void Waterfall::readAndAnalyzeFile(QString fname, bool isInt16, double timeToAnalyze)
{
    QList<qint16>cmpI, cmpQ;

    QFile file(fname);
    if (!file.open(QIODevice::ReadOnly)) {
        qDebug() << "IQ plot: Cannot open file" << fname <<", giving up";
    }
    else {
        qDebug() << "Reading IQ data from" << fname << "as" << (isInt16?"16 bit":"8 bit") << "integers";
        int filesize = file.size();

        QDataStream ds(&file);
        if (isInt16) {
            cmpI.resize(filesize / 4);
            cmpQ.resize(filesize / 4);
            int iter = 0;
            qint16 i, q;

            while (!ds.atEnd()) {
                ds >> i >> q;
                cmpI[iter] = i;
                cmpQ[iter++] = q;
                if (iter % 100 == 0) QApplication::processEvents();
            }
        }
        else {
            cmpI.resize(filesize / 2);
            cmpQ.resize(filesize / 2);
            int iter = 0;
            qint8 i, q;

            while (!ds.atEnd()) {
                ds >> i >> q;
                cmpI[iter] = i;
                cmpQ[iter++] = q;
                if (iter % 100 == 0) QApplication::processEvents();
            }
        }
        //qDebug() << "We got" << cmpI.size() << "value pairs, going to work";
        filename.clear();
        file.close();

        secsToAnalyze = timeToAnalyze;
        receiveIqData(cmpI, cmpQ);
    }
}
