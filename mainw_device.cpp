#include "mainwindow.h"

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
    instrPscanFreqChanged();
    setResolutionFunction();
}

void MainWindow::setValidators()
{
    instrStartFreq->setRange(0, 600e3);
    instrStartFreq->setDecimals(6);
    instrStartFreq->setSingleStep(1);
    instrFfmCenterFreq->setRange(1, 600e3);
    instrFfmCenterFreq->setDecimals(6);
    instrStopFreq->setRange(0, 600e3);
    instrStopFreq->setDecimals(6);
    instrStopFreq->setSingleStep(1);
    instrMeasurementTime->setRange(2, 5000);
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

    instrTrigLevel->setRange(-200, 200);
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

void MainWindow::instrPscanFreqChanged()
{
    if (measurementDevice->currentMode() == Instrument::Mode::PSCAN &&
        instrStartFreq->value() < instrStopFreq->value()) {
        measurementDevice->setPscanFrequency(instrStartFreq->value() * 1e6, instrStopFreq->value() * 1e6);
        config->setInstrStartFreq(instrStartFreq->value() * 1e6);
        config->setInstrStopFreq(instrStopFreq->value() * 1e6);
        ptrNetwork->updFrequencies(instrStartFreq->value() * 1e6, instrStopFreq->value() * 1e6);
        sdefRecorder->updFrequencies(instrStartFreq->value() * 1e6, instrStopFreq->value() * 1e6);
    }
    if (measurementDevice->isConnected()) {
        traceBuffer->restartCalcAvgLevel();
        waterfall->restartPlot();
    }

}

void MainWindow::instrFfmCenterFreqChanged()
{
    if (measurementDevice->currentMode() == Instrument::Mode::FFM) {
        config->setInstrFfmCenterFreq(1e6 * instrFfmCenterFreq->value());
        measurementDevice->setFfmCenterFrequency(1e6 * instrFfmCenterFreq->value());
        ptrNetwork->updFrequencies(1e6 * instrFfmCenterFreq->value() - instrFfmSpan->currentText().toDouble() * 5e2,
                                   1e6 * instrFfmCenterFreq->value() + instrFfmSpan->currentText().toDouble() * 5e2);
        sdefRecorder->updFrequencies(1e6 * instrFfmCenterFreq->value() - instrFfmSpan->currentText().toDouble() * 5e2,
                                   1e6 * instrFfmCenterFreq->value() + instrFfmSpan->currentText().toDouble() * 5e2);
    }

    if (measurementDevice->isConnected()) {
        traceBuffer->restartCalcAvgLevel();
        waterfall->restartPlot();
    }
    iqPlot->setFfmFrequency(instrFfmCenterFreq->value()); // Sent as MHz, used for image text
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
        instrPscanFreqChanged();
        instrFfmCenterFreqChanged();
    } else {
        traceBuffer->deviceDisconnected(); // stops buffer work when not needed
    }
    QTimer::singleShot(500, this, [this] () {
        measDeviceFinished = measurementDevice->isConnected();
    });
}

