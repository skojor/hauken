#ifndef ARDUINO_H
#define ARDUINO_H

#include <QWidget>
#include <QSerialPort>
#include <QSerialPortInfo>
#include <QDebug>
#include <QDateTime>
#include <QTimer>
#include <QElapsedTimer>
#include <QTextStream>
#include <QFile>
#include <QDir>
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QLineEdit>
#include <QCheckBox>
#include <QLabel>
#include <QDialogButtonBox>
#include <QPushButton>
#include "config.h"
#include "typedefs.h"

/*
 *  Class connecting to an Arduino via serial port.
 *
 *  For now controlling a relay, and reading temperature periodically
 */


class Arduino : public Config
{
public:
    explicit Arduino(QObject *parent = nullptr);
    void connectToPort();
    void start();
    void close() {
        qDebug() << "Closing...";
        setArduinoWindowState(wdg->saveGeometry());
        wdg->close();
    }

private slots:
    void handleBuffer();
    void relayBtnOnPressed();
    void relayBtnOffPressed();

private:
    QWidget *wdg = new QWidget;
    QSerialPort *arduino = new QSerialPort;
    QByteArray buffer;
    float temperature = 0, humidity = 0;
    bool relayState = false;
    QFormLayout *mainLayout = new QFormLayout;
    QLabel *tempBox = new QLabel;
    QLabel *humidityBox = new QLabel;
    QLabel *relayStateText = new QLabel; //(getArduinoRelayOffText());
    QPushButton *relayOnBtn = new QPushButton;
    QPushButton *relayOffBtn = new QPushButton;

    bool tempRelayActive = false, dht20Active = false;
};

#endif // ARDUINO_H
