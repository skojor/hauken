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

    start();
}

void Arduino::start()
{
    if (getArduinoEnable()) connectToPort();

    wdg->setAttribute(Qt::WA_QuitOnClose);
    wdg->setWindowFlag(Qt::WindowStaysOnTopHint);

    relayOnBtn->setText(getArduinoRelayOnText());
    relayOffBtn->setText(getArduinoRelayOffText());

    tempRelayActive = getArduinoReadTemperatureAndRelay();
    dht20Active = getArduinoReadDHT20();

    connect(relayOnBtn, &QPushButton::clicked, this, &Arduino::relayBtnOnPressed);
    connect(relayOffBtn, &QPushButton::clicked, this, &Arduino::relayBtnOffPressed);

    wdg->resize(250, 150);
    wdg->setWindowTitle("Arduino interface");
    QGridLayout *mainLayout = new QGridLayout;
    mainLayout->addWidget(new QLabel("Temperature"), 0, 0);
    mainLayout->addWidget(tempBox, 0, 1);
    if (dht20Active) {
        mainLayout->addWidget(new QLabel("Relative humidity"), 1, 0);
        mainLayout->addWidget(humidityBox, 1, 1);
    }
    if (tempRelayActive) {
        mainLayout->addWidget(new QLabel("Relay state"), 2, 0);
        mainLayout->addWidget(relayStateText, 2, 1);
        mainLayout->addWidget(relayOnBtn, 3, 0);
        mainLayout->addWidget(relayOffBtn, 3, 1);
    }
    wdg->setLayout(mainLayout);

    wdg->setMinimumSize(200, 1);
    //qDebug() << dialog->sizeHint();
    wdg->adjustSize();

    wdg->restoreGeometry(getArduinoWindowState());
    if (arduino->isOpen()) {
        wdg->show();
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
        if (arduino->open(QIODevice::ReadWrite))
            qDebug() << "Arduino: connected to" << arduino->portName();
        else
            qDebug() << "Arduino: cannot open" << arduino->portName();
    }
}

void Arduino::handleBuffer()
{
    if (buffer.contains('\n')) {
        QList<QByteArray> list = buffer.split(',');
        if (list.size() > 1 && !list[0].contains("dht20")) {
            bool ok = false;
            float tmp = list.at(0).toFloat(&ok);
            if (ok) temperature = tmp;
            if (list.at(1).contains("on")) relayState = true;
            else relayState = false;
            tempBox->setText(QString::number(temperature) + " °C");
            if (relayState) relayStateText->setText(getArduinoRelayOnText());
            else relayStateText->setText(getArduinoRelayOffText());
        }
        else if (list.size() == 5 && list[0].contains("dht20")) {
            bool ok = false;
            float tmp = list.at(2).toFloat(&ok);
            if (ok) temperature = tmp;
            tmp = list.at(4).toFloat(&ok);
            if (ok) humidity = tmp;
            tempBox->setText(QString::number(temperature) + " °C");
            humidityBox->setText(QString::number(humidity) + " %");
        }
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
