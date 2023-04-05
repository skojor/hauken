#ifndef CONFIG_H
#define CONFIG_H

#include <QSettings>
#include <QObject>
#include <QStandardPaths>
#include <QString>
#include <QDebug>
#include <QSysInfo>
#include <QDir>
#include <QFile>


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
    double getInstrStopFreq() { return settings->value("instr/StopFreq", 1610).toDouble();}
    void setInstrStopFreq(double val) { settings->setValue("instr/StopFreq", val); }
    QString getInstrResolution() { return settings->value("instr/Resolution", 1).toString(); }
    void setInstrResolution(QString f) { if (!f.isEmpty()) settings->setValue("instr/Resolution", f); }
    double getInstrFfmCenterFreq() { return settings->value("instr/FftCenterFreq", 1560).toDouble();}
    void setInstrFfmCenterFreq(double f) { settings->setValue("instr/FftCenterFreq", f);}
    int getInstrFfmSpan() { return settings->value("instr/FftSpan", 2000).toInt();}
    void setInstrFfmSpan(int f) { settings->setValue("instr/FftSpan", f);}
    int getInstrMeasurementTime() { return settings->value("instr/MeasurementTime", 18).toInt(); }
    void setInstrMeasurementTime(int val) { settings->setValue("instr/MeasurementTime", val); }
    int getInstrManAtt() { return settings->value("instr/ManAtt", 0).toInt(); }
    void setInstrManAtt(int val) { settings->setValue("instr/ManAtt", val); }
    bool getInstrAutoAtt() { return settings->value("instr/AutoAtt", true).toBool(); }
    void setInstrAutoAtt(bool b) { settings->setValue("instr/AutoAtt", b); }
    QString getInstrAntPort() { return settings->value("instr/AntPort", "Default").toString(); }
    void setInstrAntPort(QString str) { if (!str.isEmpty()) settings->setValue("instr/AntPort", str); }
    int getInstrMode() { return settings->value("instr/Mode", 0).toInt(); }
    void setInstrMode(int val) { settings->setValue("instr/Mode", val); }
    QString getInstrFftMode() { return settings->value("instr/FftMode", 0).toString(); }
    void setInstrFftMode(QString val) { if (!val.isEmpty()) settings->setValue("instr/FftMode", val); }
    QString getInstrIpAddr() { return settings->value("instr/IpAddr", "").toString(); }
    void setInstrIpAddr(QString str) { settings->setValue("instr/IpAddr", str); }
    int getInstrPort() { return settings->value("instr/Port", 5555).toInt(); }
    void setInstrPort(int val) { settings->setValue("instr/Port", val); }
    QString getInstrId() { return settings->value("instr/Id").toString();}
    void setInstrId(QString s) { settings->setValue("instr/Id", s);}
    QString getMeasurementDeviceName() { return measurementDeviceName;}
    void setMeasurementDeviceName(QString s) { measurementDeviceName = s;}

    // spectrum criterias
    int getInstrTrigLevel() { return settings->value("instr/TrigLevel", 15.0).toInt(); }
    void setInstrTrigLevel(int val) { settings->setValue("instr/TrigLevel", val); emit settingsUpdated(); }
    int getInstrMinTrigBW() { return settings->value("instr/MinTrigBW", 50).toInt(); }
    void setInstrMinTrigBW(int val) { settings->setValue("instr/MinTrigBW", val); }
    int getInstrTotalTrigBW() { return settings->value("instr/TotalTrigBW", 500).toInt();}
    void setInstrTotalTrigBW(int val) { settings->setValue("instr/TotalTrigBW", val);}
    int getInstrMinTrigTime() { return settings->value("instr/MinTrigTime", 18).toInt(); }
    void setInstrMinTrigTime(int val) { settings->setValue("instr/MinTrigTime", val); }

    // GNSS criterias
    int getGnssCnoDeviation() { return settings->value("gnss/CnoDeviation", 15).toInt(); }
    void setGnssCnoDeviation(double val) { settings->setValue("gnss/CnoDeviation", val); emit settingsUpdated(); }
    int getGnssAgcDeviation() { return settings->value("gnss/AgcDeviation", 0).toInt(); }
    void setGnssAgcDeviation(int val) { settings->setValue("gnss/AgcDeviation", val); }
    int getGnssPosOffset() { return settings->value("gnss/PosOffset", 0).toInt(); }
    void setGnssPosOffset(int val) { settings->setValue("gnss/PosOffset", val); }
    int getGnssAltOffset() { return settings->value("gnss/AltOffset", 0).toInt(); }
    void setGnssAltOffset(int val) { settings->setValue("gnss/AltOffset", val); }
    int getGnssTimeOffset() { return settings->value("gnss/TimeOffset", 0).toInt(); }
    void setGnssTimeOffset(int val) { settings->setValue("gnss/TimeOffset", val); }

    // General options
    QString getStationName() { return settings->value("station/Name", "").toString(); }
    void setStationName(QString s) { settings->setValue("station/Name", s);  emit settingsUpdated();}
    QString getStnLatitude() { return settings->value("station/Latitude", 0).toString(); }
    void setStnLatitude(QString s) { settings->setValue("station/Latitude", s); }
    QString getStnLongitude() { return settings->value("station/Longitude", 0).toString(); }
    void setStnLongitude(QString s) { settings->setValue("station/Longitude", s); }
    QString getStnAltitude() { return settings->value("station/Altitude", 0).toString(); }
    void setStnAltitude(QString s) { settings->setValue("station/Altitude", s); }

    QString findWorkFolderName();
    QString getWorkFolder() { return settings->value("workFolder",
                                                     findWorkFolderName()).toString(); }
    void setWorkFolder(QString s) { settings->setValue("workFolder", s); }
    QString getLogFolder() { return settings->value("logFolder",
                                                    findWorkFolderName() + "/logs").toString(); }
    void setLogFolder(QString s) { settings->setValue("logFolder", s); }
    bool getNewLogFolder() { return settings->value("newLogFolder", false).toBool(); }
    void setNewLogFolder(bool b) { settings->setValue("newLogFolder", b); }
    bool getPmrMode() { return settings->value("pmr/mode", false).toBool();}
    void setPmrMode(bool b) { settings->setValue("pmr/mode", b);}

    // Receiver options
    bool getInstrConnectOnStartup() { return settings->value("instr/ConnectOnStartup", false).toBool(); }
    void setInstrConnectOnStartup(bool b) { settings->setValue("instr/ConnectOnStartup", b); emit settingsUpdated(); }
    bool getInstrUseTcpDatastream() { return settings->value("instr/UseTcpDatastream", true).toBool(); }
    void setInstrUseTcpDatastream(bool b) { settings->setValue("instr/UseTcpDatastream", b); }
    bool getInstrAutoReconnect() { return settings->value("instr/AutoReconnect", true).toBool(); }
    void setInstrAutoReconnect(bool b) { settings->setValue("instr/AutoReconnect", b); }
    bool getInstrNormalizeSpectrum() { return settings->value("instr/NormalizeSpectrum", false).toBool(); }
    void setInstrNormalizeSpectrum(bool b) { settings->setValue("instr/NormalizeSpectrum", b); }
    int getInstrTracesNeededForAverage() { return settings->value("instr/tracesNeededForAverage", 250).toInt();}
    void setInstrTracesNeededForAverage(int i) { settings->setValue("instr/tracesNeededForAverage", i);}

    // SDeF options
    bool getSdefSaveToFile() { return settings->value("sdef/SaveToFile", true).toBool();}
    void setSdefSaveToFile(bool b) { settings->setValue("sdef/SaveToFile", b); emit settingsUpdated();}
    bool getSdefUploadFile() { return settings->value("sdef/UploadFile", false).toBool();}
    void setSdefUploadFile(bool b) { settings->setValue("sdef/UploadFile", b);}
    QString getSdefUsername() { return settings->value("sdef/Username", "").toString();}
    void setSdefUsername(QString s) { settings->setValue("sdef/Username", s);}
    QString getSdefPassword() { return simpleEncr(settings->value("sdef/Password", "").toByteArray());}
    void setSdefPassword(QString s) { settings->setValue("sdef/Password", simpleEncr(s.toLocal8Bit()));}
    bool getSdefAddPosition() { return settings->value("sdef/AddPosition", false).toBool();}
    void setSdefAddPosition(bool b) { settings->setValue("sdef/AddPosition", b);}
    QString getSdefGpsSource() { return settings->value("sdef/GpsSource").toString();}
    void setSdefGpsSource(QString s) { settings->setValue("sdef/GpsSource", s);}
    int getSdefRecordTime() { return settings->value("sdef/RecordTime", 2).toUInt();}
    void setSdefRecordTime(int val) { settings->setValue("sdef/RecordTime", val);}
    int getSdefMaxRecordTime() { return settings->value("sdef/MaxRecordTime", 60).toUInt();}
    void setSdefMaxRecordTime(int val) { settings->setValue("sdef/MaxRecordTime", val);}
    QString getSdefStationInitals() { return settings->value("sdef/StationInitials", "").toString();}
    void setSdefStationInitials(QString s) { settings->setValue("sdef/StationInitials", s);}
    int getSdefPreRecordTime() { return settings->value("sdef/PreRecordTime", 30).toUInt();}
    void setSdefPreRecordTime(int val) { settings->setValue("sdef/PreRecordTime", val);}
    bool getSdefZipFiles() { return settings->value("sdef/zipFiles", true).toBool();}
    void setSdefZipFiles(bool b) { settings->setValue("sdef/zipFiles", b);}

    // GNSS options
    QString getGnssSerialPort1Name() { return settings->value("gnss/SerialPort1Name").toString();}
    void setGnssSerialPort1Name(QString s) { settings->setValue("gnss/SerialPort1Name", s); emit settingsUpdated();}
    QString getGnssSerialPort1Baudrate() { return settings->value("gnss/SerialPort1Baudrate", "4800").toString();}
    void setGnssSerialPort1Baudrate(QString s) { settings->setValue("gnss/SerialPort1Baudrate", s);}
    bool getGnssSerialPort1Activate() { return settings->value("gnss/SerialPort1activate", false).toBool();}
    void setGnssSerialPort1Activate(bool b) { settings->setValue("gnss/SerialPort1activate", b);}
    bool getGnssSerialPort1LogToFile() { return settings->value("gnss/SerialPort1LogToFile", false).toBool();}
    void setGnssSerialPort1LogToFile(bool b) { settings->setValue("gnss/SerialPort1LogToFile", b);}
    bool getGnssSerialPort1MonitorAgc() { return settings->value("gnss/SerialPort1MonitorAgc", false).toBool();}
    void setGnssSerialPort1MonitorAgc(bool b) { settings->setValue("gnss/SerialPort1MonitorAgc", b);}
    bool getGnssSerialPort1TriggerRecording() { return settings->value("gnss/SerialPort1TriggerRecording", false).toBool();}
    void setGnssSerialPort1TriggerRecording(bool b) { settings->setValue("gnss/SerialPort1TriggerRecording", b);}

    QString getGnssSerialPort2Name() { return settings->value("gnss/SerialPort2Name").toString();}
    void setGnssSerialPort2Name(QString s) { settings->setValue("gnss/SerialPort2Name", s); emit settingsUpdated();}
    QString getGnssSerialPort2Baudrate() { return settings->value("gnss/SerialPort2Baudrate", "4800").toString();}
    void setGnssSerialPort2Baudrate(QString s) { settings->setValue("gnss/SerialPort2Baudrate", s);}
    bool getGnssSerialPort2Activate() { return settings->value("gnss/SerialPort2activate", false).toBool();}
    void setGnssSerialPort2Activate(bool b) { settings->setValue("gnss/SerialPort2activate", b);}
    bool getGnssSerialPort2LogToFile() { return settings->value("gnss/SerialPort2LogToFile", false).toBool();}
    void setGnssSerialPort2LogToFile(bool b) { settings->setValue("gnss/SerialPort2LogToFile", b);}
    bool getGnssSerialPort2MonitorAgc() { return settings->value("gnss/SerialPort2MonitorAgc", false).toBool();}
    void setGnssSerialPort2MonitorAgc(bool b) { settings->setValue("gnss/SerialPort2MonitorAgc", b);}
    bool getGnssSerialPort2TriggerRecording() { return settings->value("gnss/SerialPort2TriggerRecording", false).toBool();}
    void setGnssSerialPort2TriggerRecording(bool b) { settings->setValue("gnss/SerialPort2TriggerRecording", b);}

    bool getGnssUseInstrumentGnss() { return settings->value("gnss/UseInstrumentGnss", false).toBool();}
    void setGnssUseInstrumentGnss(bool b) { settings->setValue("gnss/UseInstrumentGnss", b); emit settingsUpdated();}
    bool getGnssInstrumentGnssTriggerRecording() { return settings->value("gnss/InstrumentGnssTriggerRecording", false).toBool();}
    void setGnssInstrumentGnssTriggerRecording(bool b) { settings->setValue("gnss/InstrumentGnssTriggerRecording", b);}

    // Email/notification options
    QString getEmailSmtpServer() { return settings->value("email/SmtpServer").toString();}
    void setEmailSmtpServer(QString s) { settings->setValue("email/SmtpServer", s.simplified()); emit settingsUpdated();}
    QString getEmailSmtpPort() { return settings->value("email/SmtpPort", "25").toString();}
    void setEmailSmtpPort(QString s) { settings->setValue("email/SmtpPort", s);}
    QString getEmailRecipients() { return settings->value("email/Recipients").toString();}
    void setEmailRecipients(QString s) { settings->setValue("email/Recipients", s);}
    bool getEmailNotifyMeasurementDeviceHighLevel() { return settings->value("email/NotifyMeasurementDeviceHighLevel", false).toBool();}
    void setEmailNotifyMeasurementDeviceHighLevel(bool b) { settings->setValue("email/NotifyMeasurementDeviceHighLevel", b);}
    bool getEmailNotifyMeasurementDeviceDisconnected() { return settings->value("email/NotifyMeasurementDeviceDisconnected", false).toBool();}
    void setEmailNotifyMeasurementDeviceDisconnected(bool b) { settings->setValue("email/NotifyMeasurementDeviceDisconnected", b);}
    bool getEmailNotifyGnssIncidents() { return settings->value("email/NotifyGnssIncidents", false).toBool();}
    void setEmailNotifyGnssIncidents(bool b) { settings->setValue("email/NotifyGnssIncidents", b);}
    int getEmailMinTimeBetweenEmails() { return settings->value("email/MinTimeBetweenEmails", 3600).toInt();}
    void setEmailMinTimeBetweenEmails(int s) { settings->setValue("email/MinTimeBetweenEmails", s);}
    int getNotifyTruncateTime() { return settings->value("notify/TruncateTime", 30).toInt();}
    void setNotifyTruncateTime(int s) { settings->setValue("notify/TruncateTime", s);}
    QString getEmailFromAddress() { return settings->value("email/FromAddress").toString();}
    void setEmailFromAddress(QString s) { settings->setValue("email/FromAddress", s.simplified());}
    QString getEmailSmtpUser() { return settings->value("email/SmtpUser").toString();}
    void setEmailSmtpUser(QString s) { settings->setValue("email/SmtpUser", s.simplified());}
    QString getEmailSmtpPassword() { return simpleEncr(settings->value("email/smtpPassword").toByteArray());}
    void setEmailSmtpPassword(QString s) { settings->setValue("email/smtpPassword", simpleEncr(s.simplified().toLocal8Bit()));}
    bool getEmailAddImages() { return settings->value("email/AddImages", true).toBool();}
    void setEmailAddImages(bool b) { settings->setValue("email/AddImages", b);}
    int getEmailDelayBeforeAddingImages() { return settings->value("email/DelayBeforeAddingImages", 10).toInt();}
    void setEmailDelayBeforeAddingImages(int val) { settings->setValue("email/DelayBeforeAddingImages", val);}
    QString getEmailGraphApplicationId() { return settings->value("email/graphApplicationId", "").toString(); }
    void setEmailGraphApplicationId(QString s) { settings->setValue("email/graphApplicationId", s.simplified());}
    QString getEmailGraphTenantId() { return settings->value("email/graphTenantId", "").toString(); }
    void setEmailGraphTenantId(QString s) { settings->setValue("email/graphTenantId", s.simplified());}
    QString getEmailGraphSecret() { return simpleEncr(settings->value("email/graphSecret", "").toByteArray());}
    void setEmailGraphSecret(QString s) { settings->setValue("email/graphSecret", simpleEncr(s.simplified().toLocal8Bit()));}

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
    void setArduinoSerialName(QString s) { settings->setValue("arduino/serialName", s); emit settingsUpdated();}
    QString getArduinoBaudrate() { return settings->value("arduino/baudrate").toString();}
    void setArduinoBaudrate(QString s) { settings->setValue("arduino/baudrate", s);}
    bool getArduinoReadTemperatureAndRelay() { return settings->value("arduino/temperature", false).toBool();}
    void setArduinoReadTemperatureAndRelay(bool b) { settings->setValue("arduino/temperature", b);}
    bool getArduinoReadDHT20() { return settings->value("arduino/dht20", false).toBool();}
    void setArduinoReadDHT20(bool b) { settings->setValue("arduino/dht20", b);}
    bool getArduinoEnable() { return settings->value("arduino/enable", false).toBool();}
    void setArduinoEnable(bool b) { settings->setValue("arduino/enable", b);}
    QString getArduinoRelayOnText() { return settings->value("arduino/relayOnText", "on").toString(); }
    void setArduinoRelayOnText(QString s) { settings->setValue("arduino/relayOnText", s);}
    QString getArduinoRelayOffText() { return settings->value("arduino/relayOffText", "off").toString(); }
    void setArduinoRelayOffText(QString s) { settings->setValue("arduino/relayOffText", s);}
    bool getArduinoWatchdogRelay() { return settings->value("arduino/watchdogRelay", false).toBool();}
    void setArduinoWatchdogRelay(bool b) { settings->setValue("arduino/watchdogRelay", b);}
    bool getArduinoActivateWatchdog() { return settings->value("arduino/activateWatchdog", false).toBool();}
    void setArduinoActivateWatchdog(bool b) { settings->setValue("arduino/activateWatchdog", b);}
    QString getArduinoPingAddress() { return settings->value("arduino/pingAddress", "").toString();}
    void setArduinoPingAddress(QString s) { settings->setValue("arduino/pingAddress", s);}
    int getArduinoPingInterval() { return settings->value("arduino/pingInterval", 60).toInt();}
    void setArduinoPingInterval(int i) { settings->setValue("arduino/pingInterval", i);}

    // AutoRecorder options
    bool getAutoRecorderActivate() { return settings->value("autorecorder/activate", false).toBool();}
    void setAutoRecorderActivate(bool b) { settings->setValue("autorecorder/activate", b);}

    // Window specific settings
    int getPlotYMax() { return settings->value("plot/YMax", 50).toInt();}
    void setPlotYMax(int val) { settings->setValue("plot/YMax", val); emit settingsUpdated();}
    int getPlotYMin() { return settings->value("plot/YMin", -30).toInt();}
    void setPlotYMin(int val) { settings->setValue("plot/YMin", val);}
    QByteArray getWindowGeometry() { return settings->value("windowGeometry", "").toByteArray();}
    void setWindowGeometry(QByteArray arr) { settings->setValue("windowGeometry", arr);}
    QByteArray getWindowState() { return settings->value("windowState", "").toByteArray();}
    void setWindowState(QByteArray arr)  { settings->setValue("windowState", arr);}
    QByteArray getArduinoWindowState() { return settings->value("arduino/windowState", "").toByteArray();}
    void setArduinoWindowState(QByteArray arr)  { settings->setValue("arduino/windowState", arr);}
    int getPlotMaxholdTime() { return settings->value("plot/MaxholdTime", 30).toUInt();}
    void setPlotMaxholdTime(int val) { settings->setValue("plot/MaxholdTime", val);}
    QString getShowWaterfall() { return settings->value("plot/ShowWaterfall", "Off").toString();}
    void setShowWaterfall(QString b) { settings->setValue("plot/ShowWaterfall", b);}
    int getWaterfallTime() { return settings->value("plot/WaterfallTime", 120).toInt();}
    void setWaterfallTime(int val) { settings->setValue("plot/WaterfallTime", val);}
    int getPlotResolution() { return plotResolution;}

    // trig line
    QStringList getTrigFrequencies() { return settings->value("trigFrequencies").toStringList();}
    void setTrigFrequencies(QStringList list) { settings->setValue("trigFrequencies", list); emit settingsUpdated();}

    //internal
    void newFileName(const QString file);
    QString getCurrentFilename() { return curFile;}

    // PMR
    bool getPmrUseReds() { return settings->value("pmr/useReds", true).toBool();}
    void setPmrUseReds(bool b) { settings->setValue("pmr/useReds", b);}
    bool getPmrUseYellows() { return settings->value("pmr/useYellows", true).toBool();}
    void setPmrUseYellows(bool b) { settings->setValue("pmr/useYellows", b);}
    bool getPmrUseGreens() { return settings->value("pmr/useGreens", true).toBool();}
    void setPmrUseGreens(bool b) { settings->setValue("pmr/useGreens", b);}
    bool getPmrUseWhites() { return settings->value("pmr/useWhites", true).toBool();}
    void setPmrUseWhites(bool b) { settings->setValue("pmr/useWhites", b);}

    // Position report
    bool getPosReportActivated() { return settings->value("posReport/activated", false).toBool();}
    void setPosReportActivated(bool b) { settings->setValue("posReport/activated", b); emit settingsUpdated();}
    QString getPosReportSource() { return settings->value("posReport/source", "InstrumentGNSS").toString();}
    void setPosReportSource(QString s) { settings->setValue("posReport/source", s);}
    QString getPosReportUrl() { return settings->value("posReport/url", "").toString();}
    void setPosReportUrl(QString s) { settings->setValue("posReport/url", s);}
    int getPosReportSendInterval() { return settings->value("posReport/sendInterval", 60).toInt();}
    void setPosReportSendInterval(int i) { settings->setValue("posReport/sendInterval", i);}
    bool getPosReportAddPos() { return settings->value("posReport/addPos", true).toBool();}
    void setPosReportAddPos(bool b) { settings->setValue("posReport/addPos", b);}
    bool getPosReportAddSogCog() { return settings->value("posReport/addSogCog", true).toBool();}
    void setPosReportAddSogCog(bool b) { settings->setValue("posReport/addSogCog", b);}
    bool getPosReportAddGnssStats() { return settings->value("posReport/addGnssStats", true).toBool();}
    void setPosreportAddGnssStats(bool b) { settings->setValue("posReport/addGnssStats", b);}
    bool getPosReportAddConnStats() { return settings->value("posReport/addConnStats", true).toBool();}
    void setPosreportAddConnStats(bool b) { settings->setValue("posReport/addConnStats", b);}
    QString getPosReportId() { return settings->value("posReport/id", "").toString();}
    void setPosReportId(QString s) { settings->setValue("posReport/id", s);}
    bool getPosReportAddSensorData() { return settings->value("posReport/addSensorData", true).toBool();}
    void setPosreportAddSensorData(bool b) { settings->setValue("posReport/addSensorData", b);}

    // Geo limiting
    bool getGeoLimitActive() { return settings->value("geoLimit/activated", false).toBool();}
    void setGeoLimitActive(bool b) { settings->setValue("geoLimit/activated", b); emit settingsUpdated();}
    QString getGeoLimitFilename() { return settings->value("geoLimit/filename").toString();}
    void setGeoLimitFilename(QString s) { settings->setValue("geoLimit/filename", s);}

    // MQTT options
    bool getMqttActivate() { return settings->value("mqtt/activated", false).toBool();}
    void setMqttActivate(bool b) { settings->setValue("mqtt/activated", b); emit settingsUpdated();}
    QString getMqttServer() { return settings->value("mqtt/server", "").toString();}
    void setMqttServer(QString s) { settings->setValue("mqtt/server", s);}
    QString getMqttUsername() { return settings->value("mqtt/username", "").toString();}
    void setMqttUsername(QString s) { settings->setValue("mqtt/username", s);}
    QString getMqttPassword() { return simpleEncr(settings->value("mqtt/password", "").toByteArray());}
    void setMqttPassword(QString s) { settings->setValue("mqtt/password", simpleEncr(s.simplified().toLocal8Bit()));}
    int getMqttPort() { return settings->value("mqtt/port", 1883).toInt();}
    void setMqttPort(int i) { settings->setValue("mqtt/port", i);}
    QStringList getMqttSubNames() { return settings->value("mqtt/subNames").toStringList();}
    void setMqttSubNames(QStringList l) { settings->setValue("mqtt/subNames", l);}
    QStringList getMqttSubTopics() { return settings->value("mqtt/subTopics").toStringList();}
    void setMqttSubTopics(QStringList l) { settings->setValue("mqtt/subTopics", l);}

    QString getMqttSub1Name() { return settings->value("mqtt/sub1Name", "").toString();}
    void setMqttSub1Name(QString s) { settings->setValue("mqtt/sub1Name", s);}
    QString getMqttSub1Topic() { return settings->value("mqtt/sub1Topic", "").toString();}
    void setMqttSub1Topic(QString s) { settings->setValue("mqtt/sub1Topic", s);}
    QString getMqttSub2Name() { return settings->value("mqtt/sub2Name", "").toString();}
    void setMqttSub2Name(QString s) { settings->setValue("mqtt/sub2Name", s);}
    QString getMqttSub2Topic() { return settings->value("mqtt/sub2Topic", "").toString();}
    void setMqttSub2Topic(QString s) { settings->setValue("mqtt/sub2Topic", s);}
    QString getMqttSub3Name() { return settings->value("mqtt/sub3Name", "").toString();}
    void setMqttSub3Name(QString s) { settings->setValue("mqtt/sub3Name", s);}
    QString getMqttSub3Topic() { return settings->value("mqtt/sub3Topic", "").toString();}
    void setMqttSub3Topic(QString s) { settings->setValue("mqtt/sub3Topic", s);}
    QString getMqttSub4Name() { return settings->value("mqtt/sub4Name", "").toString();}
    void setMqttSub4Name(QString s) { settings->setValue("mqtt/sub4Name", s);}
    QString getMqttSub4Topic() { return settings->value("mqtt/sub4Topic", "").toString();}
    void setMqttSub4Topic(QString s) { settings->setValue("mqtt/sub4Topic", s);}
    QString getMqttSub5Name() { return settings->value("mqtt/sub5Name", "").toString();}
    void setMqttSub5Name(QString s) { settings->setValue("mqtt/sub5Name", s);}
    QString getMqttSub5Topic() { return settings->value("mqtt/sub5Topic", "").toString();}
    void setMqttSub5Topic(QString s) { settings->setValue("mqtt/sub5Topic", s);}

    QString getMqttKeepaliveTopic() { return settings->value("mqtt/keepaliveTopic", "").toString();}
    void setMqttKeepaliveTopic(QString s) { settings->setValue("mqtt/keepaliveTopic", s);}

private slots:
    QByteArray simpleEncr(QByteArray);

private:
    QString basicFile = findWorkFolderName() + "/hauken.conf";
    QSettings *basicSettings = new QSettings(basicFile, QSettings::IniFormat);
    QString curFile = basicSettings->value("lastFile", "default.ini").toString();
    QSettings *settings = new QSettings(curFile, QSettings::IniFormat);
    bool ready = false;
    QString measurementDeviceName;
    const int plotResolution = 1200;
};

#endif // CONFIG_H
