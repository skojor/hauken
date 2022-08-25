#include "arduino.h"

Arduino::Arduino(QObject *parent)
{
    (void)parent;

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

    QWidget *wdg = new QWidget;

    relayOnBtn->setText(getArduinoRelayOnText());
    relayOffBtn->setText(getArduinoRelayOffText());

    connect(relayOnBtn, &QPushButton::clicked, this, &Arduino::relayBtnOnPressed);
    connect(relayOffBtn, &QPushButton::clicked, this, &Arduino::relayBtnOffPressed);

    wdg->resize(250, 150);
    wdg->setWindowTitle("Arduino interface");
    QGridLayout *mainLayout = new QGridLayout;
    mainLayout->addWidget(new QLabel("Temperature"), 0, 0);
    mainLayout->addWidget(tempBox, 0, 1);
    mainLayout->addWidget(new QLabel("Relay state"), 1, 0);
    mainLayout->addWidget(relayStateText, 1, 1);
    mainLayout->addWidget(relayOnBtn, 2, 0);
    mainLayout->addWidget(relayOffBtn, 2, 1);

    wdg->setLayout(mainLayout);

    if (arduino->isOpen()) {
        wdg->show();
    }

    tempBox->setEnabled(getArduinoReadTemperature());
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
        if (list.size() > 1) {
            bool ok = false;
            float tmp = list.at(0).toFloat(&ok);
            if (ok) temperature = tmp;
            if (list.at(1).contains("on")) relayState = true;
            else relayState = false;
        }
        buffer.clear();
    }
    if (relayState) relayStateText->setText(getArduinoRelayOnText());
    else relayStateText->setText(getArduinoRelayOffText());

    tempBox->setText(QString::number(temperature) + " °C");
}

void Arduino::relayBtnOnPressed()
{
    arduino->write("1");
}

void Arduino::relayBtnOffPressed()
{
    arduino->write("0");
}
