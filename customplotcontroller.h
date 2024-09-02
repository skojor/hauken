#ifndef CUSTOMPLOTCONTROLLER_H
#define CUSTOMPLOTCONTROLLER_H

#include <QPen>
#include <QObject>
#include <QVector>
#include <QScrollBar>
#include <QDebug>
#include <QTimer>
#include <QSharedPointer>
#include <QPixmap>
#include "qcustomplot.h"
#include "config.h"
#include "typedefs.h"

/* This class controls all aspects regarding what is displayed on the plot widget
 *
 */

#define overlayTransparency 25

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
    void stopFlashTrigline();
    void updDeviceConnected(bool b);
    void reqTracePlot() { *tracePlot = customPlotPtr->toPixmap(); emit retTracePlot(tracePlot);}
    void freqChanged(double a, double b);
    void resChanged(double a);

private slots:
    void setupBasics();
    void showToolTip(QMouseEvent *event);
    void onMouseClick(QMouseEvent *event);
    void showSelectionMenu(const QRect &rect, QMouseEvent *event);
    void trigInclude();
    void trigExclude();
    void trigIncludeAll();
    void trigExcludeAll();
    void flashRoutine();
    void toggleOverlay();
    void addOverlay(const int graph, const double startfreq, const double stopfreq);
    void updTextLabelPositions();
    void updOverlay();

signals:
    void reqTrigline();
    void freqSelectionChanged();
    void retTracePlot(QPixmap *);

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
    bool deviceConnected = false;
    int plotResolution = 0;
    QPixmap *tracePlot = new QPixmap;
    bool markGnss = false;
    int plotIterator = 0;
    QStringList gnssBands;
    QList<double> gnssBandfrequencies;
    QList<QColor> gnssBandColors;
    QList<QCPItemText *> gnssTextLabels;
    QList<bool> gnssBandsSelectors;
    QList<QCPItemStraightLine *> gnssCenterLine;
};

#endif // CUSTOMPLOTCONTROLLER_H
