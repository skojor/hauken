#ifndef TYPEDEFS_H
#define TYPEDEFS_H

#include <QString>
#include <QStringList>
#include <QDateTime>

enum class POSITIONSOURCE {
    INSTRUMENTGNSS,
    GNSSDEVICE1,
    GNSSDEVICE2
};

namespace NOTIFY {
enum class TYPE {
    GENERAL,
    MEASUREMENTDEVICE,
    GNSSDEVICE,
    GNSSANALYZER,
    TRACEANALYZER,
    SDEFRECORDER
};
}

namespace Instrument {
enum class Mode {
    PSCAN,
    FFM,
};

enum class FftMode {
    OFF,
    MIN,
    MAX,
    SCALAR
};

enum class Tags
{
    FSCAN   =     101,
    MSCAN   =     201,
    DSCAN   =     301,
    AUDIO   =     401,
    IFPAN   =     501,
    CW      =     801,
    PSCAN   =    1201,
    GPSC    =    1801,
    ADVPSC  =   11201
};

enum class InstrumentState {
    DISCONNECTED,
    CONNECTING,
    CHECK_INSTR_ID,
    CHECK_INSTR_AVAILABLE_UDP,
    CHECK_INSTR_AVAILABLE_TCP,
    CHECK_INSTR_USERNAME,
    CONNECTED
};

enum class InstrumentType
{
    EB500,
    PR100,
    PR200,
    EM100,
    EM200,
    ESMB,
    USRP,
    UNKNOWN
};

} // namespace Instrument

class Eb200Header
{
public:
    Eb200Header() {}
    quint32 magicNumber = 0x000eb200;
    quint16 versionMinor = 0x02;
    quint16 versionMajor = 0x40;
    quint16 seqNumber = 0;
    quint16 reserved = 0;
    quint32 dataSize = 0;
    const int size = 16;
};

class UdpDatagramAttribute
{
public:
    UdpDatagramAttribute() {}
    quint16 tag = 0;
    quint16 length = 0;
    quint16 numItems = 0;
    quint8 channelNumber = 0;
    quint8 optHeaderLength= 0;
    quint32 selectorFlags = 0;
    const int size = 12;
};

class EsmbOptHeaderDScan
{
public:
    EsmbOptHeaderDScan() {}
    quint32 startFreq = 0;
    quint32 stopFreq = 0;
    quint32 stepFreq = 0;
    quint32 markFreq = 0;
    qint16  bwZoom = 0;
    qint16  refLevel = 0;
    const int size = 20;
};

class OptHeaderPScan
{
public:
    OptHeaderPScan() {}
    quint32 startFreqLow = 0;
    quint32 stopFreqLow = 0;
    quint32 stepFreq = 0;
    quint32 startFreqHigh = 0;
    quint32 stopFreqHigh = 0;
    quint32 reserved = 0;
    quint64 outputTimestamp = 0; /* nanoseconds since Jan 1st, 1970, without leap seconds */
    const int size = 32;
};

class OptHeaderPScanEB500
{
public:
    OptHeaderPScanEB500() {}
    quint32 startFreqLow = 0;
    quint32 stopFreqLow = 0;
    quint32 stepFreq = 0;
    quint32 startFreqHigh = 0;
    quint32 stopFreqHigh = 0;
    quint32 reserved = 0;
    quint64 outputTimestamp = 0; /* nanoseconds since Jan 1st, 1970, without leap seconds */
    quint32 stepFreqNumerator = 0;
    quint32 stepFreqDenominator = 0;
    quint64 freqOffFirstStep = 0;
    quint32 dummy = 0;
    const int size = 52;
};

class OptHeaderIfPanEB500
{
public:
    OptHeaderIfPanEB500() {}
    quint32 freqLow = 0;
    quint32 freqSpan = 0;
    qint16  avgTime = 0;
    qint16  avgType = 0;
    qint32  measureTime = 0;
    quint32 freqHigh = 0;
    qint32  demodFreqChannel = 0;
    quint32 demodFreqLow = 0;
    quint32 demodFreqHigh = 0;
    quint64 outputTimestamp = 0;
    const int size = 40;
};

class GenAttrAdvanced
{
public:
    GenAttrAdvanced() {}
    quint16 tag = 0;
    quint16 reserved = 0;
    quint32 length = 0;
    quint32 res1 = 0;
    quint32 res2 = 0;
    quint32 res3 = 0;
    quint32 res4 = 0;
    quint32 numItems = 0;
    quint32 channelNumber = 0;
    quint32 optHeaderLength = 0;
    quint32 selectorFlagsLow = 0;
    quint32 selectorFlagsHigh = 0;
    quint32 res5 = 0;
    quint32 res6 = 0;
    quint32 res7 = 0;
    quint32 res8 = 0;
    const int size = 60;
};