void MainWindow::instrGainControlChanged(int index)
{
    if (index == -1)
        index = config->getInstrGainControl();
    else
        config->setInstrGainControl(index);

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

void MainWindow::changeAntennaPortName()
{
    emit antennaNameEdited(instrAntPort->currentIndex(), instrAntPort->currentText());
}

void MainWindow::instrAutoConnect()
{
    if (config->getInstrConnectOnStartup() && instrCheckSettings()) {
        //measurementDevice->instrConnect();
        btnConnectPressed();
    }
}

bool MainWindow::instrCheckSettings() // TODO: More checks here
{
    if (instrStartFreq->text().toDouble() > instrStopFreq->text().toDouble())
        return false;
    return true;
}

void MainWindow::setResolutionFunction()
{
    disconnect(instrResolution, &QComboBox::currentIndexChanged, this, &MainWindow::instrResolutionChanged);
    disconnect(instrFfmSpan, &QComboBox::currentIndexChanged, this, &MainWindow::instrFfmSpanChanged);

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

    connect(instrResolution, &QComboBox::currentIndexChanged, this, &MainWindow::instrResolutionChanged);
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
    disconnect(instrFftMode, &QComboBox::currentTextChanged, config.data(), &Config::setInstrFftMode);

    instrFftMode->clear();
    QString val = config->getInstrFftMode(); // tmp keeper
    instrFftMode->addItems(measurementDevice->deviceFftModes());
    instrFftMode->setEnabled(instrFftMode->count() > 1); // no point in having 0 choices...

    if (instrFftMode->findText(val) >= 0) {
        instrFftMode->setCurrentIndex(instrFftMode->findText(val));
    }
    connect(instrFftMode, &QComboBox::currentTextChanged, config.data(), &Config::setInstrFftMode);
}

void MainWindow::btnConnectPressed(bool state)
{
    if (instrIpAddr->currentText() != config->getInstrIpAddr()) { // Custom text written
        if (instrIpAddr->findText(instrIpAddr->currentText()) != -1) {
            // IP/hostname written is the same as was already in the list, select this entry instead
            if (config->getInstrIpAddr() == instrIpAddr->currentText()) { // Failsafe in case list changed
                instrIpAddr->setCurrentIndex(instrIpAddr->findText(instrIpAddr->currentText()));
                qDebug() << "found" << instrIpAddr->currentText() << instrIpAddr->currentData() << config->getInstrIpAddr();
            }
            else {
                qDebug() << "not found" << config->getInstrIpAddr();
                instrIpAddr->addItem("Unknown", QVariant("127.0.0.1")); // Dummy, choose sth that exists
                instrIpAddr->setCurrentIndex(instrIpAddr->count() - 1); // Select dummy
            }

        }
        else {
            // New entry, store it for later
            config->setInstrCustomEntry(instrIpAddr->currentText());
            config->setInstrIpAddr(instrIpAddr->currentText());
            instrIpAddr->setItemData(instrIpAddr->currentIndex(), QVariant());
            qDebug() << "new" << instrIpAddr->currentText() << instrIpAddr->currentData() << config->getInstrIpAddr();

        }
    }
    if (!instrIpAddr->currentData().isValid()) {
        QHostAddress address;
        if (address.setAddress(instrIpAddr->currentText())) {
            qDebug() << "valid adr";
        }
        else {
            statusBar->showMessage("Looking up host " + instrIpAddr->currentText(), 5000);
            const QHostInfo info = QHostInfo::fromName(instrIpAddr->currentText());

            if (info.error() != QHostInfo::NoError) {
                qDebug() << "Error looking up IP address" << info.errorString();
                statusBar->showMessage("DNS lookup failed, " + info.errorString(), 5000);
            }
            else {
                instrIpAddr->setCurrentText(info.addresses().constFirst().toString());
                config->setInstrIpAddr(instrIpAddr->currentText());
            }
        }
    }
    if (instrIpAddr->currentData().isValid())
        measurementDevice->setAddress(instrIpAddr->currentData().toString());
    else
        measurementDevice->setAddress(instrIpAddr->currentText());
    //qDebug() << instrIpAddr->currentText() << instrIpAddr->currentData().toString();

    qDebug() << "connecting" << instrIpAddr->currentText() << instrIpAddr->currentData() << config->getInstrIpAddr();
    if (state) measurementDevice->instrConnect();
    instrDisconnect->setEnabled(true);
}

void MainWindow::instrIpChanged()
{
    config->setInstrIpAddr(instrIpAddr->currentText());
    qDebug() << "saving" << instrIpAddr->currentText();
}

void MainWindow::btnDisconnectPressed()
{
    instrDisconnect->setEnabled(false);
    measurementDevice->instrDisconnect();
}
