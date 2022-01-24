#include "mainwindow.h"


MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
{
    setStatusBar(statusBar);
    updWindowTitle();
    resize(1200, 680);
    setMinimumSize(1024, 680);
    restoreGeometry(config->getWindowGeometry());
    restoreState(config->getWindowState());

    QFont font = QApplication::font("QMessageBox");
    font.setPixelSize(11);
    qApp->setFont(font);

    instrMode->addItems(QStringList() << "PScan" << "FFM");

    customPlot = new QCustomPlot;
    customPlotController = new CustomPlotController(customPlot, config);
    customPlotController->init();
    waterfall = new Waterfall(config);
    //waterfall->start();
    showWaterfall->addItems(QStringList() << "Off" << "Grey" << "Red" << "Blue" << "Pride");

    qcpImage = new QCPItemPixmap(customPlot);
    qcpImage->setVisible(true);
    customPlot->addLayer("image");
    qcpImage->setLayer("image");
    customPlot->layer("image")->setMode(QCPLayer::lmBuffered);
    qcpImage->setScaled(false);

    generalOptions = new GeneralOptions(config);
    gnssOptions = new GnssOptions(config);
    receiverOptions = new ReceiverOptions(config);
    sdefOptions = new SdefOptions(config);
    emailOptions = new EmailOptions(config);
    cameraOptions = new CameraOptions(config);

    sdefRecorderThread->setObjectName("SdefRecorder");
    sdefRecorder->moveToThread(sdefRecorderThread);
    notificationsThread->setObjectName("Notifications");
    notifications->moveToThread(notificationsThread);

    waterfallThread = new QThread;
    waterfallThread->setObjectName("waterfall");
    waterfall->moveToThread(waterfallThread);

    incidentLog->setAcceptRichText(true);
    incidentLog->setReadOnly(true);

    createActions();
    createMenus();
    createLayout();
    setToolTips();
    setValidators();
    setSignals();
    getConfigValues();
    instrConnected(false); // to set initial inputs state
    instrAutoConnect();
    QTimer::singleShot(50, customPlotController, [this] {
        //customPlotController->updSettings();
        waterfall->updSize(customPlot->axisRect()->rect()); // weird func, needed to set the size of the waterfall image delayed
    });

}

MainWindow::~MainWindow()
{
    delete measurementDevice;
}

void MainWindow::createActions()
{
    newAct = new QAction(tr("&New"), this);
    newAct->setShortcuts(QKeySequence::New);
    newAct->setStatusTip(tr("Create a new configuration file"));
    connect(newAct, &QAction::triggered, this, &MainWindow::newFile);

    openAct = new QAction(tr("&Open..."), this);
    openAct->setShortcuts(QKeySequence::Open);
    openAct->setStatusTip(tr("Open an existing configuration file"));
    connect(openAct, &QAction::triggered, this, &MainWindow::open);

    saveAct = new QAction(tr("&Save as..."), this);
    saveAct->setShortcuts(QKeySequence::Save);
    saveAct->setStatusTip(tr("Save the current configuration to disk"));
    connect(saveAct, &QAction::triggered, this, &MainWindow::save);

    optStation = new QAction(tr("&General setup"), this);
    optStation->setStatusTip(tr("Basic station setup (position, folders, etc.)"));
    connect(optStation, &QAction::triggered, this, &MainWindow::stnConfig);

    optGnss = new QAction(tr("G&NSS setup"), this);
    optGnss->setStatusTip(tr("GNSS ports and logging configuration"));
    connect(optGnss, &QAction::triggered, this, &MainWindow::gnssConfig);

    optStream = new QAction(tr("&Receiver options"), this);
    optStream->setStatusTip(tr("Measurement receiver device options"));
    connect(optStream, &QAction::triggered, this, &MainWindow::streamConfig);

    optSdef = new QAction(tr("&SDEF (1809 format) options"), this);
    optSdef->setStatusTip(tr("Configuration of 1809 format and options"));
    connect(optSdef, &QAction::triggered, this, &MainWindow::sdefConfig);

    optEmail = new QAction(tr("&Notifications"), this);
    optEmail->setStatusTip(tr("Setup of email server and notfications"));
    connect(optEmail, &QAction::triggered, this, [this]{ this->emailOptions->start();});

    optCamera = new QAction(tr("&Camera"), this);
    optCamera->setStatusTip("Setup of camera recording");
    connect(optCamera, &QAction::triggered, this, [this]{ this->cameraOptions->start();});

    aboutAct = new QAction(tr("&About"), this);
    aboutAct->setStatusTip(tr("Show the application's About box"));
    connect(aboutAct, &QAction::triggered, this, &MainWindow::about);

    aboutQtAct = new QAction(tr("About &Qt"), this);
    aboutQtAct->setStatusTip(tr("Show the Qt library's About box"));
    connect(aboutQtAct, &QAction::triggered, this, &MainWindow::aboutQt);

    changelogAct = new QAction(tr("&Changelog"), this);
    changelogAct->setStatusTip(tr("Show the application changelog"));
    connect(changelogAct, &QAction::triggered, this, &MainWindow::changelog);

    exitAct = new QAction(tr("Exit the application"), this);
    exitAct->setStatusTip(tr("Shuts down connections and terminates Hauken"));
    exitAct->setShortcut(QKeySequence::Close);
    connect(exitAct, &QAction::triggered, qApp, &QApplication::exit);
}

