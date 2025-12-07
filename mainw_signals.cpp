#include "mainwindow.h"

void MainWindow::setSignals()
{
    connect(instrStartFreq,
            QOverload<double>::of(&QDoubleSpinBox::valueChanged),
            this,
            &MainWindow::instrPscanFreqChanged);
    connect(instrStopFreq,
            QOverload<double>::of(&QDoubleSpinBox::valueChanged),
            this,
            &MainWindow::instrPscanFreqChanged);
    connect(instrFfmCenterFreq,
            QOverload<double>::of(&QDoubleSpinBox::valueChanged),
            this,
            &MainWindow::instrFfmCenterFreqChanged); //Taken care of after init.
    connect(instrResolution, &QComboBox::currentIndexChanged, this, &MainWindow::instrResolutionChanged);
    connect(instrMeasurementTime,
            QOverload<int>::of(&QSpinBox::valueChanged),
            config.data(),
            &Config::setInstrMeasurementTime);
    connect(instrAtt,
            QOverload<int>::of(&QSpinBox::valueChanged),
            config.data(),
            &Config::setInstrManAtt);
    connect(instrAutoAtt, &QCheckBox::toggled, this, &MainWindow::instrAutoAttChanged);
    connect(instrAntPort,
            QOverload<int>::of(&QComboBox::currentIndexChanged),
            this,
            [this](int index) {
                if (instrAntPort->currentIndex() != -1)
                    config->setInstrAntPort(index);
                emit antennaNameChanged(instrAntPort->currentText()); // Info for sdef header
            }); // to ignore signals when combo data is inserted
    connect(this,
            &MainWindow::antennaPortChanged,
            measurementDevice,
            &MeasurementDevice::setAntPort);
    connect(instrMode,
            QOverload<int>::of(&QComboBox::currentIndexChanged),
            this,
            &MainWindow::instrModeChanged);
    connect(instrFftMode, &QComboBox::currentTextChanged, config.data(), &Config::setInstrFftMode);
    connect(instrPort, &QLineEdit::editingFinished, this, &MainWindow::instrPortChanged);

    connect(instrConnect, &QPushButton::clicked, this, [this]() {
        btnConnectPressed(true);
    });
    connect(instrDisconnect,
            &QPushButton::clicked,
            this,
            &MainWindow::btnDisconnectPressed);

    connect(instrTrigLevel,
            QOverload<double>::of(&QDoubleSpinBox::valueChanged),
            config.data(),
            &Config::setInstrTrigLevel);
    connect(instrTrigLevel,
            QOverload<double>::of(&QDoubleSpinBox::valueChanged),
            traceBuffer,
            &TraceBuffer::sendDispTrigline);
    connect(instrTrigBandwidth,
            QOverload<double>::of(&QDoubleSpinBox::valueChanged),
            config.data(),
            &Config::setInstrMinTrigBW);
    connect(instrTrigTotalBandwidth,
            QOverload<double>::of(&QDoubleSpinBox::valueChanged),
            config.data(),
            &Config::setInstrTotalTrigBW);
    connect(instrTrigTime,
            QOverload<int>::of(&QSpinBox::valueChanged),
            config.data(),
            &Config::setInstrMinTrigTime);

    connect(gnssCNo,
            QOverload<int>::of(&QSpinBox::valueChanged),
            config.data(),
            &Config::setGnssCnoDeviation);
    connect(gnssAgc,
            QOverload<int>::of(&QSpinBox::valueChanged),
            config.data(),
            &Config::setGnssAgcDeviation);
    connect(gnssPosOffset,
            QOverload<int>::of(&QSpinBox::valueChanged),
            config.data(),
            &Config::setGnssPosOffset);
    connect(gnssAltOffset,
            QOverload<int>::of(&QSpinBox::valueChanged),
            config.data(),
            &Config::setGnssAltOffset);
    connect(gnssTimeOffset,
            QOverload<int>::of(&QSpinBox::valueChanged),
            config.data(),
            &Config::setGnssTimeOffset);

    connect(measurementDevice,
            &MeasurementDevice::connectedStateChanged,
            this,
            &MainWindow::instrConnected);

    connect(measurementDevice,
            &MeasurementDevice::connectedStateChanged,
            waterfall,
            &Waterfall::stopPlot); // own thread, needs own signal
    connect(measurementDevice,
            &MeasurementDevice::connectedStateChanged,
            customPlotController,
            &CustomPlotController::updDeviceConnected);
    connect(measurementDevice,
            &MeasurementDevice::connectedStateChanged,
            sdefRecorder,
            &SdefRecorder::deviceDisconnected);
    connect(measurementDevice, &MeasurementDevice::deviceStreamTimeout, this, [this] {
        emit stopPlot(false);
    });
    connect(measurementDevice, &MeasurementDevice::connectedStateChanged, this, [this](bool state) {
        if (state && config->getGeoLimitActive()
            && !geoLimit->areWeInsidePolygon()) { // don't reconnect all if outside borders
            emit stopPlot(false);
        }
    });
    connect(measurementDevice, &MeasurementDevice::connectedStateChanged, this, [this](bool b) {
        if (b) {
            QTimer::singleShot(1500, this, [this] {
                this->gnssAnalyzer3->updStateInstrumentGnss(true);
            });
        } else {
            gnssAnalyzer3->updStateInstrumentGnss(false);
        }
    });

    connect(this, &MainWindow::stopPlot, waterfall, &Waterfall::stopPlot);

    connect(measurementDevice, &MeasurementDevice::popup, this, &MainWindow::generatePopup);
    connect(measurementDevice, &MeasurementDevice::status, this, &MainWindow::updateStatusLine);
    connect(measurementDevice, &MeasurementDevice::instrId, this, &MainWindow::updWindowTitle);
    connect(measurementDevice,
            &MeasurementDevice::toIncidentLog,
            notifications,
            &Notifications::toIncidentLog);

    connect(plotMaxScroll,
            QOverload<int>::of(&QSpinBox::valueChanged),
            config.data(),
            &Config::setPlotYMax);
    connect(plotMinScroll,
            QOverload<int>::of(&QSpinBox::valueChanged),
            config.data(),
            &Config::setPlotYMin);
    connect(plotMaxholdTime,
            QOverload<int>::of(&QSpinBox::valueChanged),
            config.data(),
            &Config::setPlotMaxholdTime);
    connect(showWaterfall, &QComboBox::currentTextChanged, this, &MainWindow::setWaterfallOption);
    connect(waterfallTime,
            QOverload<int>::of(&QSpinBox::valueChanged),
            config.data(),
            &Config::setWaterfallTime);

    connect(traceBuffer,
            &TraceBuffer::newDispTrace,
            customPlotController,
            &CustomPlotController::plotTrace);
    connect(traceBuffer, &TraceBuffer::newDispTrace, waterfall, &Waterfall::receiveTrace);

    connect(traceBuffer,
            &TraceBuffer::newDispMaxhold,
            customPlotController,
            &CustomPlotController::plotMaxhold);
    connect(traceBuffer,
            &TraceBuffer::showMaxhold,
            customPlotController,
            &CustomPlotController::showMaxhold);
    connect(traceBuffer,
            &TraceBuffer::newDispTriglevel,
            customPlotController,
            &CustomPlotController::plotTriglevel);
    connect(traceBuffer, &TraceBuffer::toIncidentLog, notifications, &Notifications::toIncidentLog);
    connect(traceBuffer,
            &TraceBuffer::reqReplot,
            customPlotController,
            &CustomPlotController::doReplot);
    connect(customPlotController,
            &CustomPlotController::reqTrigline,
            traceBuffer,
            &TraceBuffer::sendDispTrigline);
    connect(traceBuffer,
            &TraceBuffer::averageLevelCalculating,
            customPlotController,
            &CustomPlotController::flashTrigline);

    connect(traceBuffer,
            &TraceBuffer::averageLevelCalculating,
            sdefRecorder,
            &SdefRecorder::endRecording); // stop ev. recording in case avg level for any reason should start recalc.
    connect(traceBuffer,
            &TraceBuffer::stopAvgLevelFlash,
            customPlotController,
            &CustomPlotController::stopFlashTrigline);
    connect(traceBuffer,
            &TraceBuffer::averageLevelCalculating,
            traceAnalyzer,
            &TraceAnalyzer::setAnalyzerNotReady);
    connect(traceBuffer,
            &TraceBuffer::stopAvgLevelFlash,
            traceAnalyzer,
            &TraceAnalyzer::setAnalyzerReady);

    if (!config->getGeoLimitActive()) {
        connect(datastreamIfPan, &DatastreamIfPan::traceReady, traceBuffer, &TraceBuffer::addTrace);
        connect(datastreamIfPan, &DatastreamIfPan::traceReady, ptrNetwork, &Network::newTraceline);
        connect(datastreamPScan, &DatastreamPScan::traceReady, traceBuffer, &TraceBuffer::addTrace);
        connect(datastreamPScan, &DatastreamPScan::traceReady, ptrNetwork, &Network::newTraceline);
    }

    connect(measurementDevice,
            &MeasurementDevice::resetBuffers,
            traceBuffer,
            &TraceBuffer::emptyBuffer);

    connect(traceBuffer,
            &TraceBuffer::averageLevelCalculating,
            traceAnalyzer,
            &TraceAnalyzer::resetAverageLevel);
    connect(traceBuffer,
            &TraceBuffer::averageLevelCalculating,
            sdefRecorder,
            &SdefRecorder::closeTempFile);
    connect(traceBuffer,
            &TraceBuffer::averageLevelReady,
            traceAnalyzer,
            &TraceAnalyzer::setAverageTrace);
    connect(traceBuffer, &TraceBuffer::traceToAnalyzer, traceAnalyzer, &TraceAnalyzer::setTrace);

    connect(traceAnalyzer,
            &TraceAnalyzer::toIncidentLog,
            notifications,
            &Notifications::toIncidentLog);
    connect(customPlotController,
            &CustomPlotController::freqSelectionChanged,
            traceAnalyzer,
            &TraceAnalyzer::updTrigFrequencyTable);

    connect(config.data(),
            &Config::settingsUpdated,
            customPlotController,
            &CustomPlotController::updSettings);
    connect(config.data(),
            &Config::settingsUpdated,
            measurementDevice,
            &MeasurementDevice::updSettings);
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
    connect(config.data(), &Config::settingsUpdated, iqPlot, &IqPlot::updSettings);
    //connect(config.data(), &Config::settingsUpdated, cameraRecorder, &CameraRecorder::updSettings);
    connect(config.data(), &Config::settingsUpdated, arduinoPtr, &Arduino::updSettings);
    connect(config.data(), &Config::settingsUpdated, positionReport, &PositionReport::updSettings);
    connect(config.data(), &Config::settingsUpdated, geoLimit, &GeoLimit::updSettings);
    connect(config.data(), &Config::settingsUpdated, mqtt, &Mqtt::updSettings);
    connect(config.data(), &Config::settingsUpdated, instrumentList, &InstrumentList::updSettings);
    connect(config.data(), &Config::settingsUpdated, gnssDisplay, &GnssDisplay::updSettings);
    connect(config.data(), &Config::settingsUpdated, accessHandler, &AccessHandler::updSettings);
    connect(config.data(), &Config::settingsUpdated, ptrNetwork, &Network::updSettings);

    connect(traceAnalyzer, &TraceAnalyzer::alarm, sdefRecorder, &SdefRecorder::triggerRecording);
    connect(traceAnalyzer, &TraceAnalyzer::alarm, traceBuffer, &TraceBuffer::incidenceTriggered);
    //connect(traceAnalyzer, &TraceAnalyzer::alarm, measurementDevice, &MeasurementDevice::collectIqData); // New
    connect(traceAnalyzer, &TraceAnalyzer::alarm, iqPlot, &IqPlot::requestIqData); // New

    connect(btnTrigRecording, &QPushButton::clicked, iqPlot, &IqPlot::resetTimer);
    connect(btnTrigRecording, &QPushButton::clicked, iqPlot, &IqPlot::requestIqData);

    connect(sdefRecorder, &SdefRecorder::publishFilename, iqPlot, &IqPlot::setFilename);

    connect(sdefRecorder,
            &SdefRecorder::recordingStarted,
            traceAnalyzer,
            &TraceAnalyzer::recorderStarted);
    connect(sdefRecorder,
            &SdefRecorder::recordingStarted,
            traceBuffer,
            &TraceBuffer::recorderStarted);
    connect(sdefRecorder,
            &SdefRecorder::recordingEnded,
            traceAnalyzer,
            &TraceAnalyzer::recorderEnded);
    connect(sdefRecorder, &SdefRecorder::recordingEnded, traceBuffer, &TraceBuffer::recorderEnded);
    connect(sdefRecorder,
            &SdefRecorder::fileReadyForUpload,
            oauthFileUploader,
            &OAuthFileUploader::fileUploadRequest);
    connect(oauthFileUploader,
            &OAuthFileUploader::reqAuthToken,
            accessHandler,
            &AccessHandler::login);
    connect(accessHandler,
            &AccessHandler::accessTokenReady,
            oauthFileUploader,
            &OAuthFileUploader::receiveAuthToken);
    /*connect(oauthFileUploader, &OAuthFileUploader::reqAuthToken, this, [this] () {
        QString token = accessHandler->getToken();
        if (!token.isEmpty())
            oauthFileUploader->receiveAuthToken(token);
        else
            qWarning() << "OAuth: File uploader requested a token, but accessHandler says it's not valid";
    });*/

    connect(traceBuffer, &TraceBuffer::traceToRecorder, sdefRecorder, &SdefRecorder::receiveTrace);
    connect(traceBuffer, &TraceBuffer::traceData, sdefRecorder, &SdefRecorder::tempFileData);
    connect(sdefRecorder,
            &SdefRecorder::reqTraceHistory,
            traceBuffer,
            &TraceBuffer::getSecondsOfBuffer);
    connect(traceBuffer,
            &TraceBuffer::historicData,
            sdefRecorder,
            &SdefRecorder::receiveTraceBuffer);
    connect(sdefRecorder,
            &SdefRecorder::toIncidentLog,
            notifications,
            &Notifications::toIncidentLog);
    connect(measurementDevice,
            &MeasurementDevice::deviceStreamTimeout,
            sdefRecorder,
            &SdefRecorder::finishRecording); // stops eventual recording if stream times out (someone takes over meas.device)

    connect(traceBuffer, &TraceBuffer::averageLevelCalculating, this, [this]() {
        if (measurementDevice->isConnected() || read1809Data->isRunning()) {
            ledTraceStatus->setState(false);
            ledTraceStatus->setOffColor(Qt::yellow);
            ledTraceStatus->setToolTip("RF average level calculation");
        }
    });

    connect(traceBuffer, &TraceBuffer::stopAvgLevelFlash, this, [this]() {
        ledTraceStatus->setState(false);
        ledTraceStatus->setOffColor(Qt::green);
        ledTraceStatus->setToolTip("RF detector ready");
    });

    connect(traceAnalyzer, &TraceAnalyzer::alarm, this, [this]() { traceIncidentAlarm(true); });
    connect(traceAnalyzer, &TraceAnalyzer::alarmEnded, this, [this]() {
        traceIncidentAlarm(false);
    });
    connect(sdefRecorder, &SdefRecorder::recordingStarted, this, [this]() {
        recordIncidentAlarm(true);
    });
    connect(sdefRecorder, &SdefRecorder::recordingEnded, this, [this]() {
        recordIncidentAlarm(false);
    });
    connect(gnssAnalyzer1, &GnssAnalyzer::visualAlarm, this, [this] { gnssIncidentAlarm(true); });
    connect(gnssAnalyzer1, &GnssAnalyzer::alarmEnded, this, [this] { gnssIncidentAlarm(false); });
    connect(gnssAnalyzer2, &GnssAnalyzer::visualAlarm, this, [this] { gnssIncidentAlarm(true); });
    connect(gnssAnalyzer2, &GnssAnalyzer::alarmEnded, this, [this] { gnssIncidentAlarm(false); });
    connect(gnssAnalyzer3, &GnssAnalyzer::visualAlarm, this, [this] { gnssIncidentAlarm(true); });
    connect(gnssAnalyzer3, &GnssAnalyzer::alarmEnded, this, [this] { gnssIncidentAlarm(false); });
    connect(sdefRecorder, &SdefRecorder::recordingDisabled, this, [this] { recordEnabled(false); });
    connect(sdefRecorder, &SdefRecorder::recordingEnabled, this, [this] { recordEnabled(true); });

    connect(sdefRecorderThread, &QThread::started, sdefRecorder, &SdefRecorder::start);
    connect(sdefRecorder, &SdefRecorder::warning, this, &MainWindow::generatePopup);
    connect(notificationsThread, &QThread::started, notifications, &Notifications::start);
    connect(waterfallThread, &QThread::started, waterfall, &Waterfall::start);
    //connect(cameraThread, &QThread::started, cameraRecorder, &CameraRecorder::start);

    connect(gnssAnalyzer1, &GnssAnalyzer::displayGnssData, this, &MainWindow::updGnssBox);
    connect(gnssDevice1, &GnssDevice::analyzeThisData, gnssAnalyzer1, &GnssAnalyzer::getData);
    connect(gnssAnalyzer1, &GnssAnalyzer::alarm, sdefRecorder, &SdefRecorder::triggerRecording);
    connect(gnssAnalyzer1,
            &GnssAnalyzer::toIncidentLog,
            notifications,
            &Notifications::toIncidentLog);
    connect(gnssDevice1, &GnssDevice::toIncidentLog, notifications, &Notifications::toIncidentLog);
    connect(gnssAnalyzer2, &GnssAnalyzer::displayGnssData, this, &MainWindow::updGnssBox);
    connect(gnssDevice2, &GnssDevice::analyzeThisData, gnssAnalyzer2, &GnssAnalyzer::getData);
    connect(gnssAnalyzer2, &GnssAnalyzer::alarm, sdefRecorder, &SdefRecorder::triggerRecording);
    connect(gnssAnalyzer2,
            &GnssAnalyzer::toIncidentLog,
            notifications,
            &Notifications::toIncidentLog);
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
    connect(mqtt, &Mqtt::triggerRecording, sdefRecorder, &SdefRecorder::triggerRecording);
    connect(mqtt, &Mqtt::endRecording, sdefRecorder, &SdefRecorder::endRecording);

    connect(measurementDevice, &MeasurementDevice::displayGnssData, this, &MainWindow::updGnssBox);
    connect(measurementDevice,
            &MeasurementDevice::updGnssData,
            gnssAnalyzer3,
            &GnssAnalyzer::getData);
    connect(gnssAnalyzer3, &GnssAnalyzer::alarm, sdefRecorder, &SdefRecorder::triggerRecording);
    connect(gnssAnalyzer3,
            &GnssAnalyzer::toIncidentLog,
            notifications,
            &Notifications::toIncidentLog);
    connect(gnssAnalyzer3, &GnssAnalyzer::displayGnssData, this, &MainWindow::updGnssBox);
    connect(positionReport, &PositionReport::reqPosition, this, [this](QString s) {
        if (s.contains("1"))
            positionReport->updPosition(this->gnssDevice1->sendGnssData());
        else if (s.contains("2"))
            positionReport->updPosition(this->gnssDevice2->sendGnssData());
        else
            positionReport->updPosition(this->measurementDevice->sendGnssData());
    });
    connect(positionReport, &PositionReport::reqMeasurementDevicePtr, this, [this] {
        if (measurementDevice)
            positionReport->setMeasurementDevicePtr(measurementDevice->measurementDeviceData());
    });
    connect(measurementDevice,
            &MeasurementDevice::connectedStateChanged,
            positionReport,
            &PositionReport::setMeasurementDeviceConnectionStatus);
    connect(measurementDevice,
            &MeasurementDevice::deviceBusy,
            positionReport,
            &PositionReport::setInUse);
    connect(measurementDevice,
            &MeasurementDevice::ipOfUser,
            positionReport,
            &PositionReport::setInUseByIp);
    connect(measurementDevice,
            &MeasurementDevice::modeUsed,
            positionReport,
            &PositionReport::setModeUsed);
    connect(measurementDevice,
            &MeasurementDevice::modeUsed,
            sdefRecorder,
            &SdefRecorder::setModeUsed);
    connect(measurementDevice,
            &MeasurementDevice::freqRangeUsed,
            positionReport,
            &PositionReport::setFreqUsed);
    connect(measurementDevice,
            &MeasurementDevice::resUsed,
            positionReport,
            &PositionReport::setResUsed);
    connect(measurementDevice,
            &MeasurementDevice::reconnected,
            positionReport,
            &PositionReport::setMeasurementDeviceReconnected);
    connect(measurementDevice,
            &MeasurementDevice::reconnected,
            udpStream.data(),
            &UdpDataStream::restartTimeoutTimer);
    connect(measurementDevice,
            &MeasurementDevice::reconnected,
            tcpStream.data(),
            &TcpDataStream::
            restartTimeoutTimer); // Added to ensure program will restart the connection if no data arrives even on reconnection
    connect(measurementDevice,
            &MeasurementDevice::newAntennaNames,
            this,
            &MainWindow::setDeviceAntPorts);
    //connect(instrAntPort, &QComboBox::editTextChanged, this, &MainWindow::changeAntennaPortName);
    connect(antPortLineEdit, &QLineEdit::returnPressed, this, &MainWindow::changeAntennaPortName);

    connect(this,
            &MainWindow::antennaNameEdited,
            measurementDevice,
            &MeasurementDevice::updateAntennaName);

    connect(positionReport, &PositionReport::reqSensorData, arduinoPtr, &Arduino::returnSensorData);

    connect(notifications, &Notifications::showIncident, this, [this](QString s) {
        this->incidentLog->insertHtml(s);
        this->incidentLog->verticalScrollBar()->setValue(
            this->incidentLog->verticalScrollBar()->maximum());
    });
    connect(notifications, &Notifications::warning, this, &MainWindow::generatePopup);
    connect(notifications,
            &Notifications::reqTracePlot,
            customPlotController,
            &CustomPlotController::reqTracePlot); // ask for image
    connect(customPlotController,
            &CustomPlotController::retTracePlot,
            notifications,
            &Notifications::recTracePlot); // be nice and send it then!
    connect(notifications, &Notifications::reqPosition, this, [this] {
        GnssData data;
        data = gnssDevice1->sendGnssData();
        if (!data.posValid)
            data = gnssDevice2->sendGnssData();
        if (!data.posValid)
            data = measurementDevice->sendGnssData();

        notifications->getLatitudeLongitude(data.posValid, data.latitude, data.longitude);
    });

    connect(waterfall, &Waterfall::imageReady, customPlot, [&](QPixmap *pixmap) {
        if (dispWaterfall) {
            customPlot->axisRect()->setBackground(*pixmap, false);
            //customPlot->replot();
            //qcpImage->setPixmap(*pixmap);
            //customPlot->layer("image")->replot();
            //qDebug() << qcpImage->clipAxisRect()->rect() << qcpImage->pixmap().size();
            customPlot->replot();
        }
    });
    connect(iqPlot, &IqPlot::iqPlotReady, notifications, &Notifications::recIqPlot);

    connect(gnssDevice1, &GnssDevice::positionUpdate, sdefRecorder, &SdefRecorder::updPosition);
    connect(gnssDevice2, &GnssDevice::positionUpdate, sdefRecorder, &SdefRecorder::updPosition);
    connect(measurementDevice,
            &MeasurementDevice::positionUpdate,
            sdefRecorder,
            &SdefRecorder::updPosition);
    connect(sdefRecorder, &SdefRecorder::reqPositionFrom, this, [this](POSITIONSOURCE p) {
        if (p == POSITIONSOURCE::GNSSDEVICE1)
            gnssDevice1->reqPosition();
        else if (p == POSITIONSOURCE::GNSSDEVICE2)
            gnssDevice2->reqPosition();
        else
            measurementDevice->reqPosition();
    });

    connect(btnTrigRecording,
            &QPushButton::clicked,
            sdefRecorder,
            &SdefRecorder::manualTriggeredRecording);

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
        if (!data.posValid)
            data = gnssDevice2->sendGnssData();
        if (!data.posValid)
            data = measurementDevice->sendGnssData();
        geoLimit->receivePosition(data);
    });
    connect(geoLimit, &GeoLimit::currentPositionOk, this, [this](bool weAreInsidePolygon) {
        if (weAreInsidePolygon) {
            connect(datastreamIfPan, &DatastreamIfPan::traceReady, traceBuffer, &TraceBuffer::addTrace);
            connect(datastreamIfPan, &DatastreamIfPan::traceReady, ptrNetwork, &Network::newTraceline);
            connect(datastreamPScan, &DatastreamPScan::traceReady, traceBuffer, &TraceBuffer::addTrace);
            connect(datastreamPScan, &DatastreamPScan::traceReady, ptrNetwork, &Network::newTraceline);

            waterfall->restartPlot();
        } else {
            disconnect(datastreamIfPan, &DatastreamIfPan::traceReady, traceBuffer, &TraceBuffer::addTrace);
            disconnect(datastreamIfPan, &DatastreamIfPan::traceReady, ptrNetwork, &Network::newTraceline);
            disconnect(datastreamPScan, &DatastreamPScan::traceReady, traceBuffer, &TraceBuffer::addTrace);
            disconnect(datastreamPScan, &DatastreamPScan::traceReady, ptrNetwork, &Network::newTraceline);

            emit stopPlot(false);
            traceBuffer->emptyBuffer();
        }
    });

    connect(mqtt, &Mqtt::newData, positionReport, &PositionReport::updMqttData);
    connect(read1809Data, &Read1809Data::newTrace, traceBuffer, &TraceBuffer::addTrace);
    connect(read1809Data, &Read1809Data::playbackRunning, this, [this](bool b) {
        if (b) {
            customPlotController->updDeviceConnected(true);
            traceBuffer->restartCalcAvgLevel();
        } else {
            customPlotController->updDeviceConnected(false);
            traceBuffer->deviceDisconnected();
        }
    });

    connect(read1809Data,
            &Read1809Data::playbackRunning,
            waterfall,
            &Waterfall::stopPlot); // own thread, needs own signal
    //connect(read1809Data, &Read1809Data::connectedStateChanged, customPlotController, &CustomPlotController::updDeviceConnected);
    connect(read1809Data,
            &Read1809Data::playbackRunning,
            sdefRecorder,
            &SdefRecorder::deviceDisconnected);
    connect(read1809Data,
            &Read1809Data::toIncidentLog,
            notifications,
            &Notifications::toIncidentLog);
    connect(read1809Data, &Read1809Data::tracesPerSec, this, [this](double d) {
        sdefRecorder->updTracesPerSecond(d);
        tracesPerSecond = d;
        showBytesPerSec(0); // to force tracePerSecond view
    });

    connect(traceAnalyzer,
            &TraceAnalyzer::alarm,
            aiPtr,
            &AI::startAiTimer); // will start analyze of data after x seconds
    connect(sdefRecorder, &SdefRecorder::recordingEnded, aiPtr, &AI::recordingHasEnded);
    connect(aiPtr, &AI::reqTraceBuffer, traceBuffer, &TraceBuffer::getAiData);
    connect(traceBuffer, &TraceBuffer::aiData, aiPtr, &AI::receiveTraceBuffer);
    connect(aiPtr, &AI::aiResult, sdefRecorder, &SdefRecorder::recPrediction);
    connect(aiPtr, &AI::aiResult, notifications, &Notifications::recPrediction);
    connect(aiPtr, &AI::toIncidentLog, notifications, &Notifications::toIncidentLog);

    connect(instrumentList,
            &InstrumentList::listReady,
            this,
            [this](QStringList name, QStringList ip, QStringList type) {
                disconnect(instrIpAddr, &QComboBox::currentIndexChanged, this, &MainWindow::instrIpChanged);
                instrIpAddr->clear();


                for (int i = 0; i < ip.size(); i++)
                    instrIpAddr->addItem(name[i] + " (" + type[i] + ")", QVariant(ip[i]));
                if (!config->getInstrCustomEntry().isEmpty())
                    instrIpAddr->addItem(config->getInstrCustomEntry());

                int index = instrIpAddr->findText(config->getInstrIpAddr());
                if (index != -1)
                    instrIpAddr->setCurrentIndex(index);
                connect(instrIpAddr, &QComboBox::currentIndexChanged, this, &MainWindow::instrIpChanged);
            });

    connect(gnssDisplay, &GnssDisplay::requestGnssData, this, [this](int id) {
        if (id == 1)
            gnssDisplay->updGnssData(gnssDevice1->sendGnssData(), 1);
        else
            gnssDisplay->updGnssData(gnssDevice2->sendGnssData(), 2);
    });
    connect(datastreamPScan, &DatastreamPScan::frequencyChanged, customPlotController, &CustomPlotController::freqChanged);
    connect(datastreamPScan, &DatastreamPScan::frequencyChanged, traceAnalyzer, &TraceAnalyzer::freqChanged);
    connect(datastreamPScan, &DatastreamPScan::frequencyChanged, aiPtr, &AI::freqChanged);
    connect(datastreamPScan, &DatastreamPScan::resolutionChanged, customPlotController, &CustomPlotController::resChanged);
    connect(datastreamPScan, &DatastreamPScan::resolutionChanged, traceAnalyzer, &TraceAnalyzer::resChanged);
    connect(datastreamPScan, &DatastreamPScan::resolutionChanged, aiPtr, &AI::resChanged);

    connect(datastreamIfPan, &DatastreamIfPan::frequencyChanged, customPlotController, &CustomPlotController::freqChanged);
    connect(datastreamIfPan, &DatastreamIfPan::frequencyChanged, traceAnalyzer, &TraceAnalyzer::freqChanged);
    connect(datastreamIfPan, &DatastreamIfPan::frequencyChanged, aiPtr, &AI::freqChanged);
    connect(datastreamIfPan, &DatastreamPScan::resolutionChanged, customPlotController, &CustomPlotController::resChanged);
    connect(datastreamIfPan, &DatastreamPScan::resolutionChanged, traceAnalyzer, &TraceAnalyzer::resChanged);
    connect(datastreamIfPan, &DatastreamPScan::resolutionChanged, aiPtr, &AI::resChanged);

    // Tcp/udp shared pointer signals
    //connect(tcpStream.data(), &TcpDataStream::newFftData, traceBuffer, &TraceBuffer::addTrace);
    //connect(udpStream.data(), &UdpDataStream::newFftData, traceBuffer, &TraceBuffer::addTrace);
    /*connect(tcpStream.data(), &TcpDataStream::freqChanged, this, [this](double a, double b) {
        customPlotController->freqChanged(a, b);
        traceAnalyzer->freqChanged(a, b);
        aiPtr->freqChanged(a, b);
    });
    connect(udpStream.data(), &UdpDataStream::freqChanged, this, [this](double a, double b) {
        customPlotController->freqChanged(a, b);
        traceAnalyzer->freqChanged(a, b);
        aiPtr->freqChanged(a, b);
    });
    connect(tcpStream.data(), &TcpDataStream::resChanged, this, [this](double a) {
        customPlotController->resChanged(a);
        traceAnalyzer->resChanged(a);
        aiPtr->resChanged(a);
    });
    connect(udpStream.data(), &UdpDataStream::resChanged, this, [this](double a) {
        customPlotController->resChanged(a);
        traceAnalyzer->resChanged(a);
        aiPtr->resChanged(a);
    });*/ /// FIX! New signal from pscan/ifpan class!

    connect(read1809Data, &Read1809Data::freqChanged, this, [this](double a, double b) {
        customPlotController->freqChanged(a, b);
        aiPtr->freqChanged(a, b);
        traceAnalyzer->freqChanged(a, b);
    });
    connect(read1809Data, &Read1809Data::resChanged, this, [this](double a) {
        customPlotController->resChanged(a);
        aiPtr->resChanged(a);
        traceAnalyzer->resChanged(a);
    });

    connect(tcpStream.data(), &TcpDataStream::bytesPerSecond, this, &MainWindow::showBytesPerSec);
    connect(udpStream.data(), &UdpDataStream::bytesPerSecond, this, &MainWindow::showBytesPerSec);
    connect(datastreamPScan, &DatastreamPScan::tracesPerSecond, this, [this](double d) {
        sdefRecorder->updTracesPerSecond(d);
        tracesPerSecond = d; // For view on mainWindow
    });
    connect(datastreamPScan, &DatastreamPScan::tracesPerSecond, this, [this](double d) {
        sdefRecorder->updTracesPerSecond(d);
        tracesPerSecond = d; // For view on mainWindow
    });

    connect(tcpStream.data(), &TcpDataStream::timeout, measurementDevice, &MeasurementDevice::handleStreamTimeout);
    connect(udpStream.data(), &UdpDataStream::timeout, measurementDevice, &MeasurementDevice::handleStreamTimeout);
    connect(traceAnalyzer, &TraceAnalyzer::trigRegistered, iqPlot, &IqPlot::setFfmFrequency);
    connect(measurementDevice, &MeasurementDevice::iqFfmFreqChanged, iqPlot, &IqPlot::setFfmFrequency);
    connect(vifStreamTcp.data(), &VifStreamTcp::newFfmCenterFrequency, iqPlot, &IqPlot::setFfmFrequency);

    connect(cameraRecorder, &CameraRecorder::reqPosition, this, [this]() {
        if (gnssDevice1->isValid())
            cameraRecorder->updPosition(gnssDevice1->sendGnssData());
        else if (gnssDevice2->isValid())
            cameraRecorder->updPosition(gnssDevice2->sendGnssData());
        else if (measurementDevice->isPositionValid())
            cameraRecorder->updPosition(measurementDevice->sendGnssData());
    });
    connect(traceAnalyzer, &TraceAnalyzer::maxLevelMeasured, cameraRecorder, &CameraRecorder::receivedSignalLevel);
    connect(traceAnalyzer, &TraceAnalyzer::alarm, cameraRecorder, &CameraRecorder::startRecorder);

    connect(oauthFileUploader, &OAuthFileUploader::toIncidentLog, notifications, &Notifications::toIncidentLog);
    connect(traceBuffer, &TraceBuffer::nrOfDatapointsChanged, sdefRecorder, &SdefRecorder::dataPointsChanged);

    connect(oauthFileUploader, &OAuthFileUploader::uploadProgress, this, &MainWindow::uploadProgress);

    connect(instrGainControl, &QComboBox::currentIndexChanged, this, &MainWindow::instrGainControlChanged);

