#include "measurementdevice.h"
#include "asciitranslator.h"

MeasurementDevice::MeasurementDevice(QSharedPointer<Config> c)
{
    config = c;
    start();
}

MeasurementDevice::~MeasurementDevice()
{
    if (connected) instrDisconnect();
}

void MeasurementDevice::start()
{
    connect(scpiSocket, &QTcpSocket::disconnected, this, &MeasurementDevice::scpiDisconnected);
    //connect(scpiSocket, &QTcpSocket::errorOccurred, this, &MeasurementDevice::scpiError);
    connect(scpiSocket, &QTcpSocket::stateChanged, this, &MeasurementDevice::scpiStateChanged);
    connect(scpiSocket, &QTcpSocket::readyRead, this, &MeasurementDevice::scpiRead);
    connect(tcpTimeoutTimer, &QTimer::timeout, this, &MeasurementDevice::tcpTimeout);
    connect(timedReconnectTimer, &QTimer::timeout, this, &MeasurementDevice::instrConnect);

    timedReconnectTimer->setSingleShot(true);
    autoReconnectTimer->setSingleShot(true);
    connect(autoReconnectTimer, &QTimer::timeout, this, &MeasurementDevice::autoReconnectCheckStatus);
    emit connectedStateChanged(connected);

    devicePtr = QSharedPointer<Device>(new Device);

    connect(updGnssDisplayTimer, &QTimer::timeout, this, &MeasurementDevice::updGnssDisplay);
    updGnssDisplayTimer->start(1000);

    connect(updFrequencyData, &QTimer::timeout, this, &MeasurementDevice::askFrequencies);

    initializeDevicePtr();
}

void MeasurementDevice::initializeDevicePtr()
{
    devicePtr->pscanStartFrequency = config->getInstrStartFreq();
    devicePtr->pscanStopFrequency = config->getInstrStopFreq();
    devicePtr->ffmCenterFrequency = config->getInstrFfmCenterFreq();
}

void MeasurementDevice::instrConnect()
{
    discPressed = false;
    scpiSocket->abort();
    scpiSocket->connectToHost(*scpiAddress, scpiPort);
    instrumentState = InstrumentState::CONNECTING;
    emit status("Connecting to measurement device...");
    if (!scpiReconnect) tcpTimeoutTimer->start(tcpTimeoutInMs);
}

void MeasurementDevice::scpiConnected()
{
    connected = true;
    emit connectedStateChanged(true); // if connected!!
    updFrequencyData->start(60000);
}

void MeasurementDevice::scpiStateChanged(QAbstractSocket::SocketState state)
{
    //qDebug() << "TCP scpi socket state" << state;

    if (state == QAbstractSocket::ConnectedState) {
        askId();
        emit status("Measurement device connected, asking for ID");
    }
    else if (state == QAbstractSocket::UnconnectedState && connected && !discPressed) { // happens if instrument restarts or SCPI conn. goes away otherwise
        if (autoReconnect)
            scpiReconnect = true;
        instrDisconnect();

        autoReconnectTimer->start(5000);
    }
    if (state == QAbstractSocket::UnconnectedState) {
        emit status("Measurement device disconnected");
        firstConnection = true;
    }
}

void MeasurementDevice::scpiWrite(QByteArray data)
{
    if (scpiSocket->state() == QAbstractSocket::ConnectedState) {
        if (scpiThrottleTimer->isValid()) {
            int throttle = scpiThrottleTime;
            if (devicePtr->id.contains("esmb", Qt::CaseInsensitive)) throttle *= 3; // esmb hack, slow interface
            while (scpiThrottleTimer->elapsed() < throttle) { // min. x ms time between scpi commands, to give device time to breathe
                QCoreApplication::processEvents();
            }
        }
        scpiThrottleTimer->start();
        scpiSocket->write(data + '\n');
        qDebug() << ">>" << data;
    }
}

void MeasurementDevice::instrDisconnect()
{
    muteNotification = false;
    discPressed = true;
    deviceInUseWarningIssued = false;
    if (instrumentState == InstrumentState::CONNECTED) {
        instrumentState = InstrumentState::DISCONNECTED;
        emit toIncidentLog(NOTIFY::TYPE::MEASUREMENTDEVICE, devicePtr->id, QString("Disconnected") + (scpiReconnect && !discPressed ? ". Trying to reconnect" : " "));
    }
    tcpTimeoutTimer->stop();
    autoReconnectTimer->stop();
    timedReconnectTimer->stop();

    emit instrId(QString());
    if (scpiSocket->state() != QAbstractSocket::UnconnectedState) emit status("Disconnecting measurement device");

    if (connected) {
        /*if (useUdpStream) delUdpStreams();
        else delTcpStreams();*/
        delTcpStreams();
        delUdpStreams();
        scpiSocket->waitForBytesWritten(10);
    }
    updFrequencyData->stop();
    scpiSocket->close();
    tcpStream->closeListener();
    udpStream->closeListener();
    vifStreamTcp->closeListener();
}

