#ifndef CUSTOMPLOTCONTROLLER_H
#define CUSTOMPLOTCONTROLLER_H

#include <QPen>
#include <QObject>
#include <QVector>
#include <QScrollBar>
#include <QDebug>
#include <QTimer>
#include <QSharedPointer>
#include "qcustomplot.h"
#include "config.h"
#include "typedefs.h"

/* This class controls all aspects regarding what is displayed on the plot widget
 *
 */

class CustomPlotController : public QObject
{
    Q_OBJECT
public:
    CustomPlotController(QCustomPlot *ptr, QSharedPointer<Config> c);

public slots:
    void init();
    void reCalc();
    void plotTrace(const QVector<double> &);
    void plotMaxhold(const QVector<double> &);
    void plotTriglevel(const QVector<double> &data);
    void showMaxhold(const bool b) { customPlotPtr->graph(1)->setVisible(b);}
    void readTrigSelectionFromConfig();
    void saveTrigSelectionToConfig();
    void doReplot();
    void updSettings();
    void flashTrigline();
    void stopFlashTrigline(const QVector<qint16>);

private slots:
    void setupBasics();
    void showToolTip(QMouseEvent *event);
    void showSelectionMenu(const QRect &rect, QMouseEvent *event);
    void trigInclude();
    void trigExclude();
    void trigIncludeAll();
    void trigExcludeAll();
    void flashRoutine();

signals:
    void reqTrigline();
    void freqSelectionChanged();

private:
    QSharedPointer<Config> config;
    QVector<double> fill;
    QVector<double> freqSelection;
    QCustomPlot *customPlotPtr;
    QScrollBar *scrollBarMax = new QScrollBar;
    QScrollBar *scrollBarMin = new QScrollBar;
    QVector<double>keyValues;
    int nrOfValues = 0;
    double startFreq = -1, stopFreq = -1, resolution = 0;
    double selMin, selMax;
    QTimer *flashTimer = new QTimer;

    QColor colorNormal = QColor(0, 150, 0, 40);
    QColor colorFlash = QColor(0, 150, 0, 20);
    bool flip = true;
    const int screenResolution = 600;
};

#endif // CUSTOMPLOTCONTROLLER_H
