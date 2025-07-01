#include "mainwindow.h"
#include "changelog.cpp"
#include "signals.cpp"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
{
    setStatusBar(statusBar);
    //statusBar->addWidget(progressBar);
    progressBar->setMinimum(0);
    progressBar->setMaximum(100);

    updWindowTitle();
    resize(1200, 780);
    setMinimumSize(1024, 780);
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
    iqdataWaterfall = new Waterfall(config);
    iqdataWaterfall->start();

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
    arduinoOptions = new ArduinoOptions(config);
    autoRecorderOptions = new AutoRecorderOptions(config);
    positionReportOptions = new PositionReportOptions(config);
    geoLimitOptions = new GeoLimitOptions(config);
    mqttOptions = new MqttOptions(config);
    oAuthOptions = new OAuthOptions(config);
    iqOptions = new IqOptions(config);

    arduinoPtr = new Arduino(config);
    aiPtr = new AI(config);
    read1809Data = new Read1809Data(config);

    sdefRecorderThread->setObjectName("SdefRecorder");
    sdefRecorder->moveToThread(sdefRecorderThread);
    notificationsThread->setObjectName("Notifications");
    notifications->moveToThread(notificationsThread);

    waterfallThread = new QThread;
    waterfallThread->setObjectName("waterfall");
    waterfall->moveToThread(waterfallThread);

    cameraRecorder = new CameraRecorder(config);
    cameraThread = new QThread;
    cameraThread->setObjectName("camera");
    cameraRecorder->moveToThread(cameraThread);
    connect(cameraThread, &QThread::started, cameraRecorder, &CameraRecorder::start);
    cameraThread->start();

    incidentLog->setAcceptRichText(true);
    incidentLog->setReadOnly(true);

#ifdef _WIN32
    if (QFile::exists(QDir(QCoreApplication::applicationDirPath()).absolutePath() + "/notify.wav")) {
        player->setSource(QUrl::fromLocalFile(
            QDir(QCoreApplication::applicationDirPath()).absolutePath() + "/notify.wav"));
        player->setAudioOutput(audioOutput);
        audioOutput->setVolume(80);
    }
#endif

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
        waterfall->updSize(
            customPlot->axisRect()
                ->rect()); // weird func, needed to set the size of the waterfall image delayed
    });
    notificationTimer->setSingleShot(true);

    instrumentList->start(); // check if instrument server is available
    gnssDisplay->start();

    measurementDevice->setUdpStreamPtr(udpStream);
    measurementDevice->setTcpStreamPtr(tcpStream);
    measurementDevice->setVifStreamTcpPtr(vifStreamTcp);
    measurementDevice->setVifStreamUdpPtr(vifStreamUdp);
    VersionUpdater versionUpdater(config); // Handles any config changes needed
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    gnssDisplay->close();
    if (arduinoPtr->isWatchdogActive())
        arduinoPtr->watchdogOff(); // Always turn off the watchdog if app is closing gracefully
    arduinoPtr->close();
    read1809Data->close();
    measurementDevice->instrDisconnect();
    config->setWindowGeometry(this->saveGeometry());
    config->setWindowState(this->saveState());
    QMainWindow::closeEvent(event);
}

