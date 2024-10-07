#include "receiver.h"

Receiver::Receiver(QSharedPointer<Config> c)
{
    config = c;
    connect(tcpSocket, &QTcpSocket::stateChanged, this, &Receiver::handleTcpSocketStateChanged);
    connect(tcpSocket, &QTcpSocket::readyRead, this, &Receiver::handleTcpSocketReadData);
    connect(tcpSocketTimeoutTimer, &QTimer::timeout, this, &Receiver::handleTcpSocketTimeout);
    connect(scpiAwaitingReplyTimer, &QTimer::timeout, this, &Receiver::handleScpiReplyTimeout);

    tcpSocketWriteThrottleElapsedTimer->start();
    device = QSharedPointer<Device>(new Device);
}


void Receiver::connectReceiver()
{
    if (receiverAddress.isEmpty() || receiverPort == 0) {
        emit popup("Missing device IP address or port");
    }
    else {
        tcpSocket->connectToHost(receiverAddress, receiverPort);
        tcpSocketTimeoutTimer->start(tcpSocketTimeout);
    }
}

void Receiver::disconnectReceiver()
{
    if (tcpSocket->isOpen() && receiverInitiateOrder != ReceiverInitiateOrder::InUseWarningSent)
        deleteOwnStream();

    tcpSocketTimeoutTimer->stop();
    scpiAwaitingReplyTimer->stop();
    tcpStream->closeListener();
    udpStream->closeListener();

    QTimer::singleShot(10, this, &Receiver::disconnectTcpSocket); // Delay disconnect until last commands has been sent
    emit status(tr("Measurement receiver disconnected"));
}

void Receiver::handleTcpSocketStateChanged(QAbstractSocket::SocketState state)
{
    qDebug() << "Receiver TCP state" << state;

    if (state == QAbstractSocket::ConnectingState) {
        receiverState = ReceiverState::Connecting;
        receiverInitiateOrder = ReceiverInitiateOrder::None;
    }
    else if (state == QAbstractSocket::ConnectedState) {
        if (receiverState != ReceiverState::Reconnecting) {
            emit receiverStateChanged(true);
        }
        receiverState = ReceiverState::Connected;
        tcpSocketTimeoutTimer->stop(); // we are connected, stop worrying
        initiateReceiver();
    }
    else if (state == QAbstractSocket::ClosingState) {
        qDebug() << "Receiver TCP socket closing";
        receiverState = ReceiverState::Disconnected;
        receiverInitiateOrder = ReceiverInitiateOrder::None;
        disconnectReceiver();
        emit receiverStateChanged(false);
    }
}

void Receiver::handleTcpSocketReadData()
{
    scpiAwaitingReplyTimer->stop();

    QByteArray buffer = tcpSocket->readAll();
    qDebug() << "<<" << buffer;
    if (receiverInitiateOrder == ReceiverInitiateOrder::None) {
        qDebug() << "Received data on receiver TCP socket without asking for anything, how sweet!" << buffer;
    }
    else if (receiverInitiateOrder == ReceiverInitiateOrder::WaitForReceiverId)
        parseReceiverId(buffer);
    else if (receiverInitiateOrder == ReceiverInitiateOrder::WaitForUdpState)
        parseReceiverUdpState(buffer);
    else if (receiverInitiateOrder == ReceiverInitiateOrder::WaitForTcpState)
        parseReceiverTcpState(buffer);
    else if (receiverInitiateOrder == ReceiverInitiateOrder::WaitForUser)
        parseUser(buffer);
    else if (receiverInitiateOrder == ReceiverInitiateOrder::WaitForSocketInfo)
        parseSocketInfo(buffer);
}

void Receiver::handleTcpSocketTimeout()
{
    qDebug() << "TCP socket operation timed out, disconnecting";
    disconnectReceiver();
}