void MainWindow::createMenus()
{
    fileMenu = new QMenu(tr("&File"), this);
    optionMenu = new QMenu(tr("&Options"), this);
    helpMenu = new QMenu(tr("&Help"), this);

    menuBar()->addMenu(fileMenu);
    menuBar()->addMenu(optionMenu);
    menuBar()->addMenu(helpMenu);

    fileMenu->addAction(newAct);
    fileMenu->addAction(openAct);
    fileMenu->addAction(saveAct);
    fileMenu->addSeparator();
    fileMenu->addAction(exitAct);

    optionMenu->addAction(optStation);
    optionMenu->addAction(optGnss);
    optionMenu->addAction(optStream);
    optionMenu->addAction(optSdef);
    optionMenu->addAction(optEmail);

    helpMenu->addAction(aboutAct);
    helpMenu->addAction(aboutQtAct);
    helpMenu->addAction(changelogAct);
}

void MainWindow::createLayout()
{
    setCentralWidget(centralWidget);

    QFrame *separator = new QFrame;
    separator->setFrameShape(QFrame::HLine);

    QGridLayout *gridLayout = new QGridLayout;
    QVBoxLayout *leftLayout = new QVBoxLayout;

    QFormLayout *instrForm = new QFormLayout;
    QGroupBox *instrGroupBox = new QGroupBox("Measurement receiver");
    instrGroupBox->setMinimumWidth(280);
    instrGroupBox->setMaximumWidth(280);

    instrForm->addRow(startFreqLabel, instrStartFreq);
    instrForm->addRow(stopFreqLabel, instrStopFreq);
    instrForm->addRow(modeLabel, instrResolution);
    instrForm->addRow(new QLabel("Measurement time (msec)"), instrMeasurementTime);
    instrForm->addRow(new QLabel("Manual attenuator"), instrAtt);
    instrForm->addRow(new QLabel("Auto attenuator"), instrAutoAtt);
    instrForm->addRow(new QLabel("Ant. port"), instrAntPort);
    instrForm->addRow(new QLabel("Instr. mode"), instrMode);
    instrForm->addRow(new QLabel("FFT mode"), instrFftMode);
    instrForm->addRow(new QLabel("IP address"), instrIpAddr);
    instrForm->addRow(new QLabel("Port"), instrPort);
    instrForm->addRow(instrConnect, instrDisconnect);

    instrGroupBox->setLayout(instrForm);

    QFormLayout *trigForm = new QFormLayout;
    QGroupBox *trigGroupBox = new QGroupBox("Trigger settings");
    trigForm->addRow(new QLabel("Trig level (dB)"), instrTrigLevel);
    trigForm->addRow(new QLabel("Min. trig bandwidth (kHz)"), instrTrigBandwidth);
    trigForm->addRow(new QLabel("Min. trig time (msec)"), instrTrigTime);
    trigForm->addRow(separator);

    trigForm->addRow(new QLabel("C/NO deviation (dB)"), gnssCNo);
    trigForm->addRow(new QLabel("AGC deviation"), gnssAgc);
    trigForm->addRow(new QLabel("Position offset (m)"), gnssPosOffset);

    trigForm->addRow(new QLabel("Altitude offset (m)"), gnssAltOffset);
    trigForm->addRow(new QLabel("Time offset (msec)"), gnssTimeOffset);
    trigGroupBox->setLayout(trigForm);
    trigGroupBox->setMinimumWidth(280);
    trigGroupBox->setMaximumWidth(280);

    trigGroupBox->setLayout(trigForm);

    leftLayout->addWidget(instrGroupBox);
    leftLayout->addWidget(trigGroupBox);

    QHBoxLayout *incLayout = new QHBoxLayout;
    QGroupBox *incBox = new QGroupBox("Incident log");
    incLayout->addWidget(incidentLog);
    incBox->setLayout(incLayout);
    incBox->setMaximumHeight(180);

    QHBoxLayout *bottomBox = new QHBoxLayout;
    bottomBox->addWidget(incBox);

    QHBoxLayout *rightLayout = new QHBoxLayout;

    rightLayout->addWidget(gnssStatus);
    rightBox->setLayout(rightLayout);
    rightBox->setMaximumWidth(200);
    rightBox->setMaximumHeight(180);
    rightLayout->addWidget(gnssStatus);

    QHBoxLayout *statusBox = new QHBoxLayout;
    statusBox->addWidget(rightBox);

    QGridLayout *plotLayout = new QGridLayout;
    plotLayout->addWidget(plotMaxScroll, 0, 0, 1, 1);
    plotLayout->addWidget(plotMinScroll, 2, 0, 1, 1);
    plotLayout->addWidget(customPlot, 0, 1, 3, 1);
    QHBoxLayout *bottomPlotLayout = new QHBoxLayout;
    bottomPlotLayout->addWidget(btnTrigRecording);
    btnTrigRecording->setFixedWidth(100);
    bottomPlotLayout->addWidget(new QLabel("Maxhold time (seconds)"));
    bottomPlotLayout->addWidget(plotMaxholdTime);
    bottomPlotLayout->addWidget(new QLabel("Waterfall type"));
    bottomPlotLayout->addWidget(showWaterfall);
    bottomPlotLayout->addWidget(new QLabel("Waterfall time"));
    bottomPlotLayout->addWidget(waterfallTime);
    plotLayout->addLayout(bottomPlotLayout, 3, 1, 1, 1, Qt::AlignHCenter);
    //plotMaxScroll->setFixedSize(40, 30);
    plotMaxScroll->setRange(-30,200);
    plotMinScroll->setFixedSize(40, 30);
    plotMinScroll->setRange(-50, 170);
    plotMaxScroll->setValue(config->getPlotYMax());
    plotMinScroll->setValue(config->getPlotYMin());
    plotMaxholdTime->setFixedSize(40,20);
    plotMaxholdTime->setRange(0, 120);
    waterfallTime->setRange(2, 86400);

    gridLayout->addLayout(leftLayout, 0, 0, 2, 1);
    gridLayout->addLayout(plotLayout, 0, 1, 1, 2);
    gridLayout->addLayout(statusBox, 1, 2, 1, 1);
    gridLayout->addLayout(bottomBox, 1, 1, 1, 1);

    centralWidget->setLayout(gridLayout);

    gnssStatus->setReadOnly(true);
    instrMeasurementTime->setFocus();
}

