#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include "config.h"
#include <QMainWindow>
#include <QApplication>
#include <QToolBar>
#include <QAction>
#include <QDebug>
#include <QHostAddress>
#include <QMessageBox>
#include <QString>
#include <QTextStream>
#include <QStandardPaths>
#include <QFileDialog>
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QDateTime>
#include <QThread>
#include <QSpinBox>
#include <QDoubleSpinBox>
#include <QSharedPointer>
#include "qcustomplot.h"
#include "measurementdevice.h"
#include "typedefs.h"
#include "generaloptions.h"
#include "receiveroptions.h"
#include "sdefoptions.h"
#include "customplotcontroller.h"
#include "tracebuffer.h"
#include "traceanalyzer.h"
#include "sdefrecorder.h"

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

public slots:
    void appendToIncidentLog(QString incident);
    void resizeEvent(QResizeEvent *event); // override to save changes to win size
    void moveEvent(QMoveEvent *event); // override to save position
protected:

private slots:
    void createMenus();
    void createActions();
    void createLayout();
    void setToolTips();
    void getConfigValues();
    void saveConfigValues();
    void instrAutoAttChanged();
    void instrModeChanged(int a = 0);
    void updInstrButtonsStatus();
    void setResolutionOrSpan();
    void setValidators();
    void setSignals();

    void instrStartFreqChanged();
    void instrStopFreqChanged();
    void instrResolutionChanged(int a = -1);
    void instrMeasurementTimeChanged();
    void instrAttChanged();
    void instrAntPortChanged(int a = 0);
    void instrFftModeChanged(int a = 0);
    void instrIpChanged();
    void instrPortChanged();
    void instrConnected(bool state);
    void instrAutoConnect();
    bool instrCheckSettings();

    void generatePopup(const QString msg);
    void updateStatusLine(const QString msg);
    void updWindowTitle(const QString msg = QString());
    void updConfigSettings();

    void newFile();
    void open();
    void save();
    void stnConfig();
    void gnssConfig();
    void streamConfig();
    void sdefConfig();
    void about();
    void aboutQt();
    void changelog();

    void setupIncidentTable();
    void showBytesPerSec(int val);
    void triggerSettingsUpdate();

private:
    QSharedPointer<Config> config = QSharedPointer<Config>(new Config, &QObject::deleteLater);
    QWidget *centralWidget = new QWidget;
    QStatusBar *statusBar = new QStatusBar;
    QCustomPlot *customPlot = new QCustomPlot;
    QMenu *fileMenu, *optionMenu, *helpMenu;
    QDoubleSpinBox *instrStartFreq = new QDoubleSpinBox;
    QDoubleSpinBox *instrStopFreq = new QDoubleSpinBox;
    QLabel *startFreqLabel = new QLabel;
    QLabel *stopFreqLabel = new QLabel("End frequency (MHz)");
    QLabel *modeLabel = new QLabel;
    QComboBox *instrResolution = new QComboBox;
    QSpinBox *instrMeasurementTime = new QSpinBox;
    QSpinBox *instrAtt = new QSpinBox;
    QCheckBox *instrAutoAtt = new QCheckBox;
    QComboBox *instrAntPort = new QComboBox;
    QComboBox *instrMode = new QComboBox;
    QComboBox *instrFftMode = new QComboBox;
    QLineEdit *instrIpAddr = new QLineEdit;
    QLineEdit *instrPort = new QLineEdit;

    QPushButton *instrConnect = new QPushButton("Connect");
    QPushButton *instrDisconnect = new QPushButton("Disconnect");

    QDoubleSpinBox *instrTrigLevel = new QDoubleSpinBox;
    QDoubleSpinBox *instrTrigBandwidth = new QDoubleSpinBox;
    QSpinBox *instrTrigTime = new QSpinBox;

    QSpinBox *gnssCNo = new QSpinBox;
    QSpinBox *gnssAgc = new QSpinBox;
    QSpinBox *gnssPosOffset = new QSpinBox;
    QSpinBox *gnssAltOffset = new QSpinBox;
    QSpinBox *gnssTimeOffset = new QSpinBox;
    QTextEdit *incidentLog = new QTextEdit;
    QTextEdit *gnssStatus = new QTextEdit;

    MeasurementDevice *measurementDevice = new MeasurementDevice(config);

    QAction *newAct;
    QAction *openAct;
    QAction *saveAct;

    QAction *optStation;
    QAction *optGnss;
    QAction *optStream;
    QAction *optSdef;

    QAction *aboutAct;
    QAction *aboutQtAct;
    QAction *changelogAct;
    QAction *exitAct;

    GeneralOptions *generalOptions;
    ReceiverOptions *receiverOptions;
    SdefOptions *sdefOptions;

    CustomPlotController *customPlotController;
    QSpinBox *plotMaxScroll = new QSpinBox;
    QSpinBox *plotMinScroll = new QSpinBox;
    QSpinBox *plotMaxholdTime = new QSpinBox;

    TraceBuffer *traceBuffer = new TraceBuffer(config);

    TraceAnalyzer *traceAnalyzer = new TraceAnalyzer(config);
    SdefRecorder *sdefRecorder = new SdefRecorder();
    QThread *sdefRecorderThread = new QThread;
};
#endif // MAINWINDOW_H
