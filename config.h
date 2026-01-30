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
#include <QFileInfo>

class Config : public QObject
{
    Q_OBJECT
public:
    explicit Config(QObject *parent = nullptr);
    QByteArray simpleEncr(QByteArray);
    QSettings *settings;

signals:
    void settingsUpdated();

public slots:
    // meas. device data
    quint64 getInstrStartFreq() { return settings->value("instr/StartFreq", 1.56e9).toULongLong();}
    void setInstrStartFreq(quint64 val) { settings->setValue("instr/StartFreq", val); emit settingsUpdated();}
    quint64 getInstrStopFreq() { return settings->value("instr/StopFreq", 1.61e9).toULongLong();}
    void setInstrStopFreq(quint64 val) { settings->setValue("instr/StopFreq", val); emit settingsUpdated();}
    QString getInstrResolution() { return settings->value("instr/Resolution", 1).toString(); }
    void setInstrResolution(QString f) { if (!f.isEmpty()) settings->setValue("instr/Resolution", f); emit settingsUpdated();}
    quint64 getInstrFfmCenterFreq() { return settings->value("instr/FfmCenterFreq", 1560).toULongLong();}
    void setInstrFfmCenterFreq(quint64 f) { settings->setValue("instr/FfmCenterFreq", f); emit settingsUpdated();}
    QString getInstrFfmSpan() { return settings->value("instr/FfmSpan", "2000").toString(); emit settingsUpdated();}
    void setInstrFfmSpan(QString s) { settings->setValue("instr/FfmSpan", s); emit settingsUpdated(); emit settingsUpdated();}
    int getInstrMeasurementTime() { return settings->value("instr/MeasurementTime", 18).toInt();}
    void setInstrMeasurementTime(int val) { settings->setValue("instr/MeasurementTime", val); emit settingsUpdated();}
    int getInstrManAtt() { return settings->value("instr/ManAtt", 0).toInt(); }
    void setInstrManAtt(int val) { settings->setValue("instr/ManAtt", val); emit settingsUpdated();}
    bool getInstrAutoAtt() { return settings->value("instr/AutoAtt", true).toBool(); }
    void setInstrAutoAtt(bool b) { settings->setValue("instr/AutoAtt", b); emit settingsUpdated();}
    int getInstrAntPort() { return settings->value("instr/AntPort", "Default").toUInt(); }
    void setInstrAntPort(int index) { settings->setValue("instr/AntPort", QString::number(index)); emit settingsUpdated();}
    QString getInstrMode() { return settings->value("instr/Mode", "PScan").toString(); }
    void setInstrMode(QString val) { settings->setValue("instr/Mode", val); emit settingsUpdated();}
    QString getInstrFftMode() { return settings->value("instr/FftMode", 0).toString(); }
    void setInstrFftMode(QString val) { if (!val.isEmpty()) settings->setValue("instr/FftMode", val); emit settingsUpdated();}
    QString getInstrIpAddr() { return settings->value("instr/IpAddr", "").toString(); }
    void setInstrIpAddr(QString str) { settings->setValue("instr/IpAddr", str); emit settingsUpdated();}
    int getInstrPort() { return settings->value("instr/Port", 5555).toInt(); }
    void setInstrPort(int val) { settings->setValue("instr/Port", val); emit settingsUpdated();}
    QString getInstrId() { return settings->value("instr/Id").toString();}
    void setInstrId(QString s) { settings->setValue("instr/Id", s); emit settingsUpdated();}
    QString getMeasurementDeviceName() { return measurementDeviceName;}
    void setMeasurementDeviceName(QString s) { measurementDeviceName = s; emit settingsUpdated();}
    int getInstrGainControl() { return settings->value("instr/gainControl", 1).toInt(); }
    void setInstrGainControl(int i) { settings->setValue("instr/gainControl", i); emit settingsUpdated(); }
    QString getInstrCustomEntry() { return settings->value("instr/customEntry", "").toString();}
    void setInstrCustomEntry(QString s) { settings->setValue("instr/customEntry", s);}
    bool getInstrRestoreAvgLevels() { return settings->value("instr/restoreAvgLevels", true).toBool(); }
    void setInstrRestoreAvgLevels(bool b) { settings->setValue("instr/restoreAvgLevels", b); }
    QVariantList getInstrAvgLevels() { return settings->value("instr/avgLevels").toList(); }
    void setInstrAvgLevels(QVariantList l) { settings->setValue("instr/avgLevels", l); }

