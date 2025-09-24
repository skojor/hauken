#ifndef MAINWINDOW_H
#define MAINWINDOW_H

/*#undef slots
#include "ai.h"
#define slots Q_SLOTS*/

#include <QAction>
#include <QApplication>
#include <QDateTime>
#include <QDebug>
#include <QDoubleSpinBox>
#include <QFile>
#include <QFileDialog>
#include <QFuture>
#include <QFutureWatcher>
#include <QHostAddress>
#include <QMainWindow>
#include <QMessageBox>
#include <QProgressBar>
#include <QPromise>
#include <QRegularExpression>
#include <QRegularExpressionValidator>
#include <QSharedPointer>
#include <QSpinBox>
#include <QStandardPaths>
#include <QString>
#include <QStyleHints>
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QTextStream>
#include <QThread>
#include <QTimer>
#include <QToolBar>
#include <QtConcurrent/QtConcurrentRun>
#include <QtMultimedia/QAudioOutput>
#include <QtMultimedia/QMediaPlayer>
#include "config.h"
#include "measurementdevice.h"
#include "qcustomplot.h"
//#include "typedefs.h"
#include "accesshandler.h"
#include "ai.h"
#include "arduino.h"
#include "arduinooptions.h"
#include "autorecorderoptions.h"
#include "cameraoptions.h"
#include "camerarecorder.h"
#include "customplotcontroller.h"
#include "emailoptions.h"
#include "generaloptions.h"
#include "geolimit.h"
#include "geolimitoptions.h"
#include "gnssanalyzer.h"
#include "gnssdevice.h"
#include "gnssdisplay.h"
#include "gnssoptions.h"
#include "instrumentlist.h"
#include "iqoptions.h"
#include "led/ledindicator.h"
#include "mqtt.h"
#include "mqttoptions.h"
#include "notifications.h"
#include "oauthfileuploader.h"
#include "oauthoptions.h"
#include "pmrtablewdg.h"
#include "positionreport.h"
#include "positionreportoptions.h"
#include "read1809data.h"
#include "receiveroptions.h"
#include "restapi.h"
#include "sdefoptions.h"
#include "sdefrecorder.h"
#include "tcpdatastream.h" // Added 300524 - moved classes from measurementDevice
#include "traceanalyzer.h"
#include "tracebuffer.h"
#include "udpdatastream.h"
#include "version.h"
#include "versionupdater.h"
#include "vifstreamtcp.h"
#include "vifstreamudp.h"
#include "waterfall.h"
#include "network.h"
#include "iqplot.h"

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

public slots:
    void resizeEvent(QResizeEvent *event) override; // override to save changes to win size
    void moveEvent(QMoveEvent *event) override; // override to save position
    void uploadProgress(int percent);

protected:
#ifndef QT_NO_CONTEXTMENU
    void contextMenuEvent(QContextMenuEvent *event) override;
#endif // QT_NO_CONTEXTMENU

private slots:
    void createMenus();
    void createActions();
    void createLayout();
    void setToolTips();
    void getConfigValues();
    void saveConfigValues();
    void instrAutoAttChanged();
    void instrModeChanged();
    void instrResolutionChanged();
    void instrFfmSpanChanged();
    void instrFfmCenterFreqChanged();
    void updInstrButtonsStatus();
    void setValidators();
    void setSignals();
    void setInputsState(const bool state);
    void setResolutionFunction();
    void setDeviceAntPorts();
    void setDeviceFftModes();

    void instrPscanFreqChanged();
    void instrMeasurementTimeChanged();
    void instrAttChanged();
    void instrIpChanged();
    void instrPortChanged();
    void instrConnected(bool state);
    void instrAutoConnect();
    bool instrCheckSettings();
    void changeAntennaPortName();
    void instrGainControlChanged(int index = -1);

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
    void closeEvent(QCloseEvent *event) override;

    void traceIncidentAlarm(bool state);
    void recordIncidentAlarm(bool state);
    void gnssIncidentAlarm(bool state);
    void recordEnabled(bool state);
    void restartWaterfall();
    void btnConnectPressed();
    void btnDisconnectPressed();