void MainWindow::setToolTips()
{
    instrStartFreq->setToolTip("Start frequency in MHz of instrument scan");
    instrStopFreq->setToolTip("End frequency in MHz of instrument scan");
    instrResolution->setToolTip("Frequency resolution (kHz per step)");
    instrMeasurementTime->setToolTip("Time spent per periodic measurement, default 18 ms");
    instrAtt->setToolTip("Manual attenuation setting, deactivate auto attenuator to use");
    instrAutoAtt->setToolTip("Receiver controls attenuation based on input level. "\
                             "Only working on newer models.");
    instrAntPort->setToolTip("For receivers with multiple antenna inputs");
    instrMode->setToolTip("Use PScan to cover a wide frequency range. "\
                          "FFM gives much better time resolution.");
    instrFftMode->setToolTip("FFT calculation method. Clear/write is the fastest method. "\
                             "Max hold increases chances to record short burst signals.");
    instrIpAddr->setToolTip("Only ipv4 supported for now.");
    instrPort->setToolTip("SCPI port to connect to, default 5555");

    instrTrigLevel->setToolTip("Decides how much above the average noise floor in dB a signal must be "\
                               "to trigger an incident. 10 dB is a reasonable value. "\
                               "See also the other trigger settings below.");
    instrTrigBandwidth->setToolTip("An incident is triggered only if the continous bandwidth of a "\
                                   "signal above the set trig level is higher than this setting.");
    instrTrigTime->setToolTip("Decides how long an incident must be present before an incident is "\
                              "triggered and a recording is started. "\
                              "Previously the \"trace count before recording is triggered\" setting.");

    gnssCNo->setToolTip("Carrier to noise level is averaged from the four best satellites in use. "\
                        "If the difference between average and instant CNO is higher than \""
                        "this setting an incident will be triggered. Set to 0 to disable.");
    gnssAgc->setToolTip("AGC detection only for supported GNSS models, currently only U-Blox. "\
                        "If the difference between the instant and the average AGC level is higher "\
                        "than this setting an incident will be triggered. Set to 0 to disable.");
    gnssPosOffset->setToolTip("Compares the current position with the one set in general station setup. "\
                              "If the offset is higher than this value an incident will be triggered. "\
                              "Set to 0 to disable.");
    gnssAltOffset->setToolTip("Compared the altitude with the value set in general station setup. "\
                              "If the offset is higher than this value an incident will be triggered. "\
                              "Set to 0 to disable.");
    gnssTimeOffset->setToolTip("Can be used to detect GNSS time spoofing. This requires the computer "\
                               "to be set up with NTP client software. The NTP time will then be "\
                               "compared with this value, and triggers an incident if the offset "\
                               "is higher than set value. Set to 0 to disable.");
    plotMaxScroll->setToolTip("Set the max. scale in dBuV");
    plotMinScroll->setToolTip("Set the min. scale in dBuV");
    plotMaxholdTime->setToolTip("Display maxhold time in seconds. Max 120 seconds. 0 for no maxhold.\nOnly affects displayed maxhold");
    showWaterfall->setToolTip("Select type of waterfall overlay");
    waterfallTime->setToolTip("Select the time in seconds a signal will be visible in the waterfall");
    btnTrigRecording->setToolTip("Starts a recording manually. The recording will end after the time specified in SDeF options: Recording time after incident,\nunless a real trig happens within this time, as this will extend the recording further.");
}

void MainWindow::getConfigValues()
{
    instrStartFreq->setValue(config->getInstrStartFreq());
    instrStopFreq->setValue(config->getInstrStopFreq());
    instrMeasurementTime->setValue(config->getInstrMeasurementTime());
    instrAtt->setValue(config->getInstrManAtt());
    instrAutoAtt->setChecked(config->getInstrAutoAtt());
    instrAntPort->setCurrentText(config->getInstrAntPort());
    instrMode->setCurrentIndex(config->getInstrMode());
    //instrFftMode->setCurrentIndex(config->getInstrFftMode());
    instrIpAddr->setText(config->getInstrIpAddr());
    instrPort->setText(QString::number(config->getInstrPort()));
    instrAutoAttChanged();
    instrModeChanged();
    //instrFftModeChanged();
    instrMeasurementTimeChanged();
    instrAttChanged();
    instrIpChanged();
    instrPortChanged();

    /*if (instrResolution->findText(QString::number(config->getInstrResolution())) >= 0)
        instrResolution->setCurrentIndex(instrResolution->findText(QString::number(config->getInstrResolution())));*/

    updInstrButtonsStatus();

    instrTrigLevel->setValue(config->getInstrTrigLevel());
    instrTrigBandwidth->setValue(config->getInstrMinTrigBW());
    instrTrigTime->setValue(config->getInstrMinTrigTime());

    gnssCNo->setValue(config->getGnssCnoDeviation());
    gnssAgc->setValue(config->getGnssAgcDeviation());
    gnssPosOffset->setValue(config->getGnssPosOffset());
    gnssAltOffset->setValue(config->getGnssAltOffset());
    gnssTimeOffset->setValue(config->getGnssTimeOffset());

    plotMaxholdTime->setValue(config->getPlotMaxholdTime());
    showWaterfall->setCurrentText(config->getShowWaterfall());
    setWaterfallOption(showWaterfall->currentText());
    waterfallTime->setValue(config->getWaterfallTime());

    updConfigSettings();
}

void MainWindow::updInstrButtonsStatus()
{
    instrDisconnect->setEnabled(measurementDevice->isConnected());

    if (!measurementDevice->isConnected()) {
        QHostAddress test(instrIpAddr->text());
        if (!test.isNull() && instrPort->text().toUInt() > 0)
            instrConnect->setEnabled(true);
        else
            instrConnect->setEnabled(false);
    }
    else {
    }
}

