#include "measurementdevice.h"

MeasurementDevice::MeasurementDevice(QSharedPointer<Config> c)
{
    config = c;
    start();
}

void MeasurementDevice::start()
{
    //connect(scpiSocket, &QTcpSocket::connected, this, scpiConnected); NO! Don't pretend to be connected until we know what we have connected to!
    connect(scpiSocket, &QTcpSocket::disconnected, this, &MeasurementDevice::scpiDisconnected);
    //connect(scpiSocket, &QTcpSocket::errorOccurred, this, &MeasurementDevice::scpiError);
    connect(scpiSocket, &QTcpSocket::stateChanged, this, &MeasurementDevice::scpiStateChanged);
    connect(scpiSocket, &QTcpSocket::readyRead, this, &MeasurementDevice::scpiRead);
    connect(tcpTimeoutTimer, &QTimer::timeout, this, &MeasurementDevice::tcpTimeout);

    connect(tcpStream, &TcpDataStream::newFftData, this, &MeasurementDevice::fftDataHandler);
    connect(udpStream, &UdpDataStream::newFftData, this, &MeasurementDevice::fftDataHandler);

    devicePtr = QSharedPointer<Device>(new Device);
    connect(tcpStream, &TcpDataStream::bytesPerSecond, this, &MeasurementDevice::forwardBytesPerSec);
    connect(udpStream, &UdpDataStream::bytesPerSecond, this, &MeasurementDevice::forwardBytesPerSec);
    connect(tcpStream, &TcpDataStream::tracesPerSecond, this, &MeasurementDevice::forwardTracesPerSec);
    connect(udpStream, &UdpDataStream::tracesPerSecond, this, &MeasurementDevice::forwardTracesPerSec);

    connect(tcpStream, &TcpDataStream::timeout, this, &MeasurementDevice::handleStreamTimeout);
    connect(udpStream, &UdpDataStream::timeout, this, &MeasurementDevice::handleStreamTimeout);

    autoReconnectTimer->setSingleShot(true);
    connect(autoReconnectTimer, &QTimer::timeout, this, &MeasurementDevice::autoReconnectCheckStatus);
    emit connectedStateChanged(connected);

    connect(tcpStream, &TcpDataStream::streamErrorResetFreq, this, &MeasurementDevice::resetFreqSettings);
    connect(udpStream, &UdpDataStream::streamErrorResetFreq, this, &MeasurementDevice::resetFreqSettings);

    connect(updGnssDisplayTimer, &QTimer::timeout, this, &MeasurementDevice::updGnssDisplay);
    updGnssDisplayTimer->start(1000);
}

void MeasurementDevice::instrConnect()
{
    discPressed = false;
    scpiSocket->connectToHost(*scpiAddress, scpiPort);
    instrumentState = InstrumentState::CONNECTING;
    emit status("Connecting to measurement device...");
    if (!scpiReconnect) tcpTimeoutTimer->start(20000);
}

void MeasurementDevice::scpiConnected()
{
    connected = true;
    emit connectedStateChanged(true); // if connected!!
}

void MeasurementDevice::scpiStateChanged(QAbstractSocket::SocketState state)
{
    qDebug() << "TCP scpi socket state" << state;

    if (state == QAbstractSocket::ConnectedState) {
        askId();
        emit status("Measurement device connected, asking for ID");
        firstConnection = false; // now we know network is up, keep calm and drink tea
    }
    else if (state == QAbstractSocket::UnconnectedState && connected && !discPressed) { // happens if instrument restarts or SCPI conn. goes away otherwise
        if (autoReconnect)
            scpiReconnect = true;
        instrDisconnect();
        autoReconnectTimer->start(15000);
    }
}

void MeasurementDevice::scpiWrite(QByteArray data)
{
    if (!scpiThrottleTimer->isValid()) scpiThrottleTimer->start();
    int throttle = scpiThrottleTime;
    if (devicePtr->id.contains("esmb", Qt::CaseInsensitive)) throttle *= 3; // esmb hack, slow interface
    while (scpiThrottleTimer->elapsed() < throttle) { // min. x ms time between scpi commands, to give device time to breathe
        QCoreApplication::processEvents();
    }
    scpiThrottleTimer->start();
    scpiSocket->write(data + '\n');
    qDebug() << data;
}