private:
    QSharedPointer<Config> config = QSharedPointer<Config>(new Config, &QObject::deleteLater);
    QWidget *centralWidget = new QWidget;
    QStatusBar *statusBar = new QStatusBar;
    QProgressBar *progressBar = new QProgressBar;
    QCustomPlot *customPlot;
    QMenu *fileMenu, *optionMenu, *helpMenu;
    QDoubleSpinBox *instrStartFreq = new QDoubleSpinBox;
    QDoubleSpinBox *instrStopFreq = new QDoubleSpinBox;
    QLabel *startFreqLabel = new QLabel;
    QLabel *stopFreqLabel = new QLabel("End frequency (MHz)");
    QLabel *modeLabel = new QLabel;
    QComboBox *instrResolution = new QComboBox;
    QDoubleSpinBox *instrFfmCenterFreq = new QDoubleSpinBox; // Added 230524
    QComboBox *instrFfmSpan = new QComboBox; // - " -
    QFormLayout *instrForm = new QFormLayout; // - " -

    QSpinBox *instrMeasurementTime = new QSpinBox;
    QSpinBox *instrAtt = new QSpinBox;
    QCheckBox *instrAutoAtt = new QCheckBox;
    QComboBox *instrAntPort = new QComboBox;
    QComboBox *instrMode = new QComboBox;
    QComboBox *instrFftMode = new QComboBox;
    QComboBox *instrIpAddr = new QComboBox;
    QLineEdit *instrPort = new QLineEdit;
    QComboBox *instrGainControl = new QComboBox;
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
    LedIndicator *ledTraceStatus = new LedIndicator;
    QLabel *labelTraceLedText = new QLabel("Awaiting trace data");
    LedIndicator *ledRecordStatus = new LedIndicator;
    QLabel *labelRecordLedText = new QLabel("Not recording");
    LedIndicator *ledGnssStatus = new LedIndicator;
    QLabel *labelGnssLedText = new QLabel("No GNSS data");

    MeasurementDevice *measurementDevice = new MeasurementDevice(config);
    GnssDevice *gnssDevice1 = new GnssDevice(config, 1);
    GnssDevice *gnssDevice2 = new GnssDevice(config, 2);
    GnssAnalyzer *gnssAnalyzer1 = new GnssAnalyzer(config, 1);
    GnssAnalyzer *gnssAnalyzer2 = new GnssAnalyzer(config, 2);
    GnssAnalyzer *gnssAnalyzer3 = new GnssAnalyzer(config, 3);

    QAction *newAct;
    QAction *openAct;
    QAction *saveAct;
    QAction *open1809Act;
    QAction *openFolderAct;
    QAction *openIqAct;

    QAction *optStation;
    QAction *optGnss;
    QAction *optStream;
    QAction *optSdef;
    QAction *optEmail;
    QAction *optCamera;
    QAction *optArduino;
    QAction *optAutoRecorder;
    QAction *optPositionReport;
    QAction *optGeoLimit;
    QAction *optMqtt;
    QAction *optOAuth;
    QAction *optIq;
    QAction *hideShowControls = new QAction("Hide/show receiver controls");
    QAction *hideShowTrigSettings = new QAction("Hide/show trigger settings");
    QAction *hideShowStatusIndicator = new QAction("Hide/show status indicators");
    QAction *hideShowGnssWindow = new QAction("Hide/show GNSS status window");
    QAction *hideShowIncidentlog = new QAction("Hide/show incident log");
    QAction *toggleDarkMode = new QAction("Toogle dark mode");
    QAction *aboutAct;
    QAction *aboutQtAct;
    QAction *changelogAct;
    QAction *exitAct;

    QGroupBox *instrGroupBox;
    QGroupBox *trigGroupBox;
    QGroupBox *grpIndicator;
    QGroupBox *incBox;

    GeneralOptions *generalOptions;
    GnssOptions *gnssOptions;
    ReceiverOptions *receiverOptions;
    SdefOptions *sdefOptions;
    EmailOptions *emailOptions;
    CameraOptions *cameraOptions;
    ArduinoOptions *arduinoOptions;
    AutoRecorderOptions *autoRecorderOptions;
    PositionReportOptions *positionReportOptions;
    GeoLimitOptions *geoLimitOptions;
    MqttOptions *mqttOptions;
    OAuthOptions *oAuthOptions;
    IqOptions *iqOptions;

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
    Notifications *notifications = new Notifications(config);
    QThread *notificationsThread = new QThread;

    IqPlot *iqPlot = new IqPlot(config);
    SdefRecorder *sdefRecorder = new SdefRecorder(config);
    QThread *sdefRecorderThread = new QThread;

    Waterfall *waterfall;
    QThread *waterfallThread;

    CameraRecorder *cameraRecorder;
    QThread *cameraThread;

    Arduino *arduinoPtr;
    AI *aiPtr;
    Read1809Data *read1809Data;
    InstrumentList *instrumentList = new InstrumentList(config);
    GnssDisplay *gnssDisplay = new GnssDisplay(config);

    int gnssLastDisplayedId = 0;
    QDateTime gnssLastDisplayedTime = QDateTime::currentDateTime();
    bool dispWaterfall;

    PositionReport *positionReport = new PositionReport(config);
    GeoLimit *geoLimit = new GeoLimit(config);
    Mqtt *mqtt = new Mqtt(config);
    QLineEdit *antPortLineEdit = new QLineEdit;

    bool traceAlarmRaised = false, recordAlarmRaised = true, gnssAlarmRaised = true, recordDisabledRaised = false;
    QMediaPlayer *player = new QMediaPlayer;
    QAudioOutput *audioOutput = new QAudioOutput;
    QTimer *notificationTimer = new QTimer;

    QSharedPointer<UdpDataStream> udpStream = QSharedPointer<UdpDataStream>(new UdpDataStream, &QObject::deleteLater);
    QSharedPointer<TcpDataStream> tcpStream = QSharedPointer<TcpDataStream>(new TcpDataStream, &QObject::deleteLater);
    QSharedPointer<VifStreamUdp> vifStreamUdp = QSharedPointer<VifStreamUdp>(new VifStreamUdp, &QObject::deleteLater);
    QSharedPointer<VifStreamTcp> vifStreamTcp = QSharedPointer<VifStreamTcp>(new VifStreamTcp, &QObject::deleteLater);

    double tracesPerSecond = 0;
    AccessHandler *accessHandler = new AccessHandler(config);
    OAuthFileUploader *oauthFileUploader = new OAuthFileUploader(config);
    RestApi *restApi = new RestApi(config);
    Network *ptrNetwork = new Network(config);
    bool useDbm;

signals:
    void stopPlot(bool);
    void antennaNameEdited(const int index, const QString name);
    void antennaPortChanged();
};
#endif // MAINWINDOW_H
