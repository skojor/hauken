#include "instrumentlist.h"

InstrumentList::InstrumentList(QObject *parent)
{
    this->setParent(parent);
    curlProcess->setWorkingDirectory(QDir(QCoreApplication::applicationDirPath()).absolutePath());

    if (QSysInfo::kernelType().contains("win")) {
        curlProcess->setProgram("curl.exe");
    }

    else if (QSysInfo::kernelType().contains("linux")) {
        curlProcess->setProgram("curl");
    }

}

void InstrumentList::start()
{
    isStarted = true;
    if (!server.isEmpty()) emit askForLogin();
    loadFile();
    if (!usableStnIps.isEmpty()) emit instrumentListReady(usableStnIps, usableStnNames, usableStnTypes);
}

void InstrumentList::updSettings()
{
    QString tmpServer = getIpAddressServer();
    if (tmpServer != server) { // address changed, reconnect
        server = tmpServer;
        if (isStarted && !server.isEmpty()) emit  askForLogin();
    }
}

void InstrumentList::checkConnection()
{
    curlProcess->close();

    QStringList reportArgs;
    reportArgs << "--cookie" << getWorkFolder() + "/.kake" << "-k" << server + "?action=ListStations";
    curlProcess->setArguments(reportArgs);
    curlProcess->start();

    connect(curlProcess, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished), this, &InstrumentList::checkStationListReturn);
}

void InstrumentList::parseStationList(const QByteArray &ba)
{
    QJsonDocument jsonDoc = QJsonDocument::fromJson(ba);
    QJsonObject jsonObject = jsonDoc.object();
    QJsonObject metaObject = jsonObject.value("metadata").toObject();
    QJsonObject dataObject = jsonObject.value("data").toObject();

    int nrOfElements = metaObject.value("totaltAntallTreff").toInt(0);
    if (nrOfElements > 0) {
        nrOfStations = nrOfElements;
        QJsonArray data = jsonObject.value("data").toArray();
        if (data.size() == nrOfElements) { // Super simple fault check, improve! TODO
            for (auto &&array : data) {
                StationInfo stn;
                stn.StationIndex = array.toObject().value("SInd").toString().toInt();
                stn.name = array.toObject().value("Navn").toString();
                stn.officialName = array.toObject().value("Off_navn").toString();
                stn.address = array.toObject().value("Adresse").toString();
                stn.latitude = array.toObject().value("Latitude").toString().toDouble();
                stn.longitude = array.toObject().value("Longitude").toString().toDouble();
                stn.status = array.toObject().value("Status").toString();
                stn.type = (StationType)array.toObject().value("Type").toString().toInt();
                stn.mmsi = array.toObject().value("MMSI").toString().toULong();
                int active = array.toObject().value("Aktiv").toString().toInt();
                if (active == 0) stn.active = false; else stn.active = true;

                stationInfo.append(stn);
            }
        }
        // We should have a list of stations here, time to ask for what they can do
        stnIndex = 0;
        disconnect(curlProcess, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished), this, &InstrumentList::checkStationListReturn);
        connect(curlProcess, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished), this, &InstrumentList::checkInstrumentListReturn);
        askForInstruments();
    }
}

void InstrumentList::parseInstrumentList(const QByteArray &ba)
{
    QJsonDocument jsonDoc = QJsonDocument::fromJson(ba);
    QJsonObject jsonObject = jsonDoc.object();
    QJsonObject metaObject = jsonObject.value("metadata").toObject();
    QJsonObject dataObject = jsonObject.value("data").toObject();

    int nrOfElements = metaObject.value("totaltAntallTreff").toInt(0);
    if (nrOfElements > 0) {
        QJsonArray data = jsonObject.value("data").toArray();
        if (data.size() == nrOfElements) { // Super simple fault check, improve! TODO
            for (auto &&array : data) {
                InstrumentInfo info;
                info.index = array.toObject().value("Ind").toString().toInt();
                info.equipmentIndex = array.toObject().value("Utstyr_ind").toString().toInt();
                QString category = array.toObject().value("Kategori").toString();
                if (category.contains("Mottaker")) info.category = InstrumentCategory::RECEIVER;
                else if (category.contains("PC")) info.category = InstrumentCategory::PC;
                else info.category = InstrumentCategory::UNKNOWN;
                info.frequencyRange = array.toObject().value("Frekvensområde").toString();
                info.ipAddress = array.toObject().value("IP").toString();
                info.producer = array.toObject().value("Produsent").toString();
                info.type = array.toObject().value("Type").toString();
                info.serialNumber = array.toObject().value("Serienummer").toString();

                stationInfo[stnIndex].instrumentInfo.append(info);
            }
        }
    }
    if (++stnIndex < nrOfStations) askForInstruments(); // keep asking until every stn is queried
    else {      // now ask for equipment list
        disconnect(curlProcess, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished), this, &InstrumentList::checkInstrumentListReturn);
        connect(curlProcess, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished), this, &InstrumentList::checkEquipmentListReturn);
        askForEquipmentList();
    }
}