MainWindow::~MainWindow()
{
    QApplication::exit();
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

    open1809Act = new QAction(tr("Open &1809 file..."), this);
    open1809Act->setShortcuts(QKeySequence::Open);
    open1809Act->setStatusTip(tr("Open a previously recorded 1809 file for playback"));
    connect(open1809Act, &QAction::triggered, this, [this] {
        if (!measurementDevice->isConnected())
            read1809Data->readFile(QFileDialog::getOpenFileName(this,
                                                                "Open 1809 file",
                                                                config->getLogFolder(),
                                                                "1809 files (*.cef *.zip)"));
        else {
            qDebug() << "Trying to read an 1809 file while connected, bad idea";
            QMessageBox::warning(this,
                                 "Hauken",
                                 "Disconnect the device before trying to read an 1809 file");
        }
    });
    openFolderAct = new QAction(tr("Open f&older (DEBUG)"), this);
    openFolderAct->setStatusTip(
        tr("Open a folder with 1809 data, for data conversion (INTERNAL, DON'T USE!"));
    connect(openFolderAct, &QAction::triggered, this, [this] {
        read1809Data->readFolder(QFileDialog::getExistingDirectory(this,
                                                                   tr("Open folder"),
                                                                   config->getLogFolder(),
                                                                   QFileDialog::ShowDirsOnly));
    });

    openIqAct = new QAction(tr("Open &IQ data file..."), this);
    openIqAct->setStatusTip(tr("Read and analyze a raw IQ data file from disk"));
    connect(openIqAct, &QAction::triggered, this, [this]() {
        QFuture<bool> future
            = QtConcurrent::run(&Waterfall::readAndAnalyzeFile,
                                iqdataWaterfall,
                                QFileDialog::getOpenFileName(this,
                                                             "Open raw IQ data file",
                                                             config->getLogFolder(),
                                                             "IQ data (*.iq)"));
    });

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
    connect(optEmail, &QAction::triggered, this, [this] { this->emailOptions->start(); });

    optCamera = new QAction(tr("&Camera"), this);
    optCamera->setStatusTip("Setup of camera recording");
    connect(optCamera, &QAction::triggered, this, [this] { this->cameraOptions->start(); });

    optArduino = new QAction(tr("&Arduino"), this);
    optArduino->setStatusTip("Setup of Arduino relay/temperature control");
    connect(optArduino, &QAction::triggered, this, [this] { this->arduinoOptions->start(); });

    optAutoRecorder = new QAction(tr("A&uto recorder"), this);
    optAutoRecorder->setStatusTip("Setup of Hauken autorecorder");
    connect(optAutoRecorder, &QAction::triggered, this, [this] {
        this->autoRecorderOptions->start();
    });

    optPositionReport = new QAction(tr("&Position report"), this);
    optPositionReport->setStatusTip("Setup of periodic report posts via http(s)");
    connect(optPositionReport, &QAction::triggered, this, [this] {
        this->positionReportOptions->start();
    });

    optGeoLimit = new QAction(tr("&Geographic blocking options"), this);
    optGeoLimit->setStatusTip(tr("Setup of geographic area where usage is allowed"));
    connect(optGeoLimit, &QAction::triggered, this, [this] { this->geoLimitOptions->start(); });

    optMqtt = new QAction(tr("&MQTT and webswitch sensor options"), this);
    optMqtt->setStatusTip(tr("Setup of sensor data input from MQTT and webswitch"));
    connect(optMqtt, &QAction::triggered, this, [this] { this->mqttOptions->start(); });

    optOAuth = new QAction(tr("&OAuth options"), this);
    optOAuth->setStatusTip(tr("Setup authentication using OAuth2 schemes"));
    connect(optOAuth, &QAction::triggered, this, [this] { this->oAuthOptions->start(); });

    optIq = new QAction(tr("IQ data and plot options"), this);
    optIq->setStatusTip(tr("Setup IQ plot and saving raw IQ data to file"));
    connect(optIq, &QAction::triggered, this, [this]() { iqOptions->start(); });

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

    connect(hideShowControls, &QAction::triggered, this, [this] {
        instrGroupBox->setVisible(!instrGroupBox->isVisible());
        config->setShowReceiverControls(instrGroupBox->isVisible());
        restartWaterfall();
    });
    connect(hideShowTrigSettings, &QAction::triggered, this, [this] {
        trigGroupBox->setVisible(!trigGroupBox->isVisible());
        config->setShowTriggerSettings(trigGroupBox->isVisible());
        restartWaterfall();
    });
    connect(hideShowStatusIndicator, &QAction::triggered, this, [this] {
        grpIndicator->setVisible(!grpIndicator->isVisible());
        config->setShowStatusIndicators(grpIndicator->isVisible());
        restartWaterfall();
    });
    connect(hideShowGnssWindow, &QAction::triggered, this, [this] {
        rightBox->setVisible(!rightBox->isVisible());
        config->setShowGnssStatusWindow(rightBox->isVisible());
        restartWaterfall();
    });
    connect(hideShowIncidentlog, &QAction::triggered, this, [this] {
        incBox->setVisible(!incBox->isVisible());
        config->setShowIncidentLog(incBox->isVisible());
        restartWaterfall();
    });
    connect(toggleDarkMode, &QAction::triggered, this, [this] {
        config->setDarkMode(!config->getDarkMode());
    });
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
    fileMenu->addAction(open1809Act);
    fileMenu->addAction(openIqAct);
    fileMenu->addAction(openFolderAct);
    fileMenu->addSeparator();
    fileMenu->addAction(exitAct);

    optionMenu->addAction(optStation);
    optionMenu->addAction(optGnss);
    optionMenu->addAction(optStream);
    optionMenu->addAction(optSdef);
    optionMenu->addAction(optEmail);
    optionMenu->addAction(optCamera);
    optionMenu->addAction(optArduino);
    optionMenu->addAction(optAutoRecorder);
    optionMenu->addAction(optPositionReport);
    optionMenu->addAction(optGeoLimit);
    optionMenu->addAction(optMqtt);
    optionMenu->addAction(optOAuth);
    optionMenu->addAction(optIq);

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

    //QFormLayout *instrForm = new QFormLayout;
    instrGroupBox = new QGroupBox("Measurement receiver");
    instrGroupBox->setMinimumWidth(320);
    instrGroupBox->setMaximumWidth(320);

    instrForm->addRow(startFreqLabel, instrStartFreq);
    instrForm->addRow(stopFreqLabel, instrStopFreq);
    instrForm->addRow(modeLabel, instrResolution);
    instrForm->addRow(new QLabel("Measurement time (msec)"), instrMeasurementTime);
    instrForm->addRow(new QLabel("Manual attenuator"), instrAtt);
    instrForm->addRow(new QLabel("Auto attenuator"), instrAutoAtt);
    instrForm->addRow(new QLabel("Ant. port"), instrAntPort);
    instrForm->addRow(new QLabel("Instr. mode"), instrMode);
    instrForm->addRow(new QLabel("FFT mode"), instrFftMode);
    instrForm->addRow(new QLabel("Gain control"), instrGainControl);
    instrForm->addRow(new QLabel("IP address"), instrIpAddr);
    instrForm->addRow(new QLabel("Port"), instrPort);
    instrForm->addRow(instrConnect, instrDisconnect);

    instrGroupBox->setLayout(instrForm);

    QFormLayout *trigForm = new QFormLayout;
    trigGroupBox = new QGroupBox("Trigger settings");
    trigForm->addRow(new QLabel("Trig level (dB)"), instrTrigLevel);
    trigForm->addRow(new QLabel("Single trig bandwidth (kHz)"), instrTrigBandwidth);
    trigForm->addRow(new QLabel("Total trig bandwidth (kHz)"), instrTrigTotalBandwidth);
    trigForm->addRow(new QLabel("Min. trig time (msec)"), instrTrigTime);
    trigForm->addRow(separator);

    trigForm->addRow(new QLabel("C/NO deviation (dB)"), gnssCNo);
    trigForm->addRow(new QLabel("AGC deviation (%)"), gnssAgc);
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
    incBox = new QGroupBox("Incident log");
    incLayout->addWidget(incidentLog);
    incBox->setLayout(incLayout);
    incBox->setMaximumHeight(220);

    QHBoxLayout *bottomBox = new QHBoxLayout;

    QGridLayout *bottomIndicatorLayout = new QGridLayout;
    grpIndicator = new QGroupBox("Status indicators");

    bottomIndicatorLayout->addWidget(ledTraceStatus, 0, 0, Qt::AlignBottom);
    bottomIndicatorLayout->addWidget(labelTraceLedText, 0, 1);
    bottomIndicatorLayout->addWidget(ledRecordStatus, 1, 0, Qt::AlignBottom);
    bottomIndicatorLayout->addWidget(labelRecordLedText, 1, 1);
    bottomIndicatorLayout->addWidget(ledGnssStatus, 2, 0, Qt::AlignBottom);
    bottomIndicatorLayout->addWidget(labelGnssLedText, 2, 1);
    ledTraceStatus->setLedSize(30);
    ledRecordStatus->setLedSize(30);
    ledGnssStatus->setLedSize(30);

    grpIndicator->setMaximumHeight(220);

    grpIndicator->setLayout(bottomIndicatorLayout);
    bottomBox->addWidget(grpIndicator);
    bottomBox->addWidget(incBox);

    QHBoxLayout *rightLayout = new QHBoxLayout;

    rightLayout->addWidget(gnssStatus);
    rightBox->setLayout(rightLayout);
    rightBox->setMaximumWidth(240);
    rightBox->setMaximumHeight(220);
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
    if (config->getPmrMode())
        bottomPlotLayout->addWidget(btnPmrTable);

    plotLayout->addLayout(bottomPlotLayout, 3, 1, 1, 1, Qt::AlignHCenter);

    //plotMaxScroll->setFixedSize(40, 30);
    plotMaxScroll->setRange(-200, 200);
    plotMinScroll->setFixedSize(40, 30);
    plotMinScroll->setRange(-200, 200);
    plotMaxScroll->setValue(config->getPlotYMax());
    plotMinScroll->setValue(config->getPlotYMin());
    plotMaxholdTime->setFixedSize(40, 20);
    plotMaxholdTime->setRange(0, 120);
    waterfallTime->setRange(10, 86400);

    gridLayout->addLayout(leftLayout, 0, 0, 2, 1);
    gridLayout->addLayout(plotLayout, 0, 1, 1, 2);
    gridLayout->addLayout(statusBox, 1, 2, 1, 1);
    gridLayout->addLayout(bottomBox, 1, 1, 1, 1);

    centralWidget->setLayout(gridLayout);

    gnssStatus->setReadOnly(true);

    // Load default view settings
    instrGroupBox->setVisible(config->getShowReceiverControls());
    trigGroupBox->setVisible(config->getShowTriggerSettings());
    grpIndicator->setVisible(config->getShowStatusIndicators());
    rightBox->setVisible(config->getShowGnssStatusWindow());
    incBox->setVisible(config->getShowIncidentLog());

    instrMeasurementTime->setFocus();
}

