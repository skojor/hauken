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
#include <QProcess>

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
    void updSettings();

    void watchdogOn();
    void watchdogOff();
    bool isWatchdogActive() { return stateWatchdog; }

private slots:
    void handleBuffer();
    void relayBtnOnPressed();
    void relayBtnOffPressed();
    void resetWatchdog();
    void ping();
    void pong(int exitCode, QProcess::ExitStatus exitStatus);

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
    QLabel *watchdogText = new QLabel;
    QPushButton *btnWatchdogOn = new QPushButton("Activate watchdog");
    QPushButton *btnWatchdogOff = new QPushButton("Deactivate watchdog");
    QLabel *tempLabel = new QLabel("Temperature");
    QLabel *humLabel = new QLabel("Humidity");
    QLabel *pingStateText = new QLabel;

    bool tempRelayActive = false, dht20Active = false, watchdogRelayActive = false;
    int secondsLeft;

    bool stateWatchdog = false;
    QTimer *watchdogTimer = new QTimer;
    QTimer *pingTimer = new QTimer;
    QProcess *pingProcess = new QProcess;
    bool pingActivated = false, lastPingValid = false;

    // Config cache
    QString pingAddress;
    int pingInterval;
};

#endif // ARDUINO_H
