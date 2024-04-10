#include "arduino.h"

Arduino::Arduino(QObject *parent)
{
    //(void)parent;
    this->setParent(parent);

    connect(arduino, &QSerialPort::readyRead, this, [this]
    {
        this->buffer.append(arduino->readAll());
        this->handleBuffer();
    });

    connect(watchdogTimer, &QTimer::timeout, this, &Arduino::resetWatchdog);
    connect(pingTimer, &QTimer::timeout, this, &Arduino::ping);
    connect(pingProcess, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished), this, &Arduino::pong);

    updSettings();

    if (QSysInfo::kernelType().contains("win")) {
        pingProcess->setProgram("ping.exe");
    }

    else if (QSysInfo::kernelType().contains("linux")) {
        pingProcess->setProgram("ping");
    }

    start();
}

void Arduino::start()
{
    if (getArduinoEnable()) connectToPort();

    wdg->setAttribute(Qt::WA_QuitOnClose);
    wdg->setWindowFlag(Qt::WindowStaysOnTopHint);

    relayOnBtn->setText(getArduinoRelayOnText());
    relayOffBtn->setText(getArduinoRelayOffText());

    connect(relayOnBtn, &QPushButton::clicked, this, &Arduino::relayBtnOnPressed);
    connect(relayOffBtn, &QPushButton::clicked, this, &Arduino::relayBtnOffPressed);
    connect(btnWatchdogOn, &QPushButton::clicked, this, &Arduino::watchdogOn);
    connect(btnWatchdogOff, &QPushButton::clicked, this, &Arduino::watchdogOff);

    wdg->resize(250, 150);
    wdg->setWindowTitle("Arduino interface");
    QGridLayout *mainLayout = new QGridLayout;

    mainLayout->addWidget(tempLabel, 0, 0);
    mainLayout->addWidget(tempBox, 0, 1);
    if (dht20Active) {
        mainLayout->addWidget(humLabel, 1, 0);
        mainLayout->addWidget(humidityBox, 1, 1);
    }
    else if (tempRelayActive) {
        mainLayout->addWidget(new QLabel("Relay state"), 2, 0);
        mainLayout->addWidget(relayStateText, 2, 1);
        mainLayout->addWidget(relayOnBtn, 3, 0);
        mainLayout->addWidget(relayOffBtn, 3, 1);
    }

    if (watchdogRelayActive) {
        mainLayout->addWidget(humLabel, 1, 0);
        mainLayout->addWidget(humidityBox, 1, 1);
        if (!dht20Active) {
            tempLabel->setDisabled(true);
            humLabel->setDisabled(true);
        }
        mainLayout->addWidget(new QLabel("Watchdog state"), 2, 0);
        mainLayout->addWidget(watchdogText, 2, 1);
        /*mainLayout->addWidget(btnWatchdogOn, 3, 0);
        mainLayout->addWidget(btnWatchdogOff, 3, 1);*/
        if (pingActivated) {
            mainLayout->addWidget(new QLabel("Ping state"), 3, 0);
            mainLayout->addWidget(pingStateText, 3, 1);
            pingStateText->setText("Waiting for first ping");
        }
    }
    wdg->setLayout(mainLayout);

    wdg->setMinimumSize(200, 1);
    //qDebug() << dialog->sizeHint();
    wdg->adjustSize();

    wdg->restoreGeometry(getArduinoWindowState());
    if (arduino->isOpen()) {
        wdg->show();
    }
    if (watchdogRelayActive && getArduinoActivateWatchdog()) {
        watchdogTimer->start(5000); // reset watchdog every 5 sec
    }
}

void Arduino::connectToPort()
{

    QSerialPortInfo portInfo;

    if (!getArduinoSerialName().isEmpty() && !getArduinoBaudrate().isEmpty()) {
        for (auto &val : QSerialPortInfo::availablePorts()) {
            if (!val.isNull() && val.portName().contains(getArduinoSerialName(), Qt::CaseInsensitive))
                portInfo = val;
        }
        if (!portInfo.isNull())
            arduino->setPort(portInfo);
        else {
            qDebug() << "Arduino: couldn't find serial port" << portInfo.portName() << ", check settings";
        }
        arduino->setBaudRate(getArduinoBaudrate().toUInt());
        if (arduino->open(QIODevice::ReadWrite)) {
            qDebug() << "Arduino: connected to" << arduino->portName();
            // This is needed to make Arduino start sending data
            //qDebug() << "DTR state" << arduino->isDataTerminalReady();
            arduino->setDataTerminalReady(true);
            //qDebug() << "DTR state" << arduino->isDataTerminalReady();
        }
        else
            qDebug() << "Arduino: cannot open" << arduino->portName();
        }
}

