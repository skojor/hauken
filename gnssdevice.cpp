#include "gnssdevice.h"
#include "asciitranslator.h"
#include "qcustomplot.h"
#include "ubloxmonrfparser.h"
#include "ubloxmonspanparser.h"
#include "fftw3.h"
#include "appclock.h"
#include <algorithm>

namespace {
QString septentrioSatelliteSystem(int satelliteId)
{
    if (satelliteId >= 1 && satelliteId <= 37) return "GPS";
    if (satelliteId >= 38 && satelliteId <= 68) return "GLONASS";
    if (satelliteId >= 71 && satelliteId <= 106) return "Galileo";
    if (satelliteId >= 107 && satelliteId <= 119) return "L-Band";
    if ((satelliteId >= 120 && satelliteId <= 140) || (satelliteId >= 198 && satelliteId <= 215)) return "SBAS";
    if ((satelliteId >= 141 && satelliteId <= 180) || (satelliteId >= 223 && satelliteId <= 245)) return "BeiDou";
    if (satelliteId >= 181 && satelliteId <= 190) return "QZSS";
    if ((satelliteId >= 191 && satelliteId <= 197) || (satelliteId >= 216 && satelliteId <= 222)) return "NavIC";
    return {};
}

QString frequencyLabel(const UbloxRfBandMetadata &metadata)
{
    QString window = "RF";
    if (metadata.centerHz >= 1520000000 && metadata.centerHz <= 1620000000)
        window = "Upper/L1";
    else if (metadata.centerHz >= 1140000000 && metadata.centerHz < 1220000000)
        window = "Lower/L5-E5";
    else if (metadata.centerHz >= 1220000000 && metadata.centerHz < 1320000000)
        window = "L2";

    return QString("%1 (%2 MHz)").arg(window).arg(metadata.centerHz / 1e6, 0, 'f', 6);
}

void applyRfBandMapping(GnssData &data, const QMap<int, UbloxRfBandMetadata> &mapping)
{
    for (auto iterator = data.agcBlocks.begin(); iterator != data.agcBlocks.end(); ++iterator) {
        const auto metadata = mapping.constFind(iterator.key());
        iterator.value().frequencyMapped = metadata != mapping.cend();
        if (metadata != mapping.cend()) {
            iterator.value().centerHz = metadata->centerHz;
            iterator.value().spanHz = metadata->spanHz;
            iterator.value().resolutionHz = metadata->resolutionHz;
            iterator.value().frequencyLabel = frequencyLabel(*metadata);
        }
        else {
            iterator.value().centerHz = 0;
            iterator.value().spanHz = 0;
            iterator.value().resolutionHz = 0;
            iterator.value().frequencyLabel.clear();
        }
    }
}

// Septentrio SBF ReceiverStatus (4014) AGCState FrontEndID bits 0-4, per SBF Reference Guide section 4.2.16.
// This enum is fixed by the SBF spec, not receiver-model-specific.
QString septentrioFrontEndLabel(int frontEndCode, quint32 centerHz)
{
    static const QMap<int, QString> names = {
        {0, "GPS L1 / Galileo E1"}, {1, "GLONASS L1"}, {2, "Galileo E6"}, {3, "GPS L2"},
        {4, "GLONASS L2"}, {5, "L5 / E5a / B2a"}, {6, "E5b / B2b"}, {7, "Galileo E5 AltBOC"},
        {8, "GPS/GLONASS/SBAS/Galileo L1 (combined)"}, {9, "GPS/GLONASS L2 (combined)"},
        {10, "L-band / MSS"}, {11, "BeiDou B1"}, {12, "BeiDou B3"}, {13, "S-band"},
        {14, "BeiDou B3 + Galileo E6 (combined)"}
    };
    QString name = names.value(frontEndCode, QString("Frontend %1").arg(frontEndCode));
    if (centerHz > 0) return QString("%1 (%2 MHz)").arg(name).arg(centerHz / 1e6, 0, 'f', 2);
    return name;
}

quint32 septentrioFrontEndCenterHz(int frontEndCode)
{
    // Nominal carrier frequencies from SBF Reference Guide section 4.1.10 (Signal Type table).
    static const QMap<int, quint32> centerHz = {
        {0, 1575420000}, {1, 1602000000}, {2, 1278750000}, {3, 1227600000}, {4, 1246000000},
        {5, 1176450000}, {6, 1207140000}, {7, 1191795000}, {8, 1575420000}, {9, 1227600000},
        {11, 1561098000}, {12, 1268520000}
        // 10 (L-band/MSS), 13 (S-band) and 14 (combined) have no single representative carrier
    };
    return centerHz.value(frontEndCode, 0);
}

// Which SBF signal types (section 4.1.10) a given AGCData front-end code can carry.
const QSet<int> &septentrioFrontEndSignalTypes(int frontEndCode)
{
    static const QMap<int, QSet<int>> table = {
        {0,  {0, 1, 5, 6, 13, 17, 24, 32, 33, 37, 38}}, // GPS/QZSS/SBAS/NavIC L1, Galileo E1, BeiDou B1C
        {1,  {8, 9}},                                    // GLONASS L1
        {2,  {19}},                                      // Galileo E6
        {3,  {2, 3, 7}},                                 // GPS/QZSS L2
        {4,  {10, 11}},                                   // GLONASS L2
        {5,  {4, 14, 15, 20, 25, 26}},                    // L5/E5a/B2a (GPS, BeiDou, NavIC, Galileo, SBAS, QZSS)
        {6,  {21, 29, 34}},                                // E5b/B2b (Galileo, BeiDou)
        {7,  {22}},                                        // Galileo E5 AltBOC
        {8,  {0, 1, 5, 6, 8, 9, 13, 17, 24, 32, 33, 37, 38}}, // combined L1
        {9,  {2, 3, 7, 10, 11}},                           // combined L2
        {10, {23}},                                        // L-band/MSS
        {11, {28}},                                        // BeiDou B1I
        {12, {30}},                                        // BeiDou B3I
        {14, {19, 30}}                                      // combined BeiDou B3 + Galileo E6
        // 13 (S-band) carries no GNSS signal type and is never correlated against MeasEpoch
    };
    static const QSet<int> empty;
    auto it = table.constFind(frontEndCode);
    return it != table.cend() ? it.value() : empty;
}
}

GnssDevice::GnssDevice(QSharedPointer<Config> c, const QString &profileId)
{
    config = c;
    gnssData.id = profileId;

    connect(gnss, &QSerialPort::readyRead, this, [this] {
        QByteArray buffer = gnss->readAll();
        gnssBuffer.append(buffer);
        //if (logToFile) appendToLogfile(buffer);
        handleBuffer();
    });

    connect(sendToAnalyzerTimer, &QTimer::timeout, this, [this]
            {
                if (this->gnss->isOpen() || tcpSocket->isOpen() || replayMode) emit analyzeThisData(gnssData);
                checkPosValid(); // run once per sec
            });

    connect(tcpSocket, &QTcpSocket::readyRead, this, [this] {
        tcpLastDataTimer.restart();
        QByteArray buffer = tcpSocket->readAll();
        gnssBuffer.append(buffer);
        //if (logToFile) appendToLogfile(buffer);
        handleBuffer();
    });

    connect(tcpSocket, &QTcpSocket::stateChanged, this, &GnssDevice::handleTcpStateChange);
    connect(tcpWatchdogTimer, &QTimer::timeout, this, &GnssDevice::checkTcpConnection);
    tcpWatchdogTimer->start(1000);
    connect(delayedReportTimer, &QTimer::timeout, this, &GnssDevice::delayedReportHandler);
    connect(backlogCleanupTimer, &QTimer::timeout, this, &GnssDevice::cleanupBacklog);
    backlogCleanupTimer->start(1000);

    const QString gnssLogFolder = QDir(config->getLogFolder()).filePath("gnssLogs");
    if (!QDir().mkpath(gnssLogFolder)) {
        qWarning() << "Couldn't create folder" << gnssLogFolder;
        return;
    }

}