void Receiver::initiateReceiver()
{
    if (receiverState == ReceiverState::Connected) { // No point in setting up a disconnected instrument
        if (receiverInitiateOrder == ReceiverInitiateOrder::None)
            reqReceiverId();
        else if (receiverInitiateOrder == ReceiverInitiateOrder::ReceivedId && device->udpStream)
            reqReceiverUdpState();
        else if (receiverInitiateOrder == ReceiverInitiateOrder::ReceivedUdpState && device->tcpStream)
            reqReceiverTcpState();
        else if (receiverInitiateOrder == ReceiverInitiateOrder::ReqUser)
            reqUser();
        else if (receiverInitiateOrder == ReceiverInitiateOrder::ReceivedUser)
            issueWarning();
        else if (receiverInitiateOrder == ReceiverInitiateOrder::InUseWarningSent)
            disconnectReceiver();
        else if (receiverInitiateOrder == ReceiverInitiateOrder::UdpStreamConnected)
            setupUdpStream();
        else if (receiverInitiateOrder == ReceiverInitiateOrder::TcpStreamConnected)
            setupTcpStream();
        else if (receiverInitiateOrder == ReceiverInitiateOrder::ReceivedTcpState ||
                 (!device->tcpStream && receiverInitiateOrder == ReceiverInitiateOrder::ReceivedUdpState ||
                  receiverInitiateOrder == ReceiverInitiateOrder::InUseByMyself)) {
            if (device->tcpStream && config->getInstrUseTcpDatastream())
                connectTcpStream();
            else
                connectUdpStream();
        }
        else if (receiverInitiateOrder == ReceiverInitiateOrder::StreamConfigured) {
            receiverInitiateOrder = ReceiverInitiateOrder::Idle; // We are done (?)
            emit toIncidentLog(NOTIFY::TYPE::MEASUREMENTDEVICE, device->id, "Connected to " + device->longId);
        }
    }

    else {
        qDebug() << "Asked to initiate a disconnected receiver, giving up";
        receiverInitiateOrder = ReceiverInitiateOrder::None;
    }
}

void Receiver::handleStreamTimeout()
{
    qDebug() << "Stream timeout triggered";
}

void Receiver::writeToReceiver(QByteArray data, bool awaitReply)
{
    int throttle = scpiThrottleTime;
    if (device->id.contains("esmb", Qt::CaseInsensitive)) throttle *= 3; // esmb hack, slow interface

    if (tcpSocketWriteThrottleElapsedTimer->isValid()) {
        while (tcpSocketWriteThrottleElapsedTimer->elapsed() < throttle) { // min. x ms time between scpi commands, to give device time to breathe
            QCoreApplication::processEvents();
        }
        while (scpiAwaitingReplyTimer->isActive()) { // still waiting for device to reply
            QCoreApplication::processEvents();
        }
        tcpSocket->write(data + '\n');
        if (awaitReply) scpiAwaitingReplyTimer->start(scpiAwaitingReplyTimeout);
    }
    qDebug() << ">>" << data << awaitReply;
}

void Receiver::handleScpiReplyTimeout()
{
    qDebug() << "Timed out waiting for receiver reply, current waiting state" << (int)receiverInitiateOrder;
    receiverInitiateOrder = ReceiverInitiateOrder::None;
    scpiAwaitingReplyTimer->stop();
}

void Receiver::reqReceiverId()
{
    writeToReceiver("*idn?", true); // Ask for device id
    receiverInitiateOrder = ReceiverInitiateOrder::WaitForReceiverId;
}

void Receiver::parseReceiverId(QByteArray buffer)
{
    scpiAwaitingReplyTimer->stop();
    device->clearSettings();

    if (buffer.contains("EB500"))
        device->setType(InstrumentType::EB500);
    else if (buffer.contains("PR100"))
        device->setType(InstrumentType::PR100);
    else if (buffer.contains("PR200"))
        device->setType(InstrumentType::PR200);
    else if (buffer.contains("EM100"))
        device->setType(InstrumentType::EM100);
    else if (buffer.contains("EM200"))
        device->setType(InstrumentType::EM200);
    else if (buffer.contains("ESMB"))
        device->setType(InstrumentType::ESMB);
    else if (buffer.contains("E310"))
        device->setType(InstrumentType::E310);
    else if (buffer.contains("ESMW"))
        device->setType(InstrumentType::ESMW);

    else device->setType(InstrumentType::UNKNOWN);

    if (device->type != InstrumentType::UNKNOWN) {
        QString msg = "Measurement receiver type " + device->id + " found, checking if available";
        device->longId = tr(buffer.simplified());
        emit status(msg);
        emit receiverName(device->longId);
        device->longId = tr(buffer.simplified());

        emit receiverName(device->longId);
        receiverInitiateOrder = ReceiverInitiateOrder::ReceivedId;
        initiateReceiver(); // Continue receiver init process
    }
    else {
        QString msg = "Unknown instrument ID: " + tr(buffer);
        disconnectReceiver();
        emit popup(msg);
    }
}

void Receiver::reqReceiverUdpState()
{
    writeToReceiver("trac:udp?", true);
    receiverInitiateOrder = ReceiverInitiateOrder::WaitForUdpState;
}