    // spectrum criterias
    int getInstrTrigLevel() { return settings->value("instr/TrigLevel", 15.0).toInt(); }
    void setInstrTrigLevel(int val) { settings->setValue("instr/TrigLevel", val); emit settingsUpdated(); }
    int getInstrMinTrigBW() { return settings->value("instr/MinTrigBW", 50).toInt(); }
    void setInstrMinTrigBW(int val) { settings->setValue("instr/MinTrigBW", val); emit settingsUpdated();}
    int getInstrTotalTrigBW() { return settings->value("instr/TotalTrigBW", 500).toInt();}
    void setInstrTotalTrigBW(int val) { settings->setValue("instr/TotalTrigBW", val); emit settingsUpdated();}
    int getInstrMinTrigTime() { return settings->value("instr/MinTrigTime", 18).toInt(); }
    void setInstrMinTrigTime(int val) { settings->setValue("instr/MinTrigTime", val); emit settingsUpdated();}

    // GNSS criterias
    int getGnssCnoDeviation() { return settings->value("gnss/CnoDeviation", 15).toInt(); }
    void setGnssCnoDeviation(double val) { settings->setValue("gnss/CnoDeviation", val); emit settingsUpdated(); }
    int getGnssAgcDeviation() { return settings->value("gnss/AgcDeviation", 0).toInt(); }
    void setGnssAgcDeviation(int val) { settings->setValue("gnss/AgcDeviation", val); emit settingsUpdated();}
    int getGnssPosOffset() { return settings->value("gnss/PosOffset", 0).toInt(); }
    void setGnssPosOffset(int val) { settings->setValue("gnss/PosOffset", val); emit settingsUpdated();}
    int getGnssAltOffset() { return settings->value("gnss/AltOffset", 0).toInt(); }
    void setGnssAltOffset(int val) { settings->setValue("gnss/AltOffset", val); emit settingsUpdated();}
    int getGnssTimeOffset() { return settings->value("gnss/TimeOffset", 0).toInt(); }
    void setGnssTimeOffset(int val) { settings->setValue("gnss/TimeOffset", val); emit settingsUpdated();}
    int getGnssTimeFilter() { return settings->value("gnss/timeFilter", 10).toInt(); }
    void setGnssTimeFilter(int val) { settings->setValue("gnss/timeFilter", val); }

    // General options
    QString getStationName() { return settings->value("station/Name", "").toString(); }
    void setStationName(QString s) { settings->setValue("station/Name", s);  }
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
    void setPmrMode(bool b) { settings->setValue("pmr/mode", b);  }
    QByteArray getRestKey() { return simpleEncr(settings->value("restKey").toByteArray()); }
    QByteArray getRestSecret() { return simpleEncr(settings->value("restSecret").toByteArray()); }

    QString getIpAddressServer() { return settings->value("ipAddressServer", "").toString(); }
    void setIpAddressServer(QString s) { settings->setValue("ipAddressServer", s); }
    bool getUseDbm() { return settings->value("useDbm", false).toBool();}
    void setUseDbm(bool b) { settings->setValue("useDbm", b); }
    bool getDarkMode() { return settings->value("darkMode", false).toBool(); }
    void setDarkMode(bool b) { settings->setValue("darkMode", b); }
    int getCorrValue() { return settings->value("corrValue", 0).toDouble();}
    void setCorrValue(double d) { settings->setValue("corrValue", d); }
    bool getSeparatedWindows() { return settings->value("separatedWindows", false).toBool();}
    void setSeparatedWindows(bool b) { settings->setValue("separatedWindows", b); }
    int getOverlayFontSize() { return settings->value("overlayFontSize", 12).toInt();}
    void setOverlayFontSize(int i) { settings->setValue("overlayFontSize", i); }

    // Window settings
    bool getShowReceiverControls() { return settings->value("showReceiverControls", true).toBool();}
    void setShowReceiverControls(bool b) { settings->setValue("showReceiverControls", b); }
    bool getShowTriggerSettings() { return settings->value("showTriggerSettings", true).toBool();}
    void setShowTriggerSettings(bool b) { settings->setValue("showTriggerSettings", b); }
    bool getShowStatusIndicators() { return settings->value("showStatusIndicators", true).toBool();}
    void setShowStatusIndicators(bool b) { settings->setValue("showStatusIndicators", b); }
    bool getShowGnssStatusWindow() { return settings->value("showGnssStatusWindow", true).toBool();}
    void setShowGnssStatusWindow(bool b) { settings->setValue("showGnssStatusWindow", b); }
    bool getShowIncidentLog() { return settings->value("showIncidentLog", true).toBool();}
    void setShowIncidentLog(bool b) { settings->setValue("showIncidentLog", b); }