void MainWindow::instrModeChanged(int a)
{
    (void)a;
    if (instrMode->currentIndex() == (int)Instrument::Mode::PSCAN) {
        startFreqLabel->setText("Start frequency (MHz)");
        stopFreqLabel->setDisabled(false);
        instrStopFreq->setDisabled(false);
        modeLabel->setText("Pscan resolution (kHz)");
    }
    else {
        startFreqLabel->setText("FFM center frequency (MHz)");
        stopFreqLabel->setDisabled(true);
        instrStopFreq->setDisabled(true);
        modeLabel->setText("FFM span (kHz)");
    }
    config->setInstrMode(instrMode->currentIndex());
    instrStartFreqChanged();
    instrStopFreqChanged();
}

void MainWindow::setValidators()
{
    instrStartFreq->setRange(20, 6e3);
    instrStartFreq->setDecimals(0);
    instrStopFreq->setRange(20, 6e3);
    instrStopFreq->setDecimals(0);
    instrMeasurementTime->setRange(1, 5000);
    instrAtt->setRange(0, 40);

    QString ipRange = "(([ 0]+)|([ 0]*[0-9] *)|([0-9][0-9] )|([ 0][0-9][0-9])|(1[0-9][0-9])|([2][0-4][0-9])|(25[0-5]))";
    QRegExp ipRegex ("^" + ipRange
                     + "\\." + ipRange
                     + "\\." + ipRange
                     + "\\." + ipRange + "$");
    QRegExpValidator *ipValidator = new QRegExpValidator(ipRegex, this);
    instrIpAddr->setValidator(ipValidator);
    instrIpAddr->setInputMask("000.000.000.000");
    instrIpAddr->setCursorPosition(0);

    instrTrigLevel->setRange(-50, 200);
    instrTrigLevel->setDecimals(0);
    instrTrigTime->setRange(0, 9e5);
    instrTrigBandwidth->setRange(0, 9.99e9);
    instrTrigBandwidth->setDecimals(0);

    gnssCNo->setRange(0,1000);
    gnssAgc->setRange(0, 8000);
    gnssPosOffset->setRange(0, 48e6);
    gnssAltOffset->setRange(0, 10e3);
    gnssTimeOffset->setRange(0, 10e6);
}