void MeasurementDevice::instrDisconnect()
{
    discPressed = true;
    if (instrumentState == InstrumentState::CONNECTED) {
        instrumentState = InstrumentState::DISCONNECTED;
        emit toIncidentLog(NOTIFY::TYPE::MEASUREMENTDEVICE, devicePtr->id, QString("Disconnected") + (scpiReconnect && !discPressed ? ". Trying to reconnect" : " "));
    }
    tcpTimeoutTimer->stop();
    autoReconnectTimer->stop();
    emit instrId(QString());
    emit status("Disconnecting measurement device");
    if (connected) {
        if (useUdpStream) delUdpStreams();
        else delTcpStreams();
        scpiSocket->waitForBytesWritten(1000);
    }
    scpiSocket->close();

}

void MeasurementDevice::scpiDisconnected()
{
    tcpTimeoutTimer->stop();
    emit status("Measurement device disconnected");
    emit connectedStateChanged(false);
}

void MeasurementDevice::scpiError(QAbstractSocket::SocketError error)
{
    tcpTimeoutTimer->stop();
    (void)error;
    QString msg = scpiSocket->errorString();
    instrDisconnect();
    emit popup(msg);
}

void MeasurementDevice::setPscanFrequency()
{
    if (connected && devicePtr->hasPscan && devicePtr->mode == Instrument::Mode::PSCAN) {
        abor();
        scpiWrite("freq:psc:stop " + QByteArray::number(devicePtr->pscanStopFrequency));
        scpiWrite("freq:psc:start " + QByteArray::number(devicePtr->pscanStartFrequency));
        initImm();
        emit resetBuffers();
    }
    else if (connected && devicePtr->hasDscan && devicePtr->mode == Instrument::Mode::PSCAN) { // esmb mode
        abor();
        scpiWrite("freq:dsc:stop " + QByteArray::number(devicePtr->pscanStopFrequency));
        scpiWrite("freq:dsc:start " + QByteArray::number(devicePtr->pscanStartFrequency));
        initImm();
        emit resetBuffers();
    }

}

void MeasurementDevice::setPscanResolution()
{
    if (connected && devicePtr->hasPscan && devicePtr->mode == Instrument::Mode::PSCAN) {
        abor();
        scpiWrite("psc:step " + QByteArray::number((int)devicePtr->pscanResolution));
        initImm();
    }
    else if (connected && devicePtr->hasDscan && devicePtr->mode == Instrument::Mode::PSCAN) {
        scpiWrite("bwid " + QByteArray::number((int)devicePtr->pscanResolution * 2));  // ESMB "hack"
        scpiWrite("dsc:coun inf");
    }
}

void MeasurementDevice::setFfmCenterFrequency()
{
    if (connected && devicePtr->hasFfm && devicePtr->mode == Instrument::Mode::FFM)
        scpiWrite("freq " + QByteArray::number(devicePtr->ffmCenterFrequency));
}

void MeasurementDevice::setFfmFrequencySpan()
{
    if (connected && devicePtr->hasFfm && devicePtr->mode == Instrument::Mode::FFM)
        scpiWrite("freq:span " + QByteArray::number(devicePtr->ffmFrequencySpan));
}

void MeasurementDevice::setMeasurementTime()
{
    if (connected) {
        scpiWrite("abor");
        scpiWrite("meas:time " + QByteArray::number(measurementTime) + " ms");
        scpiWrite("init:imm");
    }
}

void MeasurementDevice::setAttenuator()
{
    if (connected && !devicePtr->hasAttOnOff)
        scpiWrite("inp:att " + QByteArray::number(attenuator));
    else if (connected && devicePtr->hasAttOnOff) {
        if (attenuator > 0) scpiWrite("inp:att:stat on");
        else scpiWrite("inp:att:stat off");
    }
}