GnssDevice::~GnssDevice()
{
    // Members are unparented, close the port explicitly so it can be reopened
    gnss->close();
    tcpSocket->abort();
    delete gnss;
    delete tcpSocket;
    delete sendToAnalyzerTimer;
    delete delayedReportTimer;
    delete tcpWatchdogTimer;
    delete backlogCleanupTimer;
    delete hostAddress;
}

void GnssDevice::start()
{
    if (replayMode) {
        if (!sendToAnalyzerTimer->isActive())
            sendToAnalyzerTimer->start(1000);
        return;
    }
    if (config->getGnssReceiverProfile(gnssData.id).enabled &&
        (!gnss->isOpen() || tcpSocket->state() == QAbstractSocket::UnconnectedState)) connectToPort();
    sendToAnalyzerTimer->stop();

    if (gnss->isOpen() || tcpSocket->state() != QAbstractSocket::UnconnectedState) {
        sendToAnalyzerTimer->start(1000);
    }
}

void GnssDevice::connectToPort()
{
    //qDebug() << "connectToPort called";
    QSerialPortInfo portInfo;

    if (activate && !portName.isEmpty() && !baudrate.isEmpty()) {
        for (auto &val : QSerialPortInfo::availablePorts()) {
            if (!val.isNull() && val.portName().contains(portName, Qt::CaseInsensitive))
                portInfo = val;
        }
        if (!portInfo.isNull())
            gnss->setPort(portInfo);
        else { // probably hostname/ip address, try it. Baud rate should then contain a port number
            connectTcpSocket();
        }
        if (tcpSocket->state() == QAbstractSocket::UnconnectedState) {
            gnss->setBaudRate(baudrate.toUInt());
            if (gnss->open(QIODevice::ReadWrite))
                qDebug() << "Connected to" << gnss->portName();
            else
                qDebug() << "Cannot open" << gnss->portName();
            if (gnss->isOpen()) gnssData.inUse = true;
            else gnssData.inUse = false;
            gnss->setDataTerminalReady(true);
            if (uBloxDevice) askUbloxVersion();
        }
    }
}

void GnssDevice::injectBytes(const QByteArray &data)
{
    // Small pieces, handleBuffer() discards the buffer if it grows beyond 15000 bytes
    constexpr int pieceSize = 2048;
    for (int pos = 0; pos < data.size(); pos += pieceSize) {
        gnssBuffer.append(data.mid(pos, pieceSize));
        qsizetype previousSize;
        do {
            previousSize = gnssBuffer.size();
            handleBuffer();
        } while (!gnssBuffer.isEmpty() && gnssBuffer.size() < previousSize);
    }
}

void GnssDevice::handleBuffer()
{
    QByteArray binaryHeader = QByteArray::fromHex("b562");
    QByteArray sbfBinHeader = QByteArray::fromHex("2440");
    QByteArray nmeaSentence, binarySentence, sbfBinSentence;
    int binaryIndex, sbfBinIndex, nmeaIndex;
    int binarySize, nmeaSize;
    quint16 sbfBinSize;
    int failsafe = 0;
    do {
        failsafe++;
        nmeaSentence.clear();
        binarySentence.clear();
        sbfBinSentence.clear();
        binarySize = sbfBinSize = 0;

        binaryIndex = gnssBuffer.indexOf(binaryHeader); // Check if ublox data
        sbfBinIndex = gnssBuffer.indexOf(sbfBinHeader); // Check if SBF data

        if (binaryIndex != -1 && gnssBuffer.size() > binaryIndex + 6) {
            binarySize = 8 + (quint8)gnssBuffer.at(binaryIndex + 4) + (gnssBuffer.at(binaryIndex + 5) << 8);
            if (gnssBuffer.size() >= binaryIndex + binarySize) {
                binarySentence = gnssBuffer.mid(binaryIndex, binarySize);
                gnssBuffer.remove(binaryIndex, binarySize);
            }
        }

        else if (sbfBinIndex != -1 && gnssBuffer.size() > sbfBinIndex + 6) {
            sbfBinSize = qFromLittleEndian<quint16>(reinterpret_cast<const uchar*>(&gnssBuffer[sbfBinIndex + 6]));
            if (gnssBuffer.size() > sbfBinIndex + sbfBinSize) {
                sbfBinSentence = gnssBuffer.mid(sbfBinIndex, sbfBinSize);
                gnssBuffer.remove(sbfBinIndex, sbfBinSize);
            }
        }

        nmeaIndex = gnssBuffer.indexOf("$G");

        if (nmeaIndex != -1 && gnssBuffer.contains('\n')) {
            int i = 0;
            do {
                nmeaSentence.append(gnssBuffer[nmeaIndex + i++]);
            } while (nmeaIndex + i < gnssBuffer.size() && gnssBuffer[nmeaIndex + i] != '\n');

            nmeaSize = nmeaSentence.size();
            nmeaSentence = nmeaSentence.simplified();
            if (nmeaSize > 512) { // failsafe, seems stray data mixed with nmea from faulty serial lines can make hell break loose
                nmeaSentence.clear();
                nmeaSize = 0;
                gnssBuffer.clear();
            }
            if (nmeaIndex > 0 && binaryIndex == -1) {   // somehow leftover data has ended up before the nmea sentence, looks like nothing. deleting
                gnssBuffer.remove(0, nmeaIndex);
            }

            if (nmeaSize  > 10 && checkChecksum(nmeaSentence)) {
                if (nmeaSentence.contains("GGA"))
                    decodeGga(nmeaSentence);
                else if (nmeaSentence.contains("GSA"))
                    decodeGsa(nmeaSentence);
                else if (nmeaSentence.contains("RMC"))
                    decodeRmc(nmeaSentence);
                else if (nmeaSentence.contains("GSV"))
                    decodeGsv(nmeaSentence);
                else if (nmeaSentence.contains("GNS"))
                    decodeGns(nmeaSentence);
                updateBacklog(nmeaSentence, false);
            }
            gnssBuffer.remove(nmeaIndex, nmeaSize+1);
            if (logToFile) appendToLogfile(nmeaSentence + '\n');
        }

        if (binaryIndex != -1 && !binarySentence.isEmpty()) {
            if (binarySentence.size() == binarySize
                && checkBinaryChecksum(binarySentence)
                && (quint8)binarySentence[2] == 0x0a && (quint8)binarySentence[3] == 0x04) {
                decodeBinary0a04(binarySentence);
                updateBacklog(binarySentence, true);
            }
            else if (binarySentence.size() == binarySize) {
                decodeBinary(binarySentence);
                updateBacklog(binarySentence, true);
            }
            if (logToFile) {
                appendToBinaryfile(binarySentence);
            }
        }
        if (sbfBinIndex != -1 && !sbfBinSentence.isEmpty() && checkSbfChecksum(sbfBinSentence)) {
            gnssData.septentrioReceiver = true;
            if (!septentrioDetected) {
                septentrioDetected = true;
                gnssData.satsTracked = 0;
                gnssData.cno = 0;
                gnssData.gnssType.clear();
                gsvSentences.clear();
                qDebug() << "Septentrio GNSS receiver detected:" << gnssData.id << portName;
            }
            quint16 id = qFromLittleEndian<quint16>(reinterpret_cast<const uchar*>(sbfBinSentence.constData() + 4));
            id = id & 0x1fff;
            updateBacklog(sbfBinSentence, true);
            if (id == 4014)
                decodeSbf4014(sbfBinSentence);
            else if (id == 4027)
                decodeSbf4027(sbfBinSentence);
            else if (id == 4013)
                decodeSbf4013(sbfBinSentence);
            else if (id == 4092)
                decodeSbf4092(sbfBinSentence);
            else if (id == 4040)
                decodeSbf4040(sbfBinSentence);

            if (logToFile) {
                appendToBinaryfile(sbfBinSentence);
            }
        }
    } while ((binaryIndex != -1 || nmeaIndex != -1) && (binarySentence.size() > 0 || nmeaSentence.size() > 0) && failsafe < 50);

    if (failsafe >= 50) {
        //qDebug() << "Panic!" << gnssBuffer.size();
        gnssBuffer.clear();
    }
    if (gnssBuffer.size() > 15000) gnssBuffer.clear();
}

