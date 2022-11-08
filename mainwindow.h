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
#include <QFile>
#include "qcustomplot.h"
#include "measurementdevice.h"
#include "typedefs.h"
#include "generaloptions.h"
#include "gnssoptions.h"
#include "receiveroptions.h"
#include "sdefoptions.h"
#include "customplotcontroller.h"
#include "tracebuffer.h"
#include "traceanalyzer.h"
#include "sdefrecorder.h"
#include "gnssdevice.h"
#include "gnssanalyzer.h"
#include "emailoptions.h"
#include "notifications.h"
#include "waterfall.h"
#include "cameraoptions.h"
#include "camerarecorder.h"
#include "arduinooptions.h"
#include "arduino.h"
#include "autorecorderoptions.h"
#include "pmrtablewdg.h"

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

public slots:
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
    void setValidators();
    void setSignals();
    void setInputsState(const bool state);
    void setResolutionFunction();
    void setDeviceAntPorts();
    void setDeviceFftModes();

    void instrStartFreqChanged();
    void instrStopFreqChanged();
    void instrMeasurementTimeChanged();
    void instrAttChanged();
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

    void showBytesPerSec(int val);
    void triggerSettingsUpdate();

    void updGnssBox(const QString txt, const int id, bool valid);
    void setWaterfallOption(QString s);

private:
    QSharedPointer<Config> config = QSharedPointer<Config>(new Config, &QObject::deleteLater);
    QWidget *centralWidget = new QWidget;
    QStatusBar *statusBar = new QStatusBar;
    QCustomPlot *customPlot;
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
    QGroupBox *rightBox = new QGroupBox("GNSS status");

    QPushButton *instrConnect = new QPushButton("Connect");
    QPushButton *instrDisconnect = new QPushButton("Disconnect");
    QPushButton *btnTrigRecording = new QPushButton("Trigger recording");
    QPushButton *btnPmrTable = new QPushButton("PMR table");

    QDoubleSpinBox *instrTrigLevel = new QDoubleSpinBox;
    QDoubleSpinBox *instrTrigBandwidth = new QDoubleSpinBox;
    QDoubleSpinBox *instrTrigTotalBandwidth = new QDoubleSpinBox;
    QSpinBox *instrTrigTime = new QSpinBox;

    QSpinBox *gnssCNo = new QSpinBox;
    QSpinBox *gnssAgc = new QSpinBox;
    QSpinBox *gnssPosOffset = new QSpinBox;
    QSpinBox *gnssAltOffset = new QSpinBox;
    QSpinBox *gnssTimeOffset = new QSpinBox;
    QTextEdit *incidentLog = new QTextEdit;
    QTextEdit *gnssStatus = new QTextEdit;

    MeasurementDevice *measurementDevice = new MeasurementDevice(config);
    GnssDevice *gnssDevice1 = new GnssDevice(this, 1);
    GnssDevice *gnssDevice2 = new GnssDevice(this, 2);
    GnssAnalyzer *gnssAnalyzer1 = new GnssAnalyzer(this, 1);
    GnssAnalyzer *gnssAnalyzer2 = new GnssAnalyzer(this, 2);

    QAction *newAct;
    QAction *openAct;
    QAction *saveAct;

    QAction *optStation;
    QAction *optGnss;
    QAction *optStream;
    QAction *optSdef;
    QAction *optEmail;
    QAction *optCamera;
    QAction *optArduino;
    QAction *optAutoRecorder;

    QAction *aboutAct;
    QAction *aboutQtAct;
    QAction *changelogAct;
    QAction *exitAct;

    GeneralOptions *generalOptions;
    GnssOptions *gnssOptions;
    ReceiverOptions *receiverOptions;
    SdefOptions *sdefOptions;
    EmailOptions *emailOptions;
    CameraOptions *cameraOptions;
    ArduinoOptions *arduinoOptions;
    AutoRecorderOptions *autoRecorderOptions;

    PmrTableWdg *pmrTableWdg = new PmrTableWdg(config);

    CustomPlotController *customPlotController;
    QSpinBox *plotMaxScroll = new QSpinBox;
    QSpinBox *plotMinScroll = new QSpinBox;
    QSpinBox *plotMaxholdTime = new QSpinBox;
    QComboBox *showWaterfall = new QComboBox;
    QSpinBox *waterfallTime = new QSpinBox;
    QCPItemPixmap *qcpImage;

    TraceBuffer *traceBuffer = new TraceBuffer(config);
    TraceAnalyzer *traceAnalyzer = new TraceAnalyzer(config);
    Notifications *notifications = new Notifications;
    QThread *notificationsThread = new QThread;

    SdefRecorder *sdefRecorder = new SdefRecorder();
    QThread *sdefRecorderThread = new QThread;

    Waterfall *waterfall;
    QThread *waterfallThread;

    CameraRecorder *cameraRecorder;
    QThread *cameraThread;

    Arduino *arduinoPtr = new Arduino;

    int gnssLastDisplayedId = 0;
    QDateTime gnssLastDisplayedTime = QDateTime::currentDateTime();
    bool dispWaterfall;
};
#endif // MAINWINDOW_H