void MainWindow::setToolTips()
{
    instrStartFreq->setToolTip("Start frequency in MHz of instrument scan");
    instrStopFreq->setToolTip("End frequency in MHz of instrument scan");
    instrResolution->setToolTip("Frequency resolution (kHz per step)");
    instrMeasurementTime->setToolTip("Time spent per periodic measurement, default 18 ms");
    instrAtt->setToolTip("Manual attenuation setting, deactivate auto attenuator to use");
    instrAutoAtt->setToolTip("Receiver controls attenuation based on input level. "
                             "Only working on newer models.");
    instrAntPort->setToolTip(
        "For receivers with multiple antenna inputs.\n"
        "To change name: Click and edit text, and press return when finished.\n"
        "The new name will be stored on the device (EM200 / USRP only)");
    instrMode->setToolTip("Use PScan to cover a wide frequency range. "
                          "FFM gives much better time resolution.");
    instrFftMode->setToolTip("FFT calculation method. Clear/write is the fastest method. "
                             "Max hold increases chances to record short burst signals.");
    instrIpAddr->setToolTip("IP address or station name. Press ENTER after writing IP manually");
    instrPort->setToolTip("SCPI port to connect to, default 5555");
    instrGainControl->setToolTip(
        "Gain mode for newer R&S instruments. Change according to RF environment");

    instrTrigLevel->setToolTip(
        "Decides how much above the average noise floor in dB a signal must be "
        "to trigger an incident. 10 dB is a reasonable value. "
        "See also the other trigger settings below.");
    instrTrigBandwidth->setToolTip("An incident is triggered if the continous bandwidth of a "
                                   "signal above the set trig level is higher than this setting."
                                   " See also the total bandwidth setting.");
    instrTrigTotalBandwidth->setToolTip(
        "An incident is triggered if the total contribution of signals "
        "above the trig limit is higher than this setting.");
    instrTrigTime->setToolTip(
        "Decides how long an incident must be present before an incident is "
        "triggered and a recording is started. "
        "Previously the \"trace count before recording is triggered\" setting.");

    gnssCNo->setToolTip("Carrier to noise level is averaged from the four best satellites in use. "
                        "If the difference between average and instant CNO is higher than \""
                        "this setting an incident will be triggered. Set to 0 to disable.");
    gnssAgc->setToolTip("AGC detection only for supported GNSS models, currently only U-Blox. "
                        "If the difference between the instant and the average AGC level is higher "
                        "than this setting an incident will be triggered. Set to 0 to disable.");
    gnssPosOffset->setToolTip(
        "Compares the current position with the one set in general station setup. "
        "If the offset is higher than this value an incident will be triggered. "
        "Set to 0 to disable.");
    gnssAltOffset->setToolTip(
        "Compared the altitude with the value set in general station setup. "
        "If the offset is higher than this value an incident will be triggered. "
        "Set to 0 to disable.");
    gnssTimeOffset->setToolTip(
        "Can be used to detect GNSS time spoofing. This requires the computer "
        "to be set up with NTP client software. The NTP time will then be "
        "compared with this value, and triggers an incident if the offset "
        "is higher than set value. Set to 0 to disable.");
    plotMaxScroll->setToolTip("Set the max. scale in dBuV");
    plotMinScroll->setToolTip("Set the min. scale in dBuV");
    plotMaxholdTime->setToolTip("Display maxhold time in seconds. Max 120 seconds. 0 for no "
                                "maxhold.\nOnly affects displayed maxhold");
    showWaterfall->setToolTip("Select type of waterfall overlay");
    waterfallTime->setToolTip(
        "Select the time in seconds a signal will be visible in the waterfall");
    btnTrigRecording->setToolTip(
        "Starts a recording manually. The recording will end after the time specified in SDeF "
        "options: Recording time after incident,\nunless a real trig happens within this time, as "
        "this will extend the recording further.");
    btnPmrTable->setToolTip("Edit the PMR table in the currently chosen frequency range");
}