void MeasurementDevice::setAutoAttenuator()
{
    if (connected && devicePtr->hasAutoAtt) {
        QByteArray text;
        if (autoAttenuator)
            text = "inp:att:auto on";
        else
            text = "inp:att:auto off";
        scpiWrite(text);
    }
}

void MeasurementDevice::setAntPort()
{
    if (connected && !antPort.isEmpty() && !antPort.toLower().contains("default")) {
        if (!devicePtr->advProtocol) scpiWrite("syst:ant:rx " + antPort);
        else if (antPort.contains("1")) scpiWrite("route:vuhf:input (@0)");  // em200 specific
        else if (antPort.contains("2")) scpiWrite("route:vuhf:input (@1)");  // em200 specific
    }
}

QStringList MeasurementDevice::getAntPorts()
{
    return devicePtr->antPorts;
}

void MeasurementDevice::setMode()
{
    if (connected) {
        if (mode == Instrument::Mode::PSCAN) {
            if (devicePtr->hasPscan) {
                scpiWrite("freq:mode psc");
            }
            else if (devicePtr->hasDscan) {
                scpiWrite("freq:mode dsc");
            }
        }
        else {
            scpiWrite("freq:mode ffm");
        }
    }
    //if (connected) restartStream(); // rerun stream setup if mode changes (obviously)
}

void MeasurementDevice::setFftMode()
{
    if (connected && devicePtr->hasAvgType && devicePtr->hasPscan) {
        scpiWrite(QByteArray("calc:ifp:aver:type ") + fftMode.toLower());
    }
    else if (connected && devicePtr->hasAvgType && devicePtr->hasDscan) {
        scpiWrite(QByteArray("calc:dsc:aver:type ") + fftMode.toLower());
    }
}

void MeasurementDevice::setMeasurementMode()
{
    if (connected) {
        scpiWrite("meas:mode per");     //only doing periodic, at least for now
    }
}

void MeasurementDevice::askId()
{
    scpiWrite("*idn?");
    instrumentState = InstrumentState::CHECK_INSTR_ID;
    tcpTimeoutTimer->start(20000);
}

void MeasurementDevice::checkId(const QByteArray buffer)
{
    devicePtr->clearSettings();

    if (buffer.contains("EB500"))
        devicePtr->setType(InstrumentType::EB500);
    else if (buffer.contains("PR100"))
        devicePtr->setType(InstrumentType::PR100);
    else if (buffer.contains("PR200"))
        devicePtr->setType(InstrumentType::PR200);
    else if (buffer.contains("EM100"))
        devicePtr->setType(InstrumentType::EM100);
    else if (buffer.contains("EM200"))
        devicePtr->setType(InstrumentType::EM200);
    else if (buffer.contains("ESMB"))
        devicePtr->setType(InstrumentType::ESMB);
    else if (buffer.contains("USRP"))
        devicePtr->setType(InstrumentType::USRP);

    else devicePtr->setType(InstrumentType::UNKNOWN);

    if (devicePtr->type != InstrumentType::UNKNOWN) {
        QString msg = "Measurement receiver type " + devicePtr->id + " found, checking if available";
        emit status(msg);
        devicePtr->longId = tr(buffer.simplified());
        config->setInstrId(devicePtr->longId);
        config->setMeasurementDeviceName(devicePtr->id);

        emit instrId(devicePtr->longId);
        askUdp();
    }
    else {
        tcpTimeoutTimer->stop();
        QString msg = "Unknown instrument ID: " + tr(buffer);
        emit popup(msg);
        instrDisconnect();
    }
}

void MeasurementDevice::scpiRead()
{
    QByteArray buffer = scpiSocket->readAll();
    if (instrumentState == InstrumentState::CHECK_INSTR_ID)
        checkId(buffer);
    else if (instrumentState == InstrumentState::CHECK_INSTR_AVAILABLE_UDP)
        checkUdp(buffer);
    else if (instrumentState == InstrumentState::CHECK_INSTR_AVAILABLE_TCP)
        checkTcp(buffer);
    else if (instrumentState == InstrumentState::CHECK_INSTR_USERNAME)
        checkUser(buffer);
    qDebug() << "<<" << buffer;
}

