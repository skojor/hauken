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
    SDEFRECORDER,
    CAMERARECORDER,
    GEOLIMITER,
    PLAYBACK,
    AI,
    MQTT,
    OAUTHFILEUPLOAD
};
}

namespace Instrument {
enum class Mode {
    UNKNOWN,
    PSCAN,
    FFM,
};

enum class FftMode {
    OFF,
    MIN,
    MAX,
    SCALAR,
    APEAK
};

enum class Tags
{
    FSCAN   =     101,
    MSCAN   =     201,
    DSCAN   =     301,
    AUDIO   =     401,
    IFPAN   =     501,
    CW      =     801,
    IF      =     901,
    PSCAN   =    1201,
    PIFPAN  =    1601,
    FPIFPAN =    1602,
    GPSC    =    1801,
    ADVIFP  =   10501,
    ADVPSC  =   11201
};

enum class InstrumentState {
    DISCONNECTED,
    CONNECTING,
    CHECK_INSTR_ID,
    CHECK_INSTR_AVAILABLE_UDP,
    CHECK_INSTR_AVAILABLE_TCP,
    CHECK_INSTR_USERNAME,
    CHECK_USER_ONLY,
    CHECK_ANT_NAME1,
    CHECK_ANT_NAME2,
    CHECK_PSCAN_START_FREQ,
    CHECK_PSCAN_STOP_FREQ,
    CHECK_PSCAN_RESOLUTION,
    CHECK_FFM_CENTERFREQ,
    CHECK_FFM_SPAN,
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
    ESMW,
    DDF255,
    UNKNOWN
};


} // namespace Instrument

enum class JAMMINGSTATE
{
    UNKNOWN,
    NOJAMMING,
    WARNINGFIXOK,
    CRITICALNOFIX
};

enum class MODULATIONTYPE { FM, AM, PULSE, PM, IQ, ISB, CW, USB, LSB };

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
    static const int size = 16;
    bool readData(QDataStream &ds) {
        ds >> magicNumber >> versionMinor >> versionMajor >> seqNumber >> reserved >> dataSize;
        if (!ds.atEnd() and magicNumber == 0x000eb200) return true;
        else return false;
    }

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
    static const int size = 12;
    bool readData(QDataStream &ds) {
        ds >> tag >> length >> numItems >> channelNumber >> optHeaderLength >> selectorFlags;
        if (!ds.atEnd()) return true;
        else return false;
    }
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
    static const int size = 60;
};

class AttrHeaderCombined {
public:
    AttrHeaderCombined() {}
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
    int size;
    bool readData(QDataStream &ds) {
        ds >> tag;
        if (tag < 5000) { // Classic AttrHeader
            size = 12;
            quint8 chan, opt;
            quint16 len, items;
            ds >> len >> items >> chan >> opt >> selectorFlagsLow;
            channelNumber = chan;
            optHeaderLength = opt;
            length = len;
            numItems = items;
        }
        else {  // Adv. AttrHeader
            size = 60;
            ds >> reserved >> length >> res1 >> res2 >> res3 >> res4
                >> numItems >> channelNumber >> optHeaderLength >> selectorFlagsLow
                >> selectorFlagsHigh >> res5 >> res6 >> res7 >> res8;
        }
        if (!ds.atEnd()) return true;
        else return false;
    }
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
    quint64 freqOfFirstStep = 0;
    qint16  avgType = 0;
    qint16  setIndex = 0;
    qint16  avgType2 = 0;
    qint16  avgType3 = 0;
    qint64  avgType4 = 0;
    qint16  attMax = 0;
    qint16  overload = 0;
    quint32 nTotalNoTraceValues = 0;
    quint32 nOffsetOfFirstValue = 0;
    int     size = 0;
    bool readData(QDataStream &ds, const int headerSize) {
        size = headerSize;
        if (size >= 32) { // Early pscan optHeader, EM100 and others
            ds >> startFreqLow >> stopFreqLow >> stepFreq >> startFreqHigh >> stopFreqHigh >> reserved >> outputTimestamp;
            ds.skipRawData(headerSize - 32);
        }
        else ds.skipRawData(headerSize); // Weird!
        /*
        if (size >= 62) { // EB500 and others, early firmware, also EM200 new firmware!
            ds >> stepFreqNumerator >> stepFreqDenominator >> freqOfFirstStep >> avgType
                >> setIndex >> avgType2 >> avgType3 >> avgType4 >> attMax >> overload;
        }
        if (size == 70) {
            ds >> nTotalNoTraceValues >> nOffsetOfFirstValue;
        }
        if (ds.atEnd() or size != 32 or size != 62 or size != 70) return false;*/
        // Skip reading rest of header, values are not used anywhere for now
        return true;
    }
};