void MainWindow::getConfigValues()
{
    instrStartFreq->setValue(config->getInstrStartFreq());
    instrStopFreq->setValue(config->getInstrStopFreq());
    instrMeasurementTime->setValue(config->getInstrMeasurementTime());
    instrAtt->setValue(config->getInstrManAtt());
    instrAutoAtt->setChecked(config->getInstrAutoAtt());
    instrAntPort->setCurrentIndex(config->getInstrAntPort());
    instrMode->setCurrentIndex(instrMode->findText(config->getInstrMode()));
    //instrFftMode->setCurrentIndex(config->getInstrFftMode());
    instrIpAddr->setEditable(true);
    instrPort->setText(QString::number(config->getInstrPort()));
    instrAutoAttChanged();
    instrModeChanged();
    //instrFftModeChanged();
    instrMeasurementTimeChanged();
    instrAttChanged();
    //instrIpChanged();
    instrPortChanged();
    instrFfmCenterFreq->setValue(config->getInstrFfmCenterFreq());

    instrAntPort->setEditable(true);
    instrAntPort->setLineEdit(antPortLineEdit);
    instrAntPort->setInsertPolicy(QComboBox::NoInsert);

    instrGainControl->setEditable(false);
    disconnect(instrGainControl,
               &QComboBox::currentIndexChanged,
               this,
               &MainWindow::instrGainControlChanged);
    instrGainControl->addItems(QStringList() << "Low noise"
                                             << "Normal"
                                             << "Low distortion");
    connect(instrGainControl,
            &QComboBox::currentIndexChanged,
            this,
            &MainWindow::instrGainControlChanged);

    /*if (instrResolution->findText(QString::number(config->getInstrResolution())) >= 0)
        instrResolution->setCurrentIndex(instrResolution->findText(QString::number(config->getInstrResolution())));*/

    updInstrButtonsStatus();

    instrTrigLevel->setValue(config->getInstrTrigLevel());
    instrTrigBandwidth->setValue(config->getInstrMinTrigBW());
    instrTrigTotalBandwidth->setValue(config->getInstrTotalTrigBW());
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
        /*QHostAddress test(instrIpAddr->currentText());
        if (!test.isNull() && instrPort->text().toUInt() > 0)*/
        instrConnect->setEnabled(true);
        /*else
            instrConnect->setEnabled(false);*/
    }
    /*else {
    }*/
}