void MeasurementDevice::askUdp()
{
    if (devicePtr->udpStream) {
        scpiWrite("trac:udp?");
        instrumentState = InstrumentState::CHECK_INSTR_AVAILABLE_UDP;
        tcpTimeoutTimer->start(20000);
    }
}

void MeasurementDevice::checkUdp(const QByteArray buffer)
{
    QList<QByteArray> datastreamList = buffer.split('\n');
    bool inUse = false;
    for (auto&& list : datastreamList) {
        QList<QByteArray> brokenList = list.split(',');
        for (auto && ownIp : myOwnAddresses) {
            if (list.contains(ownIp.toString().toLocal8Bit())) // FIXME: Check for same UDP stream port!!
                break; // we are the users, continue
        }
        if (brokenList.size() > 2 && !list.contains("DEF")) {
            inUse = true;
        }
    }
    if (!autoReconnectInProgress) {
        QString msg = (inUse? "Instrument is in use (UDP)":"Instrument is not in use (UDP)");
        emit status(msg);

        if (inUse && !deviceInUseWarningIssued)
            askUser();
        else if (inUse && deviceInUseWarningIssued) {
            deviceInUseWarningIssued = false;
            delUdpStreams();
            askTcp();
        }
        else {
            askTcp();
        }
    }
    else if (autoReconnectInProgress && !inUse) { // device available, check tcp or take it back
        if (devicePtr->tcpStream)
            askTcp();
        else
            stateConnected();
    }
    else if (autoReconnectInProgress && inUse) {
        if (devicePtr->tcpStream)
            askTcp();
        else
            handleStreamTimeout();
    }

}

void MeasurementDevice::askTcp()
{
    if (devicePtr->tcpStream) {
        scpiWrite("trac:tcp?");
        instrumentState = InstrumentState::CHECK_INSTR_AVAILABLE_TCP;
        tcpTimeoutTimer->start(20000);
    }
    else
        stateConnected();
}

void MeasurementDevice::checkTcp(const QByteArray buffer)
{
    QList<QByteArray> datastreamList = buffer.split('\n');
    bool inUse = false;
    for (auto&& list : datastreamList) {
        QList<QByteArray> brokenList = list.split(',');
        if (list.contains(tcpOwnAdress) && list.contains(tcpOwnPort))
            break; // we are the users, continue
        if (brokenList.size() > 2 && !list.contains("DEF")) {
            inUse = true;
        }
    }
    if (!autoReconnectInProgress) {
        QString msg = (inUse? "Instrument is in use (TCP)":"Instrument is not in use (TCP)");
        emit status(msg);

        if (inUse && !deviceInUseWarningIssued)
            askUser();
        else if (inUse && deviceInUseWarningIssued) { // overtake device
            deviceInUseWarningIssued = false;
            if (!useUdpStream) delTcpStreams();
            stateConnected();
        }
        else { // all checks done, we are cool to go
            stateConnected();
        }
    }
    else if (autoReconnectInProgress && !inUse) { // device available, take it back
        stateConnected();
    }
    else if (autoReconnectInProgress && inUse) {
        handleStreamTimeout();
    }
}


void MeasurementDevice::askUser()
{
    if (devicePtr->systManLocName) {
        scpiWrite("syst:man:loc:name?");
        instrumentState = InstrumentState::CHECK_INSTR_USERNAME;
        tcpTimeoutTimer->start(20000);
    }
    else
        checkUser("");
}

void MeasurementDevice::checkUser(const QByteArray buffer)
{
    QString msg = devicePtr->id + tr(" may be in use");
    if (!buffer.isEmpty()) {
        msg += tr(" by ") + QString(buffer).simplified();
    }
    msg += tr(". Press connect once more to override");
    if (!buffer.contains(config->getStationName().toLocal8Bit())) {           // 130522: Rebuilt to reconnect upon startup if computer reboots
        tcpTimeoutTimer->stop();
        instrDisconnect();
        deviceInUseWarningIssued = true;
        emit popup(msg);
    }
    else {
        qDebug() << "In use by myself, how silly! Continuing...";
        deviceInUseWarningIssued = true;
        askUdp();
    }
}