void InstrumentList::loadFile()
{
    QFile file(getWorkFolder() + "/stationdata.csv");
    QTextStream ts(&file);

    if (!file.open(QIODevice::ReadOnly)) {
        qDebug() << "Couldn't open station data file" << file.fileName();
    }
    else {
        while (!file.atEnd()) {
            QString str = file.readLine();
            QStringList split = str.split(";");
            if (split.size() == 3) {
                usableStnIps.append(split[0].trimmed());
                usableStnNames.append(split[1].trimmed());
                usableStnTypes.append(split[2].trimmed());
            }
        }
    }
    file.close();
}

void InstrumentList::saveFile()
{
    QFile file(getWorkFolder() + "/stationdata.csv");
    QTextStream ts(&file);

    if (!file.open(QIODevice::WriteOnly)) {
        qDebug() << "Couldn't save station data to file" << file.fileName();
    }
    else {
        for (int i=0; i < usableStnIps.size(); i++) {
            ts << usableStnIps[i] << ";" << usableStnNames[i] << ";" << usableStnTypes[i] << Qt::endl;
        }
    }
    file.close();
}

void InstrumentList::checkStationListReturn(int exitCode, QProcess::ExitStatus)
{
    QByteArray ba = curlProcess->readAllStandardOutput();
    if (exitCode == 0) {
        parseStationList(ba);
    }
    else {
        qDebug() << "InstrumentList: Error connecting, giving up";
    }

}

void InstrumentList::checkInstrumentListReturn(int exitCode, QProcess::ExitStatus)
{
    QByteArray ba = curlProcess->readAllStandardOutput();
    if (exitCode == 0) {
        parseInstrumentList(ba);
    }
    else {
        qDebug() << "InstrumentList: Error retreiving instrument data, giving up";
    }

}

void InstrumentList::loginCompleted() // other class confirms login successful, continue
{
    checkConnection();
}

void InstrumentList::askForInstruments()
{
    curlProcess->close();

    QStringList reportArgs;
    reportArgs << "--cookie" << getWorkFolder() + "/.kake" << "-k"
               << server + "?action=ListInstruments&stnId=" + QString::number(stationInfo[stnIndex].StationIndex);
    curlProcess->setArguments(reportArgs);
    curlProcess->start();
}

void InstrumentList::askForEquipmentList()
{
    curlProcess->close();

    QStringList reportArgs;
    reportArgs << "--cookie" << getWorkFolder() + "/.kake" << "-k"
               << server + "?action=ListUtstyr";
    curlProcess->setArguments(reportArgs);
    curlProcess->start();
}

void InstrumentList::checkEquipmentListReturn(int exitCode, QProcess::ExitStatus)
{
    QByteArray ba = curlProcess->readAllStandardOutput();
    if (exitCode == 0) {
        parseEquipmentList(ba);
    }
    else {
        qDebug() << "InstrumentList: Error retreiving equipment data, giving up";
    }
}

void InstrumentList::parseEquipmentList(const QByteArray &ba)
{
    QJsonDocument jsonDoc = QJsonDocument::fromJson(ba);
    QJsonObject jsonObject = jsonDoc.object();
    QJsonObject metaObject = jsonObject.value("metadata").toObject();
    QJsonObject dataObject = jsonObject.value("data").toObject();

    int nrOfElements = metaObject.value("totaltAntallTreff").toInt(0);
    if (nrOfElements > 0) {
        QJsonArray data = jsonObject.value("data").toArray();
        if (data.size() == nrOfElements) { // Super simple fault check, improve! TODO
            for (auto &&array : data) {
                EquipmentInfo info;
                info.index = array.toObject().value("Ind").toString().toInt();
                info.type = array.toObject().value("Type").toString();
                equipmentInfo.append(info);
            }
        }
    }
    updStationInfoWithEquipmentList(); // Finally we should have enough data to generate a complete equipment list per station
}

void InstrumentList::updStationInfoWithEquipmentList()
{
    for (auto &station : stationInfo) {
        for (auto &instrument : station.instrumentInfo) {
            for (auto &type : equipmentInfo) {
                if (instrument.equipmentIndex == type.index) {
                    instrument.type = type.type;
                    break;
                }
            }
        }
    }
    qDebug() << "Done parsing station/instrument list";
    generateInstrumentList();
    saveFile();
}

void InstrumentList::generateInstrumentList()
{
    usableStnIps.clear();
    usableStnNames.clear();
    usableStnTypes.clear();

    for (auto &station: stationInfo) {
        for (auto &instrument : station.instrumentInfo) {
            if (instrument.type.contains("EM200") || instrument.type.contains("EM100") || instrument.type.contains("PR200") ||
                instrument.type.contains("PR100") || instrument.type.contains("EB500") || instrument.type.contains("ESMW") ||
                instrument.type.contains("ESMB") || instrument.type.contains("USRP"))
            {
                usableStnIps.append(instrument.ipAddress);
                usableStnNames.append(station.officialName);
                usableStnTypes.append(instrument.type);
            }
        }
    }
    //qDebug() << usableStnIps << usableStnNames << usableStnTypes;
}