bool GnssDevice::decodeGsa(const QByteArray &val)
{
    QList<QByteArray> split = val.split(',');
    if (split.count() > 16) {
        gnssData.gsaValid = true;
        gnssData.hdop = split.at(16).toDouble();

        /*if (val.contains("$GN")) gnssData.gnssType = "GPS+GLONASS";
        else if (val.contains("$GP")) gnssData.gnssType = "GPS";
        else if (val.contains("$GL")) gnssData.gnssType = "GLONASS";
        else gnssData.gnssType = "Other";*/

        if (split.at(2).toInt() == 3) gnssData.fixType = "3D";
        else if (split.at(2).toInt() == 2) gnssData.fixType = "2D";
        else gnssData.fixType = "No fix";
    }
    else
        gnssData.gsaValid = false;

    return gnssData.gsaValid;
}

bool GnssDevice::decodeGga(const QByteArray &val)
{
    QList<QByteArray> split = val.split(',');
    if (split.size() > 9) {
        gnssData.ggaValid = true;

        if (!gnssData.gnsValid && !septentrioDetected) gnssData.satsTracked = split.at(7).toInt();
        gnssData.fixQuality = split.at(6).toInt();
        gnssData.altitude = split.at(9).toDouble();
        //gnssData.hdop = split.at(8).toDouble();
        gnssData.latitude =
            int(split.at(2).toDouble() / 100) +
            ((split.at(2).toDouble() / 100) - int(split.at(2).toDouble() / 100)) * 1.6667;
        gnssData.longitude =
            int(split.at(4).toDouble() / 100) +
            ((split.at(4).toDouble() / 100) - int(split.at(4).toDouble() / 100)) * 1.6667;

        if (!split[3].isEmpty() && split[3][0] == 'S') gnssData.latitude *= -1;
        if (!split[5].isEmpty() && split[5][0] == 'W') gnssData.longitude *= -1;
    }
    else gnssData.ggaValid = false;
    return gnssData.ggaValid;
}

bool GnssDevice::decodeRmc(const QByteArray &val)
{
    QList<QByteArray> split = val.split(',');
    if (split.size() > 11) {
        gnssData.timestamp = convFromGnssTimeToQDateTime(split.at(9), split.at(1));
        if (split.at(2).contains('A'))
            gnssData.posValid = true;
        else
            gnssData.posValid = false;
        gnssData.sog = split.at(7).toDouble();
        gnssData.cog = split.at(8).toDouble();
    }
    return true;
}

bool GnssDevice::decodeGsv(const QByteArray &val)
{
    if (septentrioDetected) return true;
    if (!checkedAllSatSystems) {
        if (val.contains("$GP") && !gnssData.gnssType.contains("GPS")) gnssData.gnssType += "GPS ";
        else if (val.contains("$GL") && !gnssData.gnssType.contains("GLONASS")) gnssData.gnssType += "GLONASS ";
        else if (val.contains("$GA") && !gnssData.gnssType.contains("Galileo")) gnssData.gnssType += "Galileo ";
        else if (val.contains("$PQ") && !gnssData.gnssType.contains("BeiDou")) gnssData.gnssType += "BeiDou ";
        QTimer::singleShot(20000, this, [this] { // Assume after 20 secs all valid systems are active
            checkedAllSatSystems = true;
        });
    }
    QList<QByteArray> split = val.split(',');
    if (split.size() > 4) {
        if (split.at(2).toInt() == gsvSentences.size() + 1)
            gsvSentences.append(val);
    }
    if (split.at(1).toInt() == gsvSentences.size()) { // received all x sentences, let's analyze!
        QList<int> cnoList;
        for (auto &gsvSentence : gsvSentences) {
            split = gsvSentence.split(',');
            for (int i=7; i<split.size(); i += 4) {
                if (split.at(i).toInt() > 0) cnoList.append(split.at(i).toInt());
            }
        }
        while (cnoList.size() > 4) { // reduce cno list to 4 highest values, and make avg
            int low = 99, iterator = 0;
            for (int i=0; i<cnoList.size(); i++) {
                if (cnoList.at(i) < low) {
                    low = cnoList.at(i);
                    iterator = i;
                }
            }
            cnoList.removeAt(iterator);
        }
        if (cnoList.size() > 0) {
            gnssData.cno = 0;
            for (auto &v : cnoList)
                gnssData.cno += v;
            gnssData.cno /= cnoList.size();
        }
        gsvSentences.clear();
    }
    return true;
}

bool GnssDevice::decodeGns(const QByteArray &val)
{
    gnssData.gnsValid = true;
    QList<QByteArray> split = val.split(',');
    if (split.size() > 7 && !septentrioDetected) {
        gnssData.satsTracked = split[7].toInt();
    }
    return true;
}

QDateTime GnssDevice::convFromGnssTimeToQDateTime(const QByteArray date, const QByteArray time)
{
    QDateTime dt;
    QByteArray baTime;
    if (time.contains(".")) baTime = time.split('.')[0];
    if (date.size() == 6 && baTime.size() == 6) {
        dt = QDateTime::fromString(date + baTime, "ddMMyyHHmmss");
        dt = dt.addYears(100);
        dt.setTimeZone(QTimeZone::UTC);
    }
    return dt;
}