//class OptHeaderEM200  just use pscan opt header

class Device
{
public:
    Device() {
    }
    void clearSettings()
    {
        attrHeader = false;
        optHeaderDscan = false;
        optHeaderPr100 = false;
        optHeaderEb500 = false;
        optHeaderIfpan = false;

        type = Instrument::InstrumentType::UNKNOWN;
        udpStream = false;
        tcpStream = false;
        systManLocName = false;
        memLab999 = false;
        systKlocLab = false;
        advProtocol = false;
        hasFfm = true; // think all devices supported have ffm mode
        hasPscan = false;
        hasDscan = false;
        hasAvgType = false;
        hasAutoAtt = false;
        hasAttOnOff = false;
    }
    void setType(Instrument::InstrumentType t)
    {
        type = t;
        if (type == Instrument::InstrumentType::EB500) {
            udpStream = true;
            tcpStream = true;
            systManLocName = true;
            hasPscan = true;
            hasAvgType = true;
            hasAutoAtt = true;
            id = "EB500";
            attrHeader = true;
            optHeaderEb500 = true;
            optHeaderIfpan = true;

            pscanResolutions.clear();
            ffmSpans.clear();
            antPorts.clear();
            fftModes.clear();
            antPorts << "Default";
            pscanResolutions << "0.1" << "0.125" << "0.2" << "0.250" << "0.5" << "0.625" << "1"
                             << "1.25" << "2" << "2.5" << "3.125" << "5" << "6.25" << "10" << "12.5"
                             << "20" << "25" << "50" << "100" << "200" << "500" << "1000" << "2000";
            ffmSpans << "1" << "2" << "5" << "10" << "20" << "50" << "100" << "200"
                     << "500" << "1000" << "2000" << "5000" << "10000" << "20000";
            fftModes << "Off" << "Min" << "Max" << "Scalar";
            minFrequency = 20e6;
            maxFrequency = 6e9;
        }
        else if (type == Instrument::InstrumentType::EM100) {
            udpStream = true;
            hasPscan = true;
            hasAvgType = false;
            hasAutoAtt = true;
            id = "EM100";
            attrHeader = true;
            optHeaderPr100 = true;
            optHeaderIfpan = true;
            hasAttOnOff = true;
            memLab999 = true;

            pscanResolutions.clear();
            ffmSpans.clear();
            antPorts.clear();
            fftModes.clear();
            antPorts << "Default";
            pscanResolutions << "0.125" << "0.250" << "0.625"
                             << "1.25" << "2" << "3.125" << "6.25" << "12.5"
                             << "25" << "50";
            ffmSpans << "1" << "2" << "5" << "10" << "20" << "50" << "100" << "200"
                     << "500" << "1000" << "2000" << "5000" << "10000";
            fftModes << "Off";
            minFrequency = 20e6;
            maxFrequency = 6e9;
        }
        else if (type == Instrument::InstrumentType::EM200) {
            udpStream = true;
            tcpStream = true;
            systManLocName = true;
            advProtocol = true;
            id = "EM200";
            hasPscan = true;
            hasAvgType = true;
            hasAutoAtt = true;

            pscanResolutions.clear();
            ffmSpans.clear();
            antPorts.clear();
            fftModes.clear();
            antPorts << "Ant1" << "Ant2";
            pscanResolutions << "0.1" << "0.125" << "0.2" << "0.250" << "0.5" << "0.625" << "1"
                             << "1.25" << "2" << "2.5" << "3.125" << "5" << "6.25" << "10" << "12.5"
                             << "20" << "25" << "50" << "100" << "200" << "500" << "1000" << "2000";
            ffmSpans << "1" << "2" << "5" << "10" << "20" << "50" << "100" << "200"
                     << "500" << "1000" << "2000" << "5000" << "10000" << "20000";
            fftModes << "Off" << "Min" << "Max" << "Scalar" << "APeak";
            minFrequency = 8e3;
            maxFrequency = 6e9;
        }
        else if (type == Instrument::InstrumentType::ESMB) {
            udpStream = true;
            attrHeader = true;
            optHeaderDscan = true;
            id = "ESMB";
            hasDscan = true;
            hasAutoAtt = true;
            hasAvgType = true;
            systKlocLab = true;
            pscanResolutions.clear();
            ffmSpans.clear();
            antPorts.clear();
            fftModes.clear();
            pscanResolutions << "0.075" << "0.15" << "0.3" << "0.5" << "0.75"
                             << "1.2" << "1.5" << "2" << "3" << "4" << "4.5" << "7.5"
                             << "15" << "50" << "60" << "75" << "125" << "150";
            ffmSpans << "25" << "50" << "100" << "200" << "500" << "1000";
            fftModes << "Off";
            antPorts << "Default";
            minFrequency = 9e3;
            maxFrequency = 2.999e9;
            hasAttOnOff = true;
        }
        else if (type == Instrument::InstrumentType::PR100) {
            hasPscan = true;
            hasAvgType = false;
            hasAutoAtt = true;
            id = "PR100";
            attrHeader = true;
            optHeaderPr100 = true;
            optHeaderIfpan = true;
            hasAttOnOff = true;
            memLab999 = true;

            pscanResolutions.clear();
            ffmSpans.clear();
            antPorts.clear();
            fftModes.clear();
            antPorts << "Default";
            pscanResolutions << "0.125" << "0.250" << "0.625"
                             << "1.25" << "2" << "3.125" << "6.25" << "12.5"
                             << "25" << "50";
            ffmSpans << "1" << "2" << "5" << "10" << "20" << "50" << "100" << "200"
                     << "500" << "1000" << "2000" << "5000" << "10000";
            fftModes << "Off";
            minFrequency = 20e6;
            maxFrequency = 6e9;
        }
        else if (type == Instrument::InstrumentType::PR200) {
            udpStream = true;
            tcpStream = true;
            systManLocName = true;
            advProtocol = true;
            id = "PR200";
            hasPscan = true;
        }
        else if (type == Instrument::InstrumentType::USRP) {
            udpStream = true;
            tcpStream = true;
            systManLocName = true;
            id = "USRP";
            attrHeader = true;
            hasPscan = true;
            optHeaderEb500 = true;
            pscanResolutions.clear();
            ffmSpans.clear();
            antPorts.clear();
            fftModes.clear();
            hasAvgType = true;
            hasAutoAtt = true;
            pscanResolutions << "0.625" << "1.25" << "2" << "2.5" << "3.125" << "5"
                             << "6.25" << "8.33" << "10" << "12.5" << "20" << "25" << "50";
            ffmSpans << "500" << "1000" << "2000" << "5000" << "10000" << "20000" << "40000" << "50000";
            fftModes << "Off" << "Min" << "Max";
            antPorts << "RX2_A" << "TRX_A";
            minFrequency = 70e6;
            maxFrequency = 6e9;
        }
    }

