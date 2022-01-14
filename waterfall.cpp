#include "waterfall.h"

Waterfall::Waterfall(QSharedPointer<Config> c)
{
    config = c;
}

void Waterfall::start()
{
    xBeginOffset = 0;
    yBeginOffset = 0;
    testDraw = new QTimer;
    connect(testDraw, &QTimer::timeout, this, [this]
    {
        QVector<double> test;
        for (int i=0; i<config->getPlotResolution(); i++)
            test.append(-2.5 + sin(i));
        receiveTrace(test);
    });
    testDraw->start(2000);
}

void Waterfall::receiveTrace(QVector<double> trace)
{
    pixmap->scroll(0, 1, pixmap->rect());
    QPainter painter(pixmap);
    QColor color;
    qDebug() << trace.size() << pixmap->size().width() << pixmap->size().height() << scaleMax << scaleMin;
    int size = trace.size();
    for (int x=0; x<pixmap->width(); x+= size/pixmap->width()) {
        int percent = (trace.at(x) - scaleMin) / (scaleMax - scaleMin); // 0 - 100 % range
        int hue = 240 - (240 * percent);
        color.setHsv(hue, 180, 255, 127);
        qDebug() << hue;
        painter.setPen(color);
        painter.drawPoint(x, 0);
    }

    //update();
    emit imageReady(pixmap);
}

/*void Waterfall::paintEvent(QPaintEvent *)
{
    QPainter painter(pixmap);
    painter.drawPixmap(xBeginOffset, yBeginOffset, *pixmap);
}*/

void Waterfall::redraw()
{

}

void Waterfall::updSize(QRect s)
{
    QSize size(s.width() + s.left(), s.height() + s.top());
    xBeginOffset = s.left();
    yBeginOffset = s.top();
    if (pixmap != nullptr)
        delete pixmap;
    pixmap = new QPixmap(size);
    pixmap->fill(Qt::yellow);
    qDebug() << pixmap->width() << pixmap->height();
    redraw();
    //this->resize(size);
    //qDebug() << this->width() << this->height() << s.height() << s.bottom() << s.top();
}
