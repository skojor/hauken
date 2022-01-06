#ifndef CONFIG_H
#define CONFIG_H

#include <QSettings>
#include <QObject>
#include <QStandardPaths>
#include <QString>
#include <QDebug>


class Config : public QObject
{
    Q_OBJECT
public:
    explicit Config(QObject *parent = nullptr);

signals:
    void settingsUpdated();

public slots:
    // meas. device data
    double getInstrStartFreq() { return settings->value("instrStartFreq", 1560).toDouble();}
    void setInstrStartFreq(double val) { settings->setValue("instrStartFreq", val); emit settingsUpdated();}
    double getInstrStopFreq() { return settings->value("instrStopFreq", 1610).toDouble(); emit settingsUpdated();}
    void setInstrStopFreq(double val) { settings->setValue("instrStopFreq", val); emit settingsUpdated(); }
    QString getInstrResolution() { return settings->value("instrResolution", 1).toString(); }
    void setInstrResolution(QString f) { if (!f.isEmpty()) settings->setValue("instrResolution", f); emit settingsUpdated(); }
    double getInstrFfmCenterFreq() { return settings->value("instrFftCenterFreq", 1560).toDouble();}
    void setInstrFfmCenterFreq(double f) { settings->setValue("instrFftCenterFreq", f); emit settingsUpdated();}
    int getInstrFfmSpan() { return settings->value("instrFftSpan", 2000).toInt();}
    void setInstrFfmSpan(int f) { settings->setValue("instrFftSpan", f); emit settingsUpdated();}
    int getInstrMeasurementTime() { return settings->value("instrMeasurementTime", 18).toInt(); }
    void setInstrMeasurementTime(int val) { settings->setValue("instrMeasurementTime", val); emit settingsUpdated(); }
    int getInstrManAtt() { return settings->value("instrManAtt", 0).toInt(); }
    void setInstrManAtt(int val) { settings->setValue("instrManAtt", val); emit settingsUpdated(); }
    bool getInstrAutoAtt() { return settings->value("instrAutoAtt", true).toBool(); }
    void setInstrAutoAtt(bool b) { settings->setValue("instrAutoAtt", b); emit settingsUpdated(); }
    QString getInstrAntPort() { return settings->value("instrAntPort", "Default").toString(); }
    void setInstrAntPort(QString str) { if (!str.isEmpty()) settings->setValue("instrAntPort", str); emit settingsUpdated(); }
    int getInstrMode() { return settings->value("instrMode", 0).toInt(); }
    void setInstrMode(int val) { settings->setValue("instrMode", val); emit settingsUpdated(); }
    QString getInstrFftMode() { return settings->value("instrFftMode", 0).toString(); }
    void setInstrFftMode(QString val) { if (!val.isEmpty()) settings->setValue("instrFftMode", val); emit settingsUpdated(); }
    QString getInstrIpAddr() { return settings->value("instrIpAddr", "").toString(); }
    void setInstrIpAddr(QString str) { settings->setValue("instrIpAddr", str); emit settingsUpdated(); }
    int getInstrPort() { return settings->value("instrPort", 5555).toInt(); }
    void setInstrPort(int val) { settings->setValue("instrPort", val); emit settingsUpdated(); }
    QString getInstrId() { return settings->value("instrId").toString();}
    void setInstrId(QString s) { settings->setValue("instrId", s);}

    // spectrum criterias
    int getInstrTrigLevel() { return settings->value("instrTrigLevel", 15.0).toInt(); }
    void setInstrTrigLevel(int val) { settings->setValue("instrTrigLevel", val); emit settingsUpdated(); }
    int getInstrMinTrigBW() { return settings->value("instrMinTrigBW", 50).toInt(); }
    void setInstrMinTrigBW(int val) { settings->setValue("instrMinTrigBW", val); emit settingsUpdated(); }
    int getInstrMinTrigTime() { return settings->value("instrMinTrigTime", 18).toInt(); }
    void setInstrMinTrigTime(int val) { settings->setValue("instrMinTrigTime", val); emit settingsUpdated(); }

    // GNSS criterias
    int getGnssCnoDeviation() { return settings->value("gnssCnoDeviation", 15).toInt(); }
    void setGnssCnoDeviation(double val) { settings->setValue("gnssCnoDeviation", val); emit settingsUpdated(); }
    int getGnssAgcDeviation() { return settings->value("gnssAgcDeviation", 0).toInt(); }
    void setGnssAgcDeviation(int val) { settings->setValue("gnssAgcDeviation", val); emit settingsUpdated(); }
    int getGnssPosOffset() { return settings->value("gnssPosOffset", 0).toInt(); }
    void setGnssPosOffset(int val) { settings->setValue("gnssPosOffset", val); emit settingsUpdated(); }
    int getGnssAltOffset() { return settings->value("gnssAltOffset", 0).toInt(); }
    void setGnssAltOffset(int val) { settings->setValue("gnssAltOffset", val); emit settingsUpdated(); }
    int getGnssTimeOffset() { return settings->value("gnssTimeOffset", 0).toInt(); }
    void setGnssTimeOffset(int val) { settings->setValue("gnssTimeOffset", val); emit settingsUpdated(); }