void GnssDevice::decodeBinary(const QByteArray &val)
{
    ++binaryMessageSequence;
    QByteArray MONHW(QByteArray::fromHex("0a09"));
    QByteArray MONRF(QByteArray::fromHex("0a38"));
    QByteArray MONSPAN(QByteArray::fromHex("0a31"));
    //qDebug() << "BIN:" << (int)val.at(2) << (int)val.at(3);
    if (!checkBinaryChecksum(val)) {
        if (uBloxID == "X20P" && (val.indexOf(MONRF) == 2 || val.indexOf(MONSPAN) == 2)) {
            rfBandCorrelator.invalidate();
            applyRfBandMapping(gnssData, {});
        }
        return;
    }

    {
        if (val.indexOf(MONHW) == 2) {
            int agc = (quint8)val.at(24) + (quint16)(val.at(25) << 8);
            if (agc >= 0 && agc <= 8191) gnssData.agc = agc * 100 / 8191; // Changed 181123 JSK: Percentage value, 8191 = 100% gain

            int jamInd = (quint8)val.at(51);
            if (jamInd >= 0 && jamInd < 256) gnssData.jammingIndicator = jamInd * 100 / 255;

            uchar jamState = (val.at(28) & 0xc) >> 2;

            if (jamState == 0) gnssData.jammingState = JAMMINGSTATE::UNKNOWN;
            else if (jamState == 1) gnssData.jammingState = JAMMINGSTATE::NOJAMMING;
            else if (jamState == 2) gnssData.jammingState = JAMMINGSTATE::WARNINGFIXOK;
            else if (jamState == 3) gnssData.jammingState = JAMMINGSTATE::CRITICALNOFIX;
        }

        else if (val.indexOf(MONRF) == 2) {
            if (uBloxID == "X20P") {
                QVector<UbloxMonRfBlock> orderedBlocks;
                if (UbloxMonRfParser::parseAgcBlocks(val, orderedBlocks)) {
                    QMap<int, int> agcBlocks;
                    for (const UbloxMonRfBlock &block : orderedBlocks) {
                        agcBlocks.insert(block.blockId, block.agcPercent);
                        gnssData.agcBlocks[block.blockId].agc = block.agcPercent;
                        gnssData.agcBlocks[block.blockId].jammingIndicator = block.jammingIndicatorPercent;
                    }

                    const auto previousBlockIds = gnssData.agcBlocks.keys();
                    for (int blockId : previousBlockIds) {
                        if (!agcBlocks.contains(blockId)) gnssData.agcBlocks.remove(blockId);
                    }
                    if (!gnssData.agcBlocks.isEmpty()) {
                        gnssData.agc = gnssData.agcBlocks.first().agc;
                        gnssData.jammingIndicator = gnssData.agcBlocks.first().jammingIndicator;
                    }

                    rfBandCorrelator.observeMonRf(orderedBlocks, QDateTime::currentMSecsSinceEpoch(),
                                                  binaryMessageSequence);
                    applyRfBandMapping(gnssData, rfBandCorrelator.mapping());
                }
                else {
                    rfBandCorrelator.invalidate();
                    applyRfBandMapping(gnssData, {});
                }
            }
            else if (val.size() > 26) {
                int agc = (quint8)val[24] + ((quint8)val[25] << 8);
                if (agc >= 0 && agc <= 8191) gnssData.agc = agc * 100 / 8191;
            }

            if (val.size() > 26) {
                int jamInd = (quint8)val.at(26);
                if (jamInd >= 0 && jamInd < 256) gnssData.jammingIndicator = jamInd * 100 / 255;
            }

            if (val.size() > 11) {
                int jamState = (int)val[11];
                if (jamState == 0) gnssData.jammingState = JAMMINGSTATE::UNKNOWN;
                else if (jamState == 1) gnssData.jammingState = JAMMINGSTATE::NOJAMMING;
                else if (jamState == 2) gnssData.jammingState = JAMMINGSTATE::WARNINGFIXOK;
                else if (jamState == 3) gnssData.jammingState = JAMMINGSTATE::CRITICALNOFIX;
            }
        }

        else if (val.indexOf(MONSPAN) == 2 && uBloxID == "X20P") {
            QVector<UbloxMonSpanBlock> spanBlocks;
            if (UbloxMonSpanParser::parse(val, spanBlocks)) {
                rfBandCorrelator.observeMonSpan(spanBlocks, QDateTime::currentMSecsSinceEpoch(),
                                                binaryMessageSequence);
                applyRfBandMapping(gnssData, rfBandCorrelator.mapping());

                for (int blockIndex = 0; blockIndex < spanBlocks.size(); blockIndex++) {
                    const UbloxMonSpanBlock &spanBlock = spanBlocks.at(blockIndex);
                    GnssSpectrumBlock spectrumBlock;
                    spectrumBlock.levels.reserve(spanBlock.spectrum.size());
                    for (char byte : spanBlock.spectrum) spectrumBlock.levels.append(static_cast<quint8>(byte));
                    spectrumBlock.centerHz = spanBlock.centerHz;
                    spectrumBlock.spanHz = spanBlock.spanHz;
                    spectrumBlock.resolutionHz = spanBlock.resolutionHz;
                    spectrumBlock.timestamp = QDateTime::currentDateTime();
                    spectrumBlock.blockIndex = blockIndex;
                    spectrumBlock.blockCount = spanBlocks.size();
                    emit spectrumReady(spectrumBlock);
                }
            }
            else {
                rfBandCorrelator.invalidate();
                applyRfBandMapping(gnssData, {});
            }
        }
    }
}

bool GnssDevice::checkBinaryChecksum(const QByteArray &val)
{
    if (val.size() > 4) {
        quint8 calcChkA = 0, calcChkB = 0;
        quint16 size = val.length() - 4;
        quint8 chkA = (quint8)val.at(val.size()-2);
        quint8 chkB = (quint8)val.at(val.size()-1);

        for (int i = 2; i<size+2; i++) {
            calcChkA += (quint8)val.at(i);
            calcChkB += calcChkA;
        }
        if (calcChkA == chkA && calcChkB == chkB) {
            return true;
        }
        else {
            return false;
        }
    }
    else return false;
}


bool GnssDevice::checkSbfChecksum(const QByteArray &val)
{
    if (val.size() < 8) return false;

    quint16 crc = 0;
    for (qsizetype index = 4; index < val.size(); ++index) {
        crc ^= static_cast<quint16>(static_cast<quint8>(val.at(index))) << 8;
        for (int bit = 0; bit < 8; ++bit)
            crc = (crc & 0x8000) ? (crc << 1) ^ 0x1021 : crc << 1;
    }

    const auto *bytes = reinterpret_cast<const uchar *>(val.constData());
    const quint16 receivedCrc = qFromLittleEndian<quint16>(bytes + 2);
    return crc == receivedCrc;
}

bool GnssDevice::checkChecksum(const QByteArray &val)
{
    uint checksum = 0;
    QByteArray calcSum, string;
    QList<QByteArray> split = val.split('*');

    if (split.size() == 2) { // if not this is not a valid sentence (and the rest of code will fail miserably)
        calcSum = split.at(1).simplified();
        string = split.at(0);

        bool ok;
        uint val = calcSum.toUInt(&ok, 16);
        if (!ok) return false;

        for(int i=1; i<string.size(); i++) // skip inital sign ($)
            checksum ^= static_cast<quint8>(string[i]);
        if (checksum != val) return false;
        else return true;
    }
    return false;
}



