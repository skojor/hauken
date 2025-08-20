#ifndef IQPLOT_H
#define IQPLOT_H

#include <QApplication>
#include <QDateTime>
#include <QDebug>
#include <QFile>
#include <QFuture>
#include <QFutureWatcher>
#include <QMutex>
#include <QObject>
#include <QPainter>
#include <QPixmap>
#include <QPromise>
#include <QSharedPointer>
#include <QTimer>
#include <QtConcurrent>
#include "config.h"
#include "fftw3.h"
//#include "typedefs.h"
#include <math.h>


class IqPlot : public QObject
{
    Q_OBJECT
public:
    explicit IqPlot(QSharedPointer<Config> c);

public slots:

private slots:

signals:

private:
    QSharedPointer<Config> config;
};

#endif // IQPLOT_H