class OptHeaderIfPanEB500
{
public:
    OptHeaderIfPanEB500() {}
    quint32 freqLow = 0;
    quint32 freqSpan = 0;
    qint16  avgTime = 0;
    qint16  avgType = 0;
    quint32 measureTime = 0;
    quint32 freqHigh = 0;
    qint32  demodFreqChannel = 0;
    quint32 demodFreqLow = 0;
    quint32 demodFreqHigh = 0;
    quint64 outputTimestamp = 0;
    int size = 0;
    bool readData(QDataStream &ds, const int headerSize) {
        size = headerSize;
        if (size >= 40) {
            ds >> freqLow >> freqSpan >> avgTime >> avgType >> measureTime
                >> freqHigh >> demodFreqChannel >> demodFreqLow >> demodFreqHigh >> outputTimestamp;
        }
        ds.skipRawData(headerSize - 40); // Old style IfPan header, not using newer vals for now
        if (!ds.atEnd()) return true;
        else return false;
    }
};

class AudioOptHeader
{
public:
    AudioOptHeader() {}
    qint16  audioMode = 0;
    qint16  frameLength = 0;
    quint32 freqLow = 0;
    quint32 bandwidth = 0;
    quint16 demodulation = 0;
    char    demodString[8];
    quint32 freqHigh = 0;
    char    reserved[6];
    quint64 outputTimeStamp = 0;
    qint16  signalSource = 0;
    static const int size = 42;
    bool readData(QDataStream &ds) {
        ds  >> audioMode >> frameLength >> freqLow >> bandwidth >> demodulation;
        ds.readRawData(demodString, sizeof(demodString));
        ds  >> freqHigh;
        ds.readRawData(reserved, sizeof(reserved));
        ds  >> outputTimeStamp >> signalSource;
        if (!ds.atEnd()) return true;
        else return false;
    }
};

class IfOptHeader
{
public:
    IfOptHeader() {}
    qint16      ifMode = 0;
    qint16      frameLength = 0;
    quint32     sampleRate = 0;
    quint32     freqLow = 0;
    quint32     bandwidth = 0;
    quint16     demodulation = 0;
    qint16      rxAttenuation = 0;
    quint16     flags = 0;
    qint16      kFactor = 0;
    char        demodString[8] = {0};
    quint64     sampleCount = 0;
    quint32     freqHigh = 0;
    qint16      rxGain = 0;
    char        reserved[2] = {0};
    quint64     startTimestamp = 0;
    qint16      signalSource = 0;
    int size = 58;
    bool readData(QDataStream &ds, int headerSize) {
        if (headerSize >= 58)  {
        ds >> ifMode >> frameLength >> sampleRate >> freqLow >> bandwidth
            >> demodulation >> rxAttenuation >> flags >> kFactor;
        ds.readRawData(demodString, sizeof(demodString));
        ds >> sampleCount >> freqHigh >> rxGain;
        ds.readRawData(reserved, sizeof(reserved));
        ds >> startTimestamp >> signalSource;
        }
        if (headerSize > 58)
            ds.skipRawData(headerSize - 58);
        if (!ds.atEnd()) return true;
        return false;
    }
};

class GpsCompassOptHeader
{
public:
    GpsCompassOptHeader() {}
    quint64 timestamp;
    int size = 0;
    bool readData(QDataStream &ds, int optHeaderSize) {
        size = optHeaderSize;
        if (size)
            ds >> timestamp;
        if (optHeaderSize > 8)
            ds.skipRawData(optHeaderSize - 8);
        if (!ds.atEnd())
            return true;
        return false;
    }
};