void Receiver::parseReceiverUdpState(QByteArray buffer)
{
    QList<QByteArray> datastreamList = buffer.split('\n');
    bool inUse = false;
    for (auto&& list : datastreamList) {
        QList<QByteArray> brokenList = list.split(',');
        for (auto&& ownIp : myOwnAddresses) {
            if (list.contains(ownIp.toString().toLocal8Bit()) && list.contains(QByteArray::number(udpStream->getUdpPort())))
                break; // we are the users, continue
            if (brokenList.size() > 2 && !list.contains("DEF")) {
                inUse = true;
            }
        }
    }
    if (!inUse)
        receiverInitiateOrder = ReceiverInitiateOrder::ReceivedUdpState;
    else
        receiverInitiateOrder = ReceiverInitiateOrder::ReqUser;
    initiateReceiver(); // Continue receiver init process
}

void Receiver::reqReceiverTcpState()
{
    writeToReceiver("trac:tcp?", true);
    receiverInitiateOrder = ReceiverInitiateOrder::WaitForTcpState;

}

void Receiver::parseReceiverTcpState(QByteArray buffer)
{
    QList<QByteArray> datastreamList = buffer.split('\n');
    bool inUse = false;
    QString inUseByIp;
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
        if (list.contains(tcpOwnAdress) && list.contains(QByteArray::number(tcpStream->getTcpPort())))
            break; // we are the users, continue
        if (brokenList.size() > 2 && !list.contains("DEF")) {
            inUse = true;
        }
    }
    if (!inUse)
        receiverInitiateOrder = ReceiverInitiateOrder::ReceivedTcpState;
    else
        receiverInitiateOrder = ReceiverInitiateOrder::ReqUser;
    initiateReceiver(); // Continue receiver init process
}

void Receiver::reqUser()
{
    if (device->systManLocName) {
        writeToReceiver("syst:man:loc:name?", true);
        receiverInitiateOrder = ReceiverInitiateOrder::WaitForUser;
    }
    else
        parseUser("unknown user"); // Skip username process if device is not capable
}

void Receiver::parseUser(QByteArray buffer)
{
    if (!buffer.isEmpty())
        receiverUser = QString(buffer).simplified();
    else
        receiverUser.clear();
    receiverInitiateOrder = ReceiverInitiateOrder::ReceivedUser;
    initiateReceiver();
}

void Receiver::issueWarning()
{
    if (!flagWarningIssued &&
        !receiverUser.contains(config->getStationName().toLocal8Bit())) {
        QString msg = device->id + tr(" may be in use");
        if (!receiverUser.isEmpty())
            msg += tr(" by ") + receiverUser;

        msg += tr(". Press connect once more to override");
        emit receiverBusy(receiverUser);
        emit popup(msg);
        receiverInitiateOrder = ReceiverInitiateOrder::InUseWarningSent;
        flagWarningIssued = true;
    }
    else {
        receiverInitiateOrder = ReceiverInitiateOrder::InUseByMyself;
    }
    initiateReceiver();
}

void Receiver::reqSocketInfo() // TODO: Why on earth are we doing this??
{
    writeToReceiver("trac:tcp:sock?", true);
    receiverInitiateOrder = ReceiverInitiateOrder::WaitForSocketInfo;
}

void Receiver::parseSocketInfo(QByteArray buffer) // TODO: Why on earth are we doing this??
{
    // ssh tunnel hackaround
    tcpOwnAdress = tcpSocket->localAddress().toString().toLocal8Bit();
    tcpOwnPort = QByteArray::number(tcpStream->getTcpPort());

    QList<QByteArray> split = buffer.split(',');
    if (split.size() > 1) {
        tcpOwnAdress = split.at(0).simplified();
        tcpOwnPort = split.at(1).simplified();
    }
    receiverInitiateOrder = ReceiverInitiateOrder::ReceivedSocketInfo;

    //tcpOwnPort = tcpStream->getTcpPort();
    initiateReceiver();
}

void Receiver::connectUdpStream()
{
    receiverInitiateOrder = ReceiverInitiateOrder::WaitForUdpStreamConnected;
    udpStream->setDeviceType(device);
    udpStream->openListener();
}