    // Receiver options
    bool getInstrConnectOnStartup() { return settings->value("instr/ConnectOnStartup", false).toBool(); }
    void setInstrConnectOnStartup(bool b) { settings->setValue("instr/ConnectOnStartup", b);  }
    bool getInstrUseTcpDatastream() { return settings->value("instr/UseTcpDatastream", true).toBool(); }
    void setInstrUseTcpDatastream(bool b) { settings->setValue("instr/UseTcpDatastream", b);  }
    bool getInstrAutoReconnect() { return settings->value("instr/AutoReconnect", true).toBool(); }
    void setInstrAutoReconnect(bool b) { settings->setValue("instr/AutoReconnect", b);  }
    bool getInstrNormalizeSpectrum() { return settings->value("instr/NormalizeSpectrum", false).toBool(); }
    void setInstrNormalizeSpectrum(bool b) { settings->setValue("instr/NormalizeSpectrum", b);  }
    int getInstrTracesNeededForAverage() { return settings->value("instr/tracesNeededForAverage", 250).toInt();}
    void setInstrTracesNeededForAverage(int i) { settings->setValue("instr/tracesNeededForAverage", i);  }

    // IQ options
    bool getIqSaveToFile() { return settings->value("iq/saveToFile", false).toBool();}
    void setIqSaveToFile(bool b) { settings->setValue("iq/saveToFile", b);}
    bool getIqCreateFftPlot() { return settings->value("iq/createFftPlot", true).toBool();}
    void setIqCreateFftPlot(bool b) { settings->setValue("iq/createFftPlot", b); }
    int getIqFftPlotLength() { return settings->value("iq/fftPlotLength", 1000).toInt();}
    void setIqFftPlotLength(int i) { settings->setValue("iq/fftPlotLength", i); }
    int getIqFftPlotBw() { return settings->value("iq/fftPlotBw", 40000).toInt();}
    void setIqFftPlotBw(int i) { settings->setValue("iq/fftPlotBw", i); }
    double getIqLogTime() { return settings->value("iq/logTime", 0.5).toDouble();}
    void setIqLogTime(double d) { settings->setValue("iq/logTime", d);}
    bool getIqUseDB() { return settings->value("iq/useDB", false).toBool();}
    void setIqUseDB(bool b) { settings->setValue("iq/useDB", b);}
    bool getIqUseWindow() { return settings->value("iq/useWindow", true).toBool(); }
    void setIqUseWindow(bool b) { settings->setValue("iq/useWindow", b);  }
    bool getIqRecordMultipleBands() { return settings->value("iq/recordMultipleBands", false).toBool(); }
    void setIqRecordMultipleBands(bool b) { settings->setValue("iq/recordMultipleBands", b); }
    QList<double> getIqMultibandCenterFreqs();
    bool getIqRecordAllTrigArea() { return settings->value("iq/recordAllTrigArea", false).toBool();}
    void setIqRecordAllTrigArea(bool b) { settings->setValue("iq/recordAllTrigArea", b);}
    bool getIqUseAvgForPlot() { return settings->value("iq/useAvgForPlot", true).toBool();}
    void setIqUseAvgForPlot(bool b) { settings->setValue("iq/useAvgForPlot", b);}
    bool getIqSaveAs16bit() { return settings->value("iq/saveAs16bit", false).toBool();}
    void setIqSaveAs16bit(bool b) { settings->setValue("iq/saveAs16bit", b);}

    // 1809 options
    bool getSdefSaveToFile() { return settings->value("sdef/SaveToFile", true).toBool();}
    void setSdefSaveToFile(bool b) { settings->setValue("sdef/SaveToFile", b); }
    bool getSdefUploadFile() { return settings->value("sdef/UploadFile", false).toBool();}
    void setSdefUploadFile(bool b) { settings->setValue("sdef/UploadFile", b); }
    QString getSdefUsername() { return settings->value("sdef/Username", "").toString().trimmed();}
    void setSdefUsername(QString s) { settings->setValue("sdef/Username", s); }
    QString getSdefPassword() { return simpleEncr(settings->value("sdef/Password", "").toByteArray());}
    void setSdefPassword(QString s) { settings->setValue("sdef/Password", simpleEncr(s.toLocal8Bit())); }
    bool getSdefAddPosition() { return settings->value("sdef/AddPosition", false).toBool();}
    void setSdefAddPosition(bool b) { settings->setValue("sdef/AddPosition", b); }
    QString getSdefGpsSource() { return settings->value("sdef/GpsSource").toString();}
    void setSdefGpsSource(QString s) { settings->setValue("sdef/GpsSource", s); }
    int getSdefRecordTime() { return settings->value("sdef/RecordTime", 2).toUInt();}
    void setSdefRecordTime(int val) { settings->setValue("sdef/RecordTime", val); }
    int getSdefMaxRecordTime() { return settings->value("sdef/MaxRecordTime", 60).toUInt();}
    void setSdefMaxRecordTime(int val) { settings->setValue("sdef/MaxRecordTime", val); }
    QString getSdefStationInitals() { return settings->value("sdef/StationInitials", "").toString().trimmed();}
    void setSdefStationInitials(QString s) { settings->setValue("sdef/StationInitials", s); }
    int getSdefPreRecordTime() { return settings->value("sdef/PreRecordTime", 30).toUInt();}
    void setSdefPreRecordTime(int val) { settings->setValue("sdef/PreRecordTime", val); }
    bool getSdefZipFiles() { return settings->value("sdef/zipFiles", true).toBool();}
    void setSdefZipFiles(bool b) { settings->setValue("sdef/zipFiles", b); }
    QString getSdefServer() { return settings->value("sdef/Server").toString().trimmed();}
    void setSdefServer(QString s) { settings->setValue("sdef/Server", s); }
    QString getSdefAuthAddress() { return settings->value("sdef/AuthAddress").toString().trimmed();}
    void setSdefAuthAddress(QString s) { settings->setValue("sdef/AuthAddress", s); }
    bool getSdefNewMsFormat() { return true;} // Forced on since oct-25
    void setSdefNewMsFormat(bool b) { settings->setValue("sdef/newMsFormat", b); }

