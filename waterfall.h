#ifndef WATERFALL_H
#define WATERFALL_H

#include <QWidget>
#include <QPainter>
#include <QDebug>
#include <QSharedPointer>
#include <QPixmap>
#include <QDateTime>
#include "config.h"

class Waterfall : public QWidget
{
    Q_OBJECT
public:
    explicit Waterfall(QSharedPointer<Config> c, QWidget *parent);

public slots:
    void start();
    void receiveTrace(QVector<double> trace);
    QPixmap * retWaterfall() { return pixmap;}
    void updMinScale(int low) { scaleMin = low;}
    void updMaxScale(int high) { scaleMax = high;}
    void updSize(QRect s);

signals:
    void imageReady(QPixmap *);

private slots:
    void redraw();

protected:
    void paintEvent(QPaintEvent *) override;

private:
    QList<QDateTime> traceDateLog;
    QList<double> traceLog;
    QSharedPointer<Config> config;
    QPixmap *pixmap;
    int xBeginOffset, yBeginOffset;
    int scaleMin, scaleMax;
};

#endif // WATERFALL_H
