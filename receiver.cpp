#include "receiver.h"

Receiver::Receiver(QSharedPointer<Config> c)
{
    config = c;
    connect(tcpSocket, &QTcpSocket::stateChanged, this, &Receiver::handleTcpSocketStateChanged);
    connect(tcpSocket, &QTcpSocket::readyRead, this, &Receiver::handleTcpSocketReadData);
    connect(tcpSocketTimeoutTimer, &QTimer::timeout, this, &Receiver::handleTcpSocketTimeout);
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

}

void Receiver::handleTcpSocketTimeout()
{

}

void Receiver::receiverInitiate()
{
    if (receiverState == ReceiverState::Connected) { // No point in setting up a disconnected instrument

    }
    else {
        qDebug() << "Asked to initiate a disconnected receiver, giving up";
        receiverInitiateOrder = ReceiverInitiateOrder::None;
    }
}

void Receiver::reqReceiverId()
{

}