class GpsCompassData
{
public:
    GpsCompassData() {}
    quint16     compassHeading = 0;
    qint16      compassHeadingType = 0;
    // GPS header, 24 bytes
    qint16      gpsValid = 0;
    qint16      noOfSatInView = 0;
    qint16      latRef = 0;
    qint16      latDeg = 0;
    float       latMin = 0;
    qint16      lonRef = 0;
    qint16      lonDeg = 0;
    float       lonMin = 0;
    float       dilution = 0;
    // EOGPS header
    qint16      antValid = 0;
    qint16      antTiltOver = 0;
    qint16      antElevation = 0;
    qint16      antRoll = 0;
    qint16      signalSource = 0;
    qint16      angularRatesValid = 0;
    qint16      headingAngularRate = 0;
    qint16      elevationAngularRate = 0;
    qint16      rollAngularRate = 0;
    // GPS extension header, 34 bytes
    qint16      geoidalSepValid = 0;
    qint32      geoidalSep = 0;
    qint32      altitude = 0;
    qint16      sog = 0;
    qint16      tmg = 0;
    float       pdop = 0;
    float       vdop = 0;
    quint64     gpsTimestamp = 0;
    qint32      reserved = 0;
    quint64     compassTimestamp = 0;
    qint16      magneticDeclinationSource = 0;
    qint16      magneticDeclination = 0;
    qint16      antennaRollExact = 0;
    qint16      antennaElevationExact = 0;
    bool readData(QDataStream &ds) {
        ds.setFloatingPointPrecision(QDataStream::SinglePrecision);
        ds >> compassHeading >> compassHeadingType >> gpsValid >> noOfSatInView >> latRef
            >> latDeg >> latMin >> lonRef >> lonDeg >> lonMin >> dilution
            >> antValid >> antTiltOver >> antElevation >> antRoll >> signalSource >> angularRatesValid
            >> headingAngularRate >> elevationAngularRate >> rollAngularRate >> geoidalSepValid
            >> geoidalSep >> altitude >> sog >> tmg >> pdop >> vdop >> gpsTimestamp >> reserved
            >> compassTimestamp >> magneticDeclinationSource >> magneticDeclination >> antennaRollExact >> antennaElevationExact;
        if (ds.atEnd())
            return true; // Should be eos at this point CHECK
        return false;
    }
};

