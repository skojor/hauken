#include "waterfall.h"

Waterfall::Waterfall(QSharedPointer<Config> c, QWidget *parent)
    : QWidget(parent)
{
    config = c;
}

void Waterfall::start()
{
    xBeginOffset = 0;
    yBeginOffset = 0;
}

void Waterfall::receiveTrace(QVector<double> trace)
{
    pixmap->scroll(0, 1, pixmap->rect());
    QPainter painter(pixmap);
    qDebug() << trace.size() << pixmap->size().width() << pixmap->size().height() << scaleMax << scaleMin << this->width();
    int size = trace.size();
    for (int x=0; x<this->width(); x+= size/this->width()) {

        QColor c(Qt::green);
        painter.setPen(c);
        painter.drawPoint(x, 0);
    }

    update();
    emit imageReady(pixmap);
}

void Waterfall::paintEvent(QPaintEvent *)
{
    QPainter painter(this);
    painter.drawPixmap(xBeginOffset, yBeginOffset, *pixmap);
}

void Waterfall::redraw()
{

}

void Waterfall::updSize(QRect s)
{
    QSize size(s.width() + s.left(), s.height() + s.top());
    xBeginOffset = s.left();
    yBeginOffset = s.top();
    pixmap = new QPixmap(size);
    pixmap->fill(Qt::yellow);
    redraw();
    this->resize(size);
    qDebug() << this->width() << this->height() << s.height() << s.bottom() << s.top();
}
