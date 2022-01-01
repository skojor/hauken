#include "mainwindow.h"


MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
{
    setStatusBar(statusBar);
    updWindowTitle();
    resize(1200, 650);
    setMinimumSize(1024, 650);
    restoreGeometry(config->getWindowGeometry());
    restoreState(config->getWindowState());

    qApp->setFont(QApplication::font("QMessageBox"));
    instrMode->addItems(QStringList() << "PScan" << "FFM");
    instrFftMode->addItems(QStringList() << "Cl/wr" << "Min" << "Max" << "Avg");

    customPlotController = new CustomPlotController(customPlot, config);
    customPlotController->init();

    generalOptions = new GeneralOptions(config);
    receiverOptions = new ReceiverOptions(config);
    sdefOptions = new SdefOptions(config);

    sdefRecorderThread->setObjectName("SdefRecorder");
    sdefRecorder->moveToThread(sdefRecorderThread);

    createActions();
    createMenus();
    createLayout();
    setToolTips();
    setValidators();
    setSignals();
    getConfigValues();
    setupIncidentTable();
    instrAutoConnect();
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
    connect(openAct, &QAction::triggered, this, open);

    saveAct = new QAction(tr("&Save as..."), this);
    saveAct->setShortcuts(QKeySequence::Save);
    saveAct->setStatusTip(tr("Save the current configuration to disk"));
    connect(saveAct, &QAction::triggered, this, save);

    optStation = new QAction(tr("&General setup"), this);
    optStation->setStatusTip(tr("Basic station setup (position, folders, etc.)"));
    connect(optStation, &QAction::triggered, this, stnConfig);

    optGnss = new QAction(tr("G&NSS"), this);
    optGnss->setStatusTip(tr("GNSS ports and logging configuration"));
    connect(optGnss, &QAction::triggered, this, gnssConfig);

    optStream = new QAction(tr("&Receiver setup"), this);
    optStream->setStatusTip(tr("Measurement receiver device options"));
    connect(optStream, &QAction::triggered, this, streamConfig);

    optSdef = new QAction(tr("&SDEF"), this);
    optSdef->setStatusTip(tr("Configuration of 1809 format and options"));
    connect(optSdef, &QAction::triggered, this, sdefConfig);

    aboutAct = new QAction(tr("&About"), this);
    aboutAct->setStatusTip(tr("Show the application's About box"));
    connect(aboutAct, &QAction::triggered, this, about);

    aboutQtAct = new QAction(tr("About &Qt"), this);
    aboutQtAct->setStatusTip(tr("Show the Qt library's About box"));
    connect(aboutQtAct, &QAction::triggered, this, aboutQt);

    changelogAct = new QAction(tr("&Changelog"), this);
    changelogAct->setStatusTip(tr("Show the application changelog"));
    connect(changelogAct, &QAction::triggered, this, changelog);

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
    instrGroupBox->setMinimumWidth(250);
    instrGroupBox->setMaximumWidth(250);

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
    trigGroupBox->setMinimumWidth(250);
    trigGroupBox->setMaximumWidth(250);

    trigGroupBox->setLayout(trigForm);

    leftLayout->addWidget(instrGroupBox);
    leftLayout->addWidget(trigGroupBox);

    QHBoxLayout *incLayout = new QHBoxLayout;
    QGroupBox *incBox = new QGroupBox("Incident log");
    incLayout->addWidget(incidentLog);
    incBox->setLayout(incLayout);
    incBox->setMaximumHeight(150);

    QHBoxLayout *bottomBox = new QHBoxLayout;
    bottomBox->addWidget(incBox);

    QHBoxLayout *rightLayout = new QHBoxLayout;
    QGroupBox *rightBox = new QGroupBox("GNSS status");
    rightLayout->addWidget(gnssStatus);
    rightBox->setLayout(rightLayout);
    rightBox->setMaximumWidth(200);
    rightBox->setMaximumHeight(150);
    rightLayout->addWidget(gnssStatus);

    QHBoxLayout *statusBox = new QHBoxLayout;
    statusBox->addWidget(rightBox);

    QGridLayout *plotLayout = new QGridLayout;
    plotLayout->addWidget(plotMaxScroll, 0, 0, 1, 1);
    plotLayout->addWidget(plotMinScroll, 2, 0, 1, 1);
    plotLayout->addWidget(customPlot, 0, 1, 3, 1);
    QHBoxLayout *bottomPlotLayout = new QHBoxLayout;
    bottomPlotLayout->addWidget(new QLabel("Maxhold time (seconds)"));
    bottomPlotLayout->addWidget(plotMaxholdTime);
    plotLayout->addLayout(bottomPlotLayout, 3, 1, 1, 1, Qt::AlignHCenter);
    plotMaxScroll->setFixedSize(40, 30);
    plotMaxScroll->setRange(-30,200);
    plotMinScroll->setFixedSize(40, 30);
    plotMinScroll->setRange(-50, 170);
    plotMaxScroll->setValue(config->getPlotYMax());
    plotMinScroll->setValue(config->getPlotYMin());
    plotMaxholdTime->setFixedSize(40,30);
    plotMaxholdTime->setRange(0, 120);

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
    instrTrigBandwidth->setToolTip("An incident is triggered only if the total bandwidth of the "\
                                   "signal above the set trig level is at least the same "\
                                   "as this setting. Previously the \"percentage of spectrum\" setting.");
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
    instrFftMode->setCurrentIndex(config->getInstrFftMode());
    instrIpAddr->setText(config->getInstrIpAddr());
    instrPort->setText(QString::number(config->getInstrPort()));
    instrAutoAttChanged();
    instrModeChanged();
    instrFftModeChanged();
    instrFreqChanged();
    instrMeasurementTimeChanged();
    instrAttChanged();
    instrAntPortChanged();
    instrIpChanged();
    instrPortChanged();

    if (instrResolution->findText(QString::number(config->getInstrResolution())) >= 0)
        instrResolution->setCurrentIndex(instrResolution->findText(QString::number(config->getInstrResolution())));

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
    setResolutionOrSpan();
    if (a >= 0 && instrResolution->findText(QString::number(config->getInstrResolution())) > 0)
        instrResolution->setCurrentIndex(instrResolution->findText(QString::number(config->getInstrResolution())));
    config->setInstrMode(instrMode->currentIndex());
    instrFreqChanged();
    instrResolutionChanged();
}