void MainWindow::setSignals()
{
    connect(instrStartFreq, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &MainWindow::instrStartFreqChanged);
    connect(instrStopFreq, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &MainWindow::instrStopFreqChanged);
    connect(instrResolution, &QComboBox::currentTextChanged, config.data(), &Config::setInstrResolution);
    connect(instrMeasurementTime, QOverload<int>::of(&QSpinBox::valueChanged), config.data(), &Config::setInstrMeasurementTime);
    connect(instrAtt, QOverload<int>::of(&QSpinBox::valueChanged), config.data(), &Config::setInstrManAtt);
    connect(instrAutoAtt, &QCheckBox::toggled, this, &MainWindow::instrAutoAttChanged);
    connect(instrAntPort, &QComboBox::currentTextChanged, config.data(), &Config::setInstrAntPort);
    connect(instrMode, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &MainWindow::instrModeChanged);
    connect(instrFftMode, &QComboBox::currentTextChanged, config.data(), &Config::setInstrFftMode);
    connect(instrIpAddr, &QLineEdit::textChanged, this, &MainWindow::updInstrButtonsStatus);
    connect(instrIpAddr, &QLineEdit::editingFinished, this, &MainWindow::instrIpChanged);
    connect(instrPort, &QLineEdit::editingFinished, this, &MainWindow::instrPortChanged);

    connect(instrConnect, &QPushButton::clicked, measurementDevice, &MeasurementDevice::instrConnect);
    connect(instrDisconnect, &QPushButton::clicked, measurementDevice, &MeasurementDevice::instrDisconnect);

    connect(instrTrigLevel, QOverload<double>::of(&QDoubleSpinBox::valueChanged), config.data(), &Config::setInstrTrigLevel);
    connect(instrTrigBandwidth, QOverload<double>::of(&QDoubleSpinBox::valueChanged), config.data(), &Config::setInstrMinTrigBW);
    connect(instrTrigTime, QOverload<int>::of(&QSpinBox::valueChanged), config.data(), &Config::setInstrMinTrigTime);

    connect(gnssCNo, QOverload<int>::of(&QSpinBox::valueChanged), config.data(), &Config::setGnssCnoDeviation);
    connect(gnssAgc, QOverload<int>::of(&QSpinBox::valueChanged), config.data(), &Config::setGnssAgcDeviation);
    connect(gnssPosOffset, QOverload<int>::of(&QSpinBox::valueChanged), config.data(), &Config::setGnssPosOffset);
    connect(gnssAltOffset, QOverload<int>::of(&QSpinBox::valueChanged), config.data(), &Config::setGnssAltOffset);
    connect(gnssTimeOffset, QOverload<int>::of(&QSpinBox::valueChanged), config.data(), &Config::setGnssTimeOffset);

    connect(measurementDevice, &MeasurementDevice::connectedStateChanged, this, &MainWindow::instrConnected);
    connect(measurementDevice, &MeasurementDevice::connectedStateChanged, waterfall, &Waterfall::stopPlot); // own thread, needs own signal
    connect(measurementDevice, &MeasurementDevice::connectedStateChanged, customPlotController, &CustomPlotController::updDeviceConnected);
    connect(measurementDevice, &MeasurementDevice::connectedStateChanged, sdefRecorder, &SdefRecorder::deviceDisconnected);

    connect(measurementDevice, &MeasurementDevice::popup, this, &MainWindow::generatePopup);
    connect(measurementDevice, &MeasurementDevice::status, this, &MainWindow::updateStatusLine);
    connect(measurementDevice, &MeasurementDevice::instrId, this, &MainWindow::updWindowTitle);
    connect(measurementDevice, &MeasurementDevice::toIncidentLog, notifications, &Notifications::toIncidentLog);
    connect(measurementDevice, &MeasurementDevice::bytesPerSec, this, &MainWindow::showBytesPerSec);
    connect(measurementDevice, &MeasurementDevice::tracesPerSec, sdefRecorder, &SdefRecorder::updTracesPerSecond);
    //connect(measurementDevice, &MeasurementDevice::tracesPerSec, waterfall, &Waterfall::updTracesPerSecond);

    connect(plotMaxScroll, QOverload<int>::of(&QSpinBox::valueChanged), config.data(), &Config::setPlotYMax);
    connect(plotMinScroll, QOverload<int>::of(&QSpinBox::valueChanged), config.data(), &Config::setPlotYMin);
    connect(plotMaxholdTime, QOverload<int>::of(&QSpinBox::valueChanged), config.data(), &Config::setPlotMaxholdTime);
    connect(showWaterfall, &QComboBox::currentTextChanged, this, &MainWindow::setWaterfallOption);
    connect(waterfallTime, QOverload<int>::of(&QSpinBox::valueChanged), config.data(), &Config::setWaterfallTime);

    connect(traceBuffer, &TraceBuffer::newDispTrace, customPlotController, &CustomPlotController::plotTrace);
    connect(traceBuffer, &TraceBuffer::newDispTrace, waterfall, &Waterfall::receiveTrace);

    connect(traceBuffer, &TraceBuffer::newDispMaxhold, customPlotController, &CustomPlotController::plotMaxhold);
    connect(traceBuffer, &TraceBuffer::showMaxhold, customPlotController, &CustomPlotController::showMaxhold);
    connect(traceBuffer, &TraceBuffer::newDispTriglevel, customPlotController, &CustomPlotController::plotTriglevel);
    connect(traceBuffer, &TraceBuffer::toIncidentLog, notifications, &Notifications::toIncidentLog);
    connect(traceBuffer, &TraceBuffer::reqReplot, customPlotController, &CustomPlotController::doReplot);
    connect(customPlotController, &CustomPlotController::reqTrigline, traceBuffer, &TraceBuffer::sendDispTrigline);
    connect(traceBuffer, &TraceBuffer::averageLevelCalculating, customPlotController, &CustomPlotController::flashTrigline);
    connect(traceBuffer, &TraceBuffer::averageLevelCalculating, sdefRecorder, &SdefRecorder::endRecording); // stop ev. recording in case avg level for any reason should start recalc.
    connect(traceBuffer, &TraceBuffer::stopAvgLevelFlash, customPlotController, &CustomPlotController::stopFlashTrigline);
    connect(traceBuffer, &TraceBuffer::averageLevelCalculating, traceAnalyzer, &TraceAnalyzer::setAnalyzerNotReady);
    connect(traceBuffer, &TraceBuffer::stopAvgLevelFlash, traceAnalyzer, &TraceAnalyzer::setAnalyzerReady);

    connect(measurementDevice, &MeasurementDevice::newTrace, traceBuffer, &TraceBuffer::addTrace);
    connect(measurementDevice, &MeasurementDevice::resetBuffers, traceBuffer, &TraceBuffer::emptyBuffer);

    connect(traceBuffer, &TraceBuffer::averageLevelCalculating, traceAnalyzer, &TraceAnalyzer::resetAverageLevel);
    connect(traceBuffer, &TraceBuffer::averageLevelReady, traceAnalyzer, &TraceAnalyzer::setAverageTrace);
    connect(traceBuffer, &TraceBuffer::traceToAnalyzer, traceAnalyzer, &TraceAnalyzer::setTrace);

    connect(traceAnalyzer, &TraceAnalyzer::toIncidentLog, notifications, &Notifications::toIncidentLog);
    connect(customPlotController, &CustomPlotController::freqSelectionChanged, traceAnalyzer, &TraceAnalyzer::updTrigFrequencyTable);

    connect(config.data(), &Config::settingsUpdated, customPlotController, &CustomPlotController::updSettings);
    connect(config.data(), &Config::settingsUpdated, measurementDevice, &MeasurementDevice::updSettings);
    connect(config.data(), &Config::settingsUpdated, traceAnalyzer, &TraceAnalyzer::updSettings);
    connect(config.data(), &Config::settingsUpdated, traceBuffer, &TraceBuffer::updSettings);
    connect(config.data(), &Config::settingsUpdated, sdefRecorder, &SdefRecorder::updSettings);
    connect(config.data(), &Config::settingsUpdated, gnssDevice1, &GnssDevice::updSettings);
    connect(config.data(), &Config::settingsUpdated, gnssDevice2, &GnssDevice::updSettings);
    connect(config.data(), &Config::settingsUpdated, gnssAnalyzer1, &GnssAnalyzer::updSettings);
    connect(config.data(), &Config::settingsUpdated, gnssAnalyzer2, &GnssAnalyzer::updSettings);
    connect(config.data(), &Config::settingsUpdated, notifications, &Notifications::updSettings);
    connect(config.data(), &Config::settingsUpdated, waterfall, &Waterfall::updSettings);

    connect(traceAnalyzer, &TraceAnalyzer::alarm, sdefRecorder, &SdefRecorder::triggerRecording);
    connect(sdefRecorder, &SdefRecorder::recordingStarted, traceAnalyzer, &TraceAnalyzer::recorderStarted);
    connect(sdefRecorder, &SdefRecorder::recordingStarted, traceBuffer, &TraceBuffer::recorderStarted);
    connect(sdefRecorder, &SdefRecorder::recordingEnded, traceAnalyzer, &TraceAnalyzer::recorderEnded);
    connect(sdefRecorder, &SdefRecorder::recordingEnded, traceBuffer, &TraceBuffer::recorderEnded);
    connect(traceBuffer, &TraceBuffer::traceToRecorder, sdefRecorder, &SdefRecorder::receiveTrace);
    connect(sdefRecorder, &SdefRecorder::reqTraceHistory, traceBuffer, &TraceBuffer::getSecondsOfBuffer);
    connect(traceBuffer, &TraceBuffer::historicData, sdefRecorder, &SdefRecorder::receiveTraceBuffer);
    connect(sdefRecorder, &SdefRecorder::toIncidentLog, notifications, &Notifications::toIncidentLog);
    connect(measurementDevice, &MeasurementDevice::deviceStreamTimeout, sdefRecorder, &SdefRecorder::finishRecording); // stops eventual recording if stream times out (someone takes over meas.device)

    connect(sdefRecorderThread, &QThread::started, sdefRecorder, &SdefRecorder::start);
    connect(sdefRecorder, &SdefRecorder::warning, this, &MainWindow::generatePopup);
    connect(notificationsThread, &QThread::started, notifications, &Notifications::start);
    connect(waterfallThread, &QThread::started, waterfall, &Waterfall::start);

    connect(gnssAnalyzer1, &GnssAnalyzer::displayGnssData, this, &MainWindow::updGnssBox);
    connect(gnssDevice1, &GnssDevice::analyzeThisData, gnssAnalyzer1, &GnssAnalyzer::getData);
    connect(gnssAnalyzer1, &GnssAnalyzer::alarm, sdefRecorder, &SdefRecorder::triggerRecording);
    connect(gnssAnalyzer1, &GnssAnalyzer::toIncidentLog, notifications, &Notifications::toIncidentLog);
    connect(gnssDevice1, &GnssDevice::toIncidentLog, notifications, &Notifications::toIncidentLog);
    connect(gnssAnalyzer2, &GnssAnalyzer::displayGnssData, this, &MainWindow::updGnssBox);
    connect(gnssDevice2, &GnssDevice::analyzeThisData, gnssAnalyzer2, &GnssAnalyzer::getData);
    connect(gnssAnalyzer2, &GnssAnalyzer::alarm, sdefRecorder, &SdefRecorder::triggerRecording);
    connect(gnssAnalyzer2, &GnssAnalyzer::toIncidentLog, notifications, &Notifications::toIncidentLog);
    connect(gnssDevice2, &GnssDevice::toIncidentLog, notifications, &Notifications::toIncidentLog);

    connect(notifications, &Notifications::showIncident, this, [this] (QString s)
    {
        this->incidentLog->insertHtml(s);
        this->incidentLog->verticalScrollBar()->setValue(this->incidentLog->verticalScrollBar()->maximum());
    });
    connect(notifications, &Notifications::warning, this, &MainWindow::generatePopup);
    connect(notifications, &Notifications::reqTracePlot, customPlotController, &CustomPlotController::reqTracePlot); // ask for image
    connect(customPlotController, &CustomPlotController::retTracePlot, notifications, &Notifications::recTracePlot); // be nice and send it then!

    connect(waterfall, &Waterfall::imageReady, customPlot, [&] (QPixmap *pixmap)
    {
        if (dispWaterfall) {
            customPlot->axisRect()->setBackground(*pixmap, false);
            //customPlot->replot();
            //qcpImage->setPixmap(*pixmap);
            //customPlot->layer("image")->replot();
            //qDebug() << qcpImage->clipAxisRect()->rect() << qcpImage->pixmap().size();
            customPlot->replot();
        }
    });

    connect(gnssDevice1, &GnssDevice::positionUpdate, sdefRecorder, &SdefRecorder::updPosition);
    connect(gnssDevice2, &GnssDevice::positionUpdate, sdefRecorder, &SdefRecorder::updPosition);
    connect(measurementDevice, &MeasurementDevice::positionUpdate, sdefRecorder, &SdefRecorder::updPosition);
    connect(sdefRecorder, &SdefRecorder::reqPositionFrom, this, [this] (POSITIONSOURCE p) {
        if (p == POSITIONSOURCE::GNSSDEVICE1)
            gnssDevice1->reqPosition();
        else if (p == POSITIONSOURCE::GNSSDEVICE2)
            gnssDevice2->reqPosition();
        else
         measurementDevice->reqPosition();
    });

    connect(btnTrigRecording, &QPushButton::clicked, sdefRecorder, &SdefRecorder::manualTriggeredRecording);
    sdefRecorderThread->start();
    notificationsThread->start();
    waterfallThread->start();
}