void MainWindow::instrModeChanged()
{
    if (instrMode->currentText().contains("pscan", Qt::CaseInsensitive)/* &&
        measurementDevice->currentMode() != Instrument::Mode::PSCAN*/) {
        startFreqLabel->setText("Start frequency (MHz)");
        stopFreqLabel->setHidden(false);
        instrStopFreq->setHidden(false);
        modeLabel->setText("Pscan resolution (kHz)");

        instrForm->replaceWidget(instrFfmCenterFreq, instrStartFreq);
        instrForm->replaceWidget(instrFfmSpan, instrResolution);
        instrFfmCenterFreq->setHidden(true);
        instrFfmSpan->setHidden(true);
        instrStartFreq->setHidden(false);
        instrResolution->setHidden(false);
        config->setInstrMode(instrMode->currentText());
        measurementDevice->setMode();
    }
    else if (instrMode->currentText().contains("ffm", Qt::CaseInsensitive)/* &&
             measurementDevice->currentMode() != Instrument::Mode::FFM*/) {
        startFreqLabel->setText("FFM center frequency (MHz)");
        stopFreqLabel->setHidden(true);
        instrStopFreq->setHidden(true);
        modeLabel->setText("FFM span (kHz)");

        instrForm->replaceWidget(instrStartFreq, instrFfmCenterFreq);
        instrForm->replaceWidget(instrResolution, instrFfmSpan);
        instrFfmCenterFreq->setHidden(false);
        instrFfmSpan->setHidden(false);
        instrStartFreq->setHidden(true);
        instrResolution->setHidden(true);
        config->setInstrMode(instrMode->currentText());
        measurementDevice->setMode();
    }
    instrStartFreqChanged();
    instrStopFreqChanged();
    setResolutionFunction();
}

void MainWindow::setValidators()
{
    instrStartFreq->setRange(20, 6e3);
    instrStartFreq->setDecimals(0);
    instrFfmCenterFreq->setRange(0.1, 8e3);
    instrFfmCenterFreq->setDecimals(0);
    instrStopFreq->setRange(20, 6e3);
    instrStopFreq->setDecimals(0);
    instrMeasurementTime->setRange(1, 5000);
    instrAtt->setRange(-40, 40);

    /*QString ipRange = "(([ 0]+)|([ 0]*[0-9] *)|([0-9][0-9] )|([ 0][0-9][0-9])|(1[0-9][0-9])|([2][0-4][0-9])|(25[0-5]))";
    QRegularExpression ipRegex ("^" + ipRange
                               + "\\." + ipRange
                               + "\\." + ipRange
                               + "\\." + ipRange + "$");
    QRegularExpressionValidator *ipValidator = new QRegularExpressionValidator(ipRegex, this);
    instrIpAddr->setValidator(ipValidator);
    instrIpAddr->setInputMask("000.000.000.000");
    instrIpAddr->setCursorPosition(0);*/

    instrTrigLevel->setRange(-50, 200);
    instrTrigLevel->setDecimals(0);
    instrTrigTime->setRange(0, 9e5);
    instrTrigBandwidth->setRange(0, 9.99e9);
    instrTrigBandwidth->setDecimals(0);
    instrTrigTotalBandwidth->setRange(0, 9.99e9);
    instrTrigTotalBandwidth->setDecimals(0);
    gnssCNo->setRange(0, 1000);
    gnssAgc->setRange(0, 8000);
    gnssPosOffset->setRange(0, 48e6);
    gnssAltOffset->setRange(0, 10e3);
    gnssTimeOffset->setRange(0, 10e6);
}