    // General options
    QString getStationName() { return settings->value("stationName", "").toString(); }
    void setStationName(QString s) { settings->setValue("stationName", s);  emit settingsUpdated();}
    QString getStnLatitude() { return settings->value("stationLatitude", 0).toString(); }
    void setStnLatitude(QString s) { settings->setValue("stationLatitude", s); emit settingsUpdated(); }
    QString getStnLongitude() { return settings->value("stationLongitude", 0).toString(); }
    void setStnLongitude(QString s) { settings->setValue("stationLongitude", s); emit settingsUpdated(); }
    QString getStnAltitude() { return settings->value("stationAltitude", 0).toString(); }
    void setStnAltitude(QString s) { settings->setValue("stationAltitude", s); emit settingsUpdated(); }
    QString getWorkFolder() { return settings->value("workFolder",
                                                     QStandardPaths::writableLocation(QStandardPaths::HomeLocation) + "/Hauken").toString(); }
    void setWorkFolder(QString s) { settings->setValue("workFolder", s); emit settingsUpdated(); }
    QString getLogFolder() { return settings->value("logFolder",
                                                    QStandardPaths::writableLocation(QStandardPaths::HomeLocation) + "/Hauken/logs").toString(); }
    void setLogFolder(QString s) { settings->setValue("logFolder", s); emit settingsUpdated(); }
    bool getNewLogFolder() { return settings->value("newLogFolder", false).toBool(); }
    void setNewLogFolder(bool b) { settings->setValue("newLogFolder", b); emit settingsUpdated(); }

    // Receiver options
    bool getInstrConnectOnStartup() { return settings->value("instrConnectOnStartup", false).toBool(); }
    void setInstrConnectOnStartup(bool b) { settings->setValue("instrConnectOnStartup", b); emit settingsUpdated(); }
    bool getInstrUseTcpDatastream() { return settings->value("instrUseTcpDatastream", false).toBool(); }
    void setInstrUseTcpDatastream(bool b) { settings->setValue("instrUseTcpDatastream", b); emit settingsUpdated(); }
    bool getInstrAutoReconnect() { return settings->value("instrAutoReconnect", true).toBool(); }
    void setInstrAutoReconnect(bool b) { settings->setValue("instrAutoReconnect", b); emit settingsUpdated(); }
    bool getInstrNormalizeSpectrum() { return settings->value("instrNormalizeSpectrum", false).toBool(); }
    void setInstrNormalizeSpectrum(bool b) { settings->setValue("instrNormalizeSpectrum", b); emit settingsUpdated(); }

    // SDeF options
    bool getSdefSaveToFile() { return settings->value("sdefSaveToFile", true).toBool();}
    void setSdefSaveToFile(bool b) { settings->setValue("sdefSaveToFile", b); emit settingsUpdated();}
    bool getSdefUploadFile() { return settings->value("sdefUploadFile", false).toBool();}
    void setSdefUploadFile(bool b) { settings->setValue("sdefUploadFile", b); emit settingsUpdated();}
    QString getSdefUsername() { return settings->value("sdefUsername", "").toString();}
    void setSdefUsername(QString s) { settings->setValue("sdefUsername", s); emit settingsUpdated();}
    QString getSdefPassword() { return settings->value("sdefPassword", "").toString();}
    void setSdefPassword(QString s) { settings->setValue("sdefPassword", s); emit settingsUpdated();}
    bool getSdefAddPosition() { return settings->value("sdefAddPosition", false).toBool();}
    void setSdefAddPosition(bool b) { settings->setValue("sdefAddPosition", b); emit settingsUpdated();}
    int getSdefRecordTime() { return settings->value("sdefRecordTime", 2).toUInt();}
    void setSdefRecordTime(int val) { settings->setValue("sdefRecordTime", val); emit settingsUpdated();}
    int getSdefMaxRecordTime() { return settings->value("sdefMaxRecordTime", 60).toUInt();}
    void setSdefMaxRecordTime(int val) { settings->setValue("sdefMaxRecordTime", val); emit settingsUpdated();}
    QString getSdefStationInitals() { return settings->value("sdefStationInitials", "").toString();}
    void setSdefStationInitials(QString s) { settings->setValue("sdefStationInitials", s);}
    int getSdefPreRecordTime() { return settings->value("sdefPreRecordTime", 30).toUInt();}
    void setSdefPreRecordTime(int val) { settings->setValue("sdefPreRecordTime", val);}

