#ifndef TYPEDEFS_H
#define TYPEDEFS_H

#include <QString>
#include <QStringList>

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
    AUDIO   =     401,
    IFPAN   =     501,
    CW      =     801,
    PSCAN   =    1201,
    GPSC    =    1801
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
    const int size = 44;
};

class Device
{
public:
    Device() {
        antPorts << "Default";
    }
    void setType(Instrument::InstrumentType t)
    {
        type = t;
        if (type == Instrument::InstrumentType::EB500) {
            udpStream = true;
            tcpStream = true;
            systManLocName = true;
            hasPscan = true;
            id = "EB500";
            attrHeader = true;
            optHeaderEb500 = true;
        }
        else if (type == Instrument::InstrumentType::EM100) {
            udpStream = true;
            id = "EM100";
            hasPscan = true;
            optHeaderPr100 = true;
        }
        else if (type == Instrument::InstrumentType::EM200) {
            udpStream = true;
            tcpStream = true;
            systManLocName = true;
            advProtocol = true;
            id = "EM200";
            hasPscan = true;
        }
        else if (type == Instrument::InstrumentType::ESMB) {
            udpStream = true;
            attrHeader = true;
            optHeaderDscan = true;
            id = "ESMB";
        }
        else if (type == Instrument::InstrumentType::PR100) {
            udpStream = true;
            id = "PR100";
            hasPscan = true;
            attrHeader = true;
            optHeaderPr100 = true;
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
        }
    }
    Eb200Header mainHeader;
    bool attrHeader = false;
    bool optHeaderDscan = false;
    bool optHeaderPr100 = false;
    bool optHeaderEb500 = false;

    Instrument::InstrumentType type = Instrument::InstrumentType::UNKNOWN;
    bool udpStream = false;
    bool tcpStream = false;
    bool systManLocName = false;
    bool advProtocol = false;
    bool hasFfm = true; // think all devices supported have ffm mode
    bool hasPscan = false;

    Instrument::Mode mode = Instrument::Mode::PSCAN;
    quint64 pscanStartFrequency, pscanStopFrequency;
    quint32 pscanResolution;
    quint64 ffmCenterFrequency;
    quint32 ffmFrequencySpan;
    float measurementTime;
    QString id;
    QString longId;
    QStringList antPorts;
};
#endif // TYPEDEFS_H