    // OAuth2 options
    bool getOauth2Enable() { return settings->value("OAuth2/enable").toBool();}
    void setOAuth2Enable(bool b) { settings->setValue("OAuth2/enable", b); }
    QString getOAuth2AuthUrl() { return settings->value("OAuth2/AuthUrl").toString();}
    void setOAuth2AuthUrl(QString s) { settings->setValue("OAuth2/AuthUrl", s); }
    QString getOAuth2AccessTokenUrl() { return settings->value("OAuth2/AccessTokenUrl").toString();}
    void setOAuth2AccessTokenUrl(QString s) { settings->setValue("OAuth2/AccessTokenUrl", s); }
    QString getOAuth2ClientId() { return settings->value("OAuth2/ClientId").toString();}
    void setOAuth2ClientId(QString s) { settings->setValue("OAuth2/ClientId", s); }
    QString getOAuth2Scope() { return settings->value("OAuth2/Scope").toString();}
    void setOAuth2Scope(QString s) { settings->setValue("OAuth2/Scope", s); }
    QString getOauth2UploadAddress() { return settings->value("OAuth2/UploadAddress").toString();}
    void setOAuth2UploadAddress(QString s) { settings->setValue("OAuth2/UploadAddress", s); }
    QString getOauth2OperatorAddress() { return settings->value("OAuth2/OperatorAddress").toString();}
    void setOAuth2OperatorAddress(QString s) { settings->setValue("OAuth2/OperatorAddress", s); }

    // GNSS options
    QString getGnssSerialPort1Name() { return settings->value("gnss/SerialPort1Name").toString();}
    void setGnssSerialPort1Name(QString s) { settings->setValue("gnss/SerialPort1Name", s); }
    QString getGnssSerialPort1Baudrate() { return settings->value("gnss/SerialPort1Baudrate", "4800").toString();}
    void setGnssSerialPort1Baudrate(QString s) { settings->setValue("gnss/SerialPort1Baudrate", s); }
    bool getGnssSerialPort1Activate() { return settings->value("gnss/SerialPort1activate", false).toBool();}
    void setGnssSerialPort1Activate(bool b) { settings->setValue("gnss/SerialPort1activate", b); }
    bool getGnssSerialPort1LogToFile() { return settings->value("gnss/SerialPort1LogToFile", false).toBool();}
    void setGnssSerialPort1LogToFile(bool b) { settings->setValue("gnss/SerialPort1LogToFile", b); }
    bool getGnssSerialPort1MonitorAgc() { return settings->value("gnss/SerialPort1MonitorAgc", false).toBool();}
    void setGnssSerialPort1MonitorAgc(bool b) { settings->setValue("gnss/SerialPort1MonitorAgc", b); }
    bool getGnssSerialPort1TriggerRecording() { return settings->value("gnss/SerialPort1TriggerRecording", false).toBool();}
    void setGnssSerialPort1TriggerRecording(bool b) { settings->setValue("gnss/SerialPort1TriggerRecording", b); }

    QString getGnssSerialPort2Name() { return settings->value("gnss/SerialPort2Name").toString();}
    void setGnssSerialPort2Name(QString s) { settings->setValue("gnss/SerialPort2Name", s); }
    QString getGnssSerialPort2Baudrate() { return settings->value("gnss/SerialPort2Baudrate", "4800").toString();}
    void setGnssSerialPort2Baudrate(QString s) { settings->setValue("gnss/SerialPort2Baudrate", s); }
    bool getGnssSerialPort2Activate() { return settings->value("gnss/SerialPort2activate", false).toBool();}
    void setGnssSerialPort2Activate(bool b) { settings->setValue("gnss/SerialPort2activate", b); }
    bool getGnssSerialPort2LogToFile() { return settings->value("gnss/SerialPort2LogToFile", false).toBool();}
    void setGnssSerialPort2LogToFile(bool b) { settings->setValue("gnss/SerialPort2LogToFile", b); }
    bool getGnssSerialPort2MonitorAgc() { return settings->value("gnss/SerialPort2MonitorAgc", false).toBool();}
    void setGnssSerialPort2MonitorAgc(bool b) { settings->setValue("gnss/SerialPort2MonitorAgc", b); }
    bool getGnssSerialPort2TriggerRecording() { return settings->value("gnss/SerialPort2TriggerRecording", false).toBool();}
    void setGnssSerialPort2TriggerRecording(bool b) { settings->setValue("gnss/SerialPort2TriggerRecording", b); }