void GnssDevice::updSettings() // caching these settings in memory since they are used often
{
    const GnssReceiverProfile profile = config->getGnssReceiverProfile(gnssData.id);
    if (!profile.enabled) gnss->close();

    activate = profile.enabled;
    portName = profile.portName;
    baudrate = profile.baudrate;
    logToFile = profile.logToFile;
    uBloxDevice = profile.ublox;
    AsciiTranslator::Options filenameOptions;
    filenameOptions.restrictToSafeAscii = true;
    profileLabel = AsciiTranslator::toAscii(profile.name, filenameOptions).simplified().replace(' ', '_');
    if (profileLabel.isEmpty()) profileLabel = gnssData.id;

    if (!gnss->isOpen() && !tcpSocket->isOpen() && activate) start(); // reconnect if needed
    if (!activate && tcpSocket->isOpen()) {
        tcpSocket->close();
        gnssData.posValid = false;
        emit gnssDisabled();
    }
    if (!activate && gnss->isOpen()) {
        gnss->close();
        gnssData.posValid = false;
        emit gnssDisabled();
    }
}

void GnssDevice::appendToLogfile(const QByteArray &data)
{
    if (!logfile.isOpen()) {
        logfile.setFileName(config->getLogFolder() + "/gnssLogs/" +
                            QDate::currentDate().toString("yyyyMMdd_") +
                            AsciiTranslator::toAscii(config->getStationName().trimmed()) +
                            "_gnss_" + profileLabel +
                            "_nmea.log");

        if (!logfile.open(QIODevice::Append)) {
            qWarning() << "Could not open GNSS log file" << binaryFile.fileName();
            return;
        }
        logfileStartedDate = QDate::currentDate();
        qDebug() << "GNSS logfile opened" << logfile.fileName();
    }
    QString ts = QDateTime::currentDateTimeUtc().toString(Qt::ISODateWithMs);
    logfile.write(ts.toLocal8Bit() + "," + data);
    logfile.flush();

    if (logfileStartedDate.daysTo(QDate::currentDate())) {
        logfile.close();
        qDebug() << "GNSS logfile closed, starting new";
    }
}

void GnssDevice::appendToBinaryfile(const QByteArray &data)
{    
    if (!binaryFile.isOpen()) {
        binaryFile.setFileName(config->getLogFolder() + "/gnssLogs/" +
                               QDate::currentDate().toString("yyyyMMdd_") +
                               AsciiTranslator::toAscii(config->getStationName().trimmed()) +
                               "_gnss_" + profileLabel +
                               "_binary.log");

        if (!binaryFile.open(QIODevice::Append)) {
            qWarning() << "Could not open GNSS log file" << binaryFile.fileName();
            return;
        }
        logfileStartedDate = QDate::currentDate();
        qDebug() << "GNSS logfile opened" << binaryFile.fileName();
    }
    // Binary timestamp - unix epoch time in ms as uint64, length in bytes in uint32, followed by data
    //QString ts = QDateTime::currentDateTimeUtc().toString(Qt::ISODateWithMs);
    quint64 timestamp = QDateTime::currentMSecsSinceEpoch();
    binaryFile.write(reinterpret_cast<const char*>(&timestamp), sizeof(timestamp));
    quint32 size = data.size();
    binaryFile.write(reinterpret_cast<const char*>(&size), sizeof(size));
    binaryFile.write(data);
    binaryFile.flush();

    if (logfileStartedDate.daysTo(QDate::currentDate())) {
        binaryFile.close();
        qDebug() << "GNSS logfile closed, starting new";
    }
}

void GnssDevice::checkPosValid()
{
    if (gnssData.posValid) {
        delayedReportTimer->stop();
        if (posInvalidTriggered) {
            posInvalidTriggered = false;
            if (config->getGnssShowNotifications())
                emit toIncidentLog(NOTIFY::TYPE::GNSSDEVICE, profileLabel, "Position valid");
        }
    } else if (!posInvalidTriggered && !delayedReportTimer->isActive()) {
        delayedReportTimer->start(config->getNotifyTruncateTime() * 1e3);
    }
}

void GnssDevice::connectTcpSocket()
{
    if (activate && !portName.isEmpty() && !baudrate.isEmpty() && tcpSocket->state() == QAbstractSocket::UnconnectedState && baudrate.toInt() > 0) {
        hostAddress->setAddress(portName);
        tcpSocket->connectToHost(*hostAddress, baudrate.toInt());
    }
}

void GnssDevice::handleTcpStateChange(QAbstractSocket::SocketState state)
{
    //qDebug() << "GNSS TCP debug" << state;
    if (state == QAbstractSocket::ConnectedState) {
        qDebug() << "Connected to" << portName << baudrate;
        if (!sendToAnalyzerTimer->isActive()) sendToAnalyzerTimer->start(1000);
        gnssData.inUse = true;
        tcpLastDataTimer.start();
        tcpWasConnected = true;
    }
    else {
        gnssData.inUse = false;
        sendToAnalyzerTimer->stop();
    }
    if (state == QAbstractSocket::UnconnectedState) { // retry conn. every 5 sec if valid settings
        if (!activate) {
            tcpWasConnected = false;
            return;
        }
        if (tcpWasConnected) {
            emit toIncidentLog(NOTIFY::TYPE::GNSSDEVICE, profileLabel, "GNSS TCP connection lost, will attempt to reconnect");
            tcpWasConnected = false;
        }
        QTimer::singleShot(5000, this, [this] {
            if (activate) connectTcpSocket();
        });
    }
}

void GnssDevice::checkTcpConnection()
{
    if (tcpSocket->state() != QAbstractSocket::ConnectedState) return;
    if (!tcpLastDataTimer.isValid() || tcpLastDataTimer.elapsed() < tcpDataTimeoutMs) return;

    emit toIncidentLog(NOTIFY::TYPE::GNSSDEVICE, profileLabel,
                        QString("GNSS TCP connection stalled (no data for %1 s), reconnecting").arg(tcpDataTimeoutMs / 1000));
    tcpSocket->abort(); // triggers handleTcpStateChange(UnconnectedState), which schedules the reconnect
}

void GnssDevice::askUbloxVersion()
{
    qDebug() << "asking for version info from USB device";
    if (gnss->write(QByteArray::fromHex("b5620a0400000e34")) == -1) { // Ask version!
        qDebug() << "GNSS write failed";
    }
    uBloxState = UBLOX::ASKEDVERSION;
}

void GnssDevice::decodeBinary0a04(const QByteArray &val)
{
    ++binaryMessageSequence;
    uBloxFirmware = QString::fromLatin1(val.mid(6, 30)).trimmed();
    if (val.contains("M91")) {
        qDebug() << "uBlox: M9 chip identified";
        uBloxID = "M91";
    }
    else if (val.contains("M8") || val.contains("00080000")) {
        qDebug() << "uBlox: M8 chip identified";
        uBloxID = "M8";
    }
    else if (val.contains("ZED-X20P")) {
        qDebug() << "uBlox: X20P chip identified";
        uBloxID = "X20P";
    }

    else if (val.contains("M10")) {
        qDebug() << "uBlox: M10 chip identified";
        uBloxID = "M10";
    }

    const QString receiverIdentity = uBloxID + "/" + QString::fromLatin1(val.mid(6, val.size() - 8).toHex());

    if (uBloxID.isEmpty()) {
        qDebug() << "Unknown GPS chip, trying to proceed anyway" << val;
        uBloxState = UBLOX::READY; // skip programming, we have no idea what to do
    }
    else {
        rfBandCorrelator.setReceiverIdentity(receiverIdentity);
        applyRfBandMapping(gnssData, {});
        qDebug() << "Trying to set up uBlox chip according to version...";
        setupUbloxDevice();
    }
}