void Receiver::setupUdpStream()
{
    QByteArray modeStr;
    if (device->mode == Mode::PSCAN && !device->optHeaderDscan) modeStr = "pscan";
    else if (device->mode == Mode::PSCAN && device->optHeaderDscan) modeStr = "dscan";
    else {
        modeStr = "ifpan";
        device->mode = Mode::FFM;
    }

    QByteArray gpsc;
    if ((config->getSdefAddPosition() && config->getSdefGpsSource().contains("Instrument")) || config->getGnssUseInstrumentGnss()) gpsc = ", gpsc";
    QByteArray em200Specific;
    if (device->advProtocol) em200Specific = ", 'ifpan', 'swap'"; // em200/pr200 specific setting, swap system inverted since these models

    writeToReceiver("trac:udp:tag:on \"" +
                    tcpSocket->localAddress().toString().toLocal8Bit() + "\", " +
                    QByteArray::number(udpStream->getUdpPort()) + ", " + modeStr + gpsc);
    writeToReceiver("trac:udp:flag:on '" +
                    tcpSocket->localAddress().toString().toLocal8Bit() + "', " +
                    QByteArray::number(udpStream->getUdpPort()) + ", 'volt:ac', 'opt'" + em200Specific);
    receiverInitiateOrder = ReceiverInitiateOrder::StreamConfigured;
    initiateReceiver();
}

void Receiver::connectTcpStream()
{
    receiverInitiateOrder = ReceiverInitiateOrder::WaitForTcpStreamConnected;
    tcpStream->setDeviceType(device);
    QHostAddress address(config->getInstrIpAddr());
    tcpStream->openListener(address, tcpSocket->peerPort() + 10);
}

void Receiver::setupTcpStream()
{
    QByteArray modeStr;
    if (device->mode == Mode::PSCAN && !device->optHeaderDscan) modeStr = "pscan";
    else if (device->mode == Mode::PSCAN && device->optHeaderDscan) modeStr = "dscan";
    else {
        modeStr = "ifpan";
        device->mode = Mode::FFM;
    }

    QByteArray gpsc;
    if ((config->getSdefAddPosition() && config->getSdefGpsSource().contains("Instrument")) || config->getGnssUseInstrumentGnss()) gpsc = ", gpsc";
    QByteArray em200Specific;
    if (device->advProtocol) em200Specific = ", 'ifpan', 'swap'"; // em200/pr200 specific setting, swap system inverted since these models

    writeToReceiver("trac:tcp:tag:on \"" +
                    tcpSocket->localAddress().toString().toLocal8Bit() + "\", " +
                    QByteArray::number(tcpStream->getTcpPort()) + ", " + modeStr + gpsc);
    writeToReceiver("trac:tcp:flag:on \"" +
                    tcpSocket->localAddress().toString().toLocal8Bit() + "\", " +
                    QByteArray::number(tcpStream->getTcpPort()) + ", 'volt:ac', 'opt'" + em200Specific);
    receiverInitiateOrder = ReceiverInitiateOrder::StreamConfigured;
    initiateReceiver();
}

void Receiver::deleteUdpStreams()
{
    writeToReceiver("trac:udp:del all");
}

void Receiver::deleteTcpStreams()
{
    writeToReceiver("trac:tcp:del all");
}

void Receiver::deleteOwnStream()
{
    if (config->getInstrUseTcpDatastream())
        writeToReceiver("trac:tcp:del \"" + tcpSocket->localAddress().toString().toLocal8Bit() + "\", " +
                        QByteArray::number(tcpStream->getTcpPort()));
    else
        writeToReceiver("trac:udp:del \"" + tcpSocket->localAddress().toString().toLocal8Bit() + "\", " +
                        QByteArray::number(udpStream->getUdpPort()));
}

void Receiver::setPscanStartFrequency(double freq)
{
    abor();
    if (device->hasPscan && device->mode == Instrument::Mode::PSCAN)
        writeToReceiver("freq:psc:start " + QByteArray::number(freq) + "M");
    else if (device->hasDscan && device->mode == Instrument::Mode::PSCAN) // esmb mode
        writeToReceiver("freq:dsc:start " + QByteArray::number(freq) + "M");
    initImm();
    emit resetBuffers();
}

void Receiver::setPscanStopFrequency(double freq)
{
    abor();
    if (device->hasPscan && device->mode == Instrument::Mode::PSCAN)
        writeToReceiver("freq:psc:stop " + QByteArray::number(freq) + "M");
    else if (device->hasDscan && device->mode == Instrument::Mode::PSCAN) // esmb mode
        writeToReceiver("freq:dsc:stop " + QByteArray::number(freq) + "M");
    initImm();
    emit resetBuffers();
}

void Receiver::setMode(Instrument::Mode mode) {
    if (mode == Instrument::Mode::PSCAN) {
        if (device->hasPscan) {
            writeToReceiver("freq:mode psc");
        }
        else if (device->hasDscan) {
            writeToReceiver("freq:mode dsc");
        }
        emit modeUsed("pscan");
    }
    else {
        writeToReceiver("freq:mode ffm");
        emit modeUsed("ffm");
    }

}
