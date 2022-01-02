#include "measurementdevice.h"

MeasurementDevice::MeasurementDevice(QSharedPointer<Config> c)
{
    config = c;
    start();
}

void MeasurementDevice::start()
{
    //connect(scpiSocket, &QTcpSocket::connected, this, scpiConnected); NO! Don't pretend to be connected until we know what we have connected to!
    connect(scpiSocket, &QTcpSocket::disconnected, this, scpiDisconnected);
    connect(scpiSocket, &QTcpSocket::errorOccurred, this, scpiError);
    connect(scpiSocket, &QTcpSocket::stateChanged, this, scpiStateChanged);
    connect(scpiSocket, &QTcpSocket::readyRead, this, scpiRead);
    connect(tcpTimeoutTimer, &QTimer::timeout, this, tcpTimeout);

    connect(tcpStream, &TcpDataStream::newFftData, this, &MeasurementDevice::fftDataHandler);
    connect(udpStream, &UdpDataStream::newFftData, this, &MeasurementDevice::fftDataHandler);

    devicePtr = QSharedPointer<Device>(new Device);
    connect(tcpStream, &TcpDataStream::bytesPerSecond, this, forwardBytesPerSec);
    connect(udpStream, &UdpDataStream::bytesPerSecond, this, forwardBytesPerSec);
    connect(tcpStream, &TcpDataStream::tracesPerSecond, this, forwardTracesPerSec);
    connect(udpStream, &UdpDataStream::tracesPerSecond, this, forwardTracesPerSec);

    connect(tcpStream, &TcpDataStream::timeout, this, &MeasurementDevice::handleStreamTimeout);
    connect(udpStream, &UdpDataStream::timeout, this, &MeasurementDevice::handleStreamTimeout);

    autoReconnectTimer->setSingleShot(true);
    connect(autoReconnectTimer, &QTimer::timeout, this, &MeasurementDevice::autoReconnectCheckStatus);
}

void MeasurementDevice::instrConnect()
{
    scpiSocket->connectToHost(*scpiAddress, scpiPort);
    instrumentState = InstrumentState::CONNECTING;
    emit status("Connecting measurement device...");
    //tcpTimeoutTimer->start(2000);
}

void MeasurementDevice::scpiConnected()
{
    connected = true;
    emit connectedStateChanged(true); // if connected!!
}

void MeasurementDevice::scpiStateChanged(QAbstractSocket::SocketState state)
{
    if (state == QAbstractSocket::ConnectedState) {
        askId();
        emit status("Measurement device connected, asking for ID");
    }
}

void MeasurementDevice::scpiWrite(QByteArray data)
{
    if (!scpiThrottleTimer->isValid()) scpiThrottleTimer->start();

    while (scpiThrottleTimer->elapsed() < scpiThrottleTime) { // min. x ms time between scpi commands, to give device time to breathe
        QCoreApplication::processEvents();
    }
    scpiThrottleTimer->start();
    scpiSocket->write(data + '\n');
    qDebug() << data;
}

void MeasurementDevice::instrDisconnect()
{
    if (instrumentState == InstrumentState::CONNECTED) {
        instrumentState = InstrumentState::DISCONNECTED;
        emit toIncidentLog("Measurement receiver disconnected");
    }
    tcpTimeoutTimer->stop();
    emit instrId(QString());
    emit status("Disconnecting measurement device");
    if (connected) {
        delUdpStreams();
        delTcpStreams();
        scpiSocket->waitForBytesWritten(1000);
    }
    scpiSocket->close();

}

void MeasurementDevice::scpiDisconnected()
{
    tcpTimeoutTimer->stop();
    emit status("Measurement device disconnected");
    connected = false;
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
    if (connected && devicePtr->hasPscan) {
        scpiWrite("freq:psc:stop " + QByteArray::number(devicePtr->pscanStopFrequency));
        scpiWrite("freq:psc:start " + QByteArray::number(devicePtr->pscanStartFrequency));
        emit resetBuffers();
    }
}

void MeasurementDevice::setPscanResolution()
{
    if (connected && devicePtr->hasPscan)
        scpiWrite("psc:step " + QByteArray::number(devicePtr->pscanResolution, 'f', 3));
}

void MeasurementDevice::setFfmCenterFrequency()
{
    if (connected && devicePtr->hasFfm)
        scpiWrite("freq " + QByteArray::number(devicePtr->ffmCenterFrequency));
}

void MeasurementDevice::setFfmFrequencySpan()
{
    if (connected && devicePtr->hasFfm)
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
    if (connected)
        scpiWrite("inp:att " + QByteArray::number(attenuator));
}


void MeasurementDevice::setAutoAttenuator()
{
    if (connected) {
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
    if (connected) {
        if (!antPort.contains("default", Qt::CaseInsensitive))
            (void)1; // TODO!!
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
            //else if (devicePtr->hasDscan) { // ESMB support, TODO!

            //}
        }
        else {
            scpiWrite("freq:mode ffm");
        }
    }
    //if (connected) restartStream(); // rerun stream setup if mode changes (obviously)
}