void MainWindow::instrStartFreqChanged()
{
    if (config->getInstrMode() == 0) { //Pscan
        config->setInstrStartFreq(instrStartFreq->value());
    }
    else { // ffm
        config->setInstrFfmCenterFreq(instrStartFreq->value());
        measurementDevice->setFfmCenterFrequency();
    }
    if (measurementDevice->isConnected()) traceBuffer->restartCalcAvgLevel();
}

void MainWindow::instrStopFreqChanged()
{
    if (config->getInstrMode() == 0) { //Pscan
        config->setInstrStartFreq(instrStartFreq->value());
        config->setInstrStopFreq(instrStopFreq->value());
    }
    if (measurementDevice->isConnected()) traceBuffer->restartCalcAvgLevel();
}

void MainWindow::instrMeasurementTimeChanged()
{
    config->setInstrMeasurementTime(instrMeasurementTime->value());
}

void MainWindow::instrAttChanged()
{
    config->setInstrManAtt(instrAtt->text().toInt());
    if (measurementDevice->isConnected()) traceBuffer->restartCalcAvgLevel();
}

void MainWindow::instrAutoAttChanged()
{
    instrAtt->setDisabled(instrAutoAtt->isChecked());
    config->setInstrAutoAtt(instrAutoAtt->isChecked());
    if (measurementDevice->isConnected()) traceBuffer->restartCalcAvgLevel();
}