struct GpsData {
    double latitude;
    double longitude;
    double altitude;
    double dop;
    double sog;
    double cog;
    QDateTime timestamp;
    int sats;
    bool valid;
};

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
        hasAntNames = false;
        hasGainControl = false;
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
            demodBwList.clear();
            demodTypeList.clear();
            antPorts << "Default";
            pscanResolutions << "0.1" << "0.125" << "0.2" << "0.250" << "0.5" << "0.625" << "1"
                             << "1.25" << "2" << "2.5" << "3.125" << "5" << "6.25" << "10" << "12.5"
                             << "20" << "25" << "50" << "100" << "200" << "500" << "1000" << "2000";
            ffmSpans << "1" << "2" << "5" << "10" << "20" << "50" << "100" << "200"
                     << "500" << "1000" << "2000" << "5000" << "10000" << "20000";
            fftModes << "Off" << "Min" << "Max" << "Scalar";
            demodBwList << "0.15" << "0.3" << "0.6" << "1.5" << "2.4" << "6" << "9" << "12" << "15" << "25"
                        << "30" << "50" << "120" << "150" << "250" << "300" << "500" << "1000" << "5000" << "10000" << "20000";
            demodTypeList << "FM" << "AM" << "Pulse" << "PM" << "IQ" << "ISB" << "CW" << "USB" << "LSB";
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
            demodBwList.clear();
            demodTypeList.clear();
            antPorts << "Default";
            pscanResolutions << "0.125" << "0.250" << "0.625"
                             << "1.25" << "2" << "3.125" << "6.25" << "12.5"
                             << "25" << "50";
            ffmSpans << "1" << "2" << "5" << "10" << "20" << "50" << "100" << "200"
                     << "500" << "1000" << "2000" << "5000" << "10000";
            demodBwList << "0.15" << "0.3" << "0.6" << "1.5" << "2.4" << "6" << "9" << "12" << "15" << "25"
                        << "30" << "50" << "120" << "150" << "250" << "300" << "500";
            demodTypeList << "FM" << "AM" << "Pulse" << "PM" << "IQ" << "ISB" << "CW" << "USB" << "LSB";
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
            demodBwList.clear();
            demodTypeList.clear();
            antPorts << "Ant1" << "Ant2";
            pscanResolutions << "0.1" << "0.125" << "0.2" << "0.250" << "0.5" << "0.625" << "1"
                             << "1.25" << "2" << "2.5" << "3.125" << "5" << "6.25" << "10" << "12.5"
                             << "20" << "25" << "50" << "100" << "200" << "500" << "1000" << "2000";
            ffmSpans << "1" << "2" << "5" << "10" << "20" << "50" << "100" << "200"
                     << "500" << "1000" << "2000" << "5000" << "10000" << "20000" << "40000";
            fftModes << "Off" << "Min" << "Max" << "Scalar" << "APeak";
            demodBwList << "0.15" << "0.3" << "0.6" << "1.5" << "2.4" << "6" << "9" << "12" << "15" << "25"
                        << "30" << "50" << "120" << "150" << "250" << "300" << "500" << "1000" << "5000" << "10000" << "20000" << "40000";
            demodTypeList << "FM" << "AM" << "Pulse" << "PM" << "IQ" << "ISB" << "CW" << "USB" << "LSB";
            minFrequency = 8e3;
            maxFrequency = 8e9;
            hasAntNames = true;
            hasGainControl = true;
        }
        else if (type == Instrument::InstrumentType::ESMW) {
            udpStream = true;
            tcpStream = true;
            systManLocName = true;
            advProtocol = true;
            id = "ESMW";
            hasPscan = true;
            hasAvgType = true;
            hasAutoAtt = true;
            hasGainControl = true;

            pscanResolutions.clear();
            ffmSpans.clear();
            antPorts.clear();
            fftModes.clear();
            demodBwList.clear();
            demodTypeList.clear();
            antPorts << "Ant1" << "Ant2" << "Ant3";
            pscanResolutions << "0.1" << "0.125" << "0.2" << "0.250" << "0.5" << "0.625" << "1"
                             << "1.25" << "2" << "2.5" << "3.125" << "5" << "6.25" << "10" << "12.5"
                             << "20" << "25" << "50" << "100" << "200" << "500" << "1000" << "2000";
            ffmSpans << "1" << "2" << "5" << "10" << "20" << "50" << "100" << "200"
                     << "500" << "1000" << "2000" << "5000" << "10000" << "20000" << "40000" << "80000" << "125000"
                     << "250000" << "500000";
            fftModes << "Off" << "Min" << "Max" << "Scalar" << "APeak";
            demodBwList << "0.15" << "0.3" << "0.6" << "1.5" << "2.4" << "6" << "9" << "12" << "15" << "25"
                        << "30" << "50" << "120" << "150" << "250" << "300" << "500" << "1000" << "5000" << "10000" << "20000"
                        << "40000" << "80000" << "125000";
            demodTypeList << "FM" << "AM" << "Pulse" << "PM" << "IQ" << "ISB" << "CW" << "USB" << "LSB";
            minFrequency = 8e3;
            maxFrequency = 8e9;
            hasAntNames = true;
        }
        else if (type == Instrument::InstrumentType::DDF255) {
            udpStream = true;
            tcpStream = true;
            systManLocName = true;
            advProtocol = false;
            id = "DDF255";
            hasPscan = true;
            attrHeader = true;
            hasAvgType = true;
            optHeaderEb500 = true;
            hasAutoAtt = true;
            hasGainControl = true;

            pscanResolutions.clear();
            ffmSpans.clear();
            antPorts.clear();
            fftModes.clear();
            demodBwList.clear();
            demodTypeList.clear();
            antPorts << "Ant1" << "Ant2" << "Ant3";
            pscanResolutions << "0.1" << "0.125" << "0.2" << "0.250" << "0.5" << "0.625" << "1"
                             << "1.25" << "2" << "2.5" << "3.125" << "5" << "6.25" << "10" << "12.5"
                             << "20" << "25" << "50" << "100" << "200" << "500" << "1000" << "2000";
            ffmSpans << "1" << "2" << "5" << "10" << "20" << "50" << "100" << "200"
                     << "500" << "1000" << "2000" << "5000" << "10000" << "20000" << "40000" << "80000";
            fftModes << "Off" << "Min" << "Max" << "Scalar" << "APeak";
            demodBwList << "0.15" << "0.3" << "0.6" << "1.5" << "2.4" << "6" << "9" << "12" << "15" << "25"
                        << "30" << "50" << "120" << "150" << "250" << "300" << "500" << "1000" << "5000" << "10000" << "20000"
                        << "40000" << "80000";
            demodTypeList << "FM" << "AM" << "Pulse" << "PM" << "IQ" << "ISB" << "CW" << "USB" << "LSB";
            minFrequency = 8e3;
            maxFrequency = 26.5e9;
            hasAntNames = true;
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
            demodBwList.clear();
            demodTypeList.clear();
            pscanResolutions << "0.075" << "0.15" << "0.3" << "0.5" << "0.75"
                             << "1.2" << "1.5" << "2" << "3" << "4" << "4.5" << "7.5"
                             << "15" << "50" << "60" << "75" << "125" << "150";
            ffmSpans << "25" << "50" << "100" << "200" << "500" << "1000";
            fftModes << "Off";
            demodBwList << "0.15" << "0.3" << "0.6" << "1.5" << "2.4" << "6" << "9" << "12" << "15" << "25"
                        << "30" << "50" << "120" << "150" << "250" << "300" << "500";
            demodTypeList << "FM" << "AM" << "Pulse" << "PM" << "IQ" << "ISB" << "CW" << "USB" << "LSB";
            antPorts << "Default";
            minFrequency = 9e3;
            maxFrequency = 2.999e9;
            hasAttOnOff = true;
        }
        else if (type == Instrument::InstrumentType::PR100) {
            udpStream = true;
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
            demodBwList.clear();
            demodTypeList.clear();
            antPorts << "Default";
            pscanResolutions << "0.125" << "0.250" << "0.625"
                             << "1.25" << "2" << "3.125" << "6.25" << "12.5"
                             << "25" << "50";
            ffmSpans << "1" << "2" << "5" << "10" << "20" << "50" << "100" << "200"
                     << "500" << "1000" << "2000" << "5000" << "10000";
            fftModes << "Off";
            demodBwList << "0.15" << "0.3" << "0.6" << "1.5" << "2.4" << "6" << "9" << "12" << "15" << "25"
                        << "30" << "50" << "120" << "150" << "250" << "300" << "500";
            demodTypeList << "FM" << "AM" << "Pulse" << "PM" << "IQ" << "ISB" << "CW" << "USB" << "LSB";
            minFrequency = 20e6;
            maxFrequency = 6e9;
        }
        else if (type == Instrument::InstrumentType::PR200) { //TODO: Kan PR200 lagre antennenavn?
            udpStream = true;
            tcpStream = true;
            systManLocName = true;
            advProtocol = true;
            id = "EM200";
            hasPscan = true;
            hasAvgType = true;
            hasAutoAtt = true;
            hasGainControl = true;

            pscanResolutions.clear();
            ffmSpans.clear();
            antPorts.clear();
            fftModes.clear();
            antPorts.clear();
            demodBwList.clear();
            demodTypeList.clear();
            pscanResolutions << "0.1" << "0.125" << "0.2" << "0.250" << "0.5" << "0.625" << "1"
                             << "1.25" << "2" << "2.5" << "3.125" << "5" << "6.25" << "10" << "12.5"
                             << "20" << "25" << "50" << "100" << "200" << "500" << "1000" << "2000";
            ffmSpans << "1" << "2" << "5" << "10" << "20" << "50" << "100" << "200"
                     << "500" << "1000" << "2000" << "5000" << "10000" << "20000" << "40000";
            fftModes << "Off" << "Min" << "Max" << "Scalar" << "APeak";
            demodBwList << "0.15" << "0.3" << "0.6" << "1.5" << "2.4" << "6" << "9" << "12" << "15" << "25"
                        << "30" << "50" << "120" << "150" << "250" << "300" << "500" << "1000" << "5000" << "10000" << "20000" << "40000";
            demodTypeList << "FM" << "AM" << "Pulse" << "PM" << "IQ" << "ISB" << "CW" << "USB" << "LSB";
            minFrequency = 8e3;
            maxFrequency = 8e9;
            hasAntNames = false;
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
            demodBwList.clear(); // NOPE
            demodTypeList.clear(); //NOPE
            hasAvgType = true;
            hasAutoAtt = true;
            pscanResolutions << "0.625" << "1.25" << "2" << "2.5" << "3.125" << "5"
                             << "6.25" << "8.33" << "10" << "12.5" << "20" << "25" << "50";
            ffmSpans << "500" << "1000" << "2000" << "5000" << "10000" << "20000" << "40000" << "50000";
            fftModes << "Off" << "Min" << "Max";
            antPorts << "RX2_A" << "TRX_A";
            minFrequency = 70e6;
            maxFrequency = 6e9;
            hasAntNames = true;
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
    bool hasAntNames = false;
    bool hasGainControl = false;

    Instrument::Mode mode = Instrument::Mode::UNKNOWN;
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
    QStringList demodBwList;
    QStringList demodTypeList;
    double latitude = 0, longitude = 0, altitude = 0, dop = 0;
    float sog = 0, cog = 0;
    QDateTime gnssTimestamp;
    int sats = 0;
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
    double sog = 0;
    double cog = 0;
    bool gsaValid = false;
    bool gsvValid = false;
    bool ggaValid = false;
    bool posValid = false;
    bool gnsValid = false;
    QString fixType = "No fix";
    QString gnssType;
    int satsTracked = 0;
    int satsVisible = 0;
    int fixQuality = 0;
    int cno = 0;
    int agc = -1;
    int jammingIndicator = -1;
    QDateTime timestamp = QDateTime();
    int id = 0;
    QList<int> avgCnoArray;
    int avgCno = 0;
    QList<int> avgAgcArray;
    QList<int> avgJammingIndicatorArray;
    int avgAgc = -1;
    int  avgJammingIndicator = 0;
    double posOffset;
    double altOffset;
    quint64 timeOffset = 0;
    int cnoOffset = 0;
    int agcOffset = 0;
    int jammingIndicatorOffset = 0;
    JAMMINGSTATE jammingState = JAMMINGSTATE::UNKNOWN;
};