void MainWindow::instrStartFreqChanged()
{
    if (measurementDevice->currentMode() == Instrument::Mode::PSCAN) {
        config->setInstrStartFreq(instrStartFreq->value());
    }
    if (measurementDevice->isConnected())
        traceBuffer->restartCalcAvgLevel();
}

void MainWindow::instrStopFreqChanged()
{
    if (measurementDevice->currentMode() == Instrument::Mode::PSCAN) {
        config->setInstrStartFreq(instrStartFreq->value());
        config->setInstrStopFreq(instrStopFreq->value());
    }
    if (measurementDevice->isConnected())
        traceBuffer->restartCalcAvgLevel();
}

void MainWindow::instrFfmCenterFreqChanged()
{
    if (measurementDevice->currentMode() == Instrument::Mode::FFM) {
        config->setInstrFfmCenterFreq(instrFfmCenterFreq->value());
    }
    if (measurementDevice->isConnected()) {
        traceBuffer->restartCalcAvgLevel();
        waterfall->restartPlot();
    }
    iqdataWaterfall->setFfmFrequency(instrFfmCenterFreq->value());
}

void MainWindow::instrFfmSpanChanged()
{
    config->setInstrFfmSpan(instrFfmSpan->currentText());
    if (measurementDevice->isConnected()) {
        traceBuffer->restartCalcAvgLevel();
        waterfall->restartPlot();
    }
}

void MainWindow::instrMeasurementTimeChanged()
{
    config->setInstrMeasurementTime(instrMeasurementTime->value());
}

void MainWindow::instrAttChanged()
{
    config->setInstrManAtt(instrAtt->text().toInt());
    if (measurementDevice->isConnected())
        traceBuffer->restartCalcAvgLevel();
}

void MainWindow::instrAutoAttChanged()
{
    instrAtt->setDisabled(instrAutoAtt->isChecked());
    config->setInstrAutoAtt(instrAutoAtt->isChecked());
    if (measurementDevice->isConnected())
        traceBuffer->restartCalcAvgLevel();
}

void MainWindow::instrIpChanged()
{
    if (instrIpAddr->currentData().isNull()) { // custom IP/hostname added
        instrIpAddr->setItemData(instrIpAddr->currentIndex(), instrIpAddr->currentText());
    }
    //config->setInstrIpAddr(instrIpAddr->currentText());
    config->setInstrIpAddr(instrIpAddr->currentData().toString());
    measurementDevice->setAddress(config->getInstrIpAddr());
    qDebug() << instrIpAddr->currentText() << instrIpAddr->currentData();
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
        instrGainControlChanged();
        instrStartFreq->setMinimum(measurementDevice->deviceMinFreq() / 1e6);
        instrStopFreq->setMaximum(measurementDevice->deviceMaxFreq() / 1e6);
    } else {
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

    instrMode->setEnabled(state);
    instrFftMode->setEnabled(state);
    instrDisconnect->setEnabled(state);
    instrConnect->setDisabled(state);
    instrIpAddr->setDisabled(state);
    instrPort->setDisabled(state);
    instrAntPort->setEnabled(state);
    instrGainControl->setEnabled(state);
    btnTrigRecording->setEnabled(state);
}

void MainWindow::setResolutionFunction()
{
    disconnect(instrResolution,
               &QComboBox::currentIndexChanged,
               this,
               &MainWindow::instrResolutionChanged);
    disconnect(instrFfmSpan,
               &QComboBox::currentIndexChanged,
               this,
               &MainWindow::instrFfmSpanChanged);

    instrResolution->clear();
    instrResolution->addItems(measurementDevice->devicePscanResolutions());
    //qDebug() << "pscan debug" << config->getInstrResolution() << measurementDevice->devicePscanResolutions();
    if (instrResolution->findText(config->getInstrResolution()) >= 0)
        instrResolution->setCurrentIndex(instrResolution->findText(config->getInstrResolution()));

    instrFfmSpan->clear();
    instrFfmSpan->addItems(measurementDevice->deviceFfmSpans());
    //qDebug() << "FFM debug" << config->getInstrFfmSpan() << measurementDevice->deviceFfmSpans();
    if (instrFfmSpan->findText(config->getInstrFfmSpan()) >= 0)
        instrFfmSpan->setCurrentIndex(instrFfmSpan->findText(config->getInstrFfmSpan()));

    connect(instrResolution,
            &QComboBox::currentIndexChanged,
            this,
            &MainWindow::instrResolutionChanged);
    connect(instrFfmSpan, &QComboBox::currentIndexChanged, this, &MainWindow::instrFfmSpanChanged);
}