void MainWindow::instrIpChanged()
{
    config->setInstrIpAddr(instrIpAddr->text());
    measurementDevice->setAddress(config->getInstrIpAddr());
}

void MainWindow::instrPortChanged()
{
    config->setInstrPort(instrPort->text().toUInt());
    measurementDevice->setPort(config->getInstrPort());
}

void MainWindow::instrConnected(bool state) // takes care of enabling/disabling user inputs
{
    setInputsState(state);
    if (state) { // only do these funcs when connected, if not the device data has not been set
        setResolutionFunction();
        setDeviceAntPorts();
        setDeviceFftModes();
        instrStartFreq->setMinimum(measurementDevice->getDeviceMinFreq() / 1e6);
        instrStopFreq->setMaximum(measurementDevice->getDeviceMaxFreq() / 1e6);
    }
    else {
        traceBuffer->deviceDisconnected(); // stops buffer work when not needed
    }
}

void MainWindow::setInputsState(const bool state)
{
    instrStartFreq->setEnabled(state);
    instrStopFreq->setEnabled(state);
    instrResolution->setEnabled(state);
    instrMeasurementTime->setEnabled(state);
    instrAtt->setEnabled(state);
    instrAutoAtt->setEnabled(state);
    if (state && instrAutoAtt->isChecked())
        instrAtt->setDisabled(true);

    instrMode->setEnabled(false);       // disabled until further
    instrFftMode->setEnabled(state);
    instrDisconnect->setEnabled(state);
    instrConnect->setDisabled(state);
    instrIpAddr->setDisabled(state);
    instrPort->setDisabled(state);
    instrAntPort->setEnabled(state);
    btnTrigRecording->setEnabled(state);
}

void MainWindow::setResolutionFunction()
{
    instrResolution->clear();
    if (measurementDevice->getCurrentMode() == Instrument::Mode::PSCAN) {
        QString val = config->getInstrResolution();
        instrResolution->addItems(measurementDevice->getDevicePscanResolutions());
        if (instrResolution->findText(val) >= 0)
            instrResolution->setCurrentIndex(instrResolution->findText(val));
    }
    else {
        QString val = config->getInstrResolution();
        instrResolution->addItems(measurementDevice->getDeviceFfmSpans());
        if (instrResolution->findText(val) >= 0)
            instrResolution->setCurrentIndex(instrResolution->findText(val));
    }
}

void MainWindow::setDeviceAntPorts()
{
    instrAntPort->clear();
    QString val = config->getInstrAntPort();
    instrAntPort->addItems(measurementDevice->getDeviceAntPorts());
    instrAntPort->setEnabled(instrAntPort->count() > 1);

    if (instrAntPort->findText(val) >= 0)
        instrAntPort->setCurrentIndex(instrAntPort->findText(val));
}

void MainWindow::setDeviceFftModes()
{
    instrFftMode->clear();
    QString val = config->getInstrFftMode(); // tmp keeper
    instrFftMode->addItems(measurementDevice->getDeviceFftModes());
    instrFftMode->setEnabled(instrFftMode->count() > 1); // no point in having 0 choices...

    if (instrFftMode->findText(val) >= 0) {
        instrFftMode->setCurrentIndex(instrFftMode->findText(val));
    }
}

void MainWindow::generatePopup(const QString msg)
{
    QMessageBox::warning(this, tr("Warning message"), msg);
}

void MainWindow::updateStatusLine(const QString msg)
{
    statusBar->showMessage(msg);
}

void MainWindow::updWindowTitle(const QString msg)
{
    QString extra;
    if (!msg.isEmpty()) extra = tr(" using ") + msg;
    setWindowTitle(tr("Hauken v. ") + qApp->applicationVersion() + extra
                   + " (" + config->getCurrentFilename().split('/').last() + ")");
}

void MainWindow::about()
{
    QString txt;
    QTextStream ts(&txt);
    QMessageBox box;
    box.setTextFormat(Qt::RichText);
    box.setStandardButtons(QMessageBox::Ok);

    ts << "<table><tr><td>Application version</td><td>" << SW_VERSION << "</td></tr>";
    ts << "<tr><td>Build date</td><td>" << BUILD_DATE << "</td></tr><tr></tr>";
    ts << "<tr><td><a href='https://github.com/cutelyst/simple-mail'>SimpleMail Qt library SMTP mail client</a></td><td>LGPL 2.1 license" << "</td></tr>";
    ts << "<tr><td>Questions/support? => JSK</td></tr></table>";
    box.setText(txt);
    box.exec();
}

void MainWindow::aboutQt()
{
    QMessageBox::aboutQt(this);
}

void MainWindow::changelog()
{
    QMessageBox msgBox;
    msgBox.setText("Hauken changelog");
    QString txt;
    QTextStream ts(&txt);
    ts << "<table>"
       << "<tr><td>2.9</td><td>File upload and email notification auto resend on failure. Manual trig added. Minor trace display bugfixes</td></tr>"
       << "<tr><td>2.8</td><td>SDeF mobile position added. Minor bugfixes in waterfall code</td></tr>"
       << "<tr><td>2.7</td><td>Waterfall added with a ton of colors. Minor notifications bugfixes</td></tr>"
       << "<tr><td>2.6</td><td>Email notifications, inline pictures in email</td></tr>"
       << "<tr><td>2.5</td><td>Dual GNSS support added</td></tr>"
       << "<tr><td>2.4</td><td>Auto upload to Casper added, minor bugfixes</td></tr>"
       << "<tr><td>2.3</td><td>Instrument support: ESMB, EM100, EM200, EB500, USRP/Tracy</td></tr>"
       << "<tr><td>2.2</td><td>Basic file saving enabled</td></tr>"
       << "<tr><td>2.1</td><td>Plot and buffer functions working</td></tr>"
       << "<tr><td>2.0.b</td>" << "<td>Initial commit, basic instrument control</td></tr>";
    ts << "</table>";
    msgBox.setInformativeText(txt);

    msgBox.exec();
}