    // GNSS options
    QString getGnssSerialPort1Name() { return settings->value("gnss/SerialPort1/Name").toString();}
    void setGnssSerialPort1Name(QString s) { settings->setValue("gnss/SerialPort1/Name", s);}
    QString getGnssSerialPort1Baudrate() { return settings->value("gnss/SerialPort1/Baudrate", "4800").toString();}
    void setGnssSerialPort1Baudrate(QString s) { settings->setValue("gnss/SerialPort1/Baudrate", s);}
    bool getGnssSerialPort1Activate() { return settings->value("gnss/SerialPort1/activate", false).toBool();}
    void setGnssSerialPort1Activate(bool b) { settings->setValue("gnss/SerialPort1/activate", b);}
    bool getGnssSerialPort1AutoConnect() { return settings->value("gnss/SerialPort1/AutoConnect", false).toBool();}
    void setGnssSerialPort1AutoConnect(bool b) { settings->setValue("gnss/SerialPort1/AutoConnect", b);}
    bool getGnssSerialPort1LogToFile() { return settings->value("gnss/SerialPort1/LogToFile", false).toBool();}
    void setGnssSerialPort1LogToFile(bool b) { settings->setValue("gnss/SerialPort1/LogToFile", b);}
    bool getGnssSerialPort1MonitorAgc() { return settings->value("gnss/SerialPort1/MonitorAgc", false).toBool();}
    void setGnssSerialPort1MonitorAgc(bool b) { settings->setValue("gnss/SerialPort1/MonitorAgc", b);}
    bool getGnssSerialPort1TriggerRecording() { return settings->value("gnss/SerialPort1/TriggerRecording", false).toBool();}
    void setGnssSerialPort1TriggerRecording(bool b) { settings->setValue("gnss/SerialPort1/TriggerRecording", b);}


    QString getGnssSerialPort2Name() { return settings->value("gnss/SerialPort2/Name").toString();}
    void setGnssSerialPort2Name(QString s) { settings->setValue("gnss/SerialPort2/Name", s);}
    QString getGnssSerialPort2Baudrate() { return settings->value("gnss/SerialPort2/Baudrate", "4800").toString();}
    void setGnssSerialPort2Baudrate(QString s) { settings->setValue("gnss/SerialPort2/Baudrate", s);}
    bool getGnssSerialPort2Activate() { return settings->value("gnss/SerialPort2/activate", false).toBool();}
    void setGnssSerialPort2Activate(bool b) { settings->setValue("gnss/SerialPort2/activate", b);}
    bool getGnssSerialPort2AutoConnect() { return settings->value("gnss/SerialPort2/AutoConnect", false).toBool();}
    void setGnssSerialPort2AutoConnect(bool b) { settings->setValue("gnss/SerialPort2/AutoConnect", b);}
    bool getGnssSerialPort2LogToFile() { return settings->value("gnss/SerialPort2/LogToFile", false).toBool();}
    void setGnssSerialPort2LogToFile(bool b) { settings->setValue("gnss/SerialPort2/LogToFile", b);}
    bool getGnssSerialPort2MonitorAgc() { return settings->value("gnss/SerialPort2/MonitorAgc", false).toBool();}
    void setGnssSerialPort2MonitorAgc(bool b) { settings->setValue("gnss/SerialPort2/MonitorAgc", b);}
    bool getGnssSerialPort2TriggerRecording() { return settings->value("gnss/SerialPort2/TriggerRecording", false).toBool();}
    void setGnssSerialPort2TriggerRecording(bool b) { settings->setValue("gnss/SerialPort2/TriggerRecording", b);}

    // Window specific settings
    int getPlotYMax() { return settings->value("plotYMax", 50).toInt();}
    void setPlotYMax(int val) { settings->setValue("plotYMax", val); emit settingsUpdated();}
    int getPlotYMin() { return settings->value("plotYMin", -30).toInt();}
    void setPlotYMin(int val) { settings->setValue("plotYMin", val); emit settingsUpdated();}
    QByteArray getWindowGeometry() { return settings->value("windowGeometry", "").toByteArray();}
    void setWindowGeometry(QByteArray arr) { settings->setValue("windowGeometry", arr);}
    QByteArray getWindowState() { return settings->value("windowState", "").toByteArray();}
    void setWindowState(QByteArray arr)  { settings->setValue("windowState", arr);}
    int getPlotMaxholdTime() { return settings->value("plotMaxholdTime", 1).toUInt();}
    void setPlotMaxholdTime(int val) { settings->setValue("plotMaxholdTime", val); emit settingsUpdated();}

    // trig line

    QStringList getTrigFrequencies() { return settings->value("trigFrequencies").toStringList();}
    void setTrigFrequencies(QStringList list) { settings->setValue("trigFrequencies", list); emit settingsUpdated();}

    //internal
    void newFileName(const QString file);

private:
    QString basicFile = QStandardPaths::writableLocation(QStandardPaths::HomeLocation) + "/hauken.conf";
    QSettings *basicSettings = new QSettings(basicFile, QSettings::IniFormat);
    QString curFile = basicSettings->value("lastFile", "default.ini").toString();
    QSettings *settings = new QSettings(curFile, QSettings::IniFormat);
    bool ready = false;
};

#endif // CONFIG_H