    Eb200Header mainHeader;
    bool attrHeader = false;
    bool optHeaderDscan = false;
    bool optHeaderPr100 = false;
    bool optHeaderEb500 = false;
    bool optHeaderIfpan = false;

    Instrument::InstrumentType type = Instrument::InstrumentType::UNKNOWN;
    bool udpStream = false;
    bool tcpStream = false;
    bool systManLocName = false;
    bool memLab999 = false;
    bool systKlocLab = false;
    bool advProtocol = false;
    bool hasFfm = true; // think all devices supported have ffm mode
    bool hasPscan = false;
    bool hasDscan = false;
    bool hasAvgType = false;
    bool hasAutoAtt = false;
    bool hasAttOnOff = false;

    Instrument::Mode mode = Instrument::Mode::PSCAN;
    quint64 pscanStartFrequency = 0, pscanStopFrequency = 0;
    quint32 pscanResolution;
    quint64 ffmCenterFrequency;
    quint32 ffmFrequencySpan;
    float measurementTime;
    QString id;
    QString longId;

    quint64 minFrequency = 0, maxFrequency = 0;
    QStringList antPorts;
    QStringList ffmSpans;
    QStringList pscanResolutions;
    QStringList fftModes;
    double latitude = 0, longitude = 0;
    bool positionValid = false;
};

class GnssData
{
public:
    GnssData() {}
    bool inUse = false;
    double hdop = 0;
    double latitude = 0;
    double longitude = 0;
    double altitude = 0;
    bool gsaValid = false;
    bool gsvValid = false;
    bool ggaValid = false;
    bool posValid = false;
    QString fixType = "No fix";
    QString gnssType = "Unknown";
    int satsTracked = 0;
    int satsVisible = 0;
    int fixQuality = 0;
    int cno = 0;
    int agc = 0;
    QDateTime timestamp = QDateTime();
    int id = 0;
    QList<int> avgCnoArray;
    int avgCno = 0;
    QList<int> avgAgcArray;
    int avgAgc = 0;
    double posOffset;
    double altOffset;
    unsigned long timeOffset = 0;
    int cnoOffset = 0;
    int agcOffset = 0;
};

#endif // TYPEDEFS_H