void MeasurementDevice::stateConnected()
{
    if (!autoReconnectInProgress) { // don't nag user if this is an auto reconnect call
        tcpTimeoutTimer->stop();
        emit status("Connected, setting up device");
        emit toIncidentLog(NOTIFY::TYPE::MEASUREMENTDEVICE, devicePtr->id, "Connected to " + devicePtr->longId);
    }
    else {
        emit toIncidentLog(NOTIFY::TYPE::MEASUREMENTDEVICE, devicePtr->id, "Reconnected");
    }

    scpiConnected();
    autoReconnectInProgress = false;
    runAfterConnected();
    instrumentState = InstrumentState::CONNECTED;

    emit initiateDatastream();
}

void MeasurementDevice::tcpTimeout()
{
    if (instrumentState != InstrumentState::CONNECTED) {  // sth timed out
        if (firstConnection && config->getInstrConnectOnStartup()) {  // First time connecting and we are set to connect on startup, let's give it some time and retry
            emit status("TCP timeout, retrying in 15 seconds");
            QTimer::singleShot(15000, this, &MeasurementDevice::instrConnect);
        }
        else {
            instrDisconnect();
            emit popup("TCP socket operation timed out, disconnecting");
        }
    }
}

void MeasurementDevice::delUdpStreams()
{
    scpiWrite("trac:udp:del all");

    udpStream->closeListener();
}

void MeasurementDevice::delTcpStreams()
{
    scpiWrite("trac:tcp:del all");
    tcpStream->closeListener();
}

void MeasurementDevice::runAfterConnected()
{
    setUser();
    setPscanFrequency();
    setPscanResolution();
    setFfmCenterFrequency();
    setFfmFrequencySpan();
    setMeasurementTime();
    setAttenuator();
    setAutoAttenuator();
    setAntPort();
    setMeasurementMode();
    setMode();
    setFftMode();
    restartStream();
    startDevice();
}

void MeasurementDevice::setUser()
{
    if (devicePtr->systManLocName) scpiWrite("syst:man:loc:name '" + config->getStationName().toLocal8Bit() + " (hauken)'");
    else if (devicePtr->memLab999) scpiWrite("MEM:LAB 999, '" + config->getStationName().toLocal8Bit() + " (hauken)'");
    else if (devicePtr->systKlocLab) scpiWrite("syst:kloc:lab '" + config->getStationName().toLocal8Bit() + " (hauken)'");
}

void MeasurementDevice::startDevice()
{
    scpiWrite("init:imm");
}

void MeasurementDevice::restartStream()
{
    if (connected) {
        delUdpStreams();
        delTcpStreams();
        if (useUdpStream)
            setupUdpStream();
        else
            setupTcpStream();
    }
}