    bool getGnssUseInstrumentGnss() { return settings->value("gnss/UseInstrumentGnss", false).toBool();}
    void setGnssUseInstrumentGnss(bool b) { settings->setValue("gnss/UseInstrumentGnss", b); }
    bool getGnssInstrumentGnssTriggerRecording() { return settings->value("gnss/InstrumentGnssTriggerRecording", false).toBool();}
    void setGnssInstrumentGnssTriggerRecording(bool b) { settings->setValue("gnss/InstrumentGnssTriggerRecording", b); }

    bool getGnssDisplayWidget() { return settings->value("gnss/displayWidget", false).toBool();}
    void setGnssDisplayWidget(bool b) { settings->setValue("gnss/displayWidget", b); }
    QString getGnss1Name() { return settings->value("gnss/gnss1Name", "").toString();}
    void setGnss1Name(QString s) { settings->setValue("gnss/gnss1Name", s); }
    QString getGnss2Name() { return settings->value("gnss/gnss2Name", "").toString();}
    void setGnss2Name(QString s) { settings->setValue("gnss/gnss2Name", s); }
    bool getGnssShowNotifications() { return settings->value("gnss/showNotifications", true).toBool();}
    void setGnssShowNotifications(bool b) { settings->setValue("gnss/showNotifications", b); }

    // Email/notification options
    QString getEmailSmtpServer() { return settings->value("email/SmtpServer").toString().trimmed();}
    void setEmailSmtpServer(QString s) { settings->setValue("email/SmtpServer", s.simplified()); }
    QString getEmailSmtpPort() { return settings->value("email/SmtpPort", "25").toString().trimmed();}
    void setEmailSmtpPort(QString s) { settings->setValue("email/SmtpPort", s); }
    QString getEmailRecipients() { return settings->value("email/Recipients").toString().trimmed();}
    void setEmailRecipients(QString s) { settings->setValue("email/Recipients", s); }
    bool getEmailNotifyMeasurementDeviceHighLevel() { return settings->value("email/NotifyMeasurementDeviceHighLevel", false).toBool();}
    void setEmailNotifyMeasurementDeviceHighLevel(bool b) { settings->setValue("email/NotifyMeasurementDeviceHighLevel", b); }
    bool getEmailNotifyMeasurementDeviceDisconnected() { return settings->value("email/NotifyMeasurementDeviceDisconnected", false).toBool();}
    void setEmailNotifyMeasurementDeviceDisconnected(bool b) { settings->setValue("email/NotifyMeasurementDeviceDisconnected", b); }
    bool getEmailNotifyGnssIncidents() { return settings->value("email/NotifyGnssIncidents", false).toBool();}
    void setEmailNotifyGnssIncidents(bool b) { settings->setValue("email/NotifyGnssIncidents", b); }
    int getEmailMinTimeBetweenEmails() { return settings->value("email/MinTimeBetweenEmails", 3600).toInt();}
    void setEmailMinTimeBetweenEmails(int s) { settings->setValue("email/MinTimeBetweenEmails", s); }
    int getNotifyTruncateTime() { return settings->value("notify/TruncateTime", 30).toInt();}
    void setNotifyTruncateTime(int s) { settings->setValue("notify/TruncateTime", s); }
    QString getEmailFromAddress() { return settings->value("email/FromAddress").toString().trimmed();}
    void setEmailFromAddress(QString s) { settings->setValue("email/FromAddress", s.simplified()); }
    QString getEmailSmtpUser() { return settings->value("email/SmtpUser").toString().trimmed();}
    void setEmailSmtpUser(QString s) { settings->setValue("email/SmtpUser", s.simplified()); }
    QString getEmailSmtpPassword() { return simpleEncr(settings->value("email/smtpPassword").toByteArray());}
    void setEmailSmtpPassword(QString s) { settings->setValue("email/smtpPassword", simpleEncr(s.simplified().toLocal8Bit())); }
    bool getEmailAddImages() { return settings->value("email/AddImages", true).toBool();}
    void setEmailAddImages(bool b) { settings->setValue("email/AddImages", b); }
    int getEmailDelayBeforeAddingImages() { return settings->value("email/DelayBeforeAddingImages", 10).toInt();}
    void setEmailDelayBeforeAddingImages(int val) { settings->setValue("email/DelayBeforeAddingImages", val); }
    QString getEmailGraphApplicationId() { return settings->value("email/graphApplicationId", "").toString().trimmed(); }
    void setEmailGraphApplicationId(QString s) { settings->setValue("email/graphApplicationId", s.simplified()); }
    QString getEmailGraphTenantId() { return settings->value("email/graphTenantId", "").toString().trimmed(); }
    void setEmailGraphTenantId(QString s) { settings->setValue("email/graphTenantId", s.simplified()); }
    QString getEmailGraphSecret() { return simpleEncr(settings->value("email/graphSecret", "").toByteArray()).trimmed();}
    void setEmailGraphSecret(QString s) { settings->setValue("email/graphSecret", simpleEncr(s.simplified().toLocal8Bit())); }
    bool getSoundNotification() { return settings->value("email/soundNotification", false).toBool();}
    void setSoundNotification(bool b) { settings->setValue("email/soundNotification", b); }
    QString getEmailFilteredRecipients() { return settings->value("email/filteredRecipients", "").toString();}
    void setEmailFilteredRecipients(QString s) { settings->setValue("email/filteredRecipients", s.trimmed()); }
    int getEmailJammerProbabilityFilter() { return settings->value("email/jammerProbabilityFilter", 60).toInt();}
    void setEmailJammerProbabilityFilter(int i) { settings->setValue("email/jammerProbabilityFilter", i); }
    bool getNotificationLargeFonts() { return settings->value("notify/LargeFonts", false).toBool();}
    void setNotificationLargeFonts(bool b) { settings->setValue("notify/LargeFonts", b); }