#ifdef Q_OS_WIN
    connect(config.data(), &Config::settingsUpdated, this, [this]() {
        if (config->getDarkMode()
            && QApplication::styleHints()->colorScheme() != Qt::ColorScheme::Dark) {
            QApplication::styleHints()->setColorScheme(Qt::ColorScheme::Dark);
            customPlot->setBackground(QBrush(Qt::black));
            customPlot->xAxis->setTickLabelColor(Qt::white);
            customPlot->xAxis->setBasePen(QPen(Qt::white));
            customPlot->xAxis->setLabelColor(Qt::white);
            customPlot->xAxis->setTickPen(QPen(Qt::white));
            customPlot->xAxis->setSubTickPen(QPen(Qt::white));
            customPlot->yAxis->setTickLabelColor(Qt::white);
            customPlot->yAxis->setBasePen(QPen(Qt::white));
            customPlot->yAxis->setLabelColor(Qt::white);
            customPlot->yAxis->setTickPen(QPen(Qt::white));
            customPlot->yAxis->setSubTickPen(QPen(Qt::white));
            customPlot->replot();
        } else if (!config->getDarkMode()
                   && QApplication::styleHints()->colorScheme() != Qt::ColorScheme::Light) {
            QApplication::styleHints()->setColorScheme(Qt::ColorScheme::Light);
            customPlot->setBackground(QBrush(Qt::white));
            customPlot->xAxis->setTickLabelColor(Qt::black);
            customPlot->xAxis->setBasePen(QPen(Qt::black));
            customPlot->xAxis->setLabelColor(Qt::black);
            customPlot->xAxis->setTickPen(QPen(Qt::black));
            customPlot->xAxis->setSubTickPen(QPen(Qt::black));
            customPlot->yAxis->setTickLabelColor(Qt::black);
            customPlot->yAxis->setBasePen(QPen(Qt::black));
            customPlot->yAxis->setLabelColor(Qt::black);
            customPlot->yAxis->setTickPen(QPen(Qt::black));
            customPlot->yAxis->setSubTickPen(QPen(Qt::black));
            customPlot->replot();
        }
    });