void MeasurementDevice::setupTcpStream()
{
    tcpStream->setDeviceType(devicePtr);
    tcpStream->openListener(*scpiAddress, scpiPort + 10);
    QByteArray modeStr;
    if (mode == Mode::PSCAN && !devicePtr->optHeaderDscan) modeStr = "pscan";
    else if (mode == Mode::PSCAN && devicePtr->optHeaderDscan) modeStr = "dscan";
    else if (mode == Mode::FFM) modeStr = "ifpan";

    // ssh tunnel hackaround
    tcpOwnAdress = scpiSocket->localAddress().toString().toLocal8Bit();
    tcpOwnPort = QByteArray::number(tcpStream->getTcpPort());

    scpiWrite("trac:tcp:sock?");
    disconnect(scpiSocket, &QTcpSocket::readyRead, this, &MeasurementDevice::scpiRead);
    scpiSocket->waitForReadyRead(1000);
    //QThread::msleep(500);

    QByteArray tmpBuffer = scpiSocket->readAll();
    QList<QByteArray> split = tmpBuffer.split(',');
    if (split.size() > 1) {
        tcpOwnAdress = split.at(0).simplified();
        tcpOwnPort = split.at(1).simplified();
    }
    connect(scpiSocket, &QTcpSocket::readyRead, this, &MeasurementDevice::scpiRead);

    QByteArray gpsc;
    if (askForPosition) gpsc = ", gpsc";
    QByteArray em200Specific;
    if (devicePtr->advProtocol) em200Specific = ", 'ifpan', 'swap'"; // em200/pr200 specific setting, swap system inverted since these models

    scpiWrite("trac:tcp:tag:on " +
              tcpOwnAdress +
              ", " +
              //scpiSocket->localAddress().toString().toLocal8Bit() +
              //"', " +
              tcpOwnPort + ", " +
              modeStr + gpsc);
    scpiWrite("trac:tcp:flag:on " +
              tcpOwnAdress +
              ", " +
              //scpiSocket->localAddress().toString().toLocal8Bit() +
              //"', " +
              tcpOwnPort + ", 'volt:ac', 'opt'" + em200Specific);
}

void MeasurementDevice::setupUdpStream()
{
    udpStream->setDeviceType(devicePtr);
    udpStream->openListener();
    QByteArray modeStr;
    if (mode == Mode::PSCAN && !devicePtr->optHeaderDscan) modeStr = "pscan";
    else if (mode == Mode::PSCAN && devicePtr->optHeaderDscan) modeStr = "dscan";
    else if (mode == Mode::FFM) modeStr = "ifpan";

    QByteArray gpsc;
    if (askForPosition) gpsc = ", gpsc";
    QByteArray em200Specific;
    if (devicePtr->advProtocol) em200Specific = ", 'ifpan', 'swap'"; // em200/pr200 specific setting, swap system inverted since these models

    scpiWrite("trac:udp:tag:on '" +
              scpiSocket->localAddress().toString().toLocal8Bit() +
              "', " +
              QByteArray::number(udpStream->getUdpPort()) + ", " +
              modeStr + gpsc);
    scpiWrite("trac:udp:flag:on '" +
              scpiSocket->localAddress().toString().toLocal8Bit() +
              "', " +
              QByteArray::number(udpStream->getUdpPort()) + ", 'volt:ac', 'opt'" + em200Specific);
}

void MeasurementDevice::forwardBytesPerSec(int val)
{
    emit bytesPerSec(val);
}

void MeasurementDevice::fftDataHandler(QVector<qint16> &data)
{
    emit newTrace(data);
}

void MeasurementDevice::handleStreamTimeout()
{
    qDebug() << "Stream timeout triggered" << connected;

    if (connected) {
        tcpTimeoutTimer->stop();
        if (autoReconnect) { // check stream settings regularly to see if device is available again
            autoReconnectTimer->start(15000);
            if (!autoReconnectInProgress) {
                emit toIncidentLog(NOTIFY::TYPE::MEASUREMENTDEVICE, devicePtr->id, "Lost datastream. Auto reconnect enabled, waiting for device to be available again");
                emit deviceStreamTimeout();
            }
        }
        else {
            emit toIncidentLog(NOTIFY::TYPE::MEASUREMENTDEVICE, devicePtr->id, "Datastream timed out and reconnect is disabled, disconnecting");
            instrDisconnect();
        }
    }
    else {
        udpStream->closeListener();
        tcpStream->closeListener();
    }
}

void MeasurementDevice::autoReconnectCheckStatus()
{
    qDebug() << "Auto reconnect check" << scpiReconnect << scpiSocket->isOpen();

    if (scpiReconnect && !scpiSocket->isOpen()) {
        instrConnect();
    }
    else {
        scpiReconnect = false;
        autoReconnectInProgress = true;
        askUdp(); // the udp routine will call askTcp() for us, no worries
    }
}