void GnssDevice::setupUbloxDevice()
{
    if (uBloxID.contains("M9") || uBloxID.contains("M10")) {
        qDebug() << "uBlox M9/M10 configuration called";
        gnss->write(QByteArray::fromHex("b56206090c000000000000000000ffffffff1785")); // load defaults
        gnss->write(QByteArray::fromHex("b562068a0900000100005c03912001abfd")); // ubx-mon-rf on
        gnss->write(QByteArray::fromHex("b56206010300f00500ff19")); // vtg off
        gnss->write(QByteArray::fromHex("b562068a090000010000cc009120001720")); // gll off
        gnss->write(QByteArray::fromHex("b562068a090000010000b80091200104bd")); // gns on
        gnss->write(QByteArray::fromHex("b562068a0900000100000d00411001f956")); // itfm on (interference/jamming
        gnss->write(QByteArray::fromHex("b562068a09000001000010004120020d86")); // itfm active ant.
        gnss->write(QByteArray::fromHex("b562068a0900000100008e03912001ddf7")); // mon-span on (spectrum)
    }

    else if (uBloxID.contains("M8")) {
        qDebug() << "uBlox M8 configuration called";
        gnss->write(QByteArray::fromHex("b56206010300f00100fb11")); // GLL OFF
        gnss->write(QByteArray::fromHex("b562060103000a09011e70")); // mon-hw on
        gnss->write(QByteArray::fromHex("b56206010300f00500ff19")); // vtg off
        gnss->write(QByteArray::fromHex("b56206390800f3ac62ad231e000036ea")); // itfm on (interference/jamming
        gnss->write(QByteArray::fromHex("b56206010800f00d0101010101011293")); // gns on
    }

    else if (uBloxID.contains("X20P")) {
        qDebug() << "uBlox X20P configuration called";
        gnss->write(QByteArray::fromHex("b56206090c000000000000000000ffffffff1785")); // load defaults
        gnss->write(QByteArray::fromHex("b562068a0900000100005c03912001abfd")); // ubx-mon-rf on
        gnss->write(QByteArray::fromHex("b56206010300f00500ff19")); // vtg off
        gnss->write(QByteArray::fromHex("b562068a090000010000cc009120001720")); // gll off
        gnss->write(QByteArray::fromHex("b562068a090000010000b80091200104bd")); // gns on
        gnss->write(QByteArray::fromHex("b562068a0900000100000d00411001f956")); // itfm on (interference/jamming
        gnss->write(QByteArray::fromHex("b562068a09000001000010004120020d86")); // itfm active ant.
        gnss->write(QByteArray::fromHex("b562068a0900000100008e03912001ddf7")); // mon-span on (spectrum)
    }

    uBloxState = UBLOX::READY;
}

void GnssDevice::setIncidenceStartedDateTime()
{
    const QDateTime now = AppClock::now();

    // Trace/GNSS alarm signals can be emitted repeatedly while an incident is
    // active, so keep the first timestamp for the current backlog window.  If
    // no GNSS plot was created for a previous alarm (for instance because the
    // receiver had no backlog data), incidenceStartedDateTime could otherwise
    // stay valid indefinitely and be reused for a later incident, shifting the
    // plot marker by hundreds or thousands of seconds.
    const bool timestampOutsideBacklog =
        incidenceStartedDateTime.isValid()
        && !backlogTimestamp.isEmpty()
        && (incidenceStartedDateTime < backlogTimestamp.first()
            || incidenceStartedDateTime > backlogTimestamp.last());

    if (!incidenceStartedDateTime.isValid() || timestampOutsideBacklog)
        incidenceStartedDateTime = now;
}

void GnssDevice::delayedReportHandler()
{
    delayedReportTimer->stop();

    if (!posInvalidTriggered && gnssData.posValid) { // Position was invalid, but became valid while waiting the truncate time. Do nothing
        //
    }
    else if (!posInvalidTriggered && !gnssData.posValid) { // Position invalid for more than truncate time, report
        posInvalidTriggered = true;
        if (config->getGnssShowNotifications()) emit toIncidentLog(NOTIFY::TYPE::GNSSDEVICE, profileLabel, "Position invalid");
    }
    else if (posInvalidTriggered && !gnssData.posValid) { // Already reported invalid, do nothing
        //
    }
    else if (posInvalidTriggered && gnssData.posValid) { // Pos. was invalid, now ok. Report.
        posInvalidTriggered = false;
        if (config->getGnssShowNotifications()) emit toIncidentLog(NOTIFY::TYPE::GNSSDEVICE, profileLabel, "Position valid");
    }
}

QString GnssDevice::createFilename(const QString &suffix)
{
    QString dir = config->incidentFolder();
    QString filename;
    QTextStream ts(&filename);

    if (!QDir().exists(dir))
        QDir().mkpath(dir);

    ts << dir << "/" << config->incidentTimestamp().toString("yyyyMMddhhmmss")
       << "_" << AsciiTranslator::toAscii(config->getStationName())
       << "_" << "gnss-" << profileLabel << suffix << ".log";

    return filename;
}

void GnssDevice::updateBacklog(QByteArray &data, bool isBinary)
{
    backlogTimestamp.append(AppClock::now());
    backlogData.append(data);
    backlogIsBinary.append(isBinary);
    backlogAgc.append(gnssData.agc);
    for (auto iterator = backlogAgcBlocks.begin(); iterator != backlogAgcBlocks.end(); ++iterator)
        iterator.value().append(qQNaN());
    for (auto iterator = gnssData.agcBlocks.cbegin(); iterator != gnssData.agcBlocks.cend(); ++iterator) {
        if (!backlogAgcBlocks.contains(iterator.key()))
            backlogAgcBlocks.insert(iterator.key(), QVector<double>(backlogTimestamp.size(), qQNaN()));
        backlogAgcBlocks[iterator.key()].last() = iterator.value().agc;
        if (iterator.value().frequencyMapped)
            backlogAgcBlockLabels[iterator.key()] = iterator.value().frequencyLabel;
        else
            backlogAgcBlockLabels.remove(iterator.key());
    }
    backlogCno.append(gnssData.cno);
    backlogJam.append(gnssData.jammingIndicator);

}

void GnssDevice::cleanupBacklog()
{
    while (backlogTimestamp.size() && backlogData.size() && backlogTimestamp.first().secsTo(AppClock::now()) > 120) { // Keep backlog at 120 secs age
        backlogTimestamp.removeFirst();
        backlogData.removeFirst();
        backlogIsBinary.removeFirst();
        backlogAgc.removeFirst();
        for (auto iterator = backlogAgcBlocks.begin(); iterator != backlogAgcBlocks.end(); ++iterator) {
            if (!iterator.value().isEmpty()) iterator.value().removeFirst();
        }
        backlogCno.removeFirst();
        backlogJam.removeFirst();
    }
}