void MeasurementDevice::setFftMode()
{
    if (connected) {
        if (fftMode == Instrument::FftMode::OFF) scpiWrite("calc:ifp:aver:type off");
        else if (fftMode == Instrument::FftMode::MIN) scpiWrite("calc:ifp:aver:type min");
        else if (fftMode == Instrument::FftMode::MAX) scpiWrite("calc:ifp:aver:type max");
        else if (fftMode == Instrument::FftMode::SCALAR) scpiWrite("calc:ifp:aver:type scal");
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
    tcpTimeoutTimer->start(2000);
}

void MeasurementDevice::checkId(const QByteArray buffer)
{
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
        tcpTimeoutTimer->start(2000);
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
        tcpTimeoutTimer->start(2000);
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
            delTcpStreams();
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
        tcpTimeoutTimer->start(2000);
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
    tcpTimeoutTimer->stop();
    instrDisconnect();
    deviceInUseWarningIssued = true;
    popup(msg);
}

void MeasurementDevice::stateConnected()
{
    if (!autoReconnectInProgress) { // don't nag user if this is an auto reconnect call
        tcpTimeoutTimer->stop();
        emit status("Connected, setting up device");
        emit toIncidentLog("Measurement receiver connected: " + devicePtr->longId);
        scpiConnected();
    }
    else
        emit toIncidentLog(devicePtr->id + ": Reconnected");

    autoReconnectInProgress = false;
    runAfterConnected();
    instrumentState = InstrumentState::CONNECTED;

    emit initiateDatastream();
}

void MeasurementDevice::tcpTimeout()
{
    if (instrumentState != InstrumentState::CONNECTED) {  // sth timed out, give a warning and disconnect
        instrDisconnect();
        emit popup("TCP socket operation timed out, disconnecting");
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
    scpiWrite("syst:man:loc:name Hauken"); // TODO
}

void MeasurementDevice::startDevice()
{
    scpiWrite("init:imm");
}

void MeasurementDevice::restartStream()
{
    delUdpStreams();
    if (devicePtr->tcpStream) delTcpStreams();
    if (useUdpStream)
        setupUdpStream();
    else
        setupTcpStream();
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
    disconnect(scpiSocket, &QTcpSocket::readyRead, this, scpiRead);
    scpiSocket->waitForReadyRead(1000);
    //QThread::msleep(500);

    QByteArray tmpBuffer = scpiSocket->readAll();
    QList<QByteArray> split = tmpBuffer.split(',');
    if (split.size() > 1) {
        tcpOwnAdress = split.at(0).simplified();
        tcpOwnPort = split.at(1).simplified();
    }
    connect(scpiSocket, &QTcpSocket::readyRead, this, scpiRead);

    scpiWrite("trac:tcp:tag:on " +
              tcpOwnAdress +
              ", " +
              //scpiSocket->localAddress().toString().toLocal8Bit() +
              //"', " +
              tcpOwnPort + ", " +
              modeStr);
    scpiWrite("trac:tcp:flag:on " +
              tcpOwnAdress +
              ", " +
              //scpiSocket->localAddress().toString().toLocal8Bit() +
              //"', " +
              tcpOwnPort + ", 'volt:ac', 'opt'");
}

void MeasurementDevice::setupUdpStream()
{
    udpStream->setDeviceType(devicePtr);
    udpStream->openListener();
    QByteArray modeStr;
    if (mode == Mode::PSCAN && !devicePtr->optHeaderDscan) modeStr = "pscan";
    else if (mode == Mode::PSCAN && devicePtr->optHeaderDscan) modeStr = "dscan";
    else if (mode == Mode::FFM) modeStr = "ifpan";

    scpiWrite("trac:udp:tag:on '" +
              scpiSocket->localAddress().toString().toLocal8Bit() +
              "', " +
              QByteArray::number(udpStream->getUdpPort()) + ", " +
              modeStr);
    scpiWrite("trac:udp:flag:on '" +
              scpiSocket->localAddress().toString().toLocal8Bit() +
              "', " +
              QByteArray::number(udpStream->getUdpPort()) + ", 'volt:ac', 'opt'");
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
    tcpTimeoutTimer->stop();
    if (autoReconnect) { // check stream settings regularly to see if device is available again
        autoReconnectTimer->start(15000);
        if (!autoReconnectInProgress) emit toIncidentLog(devicePtr->id + ": Lost datastream. Auto reconnect enabled, waiting for device to be available again");
    }
    else {
        emit toIncidentLog(devicePtr->id + ": Datastream timed out and reconnect is disabled, disconnecting");
        instrDisconnect();
    }
}

void MeasurementDevice::autoReconnectCheckStatus()
{
    autoReconnectInProgress = true;
    askUdp(); // the udp routine will call askTcp() for us, no worries
}

void MeasurementDevice::updSettings()
{

    if ((devicePtr->pscanStartFrequency != config->getInstrStartFreq() * 1e6 ||
            devicePtr->pscanStopFrequency != config->getInstrStopFreq() * 1e6) && config->getInstrStopFreq() > config->getInstrStartFreq()) {
        devicePtr->pscanStartFrequency = config->getInstrStartFreq() * 1e6;
        devicePtr->pscanStopFrequency = config->getInstrStopFreq() * 1e6;
        setPscanFrequency();
    }
    if (devicePtr->pscanResolution != config->getInstrResolution() * 1e3) {
        devicePtr->pscanResolution = config->getInstrResolution() * 1e3;
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
    antPort = config->getInstrAntPort();  //TODO

    if (mode != (Instrument::Mode)config->getInstrMode()) {
        mode = (Instrument::Mode)config->getInstrMode();
        setMode();
    }
    if (fftMode != (Instrument::FftMode)config->getInstrFftMode()) {
        fftMode = (Instrument::FftMode)config->getInstrFftMode();
        setFftMode();
    }
    scpiAddress = new QHostAddress(config->getInstrIpAddr());
    scpiPort = config->getInstrPort();

    if (useUdpStream != !config->getInstrUseTcpDatastream())
        useUdpStream = !config->getInstrUseTcpDatastream();
}