void MeasurementDevice::scpiDisconnected()
{
    tcpTimeoutTimer->stop();
    //emit status("Measurement device disconnected");
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

void MeasurementDevice::setPscanFrequency(const quint64 startf, const quint64 stopf)
{
    quint64 startfreq = startf;
    quint64 stopfreq = stopf;

    if (startf == 0 && stopf == 0) {
        startfreq = devicePtr->pscanStartFrequency;
        stopfreq = devicePtr->pscanStopFrequency;
    }

    if (connected &&
        devicePtr->hasPscan &&
        devicePtr->mode == Instrument::Mode::PSCAN &&
        startfreq < stopfreq)
    {
        //abor();
        scpiWrite("freq:psc:stop " + QByteArray::number(stopfreq));
        scpiWrite("freq:psc:start " + QByteArray::number(startfreq));
        devicePtr->pscanStartFrequency = startfreq;
        devicePtr->pscanStopFrequency = stopfreq;
        initImm();
        emit resetBuffers();
    }
    else if (connected &&
             devicePtr->hasDscan &&
             devicePtr->mode == Instrument::Mode::PSCAN &&
             startf < stopf)
    { // esmb mode
        abor();
        scpiWrite("freq:dsc:stop " + QByteArray::number(stopfreq));
        scpiWrite("freq:dsc:start " + QByteArray::number(startfreq));
        devicePtr->pscanStartFrequency = startfreq;
        devicePtr->pscanStopFrequency = stopfreq;
        initImm();
        emit resetBuffers();
    }
}

void MeasurementDevice::setPscanResolution()
{
    if (connected && devicePtr->hasPscan && devicePtr->mode == Instrument::Mode::PSCAN) {
        //abor();
        scpiWrite("psc:step " + QByteArray::number((int)devicePtr->pscanResolution));
        initImm();
    }
    else if (connected && devicePtr->hasDscan && devicePtr->mode == Instrument::Mode::PSCAN) {
        scpiWrite("bwid " + QByteArray::number((int)devicePtr->pscanResolution * 2));  // ESMB "hack"
        scpiWrite("dsc:coun inf");
    }
}

void MeasurementDevice::setFfmCenterFrequency(const quint64 freq)
{
    quint64 f = freq;

    if (f == 0)
        f = devicePtr->ffmCenterFrequency;
    else
        devicePtr->ffmCenterFrequency = f;

    if (connected && devicePtr->hasFfm && devicePtr->mode == Instrument::Mode::FFM)
        scpiWrite("sens:freq " + QByteArray::number(f));
}

void MeasurementDevice::setFfmFrequencySpan()
{
    if (connected && devicePtr->hasFfm && devicePtr->mode == Instrument::Mode::FFM)
        scpiWrite("freq:span " + QByteArray::number(devicePtr->ffmFrequencySpan));
}

void MeasurementDevice::setMeasurementTime()
{
    if (connected) {
        //scpiWrite("abor");
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
        if (!devicePtr->advProtocol && devicePtr->type != InstrumentType::USRP && devicePtr->type != InstrumentType::DDF255) scpiWrite("syst:ant:rx " + antPort);
        else if (antPort.contains("1")) scpiWrite("route:vuhf:input (@0)");  // em200/esmw specific
        else if (antPort.contains("2")) scpiWrite("route:vuhf:input (@1)");  // em200/esmw specific
        else if (antPort.contains("3")) scpiWrite("route:vuhf:input (@2)");  // em200/esmw specific
    }
}

/*QStringList MeasurementDevice::antPorts()
{
    return devicePtr->antPorts;
}*/

void MeasurementDevice::setMode()
{
    if (connected && !autoReconnectInProgress) {
        if (devicePtr->mode == Instrument::Mode::PSCAN) {
            if (devicePtr->hasPscan) {
                scpiWrite("freq:mode psc");
            }
            else if (devicePtr->hasDscan) {
                scpiWrite("freq:mode dsc");
            }
            emit modeUsed("pscan");
        }
        else {
            scpiWrite("freq:mode ffm");
            emit modeUsed("ffm");
        }
    }
    /*if (useUdpStream && connected && !autoReconnectInProgress) {
        muteNotification = true;
        restartStream(false); // rerun stream setup if mode changes (obviously)
    }*/
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
    waitingForReply = true;
    instrumentState = InstrumentState::CHECK_INSTR_ID;
    tcpTimeoutTimer->start(tcpTimeoutInMs);
}

void MeasurementDevice::checkId(const QByteArray buffer)
{
    waitingForReply = false;
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
    else if (buffer.contains("ESMW"))
        devicePtr->setType(InstrumentType::ESMW);
    else if (buffer.contains("DDF255"))
        devicePtr->setType(InstrumentType::DDF255);

    else devicePtr->setType(InstrumentType::UNKNOWN);

    if (devicePtr->type != InstrumentType::UNKNOWN) {
        QString msg = "Measurement receiver type " + devicePtr->id + " found, checking if available";
        emit status(msg);
        devicePtr->longId = tr(buffer.simplified());
        if (!devicePtr->tcpStream && config->getInstrUseTcpDatastream()) config->setInstrUseTcpDatastream(false); // Switch off tcp stream option if device doesn't support it
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
    //qDebug() << "<<" << buffer;
    if (instrumentState == InstrumentState::CHECK_INSTR_ID)
        checkId(buffer);
    else if (instrumentState == InstrumentState::CHECK_INSTR_AVAILABLE_UDP)
        checkUdp(buffer);
    else if (instrumentState == InstrumentState::CHECK_INSTR_AVAILABLE_TCP)
        checkTcp(buffer);
    else if (instrumentState == InstrumentState::CHECK_INSTR_USERNAME)
        checkUser(buffer);
    else if (instrumentState == InstrumentState::CHECK_USER_ONLY)
        checkUserOnly(buffer);
    else if (instrumentState == InstrumentState::CHECK_ANT_NAME1 || instrumentState == InstrumentState::CHECK_ANT_NAME2)
        antennaNamesReply(buffer);
    else if (instrumentState == InstrumentState::CHECK_PSCAN_START_FREQ)
        checkPscanStartFreq(buffer);
    else if (instrumentState == InstrumentState::CHECK_PSCAN_STOP_FREQ)
        checkPscanStopFreq(buffer);
    else if (instrumentState == InstrumentState::CHECK_PSCAN_RESOLUTION)
        checkPscanResolution(buffer);
    else if (instrumentState == InstrumentState::CHECK_FFM_CENTERFREQ)
        checkFfmFreq(buffer);
    else if (instrumentState == InstrumentState::CHECK_FFM_SPAN)
        checkFfmSpan(buffer);
}

void MeasurementDevice::askUdp()
{
    if (devicePtr->udpStream) {
        scpiWrite("trac:udp?");
        waitingForReply = true;
        instrumentState = InstrumentState::CHECK_INSTR_AVAILABLE_UDP;
        tcpTimeoutTimer->start(tcpTimeoutInMs);
    }
}

void MeasurementDevice::checkUdp(const QByteArray buffer)
{
    QList<QByteArray> datastreamList = buffer.split('\n');
    bool inUse = false;
    waitingForReply = false;
    for (auto&& list : datastreamList) {
        QList<QByteArray> brokenList = list.split(',');
        //for (auto&& ownIp : myOwnAddresses) {
        //if (list.contains(ownIp.toString().toLocal8Bit()) && list.contains(QByteArray::number(udpStream->getUdpPort())))
        if (list.contains(scpiSocket->localAddress().toString().toLocal8Bit()) && list.contains(QByteArray::number(udpStream->getUdpPort())))
            break; // we are the users, continue
        if (brokenList.size() > 2 && !list.contains("DEF")) {
            inUse = true;
        }
        //}
    }
    if (!autoReconnectInProgress) {
        QString msg = (inUse? "Instrument is in use (UDP)":"Instrument is not in use (UDP)");
        emit status(msg);

        if (inUse && !deviceInUseWarningIssued)
            askUser();
        else if (inUse && deviceInUseWarningIssued) {
            deviceInUseWarningIssued = false;
            //delUdpStreams();
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
        /*if (devicePtr->tcpStream) // Why? We already know it is in use?!
            askTcp();
        else*/
        handleStreamTimeout();
    }
}

void MeasurementDevice::askTcp()
{
    if (devicePtr->tcpStream) {
        scpiWrite("trac:tcp?");
        waitingForReply = true;
        instrumentState = InstrumentState::CHECK_INSTR_AVAILABLE_TCP;
        tcpTimeoutTimer->start(tcpTimeoutInMs);
    }
    else
        stateConnected();
}

void MeasurementDevice::checkTcp(const QByteArray buffer)
{
    QList<QByteArray> datastreamList = buffer.split('\n');
    bool inUse = false;
    waitingForReply = false;
    for (auto&& list : datastreamList) {
        QList<QByteArray> brokenList = list.split(',');
        if (brokenList.size() > 2) {
            QList<QByteArray> brkList = brokenList[0].split(' ');
            if (brkList.size() > 1) inUseByIp = brokenList[0].split(' ')[1].simplified();
            else inUseByIp = brokenList[0].simplified();
            inUseByIp.remove('\"');
            QString text = "remote";
            if (inUseByIp.size() > 6) {
                for (auto && ownIp : myOwnAddresses) {
                    if (list.contains(ownIp.toString().toLocal8Bit()))
                        text = "local";
                }
                emit ipOfUser(text);
                if (list.toLower().contains("psc")) emit modeUsed("pscan");
                else if (list.toLower().contains("cw")) emit modeUsed("cw");
                else if (list.toLower().contains("ifp")) emit modeUsed("ifpan");
            }
        }
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
            //if (!useUdpStream) delTcpStreams();
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


void MeasurementDevice::askUser(bool flagCheckUserOnly)
{
    if (devicePtr->systManLocName) {
        scpiWrite("syst:man:loc:name?");
        waitingForReply = true;
        if (!flagCheckUserOnly)
            instrumentState = InstrumentState::CHECK_INSTR_USERNAME;
        else
            instrumentState = InstrumentState::CHECK_USER_ONLY;
        tcpTimeoutTimer->start(tcpTimeoutInMs);
    }
    else {
        if (!flagCheckUserOnly)
            checkUser("");
        else
            checkUserOnly("");
    }
}

void MeasurementDevice::checkUser(const QByteArray buffer)
{
    waitingForReply = false;
    QString msg = devicePtr->id + tr(" may be in use");
    if (!buffer.isEmpty()) {
        msg += tr(" by ") + QString(buffer).simplified();
        inUseBy = AsciiTranslator::toAscii(QString(buffer).simplified());
    }
    else {
        inUseBy.clear();
    }
    msg += tr(". Press connect once more to override");
    if (!firstConnection || buffer.contains(AsciiTranslator::toAscii(config->getStationName()).toLatin1())) {           // 130522: Rebuilt to reconnect upon startup if computer reboots
        //qDebug() << "In use by myself, how silly! Continuing..." << buffer << config->getStationName();
        deviceInUseWarningIssued = true;
        askUdp();
    }
    else {
        firstConnection = false; // now we know network is up
        tcpTimeoutTimer->stop();
        instrDisconnect();
        deviceInUseWarningIssued = true;
        emit popup(msg);
    }
    emit deviceBusy(inUseBy);
}

void MeasurementDevice::checkUserOnly(const QByteArray buffer)
{
    waitingForReply = false;
    if (!buffer.isEmpty()) {
        inUseBy = AsciiTranslator::toAscii(QString(buffer).simplified());
    }
    else {
        inUseBy.clear();
    }
    emit deviceBusy(inUseBy);
    instrumentState = InstrumentState::CONNECTED;
}

void MeasurementDevice::stateConnected()
{
    tcpTimeoutTimer->stop();

    if (!autoReconnectInProgress && !muteNotification) { // don't nag user if this is an auto reconnect call
        emit status("Connected, setting up device");
        emit toIncidentLog(NOTIFY::TYPE::MEASUREMENTDEVICE, devicePtr->id, "Connected to " + devicePtr->longId);
    }
    else if (autoReconnectInProgress) {
        emit toIncidentLog(NOTIFY::TYPE::MEASUREMENTDEVICE, devicePtr->id, "Reconnected");
        emit reconnected();
    }

    scpiConnected();
    autoReconnectInProgress = muteNotification = false;
    runAfterConnected();
    instrumentState = InstrumentState::CONNECTED;

    if (devicePtr->hasAntNames) askForAntennaNames();

    emit initiateDatastream();
}

void MeasurementDevice::tcpTimeout()
{
    if (instrumentState != InstrumentState::CONNECTED) {  // sth timed out
        if (firstConnection && config->getInstrConnectOnStartup()) {  // First time connecting and we are set to connect on startup, let's give it some time and retry
            emit status("TCP timeout, retrying in 15 seconds");
            timedReconnectTimer->start(15e3);
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
}

void MeasurementDevice::delTcpStreams()
{
    scpiWrite("trac:tcp:del all");
}

void MeasurementDevice::delOwnStream()
{
    if (scpiSocket->state() == QAbstractSocket::ConnectedState && (tcpStream->getTcpPort() > 0 || udpStream->getUdpPort() > 0)) {
        if (config->getInstrUseTcpDatastream())
            scpiWrite("trac:tcp:del \"" + scpiSocket->localAddress().toString().toLocal8Bit() + "\", " +
                      QByteArray::number(tcpStream->getTcpPort()));
        else
            scpiWrite("trac:udp:del \"" + scpiSocket->localAddress().toString().toLocal8Bit() + "\", " +
                      QByteArray::number(udpStream->getUdpPort()));
    }
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
    restartStream(false);
    setFftMode();
    scpiWrite("syst:if:rem:mode off"); // Don't send IF data unless specifically asked for!
    startDevice();
}

void MeasurementDevice::setUser()
{
    if (devicePtr->systManLocName) scpiWrite("syst:man:loc:name '" + AsciiTranslator::toAscii(config->getStationName()).toLatin1() + "(hauken)'");
    else if (devicePtr->memLab999) scpiWrite("MEM:LAB 999, '" + AsciiTranslator::toAscii(config->getStationName()).toLatin1() + "(hauken)'");
    else if (devicePtr->systKlocLab) scpiWrite("syst:kloc:lab '" + AsciiTranslator::toAscii(config->getStationName().toLocal8Bit()).toLatin1() + "(hauken)'");
}

void MeasurementDevice::startDevice()
{
    scpiWrite("init:imm");
    if (devicePtr->type == Instrument::InstrumentType::EM100 && devicePtr->mode == Instrument::Mode::FFM)
        scpiWrite("freq:mode cw");
}

void MeasurementDevice::restartStream(bool withDisconnect)
{
    if (connected) {
        delUdpStreams();
        delTcpStreams();
        delOwnStream();
        if (withDisconnect) {
            udpStream->closeListener();
            tcpStream->closeListener();
        }
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

    QByteArray modeStr = "cw, ifp, aud, if, psc";

    QByteArray gpsc;
    if (askForPosition) gpsc = ", gpsc";
    QByteArray em200Specific;
    if (devicePtr->advProtocol) em200Specific = ", 'ifpan'"; // em200/pr200 specific setting, swap system inverted since these models
    else em200Specific = ", 'swap'";

    scpiWrite("trac:tcp:tag:on \"" +
              scpiSocket->localAddress().toString().toLocal8Bit() + "\", " +
              QByteArray::number(tcpStream->getTcpPort()) + ", " + modeStr + gpsc);
    scpiWrite("trac:tcp:flag:on \"" +
              scpiSocket->localAddress().toString().toLocal8Bit() + "\", " +
              QByteArray::number(tcpStream->getTcpPort()) +
              ", 'volt:ac', 'opt'" + em200Specific);
    if (instrumentState == InstrumentState::CONNECTED) askTcp(); // To update the current user ip address 230908
    //askUser(true);
}

void MeasurementDevice::setupUdpStream()
{

    udpStream->setDeviceType(devicePtr);
    udpStream->openListener();

    QByteArray modeStr = "cw, ifp, aud, if, psc";

    QByteArray gpsc;
    if (askForPosition) gpsc = ", gpsc";
    QByteArray em200Specific;
    if (devicePtr->advProtocol) em200Specific = ", 'ifpan'"; // em200/pr200 specific setting, swap system inverted since these models
    else em200Specific = ", 'swap'";

    scpiWrite("trac:udp:tag:on \"" +
              scpiSocket->localAddress().toString().toLocal8Bit() +
              "\", " +
              QByteArray::number(udpStream->getUdpPort()) + ", " +
              modeStr + gpsc);
    scpiWrite("trac:udp:flag:on \"" +
              scpiSocket->localAddress().toString().toLocal8Bit() +
              "\", " +
              QByteArray::number(udpStream->getUdpPort()) +
              ", 'volt:ac', 'opt'" + em200Specific);
    if (instrumentState == InstrumentState::CONNECTED) askUdp(); // To update the current user ip address 230908
    //askUser(true);
}

/*void MeasurementDevice::forwardBytesPerSec(int val)
{
    emit bytesPerSec(val);
}

void MeasurementDevice::fftDataHandler(QVector<qint16> &data)
{
    emit newTrace(data);
}*/

void MeasurementDevice::handleStreamTimeout()
{
    //qDebug() << "Stream timeout triggered" << connected;

    if (connected) {
        tcpTimeoutTimer->stop();
        if (autoReconnect) { // check stream settings regularly to see if device is available again
            autoReconnectTimer->start(5000);
            if (!autoReconnectInProgress) {
                emit toIncidentLog(NOTIFY::TYPE::MEASUREMENTDEVICE, devicePtr->id, "Lost datastream. Auto reconnect enabled, waiting for device to be available again");
                emit deviceStreamTimeout();
                askUser(true);
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
    //qDebug() << "Auto reconnect check" << scpiReconnect << scpiSocket->isOpen();

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
    /*if ((devicePtr->pscanStartFrequency != config->getInstrStartFreq() * 1e6 ||
         devicePtr->pscanStopFrequency != config->getInstrStopFreq() * 1e6) && config->getInstrStopFreq() > config->getInstrStartFreq()) {
        devicePtr->pscanStartFrequency = config->getInstrStartFreq() * 1e6;
        devicePtr->pscanStopFrequency = config->getInstrStopFreq() * 1e6;
        setPscanFrequency();
    }*/
    if (devicePtr->pscanResolution != config->getInstrResolution().toDouble() * 1e3) {
        devicePtr->pscanResolution = config->getInstrResolution().toDouble() * 1e3;
        setPscanResolution();
    }
    /* if (devicePtr->ffmCenterFrequency != config->getInstrFfmCenterFreq() * 1e6) {
        devicePtr->ffmCenterFrequency = config->getInstrFfmCenterFreq() * 1e6;
        setFfmCenterFrequency();
    }*/
    if (devicePtr->ffmFrequencySpan != config->getInstrFfmSpan().toDouble() * 1e3) {
        devicePtr->ffmFrequencySpan = config->getInstrFfmSpan().toDouble() * 1e3;
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
    if (antPort != QByteArray::number(config->getInstrAntPort() + 1)) {
        antPort = QByteArray::number(config->getInstrAntPort() + 1);
        setAntPort();
    }

    if (config->getInstrMode().contains("pscan", Qt::CaseInsensitive) &&
        devicePtr->mode != Instrument::Mode::PSCAN) {
        devicePtr->mode = Instrument::Mode::PSCAN;
        //setMode();
        setPscanFrequency();
        setPscanResolution();
    }
    else if (config->getInstrMode().contains("ffm", Qt::CaseInsensitive) &&
             devicePtr->mode != Instrument::Mode::FFM) {
        devicePtr->mode = Instrument::Mode::FFM;
        //setMode();
        setFfmCenterFrequency();
        setFfmFrequencySpan();
    }

    else if (!fftMode.contains(config->getInstrFftMode().toLocal8Bit())) {
        fftMode = config->getInstrFftMode().toLocal8Bit();
        setFftMode();
    }
    //scpiAddress = new QHostAddress(config->getInstrIpAddr());
    scpiPort = config->getInstrPort();

    if (useUdpStream != !config->getInstrUseTcpDatastream())
        useUdpStream = !config->getInstrUseTcpDatastream();

    if ((config->getSdefAddPosition() && config->getSdefGpsSource().contains("Instrument")) || config->getGnssUseInstrumentGnss())  {// only ask device for position if it is needed
        if (!askForPosition) {
            askForPosition = true;
            restartStream(); // user changed this live, lets reconnect streams
        }
    }
    else {
        if (askForPosition) {
            askForPosition = false;
            restartStream(); // user changed this live, lets reconnect streams
        }
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
    if (connected && config->getSdefAddPosition() && !config->getGnssUseInstrumentGnss());// emit displayGnssData(out, 3, devicePtr->positionValid); // WHY?
    else if (connected && config->getGnssUseInstrumentGnss()) {
        GnssData data;
        data.id = 3; // 3 = instrumentGnss
        data.inUse = true;
        data.gnssType = devicePtr->id;
        data.posValid = devicePtr->positionValid;
        data.latitude = devicePtr->latitude;
        data.longitude = devicePtr->longitude;
        data.altitude = (float)devicePtr->altitude / 100.0;
        data.hdop = devicePtr->dop;
        data.timestamp = devicePtr->gnssTimestamp;
        data.satsTracked = devicePtr->sats;
        emit updGnssData(data);
    }
}

GnssData MeasurementDevice::sendGnssData()
{
    GnssData data;
    data.id = 3; // 3 = instrumentGnss
    data.inUse = true;
    data.gnssType = devicePtr->id;
    data.posValid = devicePtr->positionValid;
    data.latitude = devicePtr->latitude;
    data.longitude = devicePtr->longitude;
    data.cog = devicePtr->cog;
    data.sog = devicePtr->sog;
    data.altitude = (float)devicePtr->altitude / 100.0;
    data.hdop = devicePtr->dop;
    data.timestamp = devicePtr->gnssTimestamp;
    data.satsTracked = devicePtr->sats;
    return data;
}

void MeasurementDevice::askForAntennaNames()
{
    waitingForReply = true;
    if (instrumentState == InstrumentState::CHECK_ANT_NAME1) {
        scpiWrite("syst:ant:rx:name2?");
        instrumentState = InstrumentState::CHECK_ANT_NAME2;
    }
    else {
        scpiWrite("syst:ant:rx:name1?");
        instrumentState = InstrumentState::CHECK_ANT_NAME1;
    }
}

void MeasurementDevice::antennaNamesReply(QByteArray buffer)
{
    waitingForReply = false;
    if (instrumentState == InstrumentState::CHECK_ANT_NAME1) {
        devicePtr->antPorts[0] = buffer.simplified();
        devicePtr->antPorts[0].remove('\"');
        devicePtr->antPorts[0].remove('\'');
        askForAntennaNames();   // 1st name received, now ask for 2nd
    }
    else {
        instrumentState = InstrumentState::CONNECTED;   // all done, keep going

        devicePtr->antPorts[1] = buffer.simplified();
        devicePtr->antPorts[1].remove('\"');
        devicePtr->antPorts[1].remove('\'');
        //if (devicePtr->antPorts[0].isEmpty()) devicePtr->antPorts[0] = "Ant. 1";
        //if (devicePtr->antPorts[1].isEmpty()) devicePtr->antPorts[1] = "Ant. 2";
        if (devicePtr->type == InstrumentType::USRP) {
            devicePtr->antPorts[0].prepend("RX2_A:");
            devicePtr->antPorts[1].prepend("TRX_A:");
        }
        else {
            devicePtr->antPorts[0].prepend("Ant. 1:");
            devicePtr->antPorts[1].prepend("Ant. 2:");
        }
        emit newAntennaNames();
    }
}

void MeasurementDevice::updateAntennaName(const int index, const QString name)
{
    QString newName = name;
    newName.remove("Ant. 1:");
    newName.remove("Ant. 2:");
    newName.remove("RX2_A:");
    newName.remove("TRX_A:");
    newName = newName.simplified();

    if (index == 0) scpiWrite("syst:ant:rx:name1 '" + newName.toLatin1() + '\'');
    else if (index == 1) scpiWrite("syst:ant:rx:name2 '" + newName.toLatin1() + '\'');

    askForAntennaNames(); // update register
}

void MeasurementDevice::askFrequencies()
{
    if (inUseMode.contains("pscan") || devicePtr->mode == Instrument::Mode::PSCAN) {
        askPscanStartFreq();
    }
    else {
        askFfmFreq();
    }
}

void MeasurementDevice::checkPscanStartFreq(const QByteArray buffer)
{
    waitingForReply = false;
    quint64 s = buffer.simplified().toULongLong();
    if (s > 0 && s < 9e9) inUseStart = s;
    askPscanStopFreq();
}

void MeasurementDevice::checkPscanStopFreq(const QByteArray buffer)
{
    waitingForReply = false;
    quint64 s = buffer.simplified().toULongLong();
    if (s > 0 && s < 9e9) inUseStop = s;
    askPscanResolution();
}

void MeasurementDevice::checkPscanResolution(const QByteArray buffer)
{
    waitingForReply = false;
    quint32 s = buffer.simplified().toULong();
    if (s > 0 && s < 9e9) inUseRes = s;
    instrumentState = InstrumentState::CONNECTED; // Done
    if (inUseStart > 0 && inUseStop > 0 && inUseStop > inUseStart) {
        emit freqRangeUsed(inUseStart, inUseStop);
        emit resUsed(inUseRes);
    }
}

void MeasurementDevice::askPscanStartFreq()
{
    scpiWrite("freq:psc:start?");
    waitingForReply = true;
    instrumentState = InstrumentState::CHECK_PSCAN_START_FREQ;
}

void MeasurementDevice::askPscanStopFreq()
{
    scpiWrite("freq:psc:stop?");
    waitingForReply = true;
    instrumentState = InstrumentState::CHECK_PSCAN_STOP_FREQ;
}

void MeasurementDevice::askPscanResolution()
{
    scpiWrite("sens:psc:step?");
    waitingForReply = true;
    instrumentState = InstrumentState::CHECK_PSCAN_RESOLUTION;
}

void MeasurementDevice::askFfmFreq()
{
    scpiWrite("sens:freq?");
    waitingForReply = true;
    instrumentState = InstrumentState::CHECK_FFM_CENTERFREQ;
}

void MeasurementDevice::askFfmSpan()
{
    scpiWrite("sens:freq:span?");
    waitingForReply = true;
    instrumentState = InstrumentState::CHECK_FFM_SPAN;
}

void MeasurementDevice::checkFfmFreq(const QByteArray buffer)
{
    waitingForReply = false;
    quint64 s = buffer.simplified().toULongLong();
    if (s > 0 && s < 9e9) inUseStart = s;
    askFfmSpan();
}

void MeasurementDevice::checkFfmSpan(const QByteArray buffer)
{
    waitingForReply = false;
    quint32 s = buffer.simplified().toULong();
    if (s > 0 && s < 400e6) {
        inUseStart -= s / 2;
        inUseStop = inUseStart + s;
        emit freqRangeUsed(inUseStart, inUseStop);
        instrumentState = InstrumentState::CONNECTED;
    }
}

bool MeasurementDevice::isConnected()
{
    //return connected;
    if (instrumentState == InstrumentState::DISCONNECTED)
        return false;
    else
        return true;
}

void MeasurementDevice::handleNetworkError()
{
    qDebug() << "OBAbug detected, do sth smart here";
}

void MeasurementDevice::setVifFreqAndMode(const double frequency)
{
    if (devicePtr->mode == Instrument::Mode::PSCAN) {
        modeChanged = true;
        scpiWrite("freq:mode ffm");
    }
    scpiWrite("freq " + QByteArray::number((quint64)(frequency * 1e6)));
    scpiWrite("init");
}

/*void MeasurementDevice::setupIfStream()
{
    scpiWrite("trac:tcp:tag:on \"" +
              scpiSocket->localAddress().toString().toLocal8Bit() + "\", " +
              QByteArray::number(vifStreamTcp->getTcpPort()) + ", vif");
}*/

void MeasurementDevice::deleteIfStream()
{
    /*scpiWrite("trac:tcp:del \"" + scpiSocket->localAddress().toString().toLocal8Bit() + "\", " +
              QByteArray::number(vifStreamTcp->getTcpPort()));*/
    scpiWrite("syst:if:rem:mode off");
    setMeasurementTime(); // Set to prev. value
    if (centerFrequencies.size() > 0 && config->getIqRecordMultipleBands()) {
    }
    else if (modeChanged) {
        scpiWrite("freq:mode psc");
        scpiWrite("init");
    }
    else {
        setFfmCenterFrequency();
        setFfmFrequencySpan();
        emit skipNextNTraces(20); // Skip few FFM lines to not mix with old data ##FIXME!
    }
    if (useUdpStream) {
        muteNotification = true;
        restartStream(false);
    }
}

void MeasurementDevice::setupVifConnection()
{
/*    scpiWrite("trac:tcp:tag:on \"" +
              scpiSocket->localAddress().toString().toLocal8Bit() + "\", " +
              QByteArray::number(vifStreamTcp->getTcpPort()) + ", vif");*/
    scpiWrite("meas:time 100 ms"); // Slow down trace data transfer while I/Q transfer is running
    scpiWrite("dem:mode IQ");
    scpiWrite("band " + QByteArray::number((int)(config->getIqFftPlotBw() * 1e3)));
    scpiWrite("syst:if:rem:mode short");
}

void MeasurementDevice::setGainControl(int index)
{
    QByteArray mode;
    switch (index) {
    case 0:
        mode = "lown";
        break;
    case 1:
        mode = "norm";
        break;
    case 2:
        mode = "lowd";
        break;
    }

    scpiWrite("inp:att:mode " + mode);
}

double MeasurementDevice::retIqCenterFrequency()
{
    double f;
    if (devicePtr->mode == Instrument::Mode::PSCAN)
        f = devicePtr->pscanStartFrequency + (devicePtr->pscanStopFrequency - devicePtr->pscanStartFrequency) / 2;
    else
        f = devicePtr->ffmCenterFrequency;

    return f;
}

void MeasurementDevice::setAudioMode(int mode)
{
    scpiWrite("syst:aud:rem:mode " + QByteArray::number(mode));
}

void MeasurementDevice::setDemodType(QString type)
{
    scpiWrite("dem:mode " + type.toLocal8Bit());
}

void MeasurementDevice::setDemodBw(int bw)
{
    scpiWrite("band " + QByteArray::number((int)(bw * 1e3)));
}

void MeasurementDevice::setSquelch(bool sq)
{
    scpiWrite("outp:squ " + QByteArray::number(sq));
}

void MeasurementDevice::setSquelchLevel(int level)
{
    scpiWrite("outp:squ:thr " + QByteArray::number(level));
}

void MeasurementDevice::updGpsCompassData(GpsData &data)
{
    devicePtr->latitude = data.latitude;
    devicePtr->longitude = data.longitude;
    devicePtr->altitude = data.altitude;
    devicePtr->dop = data.dop;
    devicePtr->sog = data.sog;
    devicePtr->cog = data.cog;
    devicePtr->gnssTimestamp = data.timestamp;
    devicePtr->sats = data.sats;
    devicePtr->positionValid = data.valid;
}

void MeasurementDevice::setDetector(int i)
{
    QByteArray det;
    if (i == 0) det = "POS";
    else if (i == 1) det = "PAV";
    else if (i == 2) det = "FAST";
    else det = "RMS";
    scpiWrite("sens:det " + det);
}