void GnssDevice::saveBacklog()
{
    backlogCleanupTimer->stop(); // Don't delete data while saving!

    if (backlogData.size()) { // Save only if there are any data in buffer (daah)
        if (!incidenceStartedDateTime.isValid())
            incidenceStartedDateTime = AppClock::now();

        createGnssPlot();

        // Mirror the continuous log scheme: separate timestamped text and binary files.
        QFile incidentTextFile, incidentBinaryFile;
        for (int i = 0; i < backlogData.size(); i++) {
            if (backlogIsBinary[i]) {
                if (!incidentBinaryFile.isOpen()) {
                    incidentBinaryFile.setFileName(createFilename("_binary"));
                    if (!incidentBinaryFile.open(QIODevice::WriteOnly))
                        qWarning() << "Could not open file" << incidentBinaryFile.fileName() << "for writing";
                }
                if (incidentBinaryFile.isOpen()) {
                    quint64 timestamp = backlogTimestamp[i].toMSecsSinceEpoch();
                    quint32 size = backlogData[i].size();
                    incidentBinaryFile.write(reinterpret_cast<const char*>(&timestamp), sizeof(timestamp));
                    incidentBinaryFile.write(reinterpret_cast<const char*>(&size), sizeof(size));
                    incidentBinaryFile.write(backlogData[i]);
                }
            }
            else {
                if (!incidentTextFile.isOpen()) {
                    incidentTextFile.setFileName(createFilename("_nmea"));
                    if (!incidentTextFile.open(QIODevice::WriteOnly))
                        qWarning() << "Could not open file" << incidentTextFile.fileName() << "for writing";
                }
                if (incidentTextFile.isOpen()) {
                    QString ts = backlogTimestamp[i].toUTC().toString(Qt::ISODateWithMs);
                    incidentTextFile.write(ts.toLocal8Bit() + "," + backlogData[i] + "\n");
                }
            }
        }
        if (incidentTextFile.isOpen()) incidentTextFile.close();
        if (incidentBinaryFile.isOpen()) incidentBinaryFile.close();
    }
    else {
        // Do not carry an alarm timestamp into the next incident when this
        // receiver had nothing to plot for the current one.
        incidenceStartedDateTime = QDateTime();
    }
    backlogCleanupTimer->start(1000);
}

void GnssDevice::createGnssPlot()
{
    QCustomPlot *plot = new QCustomPlot;
    plot->addGraph();
    plot->addGraph();
    plot->graph(0)->setPen(QPen(Qt::green));
    plot->graph(1)->setPen(QPen(Qt::blue));

    plot->setAutoAddPlottableToLegend(true);
    plot->graph(0)->setName("C/N0");
    plot->graph(1)->setName("Jamming ind");
    plot->legend->setVisible(true);
    plot->axisRect()->insetLayout()->setInsetAlignment(0, Qt::AlignBottom | Qt::AlignLeft);
    plot->xAxis->setLabel("Seconds since incident");
    plot->yAxis->setLabel("Value");

    QVector<double> keyVals;
    for (auto && timestamp : backlogTimestamp) {
        keyVals.append(-1 * timestamp.secsTo(incidenceStartedDateTime));
    }
    plot->graph(0)->setData(keyVals, backlogCno);
    plot->graph(1)->setData(keyVals, backlogJam);
    if (backlogAgcBlocks.isEmpty()) {
        plot->addGraph();
        plot->graph(2)->setPen(QPen(Qt::red));
        plot->graph(2)->setName("AGC");
        plot->graph(2)->setData(keyVals, backlogAgc);
    }
    else {
        const QList<QColor> colors = {Qt::red, Qt::magenta, Qt::darkRed, Qt::darkYellow};
        int graphIndex = 2;
        for (auto iterator = backlogAgcBlocks.cbegin(); iterator != backlogAgcBlocks.cend(); ++iterator) {
            plot->addGraph();
            plot->graph(graphIndex)->setPen(QPen(colors.at((graphIndex - 2) % colors.size())));
            const QString label = backlogAgcBlockLabels.value(iterator.key());
            plot->graph(graphIndex)->setName(label.isEmpty()
                                                  ? QString("AGC block %1").arg(iterator.key())
                                                  : QString("AGC block %1 - %2").arg(iterator.key()).arg(label));
            plot->graph(graphIndex)->setData(keyVals, iterator.value());
            ++graphIndex;
        }
    }
    plot->rescaleAxes();
    plot->replot();
    QString filename = createFilename().split(".log")[0] + ".jpg";
    plot->saveJpg(filename);
    delete plot;
    incidenceStartedDateTime = QDateTime();
    emit sendGnssPlotFilename(filename);
}

void GnssDevice::decodeSbf4014(const QByteArray &data)
{
    const uchar* p = reinterpret_cast<const uchar*>(data.constData());
    if (data.size() < 32) return;

    uint8_t N = p[28];
    uint8_t sbLen = p[29];
    if (sbLen < 2 || data.size() < 32 + N * sbLen) return;
    const uchar* sb = p + 32;

    QMap<int, int> agcBlocks; // frontEnd band id -> gain, main antenna only
    for (int i = 0; i < N; i++) {
        AGCEntry e;
        e.frontEnd = (uint8_t)sb[0] & 0x1f;
        e.gain = (int8_t)sb[1];
        e.ant = (uint8_t)sb[0] >> 5;
        // Gain -128 means the frontend PLL is not locked (Do-Not-Use value)
        if (e.ant == 0 && e.gain != -128) {
            // Hide bands with no active tracked signal, once we've actually seen MeasEpoch data.
            // Front-end 13 (S-band) carries no GNSS signal and is always shown if present.
            const bool hasTrackedSignal = !septentrioMeasEpochSeen || e.frontEnd == 13
                || septentrioFrontEndSignalTypes(e.frontEnd).intersects(septentrioActiveSignalTypes);
            if (hasTrackedSignal) {
                agcBlocks.insert(e.frontEnd, e.gain);
                GnssAgcBlockData &block = gnssData.agcBlocks[e.frontEnd];
                block.agc = e.gain;
                block.frequencyMapped = true;
                block.centerHz = septentrioFrontEndCenterHz(e.frontEnd);
                block.frequencyLabel = septentrioFrontEndLabel(e.frontEnd, block.centerHz);
            }
        }
        sb += sbLen;
    }

    const auto previousBlockIds = gnssData.agcBlocks.keys();
    for (int blockId : previousBlockIds) {
        if (!agcBlocks.contains(blockId)) gnssData.agcBlocks.remove(blockId);
    }

    if (!agcBlocks.isEmpty()) {
        int sum = 0;
        for (int gain : agcBlocks) sum += gain;
        gnssData.agc = sum / agcBlocks.size();
    }
}

void GnssDevice::decodeSbf4092(const QByteArray &data)
{
    SeptentrioRfStatus status;
    if (SeptentrioRfStatusParser::parse(data, status)) {
        gnssData.septentrioRfStatus = status;
        gnssData.septentrioRfStatusTimestamp = AppClock::nowUtc();
    }
}