void MainWindow::setResolutionOrSpan()
{
    if (instrMode->currentIndex() == 0) {
        instrResolution->clear();
        instrResolution->addItems(QStringList() << "0.1" << "0.125" << "0.2" << "0.250" << "0.5" << "0.625" << "1"
                                  << "1.25" << "2" << "2.5" << "3.125" << "5" << "6.25" << "8.33" << "10" << "12.5"
                                  << "20" << "25" << "50" << "100" << "200" << "500" << "1000" << "2000");
    }
    else {
        instrResolution->clear();
        instrResolution->addItems(QStringList() << "1" << "2" << "5" << "10" << "20" << "50" << "100" << "200"
                                  << "500" << "1000" << "2000" << "5000" << "10000" << "20000");
    }
    if (instrResolution->findText(QString::number(config->getInstrResolution())) > 0)
        instrResolution->setCurrentIndex(instrResolution->findText(QString::number(config->getInstrResolution())));
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

    instrTrigLevel->setRange(0, 200);
    instrTrigLevel->setDecimals(1);
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
    connect(instrStartFreq, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, instrFreqChanged);
    connect(instrStopFreq, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, instrFreqChanged);
    connect(instrResolution, QOverload<int>::of(&QComboBox::currentIndexChanged), this, instrResolutionChanged);
    connect(instrMeasurementTime, QOverload<int>::of(&QSpinBox::valueChanged), config.data(), &Config::setInstrMeasurementTime);
    connect(instrAtt, QOverload<int>::of(&QSpinBox::valueChanged), config.data(), &Config::setInstrManAtt);
    connect(instrAutoAtt, &QCheckBox::toggled, this, &MainWindow::instrAutoAttChanged);
    connect(instrAntPort, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &MainWindow::instrAntPortChanged);
    connect(instrMode, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &MainWindow::instrModeChanged);
    connect(instrFftMode, QOverload<int>::of(&QComboBox::currentIndexChanged), config.data(), &Config::setInstrFftMode);
    connect(instrIpAddr, &QLineEdit::textChanged, this, updInstrButtonsStatus);
    connect(instrIpAddr, &QLineEdit::editingFinished, this, instrIpChanged);
    connect(instrPort, &QLineEdit::editingFinished, this, instrPortChanged);

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

    connect(measurementDevice, &MeasurementDevice::connectedStateChanged, this, instrConnected);
    connect(measurementDevice, &MeasurementDevice::connectedStateChanged, sdefRecorder, &SdefRecorder::deviceDisconnected);

    connect(measurementDevice, &MeasurementDevice::popup, this, generatePopup);
    connect(measurementDevice, &MeasurementDevice::status, this, updateStatusLine);
    connect(measurementDevice, &MeasurementDevice::instrId, this, updWindowTitle);
    connect(measurementDevice, &MeasurementDevice::toIncidentLog, this, appendToIncidentLog);
    connect(measurementDevice, &MeasurementDevice::bytesPerSec, this, &MainWindow::showBytesPerSec);
    connect(measurementDevice, &MeasurementDevice::tracesPerSec, sdefRecorder, &SdefRecorder::updTracesPerSecond);

    connect(plotMaxScroll, QOverload<int>::of(&QSpinBox::valueChanged), config.data(), &Config::setPlotYMax);
    connect(plotMinScroll, QOverload<int>::of(&QSpinBox::valueChanged), config.data(), &Config::setPlotYMin);
    connect(plotMaxholdTime, QOverload<int>::of(&QSpinBox::valueChanged), config.data(), &Config::setPlotMaxholdTime);

    connect(traceBuffer, &TraceBuffer::newDispTrace, customPlotController, &CustomPlotController::plotTrace);
    connect(traceBuffer, &TraceBuffer::newDispMaxhold, customPlotController, &CustomPlotController::plotMaxhold);
    connect(traceBuffer, &TraceBuffer::showMaxhold, customPlotController, &CustomPlotController::showMaxhold);
    connect(traceBuffer, &TraceBuffer::newDispTriglevel, customPlotController, &CustomPlotController::plotTriglevel);
    connect(traceBuffer, &TraceBuffer::toIncidentLog, this, &MainWindow::appendToIncidentLog);
    connect(traceBuffer, &TraceBuffer::reqReplot, customPlotController, &CustomPlotController::doReplot);
    connect(customPlotController, &CustomPlotController::reqTrigline, traceBuffer, &TraceBuffer::sendDispTrigline);
    connect(traceBuffer, &TraceBuffer::averageLevelCalculating, customPlotController, &CustomPlotController::flashTrigline);
    connect(traceBuffer, &TraceBuffer::averageLevelReady, customPlotController, &CustomPlotController::stopFlashTrigline);

    connect(measurementDevice, &MeasurementDevice::newTrace, traceBuffer, &TraceBuffer::addTrace);
    connect(measurementDevice, &MeasurementDevice::newTrace, traceAnalyzer, &TraceAnalyzer::setTrace);
    connect(measurementDevice, &MeasurementDevice::resetBuffers, traceBuffer, &TraceBuffer::emptyBuffer);

    connect(traceBuffer, &TraceBuffer::averageLevelCalculating, traceAnalyzer, &TraceAnalyzer::resetAverageLevel);
    connect(traceBuffer, &TraceBuffer::averageLevelReady, traceAnalyzer, &TraceAnalyzer::setAverageTrace);
    connect(traceAnalyzer, &TraceAnalyzer::toIncidentLog, this, &MainWindow::appendToIncidentLog);
    connect(customPlotController, &CustomPlotController::freqSelectionChanged, traceAnalyzer, &TraceAnalyzer::updTrigFrequencyTable);

    connect(config.data(), &Config::settingsUpdated, customPlotController, &CustomPlotController::updSettings);
    connect(config.data(), &Config::settingsUpdated, measurementDevice, &MeasurementDevice::updSettings);
    connect(config.data(), &Config::settingsUpdated, traceAnalyzer, &TraceAnalyzer::updSettings);
    connect(config.data(), &Config::settingsUpdated, traceBuffer, &TraceBuffer::updSettings);
    connect(config.data(), &Config::settingsUpdated, sdefRecorder, &SdefRecorder::updSettings);

    connect(traceAnalyzer, &TraceAnalyzer::alarm, sdefRecorder, &SdefRecorder::triggerRecording);
    connect(sdefRecorder, &SdefRecorder::recordingStarted, traceAnalyzer, &TraceAnalyzer::recorderStarted);
    connect(sdefRecorder, &SdefRecorder::recordingEnded, traceAnalyzer, &TraceAnalyzer::recorderEnded);
    connect(traceAnalyzer, &TraceAnalyzer::toRecorder, sdefRecorder, &SdefRecorder::receiveTrace);
    connect(sdefRecorder, &SdefRecorder::reqTraceHistory, traceBuffer, &TraceBuffer::getSecondsOfBuffer);
    connect(traceBuffer, &TraceBuffer::historicData, sdefRecorder, &SdefRecorder::receiveTraceBuffer);

    connect(sdefRecorderThread, &QThread::started, sdefRecorder, &SdefRecorder::start);
    connect(sdefRecorder, &SdefRecorder::warning, this, &MainWindow::generatePopup);

    sdefRecorderThread->start();
}