class ConnectionStatus
{
public:
    ConnectionStatus();
    bool connected = false;
    bool reconnect = false;
};

class Pmr
{
public:
    Pmr(quint64 freq = 0, unsigned int spacing = 0) { centerFrequency = freq; channelSpacing = spacing; }
    QChar           type = static_cast<QChar>(0);
    quint64         centerFrequency;
    unsigned int    channelSpacing = 0;
    unsigned int    ifBandwidth = 0;
    QDateTime       startTime;
    QDateTime       endTime;
    quint64         totalDurationInMilliseconds = 0;
    QString         comment;
    QString         demod;
    QString         casperComment;
    unsigned int    squelchLevel = 0;
    bool            squelchState = true;
    bool            active = false;
    bool            attenuation = false;
    int             maxLevel = -999;
};

enum class StationType {
    UNKNOWN,
    STATIONARY,
    OFFSHORE,
    SHIP,
    MOBILE,
    PORTABLE
};

enum class InstrumentCategory {
    UNKNOWN,
    RECEIVER,
    PC
};

class EquipmentInfo {
public:
    EquipmentInfo() {}
    int index;
    QString type;
};

class InstrumentInfo {
public:
    InstrumentInfo() {}
    unsigned char index = 0;
    InstrumentCategory category = InstrumentCategory::UNKNOWN;
    QString producer;
    QString type;
    QString frequencyRange;
    QString serialNumber;
    QString ipAddress;
    unsigned char equipmentIndex = 0;
};

class StationInfo {
public:
    StationInfo() {}
    unsigned int StationIndex = 0;
    QString name;
    QString officialName;
    QString address;
    double latitude = 0;
    double longitude = 0;
    QString status;
    StationType type = StationType::UNKNOWN;
    quint32 mmsi = 0;
    bool active = false;
    QList<InstrumentInfo> instrumentInfo;
};

struct complexInt8 {
    qint8 real;
    qint8 imag;
};

struct complexInt16 {
    qint16 real;
    qint16 imag;
};

#endif // TYPEDEFS_H