void GnssDevice::decodeSbf4027(const QByteArray &data)
{
    const uchar* p = reinterpret_cast<const uchar*>(data.constData());
    if (data.size() < 20) return;

    uint8_t N1 = p[14];
    uint8_t SB1Length = p[15];
    uint8_t SB2Length = p[16];
    if (SB1Length < 20 || SB2Length < 12) return;

    QSet<int> activeSignalTypes;
    QSet<int> trackedSatellites;
    QMap<int, double> satelliteCno;
    QStringList satelliteSystems;
    const auto addMeasurement = [&](int satelliteId, int signalIndex, quint8 rawCno) {
        const QString system = septentrioSatelliteSystem(satelliteId);
        if (system.isEmpty()) return;
        trackedSatellites.insert(satelliteId);
        if (!satelliteSystems.contains(system)) satelliteSystems.append(system);
        if (rawCno == 255) return;
        const double cno = rawCno * 0.25 + ((signalIndex == 1 || signalIndex == 2) ? 0 : 10);
        if (!satelliteCno.contains(satelliteId) || cno > satelliteCno.value(satelliteId))
            satelliteCno[satelliteId] = cno;
    };
    int off = 20;
    for (int i = 0; i < N1; i++) {
        if (off + SB1Length > data.size()) return; // malformed/truncated, keep previous state
        const uchar *type1 = p + off;
        uint8_t type = type1[1];
        uint8_t antenna = type >> 5;
        uint8_t sigIdxLo = type & 0x1f;
        uint8_t obsInfo = type1[18];
        if (antenna == 0) {
            int sigIdx = sigIdxLo == 31 ? 32 + (obsInfo >> 3) : sigIdxLo;
            activeSignalTypes.insert(sigIdx);
            addMeasurement(type1[2], sigIdx, type1[15]);
        }
        uint8_t N2 = type1[19];

        int type2Off = off + SB1Length;
        for (int j = 0; j < N2; j++) {
            if (type2Off + SB2Length > data.size()) return;
            const uchar *type2 = p + type2Off;
            uint8_t type2Field = type2[0];
            uint8_t antenna2 = type2Field >> 5;
            uint8_t sigIdxLo2 = type2Field & 0x1f;
            if (antenna2 == 0) {
                uint8_t obsInfo2 = SB2Length > 5 ? type2[5] : 0;
                int sigIdx2 = sigIdxLo2 == 31 ? 32 + (obsInfo2 >> 3) : sigIdxLo2;
                activeSignalTypes.insert(sigIdx2);
                addMeasurement(type1[2], sigIdx2, type2[2]);
            }
            type2Off += SB2Length;
        }
        off = type2Off;
    }

    septentrioActiveSignalTypes = activeSignalTypes;
    septentrioMeasEpochSeen = true;
    gnssData.satsTracked = trackedSatellites.size();
    gnssData.gnssType = satelliteSystems.join(' ');
    QList<double> cnoValues = satelliteCno.values();
    std::sort(cnoValues.begin(), cnoValues.end(), [](double first, double second) { return first > second; });
    double cnoSum = 0;
    const qsizetype cnoCount = qMin(qsizetype(4), cnoValues.size());
    for (qsizetype index = 0; index < cnoCount; ++index) cnoSum += cnoValues.at(index);
    gnssData.cno = cnoCount > 0 ? cnoSum / cnoCount : 0;
}

void GnssDevice::decodeSbf4013(const QByteArray &data)
{
    // ChannelStatus: N is the number of ChannelSatInfo sub-blocks, i.e. satellites currently
    // allocated to a tracking channel (section 4.2.16). Inactive channels are not included.
    const uchar* p = reinterpret_cast<const uchar*>(data.constData());
    if (data.size() < 20) return;

    uint8_t N = p[14];
    if (!septentrioMeasEpochSeen) gnssData.satsTracked = N;
}

void GnssDevice::decodeSbf4040(const QByteArray &data)
{
    // BBSamples: raw complex baseband I/Q samples (section 4.2.x). No pre-computed spectrum is
    // available from Septentrio, unlike u-blox MON-SPAN, so the spectrum is built here with an FFT.
    const uchar *p = reinterpret_cast<const uchar*>(data.constData());
    if (data.size() < 28) return;

    const quint16 N = qFromLittleEndian<quint16>(p + 14);
    const quint8 antenna = p[16] & 0x07;
    if (antenna != 0) return; // main antenna only, consistent with the rest of the Septentrio decoding
    if (N == 0 || data.size() < 28 + 2 * N) return;

    const quint32 sampleFreq = qFromLittleEndian<quint32>(p + 20);
    const quint32 loFreq = qFromLittleEndian<quint32>(p + 24);
    if (sampleFreq == 0) return;

    fftw_complex *in = (fftw_complex *)fftw_malloc(sizeof(fftw_complex) * N);
    fftw_complex *out = (fftw_complex *)fftw_malloc(sizeof(fftw_complex) * N);
    for (quint16 i = 0; i < N; i++) {
        // Each sample is a little-endian u2: low byte Q, high byte I, both two's complement (section 4.2.x)
        in[i][0] = static_cast<double>(static_cast<qint8>(p[28 + i * 2 + 1])); // I
        in[i][1] = static_cast<double>(static_cast<qint8>(p[28 + i * 2]));     // Q
    }

    fftw_plan plan = fftw_plan_dft_1d(N, in, out, FFTW_FORWARD, FFTW_ESTIMATE);
    fftw_execute(plan);

    GnssSpectrumBlock block;
    block.levels.resize(N);
    int index = 0;
    for (int i = N / 2; i < N; i++) { // reorder so negative frequencies come first, like IqPlot's FFT display
        double magnitude = sqrt(out[i][0] * out[i][0] + out[i][1] * out[i][1]);
        block.levels[index++] = 20 * log10(magnitude > 1e-10 ? magnitude : 1e-10);
    }
    for (int i = 0; i < N / 2; i++) {
        double magnitude = sqrt(out[i][0] * out[i][0] + out[i][1] * out[i][1]);
        block.levels[index++] = 20 * log10(magnitude > 1e-10 ? magnitude : 1e-10);
    }

    fftw_destroy_plan(plan);
    fftw_free(in);
    fftw_free(out);

    block.centerHz = loFreq;
    block.spanHz = sampleFreq; // complex I/Q sampling: unambiguous span is the full sample rate, centered on loFreq
    block.resolutionHz = sampleFreq / N;
    block.timestamp = QDateTime::currentDateTime();

    // The receiver retunes its front-end across different GNSS bands over time to build a full
    // spectrum view (section 1.28), so successive BBSamples messages can report different LOFreq
    // values - track each distinct one as its own selectable band, same concept as u-blox's
    // per-message band array.
    if (!septentrioSpectrumBandIndexByCenterHz.contains(loFreq))
        septentrioSpectrumBandIndexByCenterHz.insert(loFreq, septentrioSpectrumBandIndexByCenterHz.size());
    block.blockIndex = septentrioSpectrumBandIndexByCenterHz.value(loFreq);
    block.blockCount = septentrioSpectrumBandIndexByCenterHz.size();

    emit spectrumReady(block);
}