void Arduino::handleBuffer()
{
    if (buffer.contains('\n')) {
        QList<QByteArray> list = buffer.split(',');
        if (tempRelayActive && list.size() > 1) {
            bool ok = false;
            float tmp = list.at(0).toFloat(&ok);
            if (ok) temperature = tmp;
            if (list.at(1).contains("on")) relayState = true;
            else relayState = false;
            tempBox->setText(QString::number(temperature) + " °C");
            if (relayState) relayStateText->setText(getArduinoRelayOnText());
            else relayStateText->setText(getArduinoRelayOffText());
        }
        else if (dht20Active && !watchdogRelayActive && list.size() == 5) {
            bool ok = false;
            float tmp = list.at(2).toFloat(&ok);
            if (ok) temperature = tmp;
            tmp = list.at(4).toFloat(&ok);
            if (ok) humidity = tmp;
            tempBox->setText(QString::number(temperature) + " °C");
            humidityBox->setText(QString::number(humidity) + " %");
        }
        else if (watchdogRelayActive && list.size() > 4 && list.last().contains("DHT20&Watchdog")) {
            bool ok = false;
            float tmp = list.at(0).toFloat(&ok);
            if (ok) temperature = tmp;
            tmp = list.at(1).toFloat(&ok);
            if (ok) humidity = tmp;
            if (dht20Active) {
                tempBox->setText(QString::number(temperature) + " °C");
                humidityBox->setText(QString::number(humidity) + " %");
            }
            int t = list.at(2).toInt(&ok);
            if (ok && t == 1) stateWatchdog = true;
            else if (ok) stateWatchdog = false;

            tmp = list.at(3).toInt(&ok);
            if (ok) secondsLeft = tmp;

            if (!stateWatchdog && getArduinoActivateWatchdog()) {
                qDebug() << "Watchdog set to be activated, but is currently disabled. Asking to activate";
                watchdogOn();
            }
            else if (stateWatchdog && !getArduinoActivateWatchdog()) {
                qDebug() << "Watchdog set to be inactive, but is currently enabled. Asking to deactivate";
                watchdogOff();
            }

            if (stateWatchdog) watchdogText->setText("Enabled, " + QString::number(secondsLeft) + " sec until barking");
            else watchdogText->setText("Disabled");
        }
        //qDebug() << buffer;
        buffer.clear();
    }

}

void Arduino::relayBtnOnPressed()
{
    arduino->write("1");
}

void Arduino::relayBtnOffPressed()
{
    arduino->write("0");
}

void Arduino::resetWatchdog()
{
    if ((pingTimer->isActive() && lastPingValid && arduino->isOpen()) || (!pingTimer->isActive() && arduino->isOpen())) arduino->write("1");
}

void Arduino::watchdogOn()
{
    arduino->write("W");
    //setArduinoActivateWatchdog(true);
    watchdogTimer->start(5000);
    if (pingActivated) pingTimer->start(pingInterval * 1e3);
}

void Arduino::watchdogOff()
{
    arduino->write("w");
    //setArduinoActivateWatchdog(false);
    pingTimer->stop();
    watchdogTimer->stop();
}

void Arduino::updSettings()
{
    pingAddress = getArduinoPingAddress();
    pingInterval = getArduinoPingInterval();
    tempRelayActive = getArduinoReadTemperatureAndRelay();
    dht20Active = getArduinoReadDHT20();
    watchdogRelayActive  = getArduinoWatchdogRelay();

    if (!pingAddress.isEmpty() && pingInterval > 0) {
        pingTimer->start(pingInterval * 1e3);
        pingActivated = true;
    }
    else if (pingAddress.isEmpty() || pingInterval <= 0) pingTimer->stop();
}

void Arduino::ping()
{
    if (pingProcess->state() != QProcess::NotRunning) { // for some reason the process hangs
        qDebug() << "Ping process stuck, closing" << pingProcess->state() << pingProcess->processId();
        pingProcess->close();
        lastPingValid = false;
    }

    if (QSysInfo::kernelType().contains("win")) {
        pingProcess->setArguments(QStringList() << "-n" << "1" << pingAddress);
    }

    else if (QSysInfo::kernelType().contains("linux")) {
        pingProcess->setArguments(QStringList() << "-c" << "1" << pingAddress);
    }

    pingProcess->start();
}

void Arduino::pong(int exitCode, QProcess::ExitStatus exitStatus)
{
    (void)exitStatus;

    if (exitCode != 0) {
        if (lastPingValid) qDebug() << "Last ping failed (" + pingAddress + ")";
        lastPingValid = false;
        pingStateText->setText("Last ping failed (" + pingAddress + ")");
    }
    else {
        if (!lastPingValid) qDebug() << "Ping ok";
        lastPingValid = true;
        pingStateText->setText("Last ping ok at " + QDateTime::currentDateTime().toString("hh:mm:ss"));
    }
}