void MainWindow::instrFreqChanged()
{
    if (config->getInstrMode() == 0 && instrStartFreq->value() < instrStopFreq->value()) { //Pscan
        config->setInstrStartFreq(instrStartFreq->value());
        config->setInstrStopFreq(instrStopFreq->value());
    }
    else { // ffm
        config->setInstrFfmCenterFreq(instrStartFreq->value());
        measurementDevice->setFfmCenterFrequency();
    }
    traceBuffer->restartCalcAvgLevel();
}

void MainWindow::instrResolutionChanged(int a)
{
    if (a > 0) config->setInstrResolution(instrResolution->currentText().toDouble());
    traceBuffer->restartCalcAvgLevel();
}

void MainWindow::instrMeasurementTimeChanged()
{
    config->setInstrMeasurementTime(instrMeasurementTime->value());
}

void MainWindow::instrAttChanged()
{
    config->setInstrManAtt(instrAtt->text().toInt());
}

void MainWindow::instrAutoAttChanged()
{
    instrAtt->setDisabled(instrAutoAtt->isChecked());
}

void MainWindow::instrAntPortChanged(int a)
{
    (void)a;
    config->setInstrAntPort(instrAntPort->currentText());
}

void MainWindow::instrFftModeChanged(int a)
{
    (void)a;
    config->setInstrFftMode(instrFftMode->currentIndex());
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

void MainWindow::instrConnected(bool state)
{
    instrDisconnect->setEnabled(state);
    instrConnect->setDisabled(state);
    instrIpAddr->setDisabled(state);
    instrPort->setDisabled(state);
    instrAntPort->clear();
    instrAntPort->addItems(measurementDevice->getAntPorts());

    instrAntPort->setDisabled(instrAntPort->count() <= 1);
    if (!state) {
        traceBuffer->deviceDisconnected(); // stops buffer work when not needed
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
    setWindowTitle(tr("Hauken v. ") + qApp->applicationVersion() + extra);
}

void MainWindow::about()
{
    QString txt;
    QTextStream ts(&txt);
    ts << "Application version " << SW_VERSION;
    ts << "\nAdd more info here...";
    QMessageBox::about(this, "About Hauken", txt);
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
    ts << "<table><tr><td>2.0.b</td>" << "<td>Initial commit, basic instrument control</td>";

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
    QString filename = QFileDialog::getSaveFileName(this, tr("New configuration file"),
                                                    config->getWorkFolder(),
                                                    tr("Configuration files (*.ini)"));
    if (!filename.isEmpty()) config->newFileName(filename);
    if (measurementDevice->isConnected())
        measurementDevice->instrDisconnect();
    getConfigValues();
}

void MainWindow::open()
{
    QString fileName = QFileDialog::getOpenFileName(this, tr("Open configuraton file"),
                                                    config->getWorkFolder(),
                                                    tr("Configuration files (*.ini)"));

    if (!fileName.isEmpty()) config->newFileName(fileName);
    getConfigValues();
}

void MainWindow::save()
{
    QString fileName = QFileDialog::getSaveFileName(this, tr("Save current configuration"),
                                                    config->getWorkFolder(),
                                                    tr("Configuration files (*.ini)"));
    if (!fileName.isEmpty()) config->newFileName(fileName);
    saveConfigValues();
}

void MainWindow::saveConfigValues()
{
    config->setInstrStartFreq(instrStartFreq->value());
    config->setInstrStopFreq(instrStopFreq->value());
    config->setInstrResolution(instrResolution->currentText().toDouble());
    config->setInstrMeasurementTime(instrMeasurementTime->value());
    config->setInstrManAtt(instrAtt->text().toUInt());
    config->setInstrAutoAtt(instrAutoAtt->isChecked());
    config->setInstrAntPort(instrAntPort->currentText());
    config->setInstrMode(instrMode->currentIndex());
    config->setInstrFftMode(instrFftMode->currentIndex());
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
}

void MainWindow::appendToIncidentLog(QString incident)
{
    QString text;
    QTextStream ts(&text);
    ts << "<tr><td>" << QDate::currentDate().toString("dd.MM.yy") << "</td><td>"
       << QTime::currentTime().toString("hh:mm:ss") << "</td><td>" << incident
       << "</td></tr>";

    incidentLog->append(text);
}

void MainWindow::setupIncidentTable()
{
    incidentLog->setAcceptRichText(true);
    incidentLog->setReadOnly(true);
    incidentLog->append("<table><td><th width=\50>Date</th><th width=50>Time</th><th width=100 valign=left>Text</th>");
    appendToIncidentLog("Application started");
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