    // Camera options
    QString getCameraName() { return settings->value("camera/Name").toString();}
    void setCameraName(QString s) { settings->setValue("camera/Name", s); }
    QString getCameraStreamAddress() { return settings->value("camera/StreamAddress", "first").toString();}
    void setCameraStreamAddress(QString s) { settings->setValue("camera/StreamAddress", s); }
    bool getCameraDeviceTrigger() { return settings->value("camera/DeviceTrigger").toBool();}
    void setCameraDeviceTrigger(bool b) { settings->setValue("camera/DeviceTrigger", b); }
    int getCameraRecordTime() { return settings->value("camera/RecordTime", 60).toInt();}
    void setCameraRecordTime(int s) { settings->setValue("camera/RecordTime", s); }

    // Relay control (and temp) via serial (Arduino custom)
    QString getArduinoSerialName() { return settings->value("arduino/serialName", "").toString();}
    void setArduinoSerialName(QString s) { settings->setValue("arduino/serialName", s); }
    QString getArduinoBaudrate() { return settings->value("arduino/baudrate").toString();}
    void setArduinoBaudrate(QString s) { settings->setValue("arduino/baudrate", s); }
    bool getArduinoReadTemperatureAndRelay() { return settings->value("arduino/temperature", false).toBool();}
    void setArduinoReadTemperatureAndRelay(bool b) { settings->setValue("arduino/temperature", b); }
    bool getArduinoReadDHT20() { return settings->value("arduino/dht20", false).toBool();}
    void setArduinoReadDHT20(bool b) { settings->setValue("arduino/dht20", b); }
    bool getArduinoEnable() { return settings->value("arduino/enable", false).toBool();}
    void setArduinoEnable(bool b) { settings->setValue("arduino/enable", b); }
    QString getArduinoRelayOnText() { return settings->value("arduino/relayOnText", "on").toString(); }
    void setArduinoRelayOnText(QString s) { settings->setValue("arduino/relayOnText", s); }
    QString getArduinoRelayOffText() { return settings->value("arduino/relayOffText", "off").toString(); }
    void setArduinoRelayOffText(QString s) { settings->setValue("arduino/relayOffText", s); }
    bool getArduinoWatchdogRelay() { return settings->value("arduino/watchdogRelay", false).toBool();}
    void setArduinoWatchdogRelay(bool b) { settings->setValue("arduino/watchdogRelay", b); }
    bool getArduinoActivateWatchdog() { return settings->value("arduino/activateWatchdog", false).toBool();}
    void setArduinoActivateWatchdog(bool b) { settings->setValue("arduino/activateWatchdog", b); }
    QString getArduinoPingAddress() { return settings->value("arduino/pingAddress", "").toString();}
    void setArduinoPingAddress(QString s) { settings->setValue("arduino/pingAddress", s); }
    int getArduinoPingInterval() { return settings->value("arduino/pingInterval", 60).toInt();}
    void setArduinoPingInterval(int i) { settings->setValue("arduino/pingInterval", i); }