void MeasurementDevice::updSettings()
{
    if ((devicePtr->pscanStartFrequency != config->getInstrStartFreq() * 1e6 ||
         devicePtr->pscanStopFrequency != config->getInstrStopFreq() * 1e6) && config->getInstrStopFreq() > config->getInstrStartFreq()) {
        devicePtr->pscanStartFrequency = config->getInstrStartFreq() * 1e6;
        devicePtr->pscanStopFrequency = config->getInstrStopFreq() * 1e6;
        setPscanFrequency();
    }
    if (devicePtr->pscanResolution != config->getInstrResolution().toDouble() * 1e3) {
        devicePtr->pscanResolution = config->getInstrResolution().toDouble() * 1e3;
        setPscanResolution();
    }
    if (devicePtr->ffmCenterFrequency != config->getInstrFfmCenterFreq() * 1e6) {
        devicePtr->ffmCenterFrequency = config->getInstrFfmCenterFreq() * 1e6;
        setFfmCenterFrequency();
    }
    if (devicePtr->ffmFrequencySpan != config->getInstrFfmSpan() * 1e3) {
        devicePtr->ffmFrequencySpan = config->getInstrFfmSpan() * 1e3;
        setFfmFrequencySpan();
    }
    if (measurementTime != config->getInstrMeasurementTime()) {
        measurementTime = config->getInstrMeasurementTime();
        setMeasurementTime();
    }
    if (attenuator != config->getInstrManAtt()) {
        attenuator = config->getInstrManAtt();
        setAttenuator();
    }
    if (autoAttenuator != config->getInstrAutoAtt()) {
        autoAttenuator = config->getInstrAutoAtt();
        setAutoAttenuator();
    }
    if (antPort != config->getInstrAntPort().toLocal8Bit()) {
        antPort = config->getInstrAntPort().toLocal8Bit();
        setAntPort();
    }

    if (mode != (Instrument::Mode)config->getInstrMode()) {
        mode = (Instrument::Mode)config->getInstrMode();
        setMode();
    }
    if (!fftMode.contains(config->getInstrFftMode().toLocal8Bit())) {
        fftMode = config->getInstrFftMode().toLocal8Bit();
        setFftMode();
    }
    scpiAddress = new QHostAddress(config->getInstrIpAddr());
    scpiPort = config->getInstrPort();

    if (useUdpStream != !config->getInstrUseTcpDatastream())
        useUdpStream = !config->getInstrUseTcpDatastream();

    if (config->getSdefAddPosition() && config->getSdefGpsSource().contains("Instrument"))  {// only ask device for position if it is needed
        if (!askForPosition) restartStream(); // user changed this live, lets reconnect streams
        askForPosition = true;
    }
    else {
        if (askForPosition) restartStream(); // user changed this live, lets reconnect streams
        askForPosition = false;
    }
}

void MeasurementDevice::resetFreqSettings()
{
    setPscanFrequency();
    setPscanResolution();
    setFfmCenterFrequency();
    setFfmFrequencySpan();
    qDebug() << "error handling freq settings" << devicePtr->pscanStartFrequency << devicePtr->pscanStopFrequency << devicePtr->pscanResolution;
}

void MeasurementDevice::updGnssDisplay()
{
    QString out;
    QTextStream ts(&out);
    if (devicePtr->positionValid)
        ts << "<table style='color:black'>";
    else
        ts << "<table style='color:grey'>";
    ts.setRealNumberNotation(QTextStream::FixedNotation);
    ts.setRealNumberPrecision(5);
    ts << "<tr><td>Latitude</td><td align=right>" << devicePtr->latitude << "</td></tr>"
       << "<tr><td>Longitude</td><td align=right>" << devicePtr->longitude << "</td></tr>";
    ts.setRealNumberPrecision(1);
    ts << "<tr><td>Altitude</td><td align=right>" << devicePtr->altitude / 100 << "</td></tr>"
       << "<tr><td>DOP</td><td align=right>" << devicePtr->dop << "</td></tr>"
       << "</font></table>";
    if (connected && config->getSdefAddPosition()) emit displayGnssData(out, 3, devicePtr->positionValid);
}
