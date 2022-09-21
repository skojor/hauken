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
    double getInstrStartFreq() { return settings->value("instr/StartFreq", 1560).toDouble();}
    void setInstrStartFreq(double val) { settings->setValue("instr/StartFreq", val); emit settingsUpdated();}
    double getInstrStopFreq() { return settings->value("instr/StopFreq", 1610).toDouble(); emit settingsUpdated();}
    void setInstrStopFreq(double val) { settings->setValue("instr/StopFreq", val); emit settingsUpdated(); }
    QString getInstrResolution() { return settings->value("instr/Resolution", 1).toString(); }
    void setInstrResolution(QString f) { if (!f.isEmpty()) settings->setValue("instr/Resolution", f); emit settingsUpdated(); }
    double getInstrFfmCenterFreq() { return settings->value("instr/FftCenterFreq", 1560).toDouble();}
    void setInstrFfmCenterFreq(double f) { settings->setValue("instr/FftCenterFreq", f); emit settingsUpdated();}
    int getInstrFfmSpan() { return settings->value("instr/FftSpan", 2000).toInt();}
    void setInstrFfmSpan(int f) { settings->setValue("instr/FftSpan", f); emit settingsUpdated();}
    int getInstrMeasurementTime() { return settings->value("instr/MeasurementTime", 18).toInt(); }
    void setInstrMeasurementTime(int val) { settings->setValue("instr/MeasurementTime", val); emit settingsUpdated(); }
    int getInstrManAtt() { return settings->value("instr/ManAtt", 0).toInt(); }
    void setInstrManAtt(int val) { settings->setValue("instr/ManAtt", val); emit settingsUpdated(); }
    bool getInstrAutoAtt() { return settings->value("instr/AutoAtt", true).toBool(); }
    void setInstrAutoAtt(bool b) { settings->setValue("instr/AutoAtt", b); emit settingsUpdated(); }
    QString getInstrAntPort() { return settings->value("instr/AntPort", "Default").toString(); }
    void setInstrAntPort(QString str) { if (!str.isEmpty()) settings->setValue("instr/AntPort", str); emit settingsUpdated(); }
    int getInstrMode() { return settings->value("instr/Mode", 0).toInt(); }
    void setInstrMode(int val) { settings->setValue("instr/Mode", val); emit settingsUpdated(); }
    QString getInstrFftMode() { return settings->value("instr/FftMode", 0).toString(); }
    void setInstrFftMode(QString val) { if (!val.isEmpty()) settings->setValue("instr/FftMode", val); emit settingsUpdated(); }
    QString getInstrIpAddr() { return settings->value("instr/IpAddr", "").toString(); }
    void setInstrIpAddr(QString str) { settings->setValue("instr/IpAddr", str); emit settingsUpdated(); }
    int getInstrPort() { return settings->value("instr/Port", 5555).toInt(); }
    void setInstrPort(int val) { settings->setValue("instr/Port", val); emit settingsUpdated(); }
    QString getInstrId() { return settings->value("instr/Id").toString();}
    void setInstrId(QString s) { settings->setValue("instr/Id", s);}
    QString getMeasurementDeviceName() { return measurementDeviceName;}
    void setMeasurementDeviceName(QString s) { measurementDeviceName = s;}

    // spectrum criterias
    int getInstrTrigLevel() { return settings->value("instr/TrigLevel", 15.0).toInt(); }
    void setInstrTrigLevel(int val) { settings->setValue("instr/TrigLevel", val); emit settingsUpdated(); }
    int getInstrMinTrigBW() { return settings->value("instr/MinTrigBW", 50).toInt(); }
    void setInstrMinTrigBW(int val) { settings->setValue("instr/MinTrigBW", val); emit settingsUpdated(); }
    int getInstrTotalTrigBW() { return settings->value("instr/TotalTrigBW", 500).toInt();}
    void setInstrTotalTrigBW(int val) { settings->setValue("instr/TotalTrigBW", val); emit settingsUpdated();}
    int getInstrMinTrigTime() { return settings->value("instr/MinTrigTime", 18).toInt(); }
    void setInstrMinTrigTime(int val) { settings->setValue("instr/MinTrigTime", val); emit settingsUpdated(); }

    // GNSS criterias
    int getGnssCnoDeviation() { return settings->value("gnss/CnoDeviation", 15).toInt(); }
    void setGnssCnoDeviation(double val) { settings->setValue("gnss/CnoDeviation", val); emit settingsUpdated(); }
    int getGnssAgcDeviation() { return settings->value("gnss/AgcDeviation", 0).toInt(); }
    void setGnssAgcDeviation(int val) { settings->setValue("gnss/AgcDeviation", val); emit settingsUpdated(); }
    int getGnssPosOffset() { return settings->value("gnss/PosOffset", 0).toInt(); }
    void setGnssPosOffset(int val) { settings->setValue("gnss/PosOffset", val); emit settingsUpdated(); }
    int getGnssAltOffset() { return settings->value("gnss/AltOffset", 0).toInt(); }
    void setGnssAltOffset(int val) { settings->setValue("gnss/AltOffset", val); emit settingsUpdated(); }
    int getGnssTimeOffset() { return settings->value("gnss/TimeOffset", 0).toInt(); }
    void setGnssTimeOffset(int val) { settings->setValue("gnss/TimeOffset", val); emit settingsUpdated(); }

    // General options
    QString getStationName() { return settings->value("station/Name", "").toString(); }
    void setStationName(QString s) { settings->setValue("station/Name", s);  emit settingsUpdated();}
    QString getStnLatitude() { return settings->value("station/Latitude", 0).toString(); }
    void setStnLatitude(QString s) { settings->setValue("station/Latitude", s); emit settingsUpdated(); }
    QString getStnLongitude() { return settings->value("station/Longitude", 0).toString(); }
    void setStnLongitude(QString s) { settings->setValue("station/Longitude", s); emit settingsUpdated(); }
    QString getStnAltitude() { return settings->value("station/Altitude", 0).toString(); }
    void setStnAltitude(QString s) { settings->setValue("station/Altitude", s); emit settingsUpdated(); }
    QString getWorkFolder() { return settings->value("workFolder",
                                                     QStandardPaths::writableLocation(QStandardPaths::HomeLocation) + "/Hauken").toString(); }
    void setWorkFolder(QString s) { settings->setValue("workFolder", s); emit settingsUpdated(); }
    QString getLogFolder() { return settings->value("logFolder",
                                                    QStandardPaths::writableLocation(QStandardPaths::HomeLocation) + "/Hauken/logs").toString(); }
    void setLogFolder(QString s) { settings->setValue("logFolder", s); emit settingsUpdated(); }
    bool getNewLogFolder() { return settings->value("newLogFolder", false).toBool(); }
    void setNewLogFolder(bool b) { settings->setValue("newLogFolder", b); emit settingsUpdated(); }

    // Receiver options
    bool getInstrConnectOnStartup() { return settings->value("instr/ConnectOnStartup", false).toBool(); }
    void setInstrConnectOnStartup(bool b) { settings->setValue("instr/ConnectOnStartup", b); emit settingsUpdated(); }
    bool getInstrUseTcpDatastream() { return settings->value("instr/UseTcpDatastream", false).toBool(); }
    void setInstrUseTcpDatastream(bool b) { settings->setValue("instr/UseTcpDatastream", b); emit settingsUpdated(); }
    bool getInstrAutoReconnect() { return settings->value("instr/AutoReconnect", true).toBool(); }
    void setInstrAutoReconnect(bool b) { settings->setValue("instr/AutoReconnect", b); emit settingsUpdated(); }
    bool getInstrNormalizeSpectrum() { return settings->value("instr/NormalizeSpectrum", false).toBool(); }
    void setInstrNormalizeSpectrum(bool b) { settings->setValue("instr/NormalizeSpectrum", b); emit settingsUpdated(); }
    int getInstrTracesNeededForAverage() { return settings->value("instr/tracesNeededForAverage", 250).toInt();}
    void setInstrTracesNeededForAverage(int i) { settings->setValue("instr/tracesNeededForAverage", i);}

    // SDeF options
    bool getSdefSaveToFile() { return settings->value("sdef/SaveToFile", true).toBool();}
    void setSdefSaveToFile(bool b) { settings->setValue("sdef/SaveToFile", b); emit settingsUpdated();}
    bool getSdefUploadFile() { return settings->value("sdef/UploadFile", false).toBool();}
    void setSdefUploadFile(bool b) { settings->setValue("sdef/UploadFile", b); emit settingsUpdated();}
    QString getSdefUsername() { return settings->value("sdef/Username", "").toString();}
    void setSdefUsername(QString s) { settings->setValue("sdef/Username", s); emit settingsUpdated();}
    QString getSdefPassword() { return simpleEncr(settings->value("sdef/Password", "").toByteArray());}
    void setSdefPassword(QString s) { settings->setValue("sdef/Password", simpleEncr(s.toLocal8Bit())); emit settingsUpdated();}
    bool getSdefAddPosition() { return settings->value("sdef/AddPosition", false).toBool();}
    void setSdefAddPosition(bool b) { settings->setValue("sdef/AddPosition", b); emit settingsUpdated();}
    QString getSdefGpsSource() { return settings->value("sdef/GpsSource").toString();}
    void setSdefGpsSource(QString s) { settings->setValue("sdef/GpsSource", s); emit settingsUpdated();}
    int getSdefRecordTime() { return settings->value("sdef/RecordTime", 2).toUInt();}
    void setSdefRecordTime(int val) { settings->setValue("sdef/RecordTime", val); emit settingsUpdated();}
    int getSdefMaxRecordTime() { return settings->value("sdef/MaxRecordTime", 60).toUInt();}
    void setSdefMaxRecordTime(int val) { settings->setValue("sdef/MaxRecordTime", val); emit settingsUpdated();}
    QString getSdefStationInitals() { return settings->value("sdef/StationInitials", "").toString();}
    void setSdefStationInitials(QString s) { settings->setValue("sdef/StationInitials", s); emit settingsUpdated();}
    int getSdefPreRecordTime() { return settings->value("sdef/PreRecordTime", 30).toUInt();}
    void setSdefPreRecordTime(int val) { settings->setValue("sdef/PreRecordTime", val); emit settingsUpdated();}

    // GNSS options
    QString getGnssSerialPort1Name() { return settings->value("gnss/SerialPort1Name").toString();}
    void setGnssSerialPort1Name(QString s) { settings->setValue("gnss/SerialPort1Name", s); emit settingsUpdated();}
    QString getGnssSerialPort1Baudrate() { return settings->value("gnss/SerialPort1Baudrate", "4800").toString();}
    void setGnssSerialPort1Baudrate(QString s) { settings->setValue("gnss/SerialPort1Baudrate", s); emit settingsUpdated();}
    bool getGnssSerialPort1Activate() { return settings->value("gnss/SerialPort1activate", false).toBool();}
    void setGnssSerialPort1Activate(bool b) { settings->setValue("gnss/SerialPort1activate", b); emit settingsUpdated();}
    bool getGnssSerialPort1LogToFile() { return settings->value("gnss/SerialPort1LogToFile", false).toBool();}
    void setGnssSerialPort1LogToFile(bool b) { settings->setValue("gnss/SerialPort1LogToFile", b); emit settingsUpdated();}
    bool getGnssSerialPort1MonitorAgc() { return settings->value("gnss/SerialPort1MonitorAgc", false).toBool();}
    void setGnssSerialPort1MonitorAgc(bool b) { settings->setValue("gnss/SerialPort1MonitorAgc", b); emit settingsUpdated();}
    bool getGnssSerialPort1TriggerRecording() { return settings->value("gnss/SerialPort1TriggerRecording", false).toBool();}
    void setGnssSerialPort1TriggerRecording(bool b) { settings->setValue("gnss/SerialPort1TriggerRecording", b); emit settingsUpdated();}


    QString getGnssSerialPort2Name() { return settings->value("gnss/SerialPort2Name").toString();}
    void setGnssSerialPort2Name(QString s) { settings->setValue("gnss/SerialPort2Name", s); emit settingsUpdated();}
    QString getGnssSerialPort2Baudrate() { return settings->value("gnss/SerialPort2Baudrate", "4800").toString();}
    void setGnssSerialPort2Baudrate(QString s) { settings->setValue("gnss/SerialPort2Baudrate", s); emit settingsUpdated();}
    bool getGnssSerialPort2Activate() { return settings->value("gnss/SerialPort2activate", false).toBool();}
    void setGnssSerialPort2Activate(bool b) { settings->setValue("gnss/SerialPort2activate", b); emit settingsUpdated();}
    bool getGnssSerialPort2LogToFile() { return settings->value("gnss/SerialPort2LogToFile", false).toBool();}
    void setGnssSerialPort2LogToFile(bool b) { settings->setValue("gnss/SerialPort2LogToFile", b); emit settingsUpdated();}
    bool getGnssSerialPort2MonitorAgc() { return settings->value("gnss/SerialPort2MonitorAgc", false).toBool();}
    void setGnssSerialPort2MonitorAgc(bool b) { settings->setValue("gnss/SerialPort2MonitorAgc", b); emit settingsUpdated();}
    bool getGnssSerialPort2TriggerRecording() { return settings->value("gnss/SerialPort2TriggerRecording", false).toBool();}
    void setGnssSerialPort2TriggerRecording(bool b) { settings->setValue("gnss/SerialPort2TriggerRecording", b); emit settingsUpdated();}


    // Email/notification options
    QString getEmailSmtpServer() { return settings->value("email/SmtpServer").toString();}
    void setEmailSmtpServer(QString s) { settings->setValue("email/SmtpServer", s); emit settingsUpdated();}
    QString getEmailSmtpPort() { return settings->value("email/SmtpPort", "25").toString();}
    void setEmailSmtpPort(QString s) { settings->setValue("email/SmtpPort", s); emit settingsUpdated();}
    QString getEmailRecipients() { return settings->value("email/Recipients").toString();}
    void setEmailRecipients(QString s) { settings->setValue("email/Recipients", s); emit settingsUpdated();}
    bool getEmailNotifyMeasurementDeviceHighLevel() { return settings->value("email/NotifyMeasurementDeviceHighLevel", false).toBool();}
    void setEmailNotifyMeasurementDeviceHighLevel(bool b) { settings->setValue("email/NotifyMeasurementDeviceHighLevel", b); emit settingsUpdated();}
    bool getEmailNotifyMeasurementDeviceDisconnected() { return settings->value("email/NotifyMeasurementDeviceDisconnected", false).toBool();}
    void setEmailNotifyMeasurementDeviceDisconnected(bool b) { settings->setValue("email/NotifyMeasurementDeviceDisconnected", b); emit settingsUpdated();}
    bool getEmailNotifyGnssIncidents() { return settings->value("email/NotifyGnssIncidents", false).toBool();}
    void setEmailNotifyGnssIncidents(bool b) { settings->setValue("email/NotifyGnssIncidents", b); emit settingsUpdated();}
    int getEmailMinTimeBetweenEmails() { return settings->value("email/MinTimeBetweenEmails", 3600).toInt();}
    void setEmailMinTimeBetweenEmails(int s) { settings->setValue("email/MinTimeBetweenEmails", s); emit settingsUpdated();}
    int getNotifyTruncateTime() { return settings->value("notify/TruncateTime", 30).toInt();}
    void setNotifyTruncateTime(int s) { settings->setValue("notify/TruncateTime", s); emit settingsUpdated();}
    QString getEmailFromAddress() { return settings->value("email/FromAddress").toString();}
    void setEmailFromAddress(QString s) { settings->setValue("email/FromAddress", s); emit settingsUpdated();}
    QString getEmailSmtpUser() { return settings->value("email/SmtpUser").toString();}
    void setEmailSmtpUser(QString s) { settings->setValue("email/SmtpUser", s); emit settingsUpdated();}
    QString getEmailSmtpPassword() { return simpleEncr(settings->value("email/smtpPassword").toByteArray());}
    void setEmailSmtpPassword(QString s) { settings->setValue("email/smtpPassword", simpleEncr(s.toLocal8Bit())); emit settingsUpdated();}
    bool getEmailAddImages() { return settings->value("email/AddImages", true).toBool();}
    void setEmailAddImages(bool b) { settings->setValue("email/AddImages", b); emit settingsUpdated();}
    int getEmailDelayBeforeAddingImages() { return settings->value("email/DelayBeforeAddingImages", 10).toInt();}
    void setEmailDelayBeforeAddingImages(int val) { settings->setValue("email/DelayBeforeAddingImages", val); emit settingsUpdated();}

    // Camera options
    QString getCameraName() { return settings->value("camera/Name").toString();}
    void setCameraName(QString s) { settings->setValue("camera/Name", s);}
    QString getCameraStreamAddress() { return settings->value("camera/StreamAddress").toString();}
    void setCameraStreamAddress(QString s) { settings->setValue("camera/StreamAddress", s);}
    bool getCameraDeviceTrigger() { return settings->value("camera/DeviceTrigger").toBool();}
    void setCameraDeviceTrigger(bool b) { settings->setValue("camera/DeviceTrigger", b);}
    int getCameraRecordTime() { return settings->value("camera/RecordTime", 60).toInt();}
    void setCameraRecordTime(int s) { settings->setValue("camera/RecordTime", s);}

    // Relay control (and temp) via serial (Arduino custom)
    QString getArduinoSerialName() { return settings->value("arduino/serialName", "").toString();}
    void setArduinoSerialName(QString s) { settings->setValue("arduino/serialName", s);}
    QString getArduinoBaudrate() { return settings->value("arduino/baudrate").toString();}
    void setArduinoBaudrate(QString s) { settings->setValue("arduino/baudrate", s);}
    bool getArduinoReadTemperature() { return settings->value("arduino/temperature", false).toBool();}
    void setArduinoReadTemperature(bool b) { settings->setValue("arduino/temperature", b);}
    bool getArduinoEnable() { return settings->value("arduino/enable", false).toBool();}
    void setArduinoEnable(bool b) { settings->setValue("arduino/enable", b);}
    QString getArduinoRelayOnText() { return settings->value("arduino/relayOnText", "on").toString(); }
    void setArduinoRelayOnText(QString s) { settings->setValue("arduino/relayOnText", s);}
    QString getArduinoRelayOffText() { return settings->value("arduino/relayOffText", "off").toString(); }
    void setArduinoRelayOffText(QString s) { settings->setValue("arduino/relayOffText", s);}

    // AutoRecorder options
    bool getAutoRecorderActivate() { return settings->value("autorecorder/activate", false).toBool();}
    void setAutoRecorderActivate(bool b) { settings->setValue("autorecorder/activate", b);}


    // Window specific settings
    int getPlotYMax() { return settings->value("plot/YMax", 50).toInt();}
    void setPlotYMax(int val) { settings->setValue("plot/YMax", val); emit settingsUpdated();}
    int getPlotYMin() { return settings->value("plot/YMin", -30).toInt();}
    void setPlotYMin(int val) { settings->setValue("plot/YMin", val); emit settingsUpdated();}
    QByteArray getWindowGeometry() { return settings->value("windowGeometry", "").toByteArray();}
    void setWindowGeometry(QByteArray arr) { settings->setValue("windowGeometry", arr);}
    QByteArray getWindowState() { return settings->value("windowState", "").toByteArray();}
    void setWindowState(QByteArray arr)  { settings->setValue("windowState", arr);}
    int getPlotMaxholdTime() { return settings->value("plot/MaxholdTime", 30).toUInt();}
    void setPlotMaxholdTime(int val) { settings->setValue("plot/MaxholdTime", val); emit settingsUpdated();}
    QString getShowWaterfall() { return settings->value("plot/ShowWaterfall", "Off").toString();}
    void setShowWaterfall(QString b) { settings->setValue("plot/ShowWaterfall", b); emit settingsUpdated();}
    int getWaterfallTime() { return settings->value("plot/WaterfallTime", 120).toInt();}
    void setWaterfallTime(int val) { settings->setValue("plot/WaterfallTime", val); emit settingsUpdated();}
    int getPlotResolution() { return plotResolution;}

    // trig line
    QStringList getTrigFrequencies() { return settings->value("trigFrequencies").toStringList();}
    void setTrigFrequencies(QStringList list) { settings->setValue("trigFrequencies", list); emit settingsUpdated();}

    //internal
    void newFileName(const QString file);
    QString getCurrentFilename() { return curFile;}

private slots:
    QByteArray simpleEncr(QByteArray);

private:
    QString basicFile = QStandardPaths::writableLocation(QStandardPaths::HomeLocation) + "/hauken.conf";
    QSettings *basicSettings = new QSettings(basicFile, QSettings::IniFormat);
    QString curFile = basicSettings->value("lastFile", "default.ini").toString();
    QSettings *settings = new QSettings(curFile, QSettings::IniFormat);
    bool ready = false;
    QString measurementDeviceName;
    const int plotResolution = 1200;
};

#endif // CONFIG_H