void MainWindow::stnConfig()
{
    generalOptions->start();
}

void MainWindow::gnssConfig()
{
    gnssOptions->start();
}

void MainWindow::streamConfig()
{
    receiverOptions->start();
}

void MainWindow::sdefConfig()
{
    sdefOptions->start();
}

void MainWindow::newFile()
{
    qDebug() << config->getWorkFolder();
    QString filename = QFileDialog::getSaveFileName(this, tr("New configuration file"),
                                                    config->getWorkFolder(),
                                                    tr("Configuration files (*.ini)"));
    if (!filename.isEmpty()) config->newFileName(filename);
    if (measurementDevice->isConnected())
        measurementDevice->instrDisconnect();
    getConfigValues();
    updWindowTitle();
}

void MainWindow::open()
{
    QString fileName = QFileDialog::getOpenFileName(this, tr("Open configuraton file"),
                                                    config->getWorkFolder(),
                                                    tr("Configuration files (*.ini)"));

    if (!fileName.isEmpty()) config->newFileName(fileName);
    getConfigValues();
    updWindowTitle();
}

void MainWindow::save()
{
    QString fileName = QFileDialog::getSaveFileName(this, tr("Save current configuration"),
                                                    config->getWorkFolder(),
                                                    tr("Configuration files (*.ini)"));
    if (!fileName.isEmpty()) {  //config->newFileName(fileName);
        QFile::copy(config->getCurrentFilename(), fileName);
        config->newFileName(fileName);
    }
    saveConfigValues();
    updWindowTitle();
}

void MainWindow::saveConfigValues()
{
    config->setInstrStartFreq(instrStartFreq->value());
    config->setInstrStopFreq(instrStopFreq->value());
    config->setInstrResolution(instrResolution->currentText());
    config->setInstrMeasurementTime(instrMeasurementTime->value());
    config->setInstrManAtt(instrAtt->text().toUInt());
    config->setInstrAutoAtt(instrAutoAtt->isChecked());
    config->setInstrAntPort(instrAntPort->currentText());
    config->setInstrMode(instrMode->currentIndex());
    config->setInstrFftMode(instrFftMode->currentText());
    config->setInstrIpAddr(instrIpAddr->text());
    config->setInstrPort(instrPort->text().toUInt());
    config->setInstrTrigLevel(instrTrigLevel->value());
    config->setInstrMinTrigBW(instrTrigBandwidth->value());
    config->setInstrMinTrigTime(instrTrigTime->value());
    config->setGnssCnoDeviation(gnssCNo->value());
    config->setGnssAgcDeviation(gnssAgc->value());
    config->setGnssPosOffset(gnssPosOffset->value());
    config->setGnssAltOffset(gnssAltOffset->value());
    config->setGnssTimeOffset(gnssTimeOffset->value());

    generalOptions->saveCurrentSettings();
    gnssOptions->saveCurrentSettings();
    receiverOptions->saveCurrentSettings();
    sdefOptions->saveCurrentSettings();
    emailOptions->saveCurrentSettings();
}

void MainWindow::showBytesPerSec(int val)
{
    QString msg = "Traffic " + QString::number(val / 1000) + " kB/sec";
    if (measurementDevice->getTracesPerSec() > 0)
        msg += ", " + QString::number(measurementDevice->getTracesPerSec(), 'f', 1) + " traces/sec";
    if (traceAnalyzer->getKhzAboveLimit() > 0)
        msg += ", " + QString::number(traceAnalyzer->getKhzAboveLimit(), 'f', 0) + " kHz above limit";
    statusBar->showMessage(msg, 2000);
}

void MainWindow::resizeEvent(QResizeEvent *event)
{
    config->setWindowGeometry(saveGeometry());
    QWidget::resizeEvent(event);
    waterfall->updSize(customPlot->axisRect()->rect());
}

void MainWindow::moveEvent(QMoveEvent *event)
{
    config->setWindowState(saveState());
    QWidget::moveEvent(event);
}

void MainWindow::instrAutoConnect()
{
    if (config->getInstrConnectOnStartup() && instrCheckSettings())
        measurementDevice->instrConnect();
}

bool MainWindow::instrCheckSettings() // TODO: More checks here
{
    if (instrStartFreq->text().toDouble() > instrStopFreq->text().toDouble())
        return false;
    return true;
}

void MainWindow::updConfigSettings()
{
    measurementDevice->setAutoReconnect(config->getInstrAutoReconnect());
    measurementDevice->setUseTcp(config->getInstrUseTcpDatastream());
}

void MainWindow::triggerSettingsUpdate()
{

}

void MainWindow::updGnssBox(const QString txt, const int id, bool valid)
{
    if (id == gnssLastDisplayedId) { // same as previous update, just show result
        gnssStatus->setText(txt);
    }
    else if (gnssLastDisplayedTime.secsTo(QDateTime::currentDateTime()) >= 5) {
        gnssLastDisplayedId = id;
        gnssLastDisplayedTime = QDateTime::currentDateTime();
        rightBox->setTitle("GNSS " + QString::number(id) + " status (" + (valid ? "pos. valid":"pos. invalid") + ")");
        gnssStatus->setText(txt);
    }
}

void MainWindow::setWaterfallOption(QString s)
{
    if (s == "Off") dispWaterfall = false;
    else dispWaterfall = true;

    if (!dispWaterfall) {
        customPlot->axisRect()->setBackground(QPixmap());
        customPlot->replot();
    }
    if (s != "Pride")
        customPlot->graph(0)->setPen(QPen(Qt::blue));
    else
        customPlot->graph(0)->setPen(QPen(Qt::black));


    config->setShowWaterfall(s);
}