    // AutoRecorder options
    bool getAutoRecorderActivate() { return settings->value("autorecorder/activate", false).toBool();}
    void setAutoRecorderActivate(bool b) { settings->setValue("autorecorder/activate", b); }
    bool getSaveToTempFile()
    {
        return settings->value("autorecorder/saveToTempFile", false).toBool();
    }
    void setSaveToTempFile(bool b)
    {
        settings->setValue("autorecorder/saveToTempFile", b);

    }
    int getSaveToTempFileMaxhold()
    {
        return settings->value("autorecorder/saveToTempFileMaxhold", 0).toInt();
    }
    void setSaveToTempFileMaxhold(int i)
    {
        settings->setValue("autorecorder/saveToTempFileMaxhold", i);

    }

    // Window specific settings
    int getPlotYMax() { return settings->value("plot/YMax", 50).toInt();}
    void setPlotYMax(int val) { settings->setValue("plot/YMax", val); emit settingsUpdated();}
    int getPlotYMin() { return settings->value("plot/YMin", -30).toInt();}
    void setPlotYMin(int val) { settings->setValue("plot/YMin", val); emit settingsUpdated();}
    QByteArray getWindowGeometry() { return settings->value("windowGeometry", "").toByteArray();}
    void setWindowGeometry(QByteArray arr) { settings->setValue("windowGeometry", arr); emit settingsUpdated();}
    QByteArray getWindowState() { return settings->value("windowState", "").toByteArray();}
    void setWindowState(QByteArray arr)  { settings->setValue("windowState", arr); emit settingsUpdated();}
    QByteArray getArduinoWindowState() { return settings->value("arduino/windowState", "").toByteArray();}
    void setArduinoWindowState(QByteArray arr)  { settings->setValue("arduino/windowState", arr); emit settingsUpdated();}
    int getPlotMaxholdTime() { return settings->value("plot/MaxholdTime", 30).toUInt();}
    void setPlotMaxholdTime(int val) { settings->setValue("plot/MaxholdTime", val); emit settingsUpdated();}
    QString getShowWaterfall() { return settings->value("plot/ShowWaterfall", "Off").toString();}
    void setShowWaterfall(QString b) { settings->setValue("plot/ShowWaterfall", b); emit settingsUpdated();}
    int getWaterfallTime() { return settings->value("plot/WaterfallTime", 120).toInt();}
    void setWaterfallTime(int val) { settings->setValue("plot/WaterfallTime", val); emit settingsUpdated();}
    int getPlotResolution() { return plotResolution;}
    QByteArray getGnssDisplayWindowState() { return settings->value("gnssDisplay/windowState", "").toByteArray();}
    void setGnssDisplayWindowState(QByteArray arr)  { settings->setValue("gnssDisplay/windowState", arr); emit settingsUpdated();}


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
    void setPosReportActivated(bool b) { settings->setValue("posReport/activated", b); }
    QString getPosReportSource() { return settings->value("posReport/source", "InstrumentGNSS").toString();}
    void setPosReportSource(QString s) { settings->setValue("posReport/source", s); }
    QString getPosReportUrl() { return settings->value("posReport/url", "").toString().trimmed();}
    void setPosReportUrl(QString s) { settings->setValue("posReport/url", s); }
    int getPosReportSendInterval() { return settings->value("posReport/sendInterval", 60).toInt();}
    void setPosReportSendInterval(int i) { settings->setValue("posReport/sendInterval", i); }
    bool getPosReportAddPos() { return settings->value("posReport/addPos", true).toBool();}
    void setPosReportAddPos(bool b) { settings->setValue("posReport/addPos", b); }
    bool getPosReportAddSogCog() { return settings->value("posReport/addSogCog", true).toBool();}
    void setPosReportAddSogCog(bool b) { settings->setValue("posReport/addSogCog", b); }
    bool getPosReportAddGnssStats() { return settings->value("posReport/addGnssStats", true).toBool();}
    void setPosreportAddGnssStats(bool b) { settings->setValue("posReport/addGnssStats", b); }
    bool getPosReportAddConnStats() { return settings->value("posReport/addConnStats", true).toBool();}
    void setPosreportAddConnStats(bool b) { settings->setValue("posReport/addConnStats", b); }
    QString getPosReportId() { return settings->value("posReport/id", "").toString().trimmed();}
    void setPosReportId(QString s) { settings->setValue("posReport/id", s); }
    bool getPosReportAddSensorData() { return settings->value("posReport/addSensorData", true).toBool();}
    void setPosreportAddSensorData(bool b) { settings->setValue("posReport/addSensorData", b); }
    bool getPosReportAddMqttData() { return settings->value("posReport/addMqttData", true).toBool();}
    void setPosreportAddMqttData(bool b) { settings->setValue("posReport/addMqttData", b); }

    // Geo limiting
    bool getGeoLimitActive() { return settings->value("geoLimit/activated", false).toBool();}
    void setGeoLimitActive(bool b) { settings->setValue("geoLimit/activated", b); }
    QString getGeoLimitFilename() { return settings->value("geoLimit/filename").toString();}
    void setGeoLimitFilename(QString s) { settings->setValue("geoLimit/filename", s); }

