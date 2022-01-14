#ifndef WATERFALL_H
#define WATERFALL_H

#include <QObject>
#include <QPainter>
#include <QDebug>
#include <QSharedPointer>
#include <QPixmap>
#include <QDateTime>
#include <QTimer>
#include <math.h>
#include "config.h"

class Waterfall : public QObject
{
    Q_OBJECT
public:
    explicit Waterfall(QSharedPointer<Config> c);

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
    //void paintEvent(QPaintEvent *) override;

private:
    QTimer *testDraw;
    QList<QDateTime> traceDateLog;
    QList<double> traceLog;
    QSharedPointer<Config> config;
    QPixmap *pixmap = nullptr;
    int xBeginOffset, yBeginOffset;
    int scaleMin, scaleMax;
};

#endif // WATERFALL_H