#endif

    connect(restApi, &RestApi::pscanStartFreq, this, [this](double f) {
        instrStartFreq->setValue(f);
        instrPscanFreqChanged();
    });
    connect(restApi, &RestApi::pscanStopFreq, this, [this](double f) {
        instrStopFreq->setValue(f);
        instrPscanFreqChanged();
    });
    connect(restApi, &RestApi::pscanResolution, this, [this](QString res) {
        //config->setInstrResolution(res);
        if (instrResolution->findText(config->getInstrResolution()) >= 0)
            instrResolution->setCurrentIndex(
                instrResolution->findText(config->getInstrResolution()));
    });
    connect(restApi, &RestApi::measurementTime, this, [this](int f) {
        instrMeasurementTime->setValue(f);
        instrMeasurementTimeChanged();
    });
    connect(restApi, &RestApi::manualAtt, this, [this](int f) {
        if (instrAutoAtt->isChecked()) { // Switch to manual att before changing value
            instrAutoAtt->setChecked(false);
            //instrAutoAttChanged(); // Already signaled
        }
        instrAtt->setValue(f);
        instrAttChanged();
    });
    connect(restApi, &RestApi::autoAtt, this, [this](bool b) {
        instrAutoAtt->setChecked(b);
        instrAutoAttChanged();
    });
    connect(restApi, &RestApi::antport, this, [this](int i) {
        instrAntPort->setCurrentIndex(i - 1); // Ant.port 1 or 2 = index 0 or 1
        emit antennaPortChanged();
    });
    connect(restApi, &RestApi::mode, this, [this](QString s) {
        if (s.contains("pscan")) {
            instrMode->setCurrentIndex(0);
        } else {
            instrMode->setCurrentIndex(1);
        }
        instrModeChanged();
    });
    connect(restApi, &RestApi::fftmode, this, [this](QString s) {
        instrFftMode->setCurrentIndex(instrFftMode->findText(s));
    });
    connect(restApi, &RestApi::gaincontrol, this, [this](QString s) {
        instrGainControl->setCurrentIndex(instrGainControl->findText(s));
    });

    connect(config.data(), &Config::settingsUpdated, this, [this] () {
        if (config->getUseDbm() != useDbm) {
            useDbm = config->getUseDbm();
            if (useDbm) {
                plotMaxScroll->setValue(plotMaxScroll->value() - 107);
                plotMinScroll->setValue(plotMinScroll->value() - 107);
            }
            else {
                plotMaxScroll->setValue(plotMaxScroll->value() + 107);
                plotMinScroll->setValue(plotMinScroll->value() + 107);
            }
            traceBuffer->emptyBuffer();
            traceBuffer->sendDispTrigline();
        }
    });

    connect(iqPlot, &IqPlot::folderDateTimeSet, sdefRecorder, &SdefRecorder::setFolderDateTime);
    connect(sdefRecorder, &SdefRecorder::folderDateTimeSet, iqPlot, &IqPlot::setFolderDateTime);
    connect(measurementDevice, &MeasurementDevice::skipNextNTraces, sdefRecorder, &SdefRecorder::skipNextNTraces);

    // Rebuilt I/Q functions
    connect(traceAnalyzer, &TraceAnalyzer::trigRegistered, iqPlot, &IqPlot::setTrigFrequency);
    connect(iqPlot, &IqPlot::reqVifConnection, measurementDevice, &MeasurementDevice::setupVifConnection);
    connect(iqPlot, &IqPlot::setFfmCenterFrequency, measurementDevice, &MeasurementDevice::setVifFreqAndMode);
    connect(iqPlot, &IqPlot::busyRecording, sdefRecorder, &SdefRecorder::setIqRecordingInProgress);
    connect(vifStreamTcp.data(), &VifStreamTcp::iqHeaderData, iqPlot, &IqPlot::validateHeader);
    connect(iqPlot, &IqPlot::headerValidated, vifStreamTcp.data(), &VifStreamTcp::setHeaderValidated);
    connect(iqPlot, &IqPlot::endVifConnection, measurementDevice, &MeasurementDevice::deleteIfStream);
    connect(vifStreamTcp.data(), &VifStreamTcp::newIqData, iqPlot, &IqPlot::getIqData);
    connect(iqPlot, &IqPlot::resetTimeoutTimer, tcpStream.data(), &TcpDataStream::restartTimeoutTimer);
    connect(iqPlot, &IqPlot::resetTimeoutTimer, udpStream.data(), &UdpDataStream::restartTimeoutTimer);
    connect(iqPlot, &IqPlot::resetTimeoutTimer, measurementDevice, &MeasurementDevice::restartTcpTimeoutTimer);
    connect(iqPlot, &IqPlot::reqIqCenterFrequency, this, [this]() {
        iqPlot->getIqCenterFrequency(measurementDevice->retIqCenterFrequency());
    });

    connect(instrIpAddr, &MyComboBox::enterPressed, this, [this]() {
        btnConnectPressed(true);
    });
    connect(this, &MainWindow::antennaNameChanged, sdefRecorder, &SdefRecorder::updAntName);

    connect(accessHandler, &AccessHandler::accessTokenInvalid, this, [this](QString s) {
        ledAzureStatus->setOffColor(Qt::red);
        ledAzureStatus->setToolTip(s);
    });
    connect(accessHandler, &AccessHandler::accessTokenValid, this, [this](QString user) {
        ledAzureStatus->setOffColor(Qt::green);
        ledAzureStatus->setToolTip("Logged in as " + user);
    });
    connect(accessHandler, &AccessHandler::reqAccessToken, this, [this]() {
        ledAzureStatus->setOffColor(Qt::yellow);
    });
    connect(accessHandler, &AccessHandler::settingsInvalid, this, [this](QString s) {
        ledAzureStatus->setOffColor(Qt::red);
        ledAzureStatus->setToolTip(s);
    });
    connect(instrumentList, &InstrumentList::instrumentListDownloaded, this, [this](QString s) {
        ledInstrListStatus->setOffColor(Qt::green);
        ledInstrListStatus->setToolTip(QDateTime::currentDateTime().toString("hh:mm:ss: ") + s);
    });
    connect(instrumentList, &InstrumentList::instrumentListStarted, this, [this](QString s) {
        ledInstrListStatus->setOffColor(Qt::yellow);
        ledInstrListStatus->setToolTip(s);
    });
    connect(instrumentList, &InstrumentList::instrumentListFailed, this, [this](QString s) {
        ledInstrListStatus->setOffColor(Qt::red);
        ledInstrListStatus->setToolTip(s);
    });
    connect(btnRestartAvgCalc, &QPushButton::clicked, this, [this] () {
        traceBuffer->restartCalcAvgLevel(true); // Force restart
    });
    connect(tcpStream.data(), &DataStreamBaseClass::newAudioData, datastreamAudio, &DatastreamAudio::parseData);
    connect(udpStream.data(), &DataStreamBaseClass::newAudioData, datastreamAudio, &DatastreamAudio::parseData);
    connect(tcpStream.data(), &DataStreamBaseClass::newIfData, datastreamIf, &DatastreamIf::parseData);
    connect(udpStream.data(), &DataStreamBaseClass::newIfData, datastreamIf, &DatastreamIf::parseData);
    connect(tcpStream.data(), &DataStreamBaseClass::newPscanData, datastreamPScan, &DatastreamPScan::parseData);
    connect(udpStream.data(), &DataStreamBaseClass::newPscanData, datastreamPScan, &DatastreamPScan::parseData);

    connect(datastreamAudio, &DatastreamAudio::audioDataReady, &audioPlayer, &AudioPlayer::playChunk);
    connect(datastreamAudio, &DatastreamAudio::audioModeChanged, &audioPlayer, &AudioPlayer::setFormat);

    connect(btnAudioOpt, &QPushButton::clicked, audioOptions, &AudioOptions::start);
    connect(audioOptions, &AudioOptions::askForDemodBwList, this, [this] {
        audioOptions->getDemodBwList(measurementDevice->retDemodBwList());
    });
    connect(audioOptions, &AudioOptions::askForDemodTypeList, this, [this] {
        audioOptions->getDemodTypeList(measurementDevice->retDemodTypeList());
    });
    connect(audioOptions, &AudioOptions::audioMode, datastreamAudio, &DatastreamAudio::reportAudioMode); // Needed to have initial setup of player format
    connect(audioOptions, &AudioOptions::audioMode, measurementDevice, &MeasurementDevice::setAudioMode);
    connect(audioOptions, &AudioOptions::demodType, measurementDevice, &MeasurementDevice::setDemodType);
    connect(audioOptions, &AudioOptions::demodBw, measurementDevice, &MeasurementDevice::setDemodBw);
    connect(audioOptions, &AudioOptions::squelch, measurementDevice, &MeasurementDevice::setSquelch);
    connect(audioOptions, &AudioOptions::squelchLevel, measurementDevice, &MeasurementDevice::setSquelchLevel);
    connect(audioOptions, &AudioOptions::audioDevice, &audioPlayer, &AudioPlayer::setAudioDevice);
    connect(audioOptions, &AudioOptions::activateAudio, &audioPlayer, &AudioPlayer::playAudio);

    connect(audioOptions, &AudioOptions::record, &audioRecorder, &AudioRecorder::enableRecorder);
    connect(datastreamAudio, &DatastreamAudio::audioModeChanged, &audioRecorder, &AudioRecorder::updFormat);
    connect(datastreamAudio, &DatastreamAudio::audioDataReady, &audioRecorder, &AudioRecorder::receiveAudioData);
    connect(datastreamAudio, &DatastreamAudio::headerDataChanged, &audioRecorder, &AudioRecorder::demodChanged);
    connect(config.data(), &Config::settingsUpdated, this, [this]() {
        audioRecorder.setFileLocation(config->getLogFolder());
    });
    connect(iqPlot, &IqPlot::busyRecording, this, [this](bool b) {
        if (b)
            measurementDevice->setAudioMode(0); // Disable audio demod while I/Q transfer is running
        else if (config->getAudioActivate())
            audioOptions->report(); // Restore mode when done (if it should be active)
    });
    connect(customPlotController, &CustomPlotController::centerFrequency, this, [this] (double d) {
        instrFfmCenterFreq->setValue(d);
        if (measurementDevice->currentMode() != Instrument::Mode::FFM) {
            instrMode->setCurrentIndex(instrMode->findText("FFM"));
            instrModeChanged();
        }
        instrFfmCenterFreqChanged();
    });
    connect(customPlotController, &CustomPlotController::scroll, this, [this] (int i) {
        if (measurementDevice->currentMode() == Instrument::Mode::FFM) {
            instrFfmCenterFreq->setValue( ( 1e6 * instrFfmCenterFreq->value() + i * (1e3 * instrFfmSpan->currentText().toDouble() / 40.0) ) / 1e6 );
            instrFfmCenterFreqChanged();
        }
    });
    connect(tcpStream.data(), &DataStreamBaseClass::waitForPscanEndMarker, datastreamPScan, &DatastreamPScan::updWaitForPscanEndMarker);
    connect(udpStream.data(), &DataStreamBaseClass::waitForPscanEndMarker, datastreamPScan, &DatastreamPScan::updWaitForPscanEndMarker);
    connect(tcpStream.data(), &DataStreamBaseClass::newIfPanData, datastreamIfPan, &DatastreamIfPan::parseData);
    connect(udpStream.data(), &DataStreamBaseClass::newIfPanData, datastreamIfPan, &DatastreamIfPan::parseData);
    connect(tcpStream.data(), &DataStreamBaseClass::newGpsCompassData, datastreamGpsCompass, &DatastreamGpsCompass::parseData);
    connect(udpStream.data(), &DataStreamBaseClass::newGpsCompassData, datastreamGpsCompass, &DatastreamGpsCompass::parseData);

    connect(datastreamGpsCompass, &DatastreamGpsCompass::gpsdataReady, measurementDevice, &MeasurementDevice::updGpsCompassData);
}
