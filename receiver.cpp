#include "receiver.h"

Receiver::Receiver(QSharedPointer<Config> c)
{
    config = c;
    connect(tcpSocket, &QTcpSocket::stateChanged, this, &Receiver::handleTcpSocketStateChanged);
    connect(tcpSocket, &QTcpSocket::readyRead, this, &Receiver::handleTcpSocketReadData);
    connect(tcpSocketTimeoutTimer, &QTimer::timeout, this, &Receiver::handleTcpSocketTimeout);
    connect(scpiAwaitingReplyTimer, &QTimer::timeout, this, &Receiver::handleScpiReplyTimeout);

    tcpSocketWriteThrottleElapsedTimer->start();
}


void Receiver::connectReceiver()
{
    if (receiverAddress.isEmpty() || receiverPort == 0) {
        emit warning("Missing device IP address or port");
    }
    else {
        tcpSocket->connectToHost(receiverAddress, receiverPort);
        tcpSocketTimeoutTimer->start(tcpSocketTimeout);
    }

}

void Receiver::disconnectReceiver()
{
    // Tidy up after disconnect here
}

void Receiver::handleTcpSocketStateChanged(QAbstractSocket::SocketState state)
{
    if (state == QAbstractSocket::ConnectingState) {
        receiverState = ReceiverState::Connecting;
    }
    else if (state == QAbstractSocket::ConnectedState) {
        if (receiverState != ReceiverState::Reconnecting) {
            emit receiverConnected();
        }
        receiverState = ReceiverState::Connected;
        initiateReceiver();
    }
    else if (state == QAbstractSocket::ClosingState) {
        qDebug() << "Receiver TCP socket closing";
        receiverState = ReceiverState::Disconnected;
        disconnectReceiver();
        emit receiverDisconnected();
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
}

void Receiver::handleTcpSocketTimeout()
{

}

void Receiver::initiateReceiver()
{
    if (receiverState == ReceiverState::Connected) { // No point in setting up a disconnected instrument
        if (receiverInitiateOrder == ReceiverInitiateOrder::None)
            reqReceiverId();
        else if (receiverInitiateOrder == ReceiverInitiateOrder::ReceivedId && device.udpStream)
            reqReceiverUdpState();
        else if (receiverInitiateOrder == ReceiverInitiateOrder::ReceivedUdpState && device.tcpStream)
            reqReceiverTcpState();
        else if (receiverInitiateOrder == ReceiverInitiateOrder::ReceivedTcpState) {
            receiverInitiateOrder = ReceiverInitiateOrder::Ready;
        }
    }
    else {
        qDebug() << "Asked to initiate a disconnected receiver, giving up";
        receiverInitiateOrder = ReceiverInitiateOrder::None;
    }
}

void Receiver::writeToReceiver(QByteArray data, bool awaitReply)
{
    int throttle = scpiThrottleTime;
    if (device.id.contains("esmb", Qt::CaseInsensitive)) throttle *= 3; // esmb hack, slow interface

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
    device.clearSettings();

    if (buffer.contains("EB500"))
        device.setType(InstrumentType::EB500);
    else if (buffer.contains("PR100"))
        device.setType(InstrumentType::PR100);
    else if (buffer.contains("PR200"))
        device.setType(InstrumentType::PR200);
    else if (buffer.contains("EM100"))
        device.setType(InstrumentType::EM100);
    else if (buffer.contains("EM200"))
        device.setType(InstrumentType::EM200);
    else if (buffer.contains("ESMB"))
        device.setType(InstrumentType::ESMB);
    else if (buffer.contains("E310"))
        device.setType(InstrumentType::E310);
    else if (buffer.contains("ESMW"))
        device.setType(InstrumentType::ESMW);

    else device.setType(InstrumentType::UNKNOWN);

    if (device.type != InstrumentType::UNKNOWN) {
        QString msg = "Measurement receiver type " + device.id + " found, checking if available";
        emit status(msg);
        device.longId = tr(buffer.simplified());

        emit receiverName(device.longId);
        receiverInitiateOrder = ReceiverInitiateOrder::ReceivedId;
        initiateReceiver(); // Continue receiver init process
    }
    else {
        QString msg = "Unknown instrument ID: " + tr(buffer);
        emit warning(msg);
        disconnectReceiver();
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
            if (list.contains(ownIp.toString().toLocal8Bit())) // FIXME: Check for same stream port!!
                break; // we are the users, continue
            if (brokenList.size() > 2 && !list.contains("DEF")) {
                inUse = true;
            }
        }
    }
    receiverInitiateOrder = ReceiverInitiateOrder::ReceivedUdpState;
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
        if (list.contains(tcpOwnAdress) && list.contains(tcpOwnPort))
            break; // we are the users, continue
        if (brokenList.size() > 2 && !list.contains("DEF")) {
            inUse = true;
        }
    }
    receiverInitiateOrder = ReceiverInitiateOrder::ReceivedTcpState;
    initiateReceiver(); // Continue receiver init process
}

void Receiver::setupUdpStream()
{
    udpStream->setDeviceType(device);
    udpStream->openListener();
    QByteArray modeStr;
    if (devicePtr->mode == Mode::PSCAN && !devicePtr->optHeaderDscan) modeStr = "pscan";
    else if (devicePtr->mode == Mode::PSCAN && devicePtr->optHeaderDscan) modeStr = "dscan";
    else if (devicePtr->mode == Mode::FFM) modeStr = "ifpan";

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
    if (instrumentState == InstrumentState::CONNECTED) askUdp(); // To update the current user ip address 230908
    //askUser(true);
}


void Receiver::setupTcpStream()
{
    tcpStream->setDeviceType(devicePtr);
    tcpStream->openListener(*scpiAddress, scpiPort + 10);
    QByteArray modeStr;
    if (devicePtr->mode == Mode::PSCAN && !devicePtr->optHeaderDscan) modeStr = "pscan";
    else if (devicePtr->mode == Mode::PSCAN && devicePtr->optHeaderDscan) modeStr = "dscan";
    else if (devicePtr->mode == Mode::FFM) modeStr = "ifpan";

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
    if (instrumentState == InstrumentState::CONNECTED) askTcp(); // To update the current user ip address 230908
    //askUser(true);
}