    // MQTT options
    bool getMqttActivate() { return settings->value("mqtt/activated", false).toBool();}
    void setMqttActivate(bool b) { settings->setValue("mqtt/activated", b); }
    QString getMqttServer() { return settings->value("mqtt/server", "").toString().trimmed();}
    void setMqttServer(QString s) { settings->setValue("mqtt/server", s); }
    QString getMqttUsername() { return settings->value("mqtt/username", "").toString().trimmed();}
    void setMqttUsername(QString s) { settings->setValue("mqtt/username", s); }
    QString getMqttPassword() { return simpleEncr(settings->value("mqtt/password", "").toByteArray());}
    void setMqttPassword(QString s) { settings->setValue("mqtt/password", simpleEncr(s.simplified().toLocal8Bit())); }
    int getMqttPort() { return settings->value("mqtt/port", 1883).toInt();}
    void setMqttPort(int i) { settings->setValue("mqtt/port", i); }
    QStringList getMqttSubNames() { return settings->value("mqtt/subNames").toStringList();}
    void setMqttSubNames(QStringList l) { settings->setValue("mqtt/subNames", l); }
    QStringList getMqttSubTopics() { return settings->value("mqtt/subTopics").toStringList();}
    void setMqttSubTopics(QStringList l) { settings->setValue("mqtt/subTopics", l); }
    QStringList getMqttSubToIncidentlog() { return settings->value("mqtt/subToIncidentlog", QStringList()).toStringList();}
    void setMqttSubToIncidentlog(QStringList l) { settings->setValue("mqtt/subToIncidentlog", l); }

    QString getMqttWebswitchAddress() { return settings->value("mqtt/webswitchAddress", "").toString().trimmed();}
    void setMqttWebswitchAddress(QString s) { settings->setValue("mqtt/webswitchAddress", s); }

    QString getMqttKeepaliveTopic() { return settings->value("mqtt/keepaliveTopic", "").toString().trimmed();}
    void setMqttKeepaliveTopic(QString s) { settings->setValue("mqtt/keepaliveTopic", s); }

    bool getMqttTestTriggersRecording() { return settings->value("mqtt/testTriggersRecording", false).toBool();}
    void setMqttTestTriggersRecording(bool b) { settings->setValue("mqtt/testTriggersRecording", b);}
    int getMqttSiteFilter() { return settings->value("mqtt/siteFilter", 0).toInt();}
    void setMqttSiteFilter(int i) { settings->setValue("mqtt/siteFilter", i); }

    // Audio options
    bool getAudioActivate() { return settings->value("audio/activate", false).toBool();}
    void setAudioActivate(bool b) { settings->setValue("audio/activate", b);}
    bool getAudioLivePlayback() { return settings->value("audio/livePlayback", false).toBool();}
    void setAudioLivePlayback(bool b) { settings->setValue("audio/livePlayback", b); }
    bool getAudioRecordToFile() { return settings->value("audio/recordToFile", false).toBool();}
    void setAudioRecordToFile(bool b) { settings->setValue("audio/recordToFile", b);}
    int getPlaybackDevice() { return settings->value("audio/playbackDevice", 0).toInt();}
    void setPlaybackDevice(int i) { settings->setValue("audio/playbackDevice", i);}
    int getAudioMode() { return settings->value("audio/mode", 5).toInt();}
    void setAudioMode(int i) { settings->setValue("audio/mode", i);}
    QString getAudioModulationType() { return settings->value("audio/modulationType", "FM").toString();}
    void setAudioModulationType(QString s) { settings->setValue("audio/modulationType", s);}
    QString getAudioModulationBw() { return settings->value("audio/modulationBw", "12").toString();}
    void setAudioModulationBw(QString s) { settings->setValue("audio/modulationBw", s);}
    bool getAudioSquelch() { return settings->value("audio/squelch", true).toBool();};
    void setAudioSquelch(bool b) { settings->setValue("audio/squelch", b);}
    int getAudioSquelchLevel() { return settings->value("audio/squelchLevel", 0).toInt();}
    void setAudioSquelchLevel(int i) { settings->setValue("audio/squelchLevel", i);}
    int getAudioDetector() { return settings->value("audio/detector", 0).toInt();}
    void setAudioDetector(int i) { settings->setValue("audio/detector", i);}


    // Incident filename control
    void incidentStarted();
    void incidentEnded();
    void incidentRestart() { incidentEnded(); incidentStarted();}
    QString incidentFolder();
    QDateTime incidentTimestamp() { return incidentDateTime;}

private slots:

private:
    QSettings *basicSettings;
    QString curFile;
    bool ready = false;
    QString measurementDeviceName;
    const int plotResolution = 1200;
    bool flagIncident = false;
    QDateTime incidentDateTime;
};

#endif // CONFIG_H
