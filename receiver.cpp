#include "receiver.h"

Receiver::Receiver(QSharedPointer<Config> c)
{
    config = c;
    connect(tcpSocket, &QTcpSocket::stateChanged, this, &Receiver::handleTcpSocketStateChanged);
    connect(tcpSocket, &QTcpSocket::readyRead, this, &Receiver::handleTcpSocketReadData);
    connect(tcpSocketTimeoutTimer, &QTimer::timeout, this, &Receiver::handleTcpSocketTimeout);

    tcpSocketWriteThrottleElapsedTimer->start();
}


void Receiver::connectInstrument()
{
    if (receiverAddress.isEmpty() || receiverPort == 0) {
        emit warning("Missing device IP address or port");
    }
    else {
        tcpSocket->connectToHost(receiverAddress, receiverPort);
        tcpSocketTimeoutTimer->start(tcpSocketTimeout);
    }

}

void Receiver::disconnectInstrument()
{

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
        receiverInitiate();
    }
}

void Receiver::handleTcpSocketReadData()
{
    QByteArray buffer = tcpSocket->readAll();
    qDebug() << "<<" << buffer;
}

void Receiver::handleTcpSocketTimeout()
{

}

void Receiver::receiverInitiate()
{
    if (receiverState == ReceiverState::Connected) { // No point in setting up a disconnected instrument
        reqReceiverId();
    }
    else {
        qDebug() << "Asked to initiate a disconnected receiver, giving up";
        receiverInitiateOrder = ReceiverInitiateOrder::None;
    }
}

void Receiver::reqReceiverId()
{

}

void Receiver::writeToReceiver(QByteArray data)
{
    int throttle = scpiThrottleTime;
    if (devicePtr->id.contains("esmb", Qt::CaseInsensitive)) throttle *= 3; // esmb hack, slow interface

    if (tcpSocketWriteThrottleElapsedTimer->isValid()) {
        if (tcpSocketWriteThrottleElapsedTimer->elapsed() < throttle) { // min. x ms time between scpi commands, to give device time to breathe
            QTimer::singleShot(throttle, this, &Receiver::writeToReceiver(data)); // call yourself after a little breather
        }
        else {
            tcpSocket->write(data + '\n');
        }
    }
    qDebug() << ">>" << data;
}
