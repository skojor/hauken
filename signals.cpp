#include "mainwindow.h"

void MainWindow::setSignals()
{
    connect(instrStartFreq, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &MainWindow::instrStartFreqChanged);
    connect(instrStopFreq, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &MainWindow::instrStopFreqChanged);
    connect(instrFfmCenterFreq, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &MainWindow::instrFfmCenterFreqChanged); //Taken care of after init.
    connect(instrResolution, &QComboBox::currentIndexChanged, this, &MainWindow::instrResolutionChanged);
    //connect(instrFfmSpan, &QComboBox::currentIndexChanged, this, &MainWindow::instrFfmSpanChanged); // Taken care of after init.

    connect(instrMeasurementTime, QOverload<int>::of(&QSpinBox::valueChanged), config.data(), &Config::setInstrMeasurementTime);
    connect(instrAtt, QOverload<int>::of(&QSpinBox::valueChanged), config.data(), &Config::setInstrManAtt);
    connect(instrAutoAtt, &QCheckBox::toggled, this, &MainWindow::instrAutoAttChanged);
    connect(instrAntPort, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [this](int index) {
        if (instrAntPort->currentIndex() != -1) config->setInstrAntPort(index);
    }); // to ignore signals when combo data is inserted
    connect(this, &MainWindow::antennaPortChanged, measurementDevice, &MeasurementDevice::setAntPort);
    connect(instrMode, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &MainWindow::instrModeChanged);
    connect(instrFftMode, &QComboBox::currentTextChanged, config.data(), &Config::setInstrFftMode);
    connect(instrPort, &QLineEdit::editingFinished, this, &MainWindow::instrPortChanged);

    connect(instrConnect, &QPushButton::clicked, measurementDevice, &MeasurementDevice::instrConnect); //TBR
    connect(instrConnect, &QPushButton::clicked, receiver, &Receiver::connectInstrument);

    connect(instrDisconnect, &QPushButton::clicked, measurementDevice, &MeasurementDevice::instrDisconnect);

    connect(instrTrigLevel, QOverload<double>::of(&QDoubleSpinBox::valueChanged), config.data(), &Config::setInstrTrigLevel);
    connect(instrTrigBandwidth, QOverload<double>::of(&QDoubleSpinBox::valueChanged), config.data(), &Config::setInstrMinTrigBW);
    connect(instrTrigTotalBandwidth, QOverload<double>::of(&QDoubleSpinBox::valueChanged), config.data(), &Config::setInstrTotalTrigBW);
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
    connect(measurementDevice, &MeasurementDevice::deviceStreamTimeout, this, [this] {
        emit stopPlot(false);
    });
    connect(measurementDevice, &MeasurementDevice::connectedStateChanged, this, [this](bool state) {
        if (state && config->getGeoLimitActive() && !geoLimit->areWeInsidePolygon()) { // don't reconnect all if outside borders
            emit stopPlot(false);
        }
    });
    connect(measurementDevice, &MeasurementDevice::connectedStateChanged, this, [this] (bool b) {
        if (b) {
            QTimer::singleShot(1500, this, [this] {
                this->gnssAnalyzer3->updStateInstrumentGnss(true);
            });
        }
        else {
            gnssAnalyzer3->updStateInstrumentGnss(false);
        }
    });

    connect(this, &MainWindow::stopPlot, waterfall, &Waterfall::stopPlot);

    connect(measurementDevice, &MeasurementDevice::popup, this, &MainWindow::generatePopup);
    connect(measurementDevice, &MeasurementDevice::status, this, &MainWindow::updateStatusLine);
    connect(measurementDevice, &MeasurementDevice::instrId, this, &MainWindow::updWindowTitle);
    connect(measurementDevice, &MeasurementDevice::toIncidentLog, notifications, &Notifications::toIncidentLog);

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

    if (!config->getGeoLimitActive()) {
        connect(tcpStream.data(), &TcpDataStream::newFftData, traceBuffer, &TraceBuffer::addTrace);
        connect(udpStream.data(), &UdpDataStream::newFftData, traceBuffer, &TraceBuffer::addTrace);
    }

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
    connect(config.data(), &Config::settingsUpdated, gnssAnalyzer3, &GnssAnalyzer::updSettings);
    connect(config.data(), &Config::settingsUpdated, notifications, &Notifications::updSettings);
    connect(config.data(), &Config::settingsUpdated, waterfall, &Waterfall::updSettings);
    //connect(config.data(), &Config::settingsUpdated, cameraRecorder, &CameraRecorder::updSettings);
    connect(config.data(), &Config::settingsUpdated, arduinoPtr, &Arduino::updSettings);
    connect(config.data(), &Config::settingsUpdated, positionReport, &PositionReport::updSettings);
    connect(config.data(), &Config::settingsUpdated, geoLimit, &GeoLimit::updSettings);
    connect(config.data(), &Config::settingsUpdated, mqtt, &Mqtt::updSettings);
    connect(config.data(), &Config::settingsUpdated, instrumentList, &InstrumentList::updSettings);
    connect(config.data(), &Config::settingsUpdated, gnssDisplay, &GnssDisplay::updSettings);

    connect(traceAnalyzer, &TraceAnalyzer::alarm, sdefRecorder, &SdefRecorder::triggerRecording);
    connect(traceAnalyzer, &TraceAnalyzer::alarm, traceBuffer, &TraceBuffer::incidenceTriggered);
    connect(sdefRecorder, &SdefRecorder::recordingStarted, traceAnalyzer, &TraceAnalyzer::recorderStarted);
    connect(sdefRecorder, &SdefRecorder::recordingStarted, traceBuffer, &TraceBuffer::recorderStarted);
    connect(sdefRecorder, &SdefRecorder::recordingEnded, traceAnalyzer, &TraceAnalyzer::recorderEnded);
    connect(sdefRecorder, &SdefRecorder::recordingEnded, traceBuffer, &TraceBuffer::recorderEnded);
    connect(traceBuffer, &TraceBuffer::traceToRecorder, sdefRecorder, &SdefRecorder::receiveTrace);
    connect(sdefRecorder, &SdefRecorder::reqTraceHistory, traceBuffer, &TraceBuffer::getSecondsOfBuffer);
    connect(traceBuffer, &TraceBuffer::historicData, sdefRecorder, &SdefRecorder::receiveTraceBuffer);
    connect(sdefRecorder, &SdefRecorder::toIncidentLog, notifications, &Notifications::toIncidentLog);
    connect(measurementDevice, &MeasurementDevice::deviceStreamTimeout, sdefRecorder, &SdefRecorder::finishRecording); // stops eventual recording if stream times out (someone takes over meas.device)

    connect(traceBuffer, &TraceBuffer::averageLevelCalculating, this, [this] (){
        if (measurementDevice->isConnected() || read1809Data->isRunning()) {
            ledTraceStatus->setState(false);
            labelTraceLedText->setText("Calc. avg. noise floor");
        }
    });

    connect(traceBuffer, &TraceBuffer::stopAvgLevelFlash, this, [this] () {
        ledTraceStatus->setState(true);
        labelTraceLedText->setText("Detector ready");
    });

    connect(traceAnalyzer, &TraceAnalyzer::alarm, this, [this] () {traceIncidentAlarm(true);});
    connect(traceAnalyzer, &TraceAnalyzer::alarmEnded, this, [this] () {traceIncidentAlarm(false);});
    connect(sdefRecorder, &SdefRecorder::recordingStarted, this, [this] () {recordIncidentAlarm(true);});
    connect(sdefRecorder, &SdefRecorder::recordingEnded, this, [this] () {recordIncidentAlarm(false);});
    connect(gnssAnalyzer1, &GnssAnalyzer::visualAlarm, this, [this] {gnssIncidentAlarm(true);});
    connect(gnssAnalyzer1, &GnssAnalyzer::alarmEnded, this, [this] {gnssIncidentAlarm(false);});
    connect(gnssAnalyzer2, &GnssAnalyzer::visualAlarm, this, [this] {gnssIncidentAlarm(true);});
    connect(gnssAnalyzer2, &GnssAnalyzer::alarmEnded, this, [this] {gnssIncidentAlarm(false);});
    connect(gnssAnalyzer3, &GnssAnalyzer::visualAlarm, this, [this]{gnssIncidentAlarm(true);});
    connect(gnssAnalyzer3, &GnssAnalyzer::alarmEnded, this, [this] {gnssIncidentAlarm(false);});
    connect(sdefRecorder, &SdefRecorder::recordingDisabled, this, [this] {recordEnabled(false);});
    connect(sdefRecorder, &SdefRecorder::recordingEnabled, this, [this] {recordEnabled(true);});

    connect(sdefRecorderThread, &QThread::started, sdefRecorder, &SdefRecorder::start);
    connect(sdefRecorder, &SdefRecorder::warning, this, &MainWindow::generatePopup);
    connect(notificationsThread, &QThread::started, notifications, &Notifications::start);
    connect(waterfallThread, &QThread::started, waterfall, &Waterfall::start);
    //connect(cameraThread, &QThread::started, cameraRecorder, &CameraRecorder::start);

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
    connect(gnssDevice1, &GnssDevice::gnssDisabled, this, [this] {
        gnssAnalyzer1->updStateInstrumentGnss(false);
        gnssStatus->clear();
        rightBox->setTitle("GNSS status");
    });

    connect(gnssDevice2, &GnssDevice::gnssDisabled, this, [this] {
        gnssAnalyzer2->updStateInstrumentGnss(false);
        gnssStatus->clear();
        rightBox->setTitle("GNSS status");
    });

    //connect(cameraRecorder, &CameraRecorder::toIncidentLog, notifications, &Notifications::toIncidentLog);
    connect(mqtt, &Mqtt::toIncidentLog, notifications, &Notifications::toIncidentLog);

    connect(measurementDevice, &MeasurementDevice::displayGnssData, this, &MainWindow::updGnssBox);
    connect(measurementDevice, &MeasurementDevice::updGnssData, gnssAnalyzer3, &GnssAnalyzer::getData);
    connect(gnssAnalyzer3, &GnssAnalyzer::alarm, sdefRecorder, &SdefRecorder::triggerRecording);
    connect(gnssAnalyzer3, &GnssAnalyzer::toIncidentLog, notifications, &Notifications::toIncidentLog);
    connect(gnssAnalyzer3, &GnssAnalyzer::displayGnssData, this, &MainWindow::updGnssBox);
    connect(positionReport, &PositionReport::reqPosition, this, [this] (QString s) {
        if (s.contains("1")) positionReport->updPosition(this->gnssDevice1->sendGnssData());
        else if (s.contains("2")) positionReport->updPosition(this->gnssDevice2->sendGnssData());
        else positionReport->updPosition(this->measurementDevice->sendGnssData());
    });
    connect(positionReport, &PositionReport::reqMeasurementDevicePtr, this, [this] {
        if (measurementDevice) positionReport->setMeasurementDevicePtr(measurementDevice->measurementDeviceData());
    });
    connect(measurementDevice, &MeasurementDevice::connectedStateChanged, positionReport, &PositionReport::setMeasurementDeviceConnectionStatus);
    connect(measurementDevice, &MeasurementDevice::deviceBusy, positionReport, &PositionReport::setInUse);
    connect(measurementDevice, &MeasurementDevice::ipOfUser, positionReport, &PositionReport::setInUseByIp);
    connect(measurementDevice, &MeasurementDevice::modeUsed, positionReport, &PositionReport::setModeUsed);
    connect(measurementDevice, &MeasurementDevice::freqRangeUsed, positionReport, &PositionReport::setFreqUsed);
    connect(measurementDevice, &MeasurementDevice::resUsed, positionReport, &PositionReport::setResUsed);
    connect(measurementDevice, &MeasurementDevice::reconnected, positionReport, &PositionReport::setMeasurementDeviceReconnected);
    connect(measurementDevice, &MeasurementDevice::newAntennaNames, this, &MainWindow::setDeviceAntPorts);
    //connect(instrAntPort, &QComboBox::editTextChanged, this, &MainWindow::changeAntennaPortName);
    connect(antPortLineEdit, &QLineEdit::returnPressed, this, &MainWindow::changeAntennaPortName);

    connect(this, &MainWindow::antennaNameEdited, measurementDevice, &MeasurementDevice::updateAntennaName);

    connect (positionReport, &PositionReport::reqSensorData, arduinoPtr, &Arduino::returnSensorData);

    connect(notifications, &Notifications::showIncident, this, [this] (QString s)
            {
                this->incidentLog->insertHtml(s);
                this->incidentLog->verticalScrollBar()->setValue(this->incidentLog->verticalScrollBar()->maximum());
            });
    connect(notifications, &Notifications::warning, this, &MainWindow::generatePopup);
    connect(notifications, &Notifications::reqTracePlot, customPlotController, &CustomPlotController::reqTracePlot); // ask for image
    connect(customPlotController, &CustomPlotController::retTracePlot, notifications, &Notifications::recTracePlot); // be nice and send it then!
    connect(notifications, &Notifications::reqPosition, this, [this] {
        GnssData data;
        data = gnssDevice1->sendGnssData();
        if (!data.posValid) data = gnssDevice2->sendGnssData();
        if (!data.posValid) data = measurementDevice->sendGnssData();

        notifications->getLatitudeLongitude(data.posValid, data.latitude, data.longitude);
    });

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
    connect(btnPmrTable, &QPushButton::clicked, pmrTableWdg, &PmrTableWdg::start);
    sdefRecorderThread->start();
    notificationsThread->start();
    waterfallThread->start();
    //cameraThread->start();

    connect(geoLimit, &GeoLimit::toIncidentLog, notifications, &Notifications::toIncidentLog);
    connect(geoLimit, &GeoLimit::warning, this, &MainWindow::generatePopup);
    connect(geoLimit, &GeoLimit::requestPosition, this, [this] {
        GnssData data;
        data = gnssDevice1->sendGnssData();
        if (!data.posValid) data = gnssDevice2->sendGnssData();
        if (!data.posValid) data = measurementDevice->sendGnssData();
        geoLimit->receivePosition(data);
    });
    connect(geoLimit, &GeoLimit::currentPositionOk, this, [this](bool weAreInsidePolygon) {
        if (weAreInsidePolygon) {
            connect(tcpStream.data(), &TcpDataStream::newFftData, traceBuffer, &TraceBuffer::addTrace);
            connect(udpStream.data(), &UdpDataStream::newFftData, traceBuffer, &TraceBuffer::addTrace);
            waterfall->restartPlot();
        }
        else {
            disconnect(tcpStream.data(), &TcpDataStream::newFftData, traceBuffer, &TraceBuffer::addTrace);
            disconnect(udpStream.data(), &UdpDataStream::newFftData, traceBuffer, &TraceBuffer::addTrace);
            emit stopPlot(false);
            traceBuffer->emptyBuffer();
        }
    });

    connect(mqtt, &Mqtt::newData, positionReport, &PositionReport::updMqttData);
    connect(read1809Data, &Read1809Data::newTrace, traceBuffer, &TraceBuffer::addTrace);
    connect(read1809Data, &Read1809Data::playbackRunning, this, [this] (bool b){
        if (b) {
            customPlotController->updDeviceConnected(true);
            traceBuffer->restartCalcAvgLevel();
        }
        else {
            customPlotController->updDeviceConnected(false);
            traceBuffer->deviceDisconnected();
        }
    });

    connect(read1809Data, &Read1809Data::playbackRunning, waterfall, &Waterfall::stopPlot); // own thread, needs own signal
    //connect(read1809Data, &Read1809Data::connectedStateChanged, customPlotController, &CustomPlotController::updDeviceConnected);
    connect(read1809Data, &Read1809Data::playbackRunning, sdefRecorder, &SdefRecorder::deviceDisconnected);
    connect(read1809Data, &Read1809Data::toIncidentLog, notifications, &Notifications::toIncidentLog);
    connect(read1809Data, &Read1809Data::tracesPerSec, this, [this] (double d) {
        sdefRecorder->updTracesPerSecond(d);
        tracesPerSecond = d;
        showBytesPerSec(0); // to force tracePerSecond view
    });

    connect(traceAnalyzer, &TraceAnalyzer::alarm, aiPtr, &AI::startAiTimer); // will start analyze of data after x seconds
    connect(sdefRecorder, &SdefRecorder::recordingEnded, aiPtr, &AI::recordingHasEnded);
    connect(aiPtr, &AI::reqTraceBuffer, traceBuffer, &TraceBuffer::getAiData);
    connect(traceBuffer, &TraceBuffer::aiData, aiPtr, &AI::receiveTraceBuffer);
    connect(aiPtr, &AI::aiResult, sdefRecorder, &SdefRecorder::recPrediction);
    connect(aiPtr, &AI::aiResult, notifications, &Notifications::recPrediction);
    connect(aiPtr, &AI::toIncidentLog, notifications, &Notifications::toIncidentLog);

    connect(instrumentList, &InstrumentList::askForLogin, sdefRecorder, &SdefRecorder::loginRequest);
    connect(sdefRecorder, &SdefRecorder::loginSuccessful, instrumentList, &InstrumentList::loginCompleted);
    connect(instrumentList, &InstrumentList::instrumentListReady, this, [this] (QStringList ip, QStringList name, QStringList type) {
        disconnect(instrIpAddr, &QComboBox::currentTextChanged, this, &MainWindow::instrIpChanged);
        instrIpAddr->clear();

        int selIndex = -1;
        for (int i = 0; i < ip.size(); i++) {
            instrIpAddr->addItem(name[i] + " (" + type[i] + ")", ip[i]);
            if (ip[i] == config->getInstrIpAddr()) {
                selIndex = i;
            }
        }
        if (selIndex == -1) { // IP not found in list, assuming it is manually entered earlier
            instrIpAddr->addItem(config->getInstrIpAddr(), config->getInstrIpAddr());
            instrIpAddr->setCurrentIndex(instrIpAddr->count() - 1);
        }
        else instrIpAddr->setCurrentIndex(selIndex);
        connect(instrIpAddr, &QComboBox::currentIndexChanged, this, &MainWindow::instrIpChanged);
        receiver->setIpAddress(instrIpAddr->currentData().toString());
    });

    connect(gnssDisplay, &GnssDisplay::requestGnssData, this, [this] (int id) {
        if (id == 1) gnssDisplay->updGnssData(gnssDevice1->sendGnssData(), 1);
        else gnssDisplay->updGnssData(gnssDevice2->sendGnssData(), 2);
    });

    // Tcp/udp shared pointer signals
    //connect(tcpStream.data(), &TcpDataStream::newFftData, traceBuffer, &TraceBuffer::addTrace);
    //connect(udpStream.data(), &UdpDataStream::newFftData, traceBuffer, &TraceBuffer::addTrace);
    connect(tcpStream.data(), &TcpDataStream::freqChanged, this, [this] (double a, double b) {
        customPlotController->freqChanged(a, b);
        traceAnalyzer->freqChanged(a, b);
        aiPtr->freqChanged(a, b);
    });
    connect(udpStream.data(), &UdpDataStream::freqChanged, this, [this] (double a, double b) {
        customPlotController->freqChanged(a, b);
        traceAnalyzer->freqChanged(a, b);
        aiPtr->freqChanged(a, b);
    });
    connect(tcpStream.data(), &TcpDataStream::resChanged, this, [this] (double a) {
        customPlotController->resChanged(a);
        traceAnalyzer->resChanged(a);
        aiPtr->resChanged(a);
    });
    connect(udpStream.data(), &UdpDataStream::resChanged, this, [this] (double a) {
        customPlotController->resChanged(a);
        traceAnalyzer->resChanged(a);
        aiPtr->resChanged(a);
    });

    connect(read1809Data, &Read1809Data::freqChanged, this, [this] (double a, double b) {
        customPlotController->freqChanged(a, b);
        aiPtr->freqChanged(a, b);
        traceAnalyzer->freqChanged(a, b);
    });
    connect(read1809Data, &Read1809Data::resChanged, this, [this] (double a) {
        customPlotController->resChanged(a);
        aiPtr->resChanged(a);
        traceAnalyzer->resChanged(a);
    });

    connect(tcpStream.data(), &TcpDataStream::bytesPerSecond, this, &MainWindow::showBytesPerSec);
    connect(udpStream.data(), &UdpDataStream::bytesPerSecond, this, &MainWindow::showBytesPerSec);
    connect(tcpStream.data(), &TcpDataStream::tracesPerSecond, this, [this] (double d) {
        sdefRecorder->updTracesPerSecond(d);
        tracesPerSecond = d; // For view on mainWindow
    });
    connect(udpStream.data(), &UdpDataStream::tracesPerSecond, this, [this] (double d) {
        sdefRecorder->updTracesPerSecond(d);
        tracesPerSecond = d; // For view on mainWindow
    });

    connect(tcpStream.data(), &TcpDataStream::timeout, measurementDevice, &MeasurementDevice::handleStreamTimeout);
    connect(udpStream.data(), &UdpDataStream::timeout, measurementDevice, &MeasurementDevice::handleStreamTimeout);

    connect(tcpStream.data(), &TcpDataStream::streamErrorResetFreq, measurementDevice, &MeasurementDevice::resetFreqSettings);
    connect(udpStream.data(), &UdpDataStream::streamErrorResetFreq, measurementDevice, &MeasurementDevice::resetFreqSettings);

    connect(tcpStream.data(), &TcpDataStream::streamErrorResetConnection, measurementDevice, &MeasurementDevice::handleNetworkError);
    connect(udpStream.data(), &UdpDataStream::streamErrorResetConnection, measurementDevice, &MeasurementDevice::handleNetworkError);

    connect(traceAnalyzer, &TraceAnalyzer::trigRegistered, aiPtr, &AI::setTrigCenterFrequency);

    connect(cameraRecorder, &CameraRecorder::reqPosition, this, [this] () {
        if (gnssDevice1->isValid()) cameraRecorder->updPosition(gnssDevice1->sendGnssData());
        else if (gnssDevice2->isValid()) cameraRecorder->updPosition(gnssDevice2->sendGnssData());
        else if (measurementDevice->isPositionValid()) cameraRecorder->updPosition(measurementDevice->sendGnssData());
    });
    connect(traceAnalyzer, &TraceAnalyzer::maxLevelMeasured, cameraRecorder, &CameraRecorder::receivedSignalLevel);
    connect(traceAnalyzer, &TraceAnalyzer::alarm, cameraRecorder, &CameraRecorder::startRecorder);

}