void MainWindow::instrResolutionChanged() // pscan res. change
{
    //if (measurementDevice->isConnected()) { // save settings only when connected and mode established
    config->setInstrResolution(instrResolution->currentText());
    //}
}

void MainWindow::setDeviceAntPorts()
{
    instrAntPort->clear();
    int index = config->getInstrAntPort();
    instrAntPort->addItems(measurementDevice->deviceAntPorts());
    instrAntPort->setEnabled(instrAntPort->count() > 1);

    //qDebug() << "ant index change:" << index;
    instrAntPort->setCurrentIndex(index);
    emit antennaPortChanged();
}

void MainWindow::setDeviceFftModes()
{
    instrFftMode->clear();
    QString val = config->getInstrFftMode(); // tmp keeper
    instrFftMode->addItems(measurementDevice->deviceFftModes());
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

void MainWindow::uploadProgress(int percent)
{
    progressBar->setValue(percent);
}

void MainWindow::updWindowTitle(const QString msg)
{
    QString extra;
    if (!msg.isEmpty())
        extra = tr(" using ") + msg + " - " + instrIpAddr->currentText() + " ("
                + instrIpAddr->currentData().toString() + ")";
    setWindowTitle(tr("Hauken v") + PROJECT_VERSION + extra + " ("
                   + config->getCurrentFilename().split('/').last() + ")");
}

void MainWindow::about()
{
    QString txt;
    QTextStream ts(&txt);
    QMessageBox box;
    box.setTextFormat(Qt::RichText);
    box.setStandardButtons(QMessageBox::Ok);


    ts << "<table><tr><td>Application version</td><td>" << QString(FULL_VERSION) << "</td></tr>";
    ts << "<tr><td>Build date and time</td><td>" << __DATE__ << " / " << __TIME__
       << "</td></tr><tr></tr>";
    ts << "<tr><td><a href='https://github.com/cutelyst/simple-mail'>SimpleMail Qt library SMTP "
          "mail client</a></td><td>LGPL 2.1 license"
       << "</td></tr>";
    ts << "<tr><td>Questions/support? => JSK</td></tr></table>";
    box.setText(txt);
    box.exec();
}

void MainWindow::aboutQt()
{
    QMessageBox::aboutQt(this);
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
    QString filename = QFileDialog::getSaveFileName(this,
                                                    tr("New configuration file"),
                                                    config->getWorkFolder(),
                                                    tr("Configuration files (*.ini)"));
    if (!filename.isEmpty())
        config->newFileName(filename);
    if (measurementDevice->isConnected())
        measurementDevice->instrDisconnect();
    getConfigValues();
    updWindowTitle();
}

void MainWindow::open()
{
    QString fileName = QFileDialog::getOpenFileName(this,
                                                    tr("Open configuraton file"),
                                                    config->getWorkFolder(),
                                                    tr("Configuration files (*.ini)"));

    if (!fileName.isEmpty())
        config->newFileName(fileName);
    getConfigValues();
    updWindowTitle();
}

void MainWindow::save()
{
    QString fileName = QFileDialog::getSaveFileName(this,
                                                    tr("Save current configuration"),
                                                    config->getWorkFolder(),
                                                    tr("Configuration files (*.ini)"));
    if (!fileName.isEmpty()) { //config->newFileName(fileName);
        QFile::copy(config->getCurrentFilename(), fileName);
        config->newFileName(fileName);
    }
    //saveConfigValues();
    updWindowTitle();
}

void MainWindow::saveConfigValues()
{
 
}

void MainWindow::showBytesPerSec(int val)
{
    if (!config->getGeoLimitActive() || geoLimit->areWeInsidePolygon()) {
        QString msg = "Traffic " + QString::number(val / 1000) + " kB/sec";
        if (tracesPerSecond > 0)
            msg += ", " + QString::number(tracesPerSecond, 'f', 1) + " traces/sec";
        int khzAboveLimit = traceAnalyzer->getKhzAboveLimit();
        int khzAboveLimitTotal = traceAnalyzer->getKhzAboveLimitTotal();
        if (khzAboveLimit || khzAboveLimitTotal)
            msg += ", single/total signal above limit: " + QString::number(khzAboveLimit) + " / "
                   + QString::number(khzAboveLimitTotal) + " kHz";

        statusBar->showMessage(msg, 2000);
    }
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
    if (config->getInstrConnectOnStartup() && instrCheckSettings()) {
        measurementDevice->instrConnect();
    }
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

void MainWindow::triggerSettingsUpdate() {}

void MainWindow::updGnssBox(const QString txt, const int id, bool valid)
{
    if (id == gnssLastDisplayedId) { // same as previous update, just show result
        gnssStatus->setText(txt);
        rightBox->setTitle((id < 3 ? "GNSS " + QString::number(id) : "InstrumentGNSS") + " status ("
                           + (valid ? "pos. valid" : "pos. invalid") + ")");
    } else if (gnssLastDisplayedTime.secsTo(QDateTime::currentDateTime()) >= 5) {
        gnssLastDisplayedId = id;
        gnssLastDisplayedTime = QDateTime::currentDateTime();
        rightBox->setTitle((id < 3 ? "GNSS " + QString::number(id) : "InstrumentGNSS") + " status ("
                           + (valid ? "pos. valid" : "pos. invalid") + ")");
        gnssStatus->setText(txt);
    }
}

void MainWindow::setWaterfallOption(QString s)
{
    if (s == "Off")
        dispWaterfall = false;
    else
        dispWaterfall = true;

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

void MainWindow::changeAntennaPortName()
{
    emit antennaNameEdited(instrAntPort->currentIndex(), instrAntPort->currentText());
}

void MainWindow::traceIncidentAlarm(bool state)
{
    if (state != traceAlarmRaised) {
        if (state) {
            ledTraceStatus->setState(false);
            labelTraceLedText->setText("Detector triggered");
            qApp->alert(this->parentWidget());
            if (config->getSoundNotification() && !notificationTimer->isActive()) {
                player->play();
                notificationTimer->start(config->getNotifyTruncateTime()
                                         * 1000); // Mute for x secs to not annoy user
            }
            traceAlarmRaised = true;
        } else {
            ledTraceStatus->setState(true);
            labelTraceLedText->setText("Normal");
            traceAlarmRaised = false;
        }
    }
}

void MainWindow::recordIncidentAlarm(bool state)
{
    if (state != recordAlarmRaised) {
        if (state) {
            ledRecordStatus->setState(false);
            labelRecordLedText->setText("Recording");
            recordAlarmRaised = true;
        } else {
            ledRecordStatus->setState(true);
            labelRecordLedText->setText("Ready to record");
            recordAlarmRaised = false;
        }
    }
}

void MainWindow::gnssIncidentAlarm(bool state)
{
    if (state != gnssAlarmRaised) {
        if (state) {
            ledGnssStatus->setState(false);
            labelGnssLedText->setText("GNSS alarm");
            gnssAlarmRaised = true;
            qApp->alert(this);
        } else {
            ledGnssStatus->setState(true);
            labelGnssLedText->setText("GNSS normal");
            gnssAlarmRaised = false;
        }
    }
}

void MainWindow::recordEnabled(bool state)
{
    if (state != !recordDisabledRaised) {
        if (!state) {
            ledRecordStatus->setOffColor(Qt::gray);
            ledRecordStatus->setState(false);
            labelRecordLedText->setText("Recording disabled");
            recordDisabledRaised = true;
        } else {
            ledRecordStatus->setState(true);
            ledRecordStatus->setOffColor(Qt::red);
            labelRecordLedText->setText("Ready to record");
            recordDisabledRaised = false;
        }
    }
}

#ifndef QT_NO_CONTEXTMENU
void MainWindow::contextMenuEvent(QContextMenuEvent *event)
{
    QMenu menu(this);
    menu.addAction(hideShowControls);
    menu.addAction(hideShowTrigSettings);
    menu.addAction(hideShowStatusIndicator);
    menu.addAction(hideShowGnssWindow);
    menu.addAction(hideShowIncidentlog);
    menu.addAction(toggleDarkMode);
    menu.exec(event->globalPos());
}
#endif // QT_NO_CONTEXTMENU

void MainWindow::restartWaterfall()
{
    QTimer::singleShot(50, this, [this] { // short delay to allow screen to update before restarting
        waterfall->updSize(customPlot->axisRect()->rect());
    });
}

void MainWindow::instrGainControlChanged(int index)
{
    if (index == -1)
        index = config->getInstrGainControl();
    else
        config->setInstrGainControl(index);

    qDebug() << "Gain control changed:" << index;
    instrGainControl->setCurrentIndex(index);

    if (measurementDevice->deviceHasGainControl()) {
        measurementDevice->setGainControl(index);
        if (!instrGainControl->isEnabled())
            instrGainControl->setEnabled(true);
        traceBuffer->restartCalcAvgLevel();
    } else {
        if (instrGainControl->isEnabled())
            instrGainControl->setEnabled(false);
    }
}
