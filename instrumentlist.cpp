#include "instrumentlist.h"
#include <locale>
#include <algorithm>

InstrumentList::InstrumentList(QSharedPointer<Config> c)
{
    config = c;
    connect(tmNetworkTimeout, &QTimer::timeout, this, &InstrumentList::tmNetworkTimeoutHandler);
    connect(networkAccessManager,
            &QNetworkAccessManager::finished,
            this,
            &InstrumentList::networkAccessManagerFinished);

    networkAccessManager->setCookieJar(cookieJar);
    //QTimer::singleShot(100, this, [this]() { loadFile(); });
}

void InstrumentList::loadFile()
{
    QFile file(config->getWorkFolder() + "/stationdata.csv");
    //QTextStream ts(&file);

    if (!file.open(QIODevice::ReadOnly)) {
        qDebug() << "Couldn't open station data file" << file.fileName();
    } else {
        while (!file.atEnd()) {
            QString str = file.readLine();
            QStringList split = str.split(";");
            if (split.size() == 3) {
                usableStnNames.append(split[0].trimmed());
                usableStnIps.append(split[1].trimmed());
                usableStnTypes.append(split[2].trimmed());
            }
        }
        qDebug() << "Loaded station data from file" << file.fileName();
    }
    file.close();
    sortList();
    emit listReady(usableStnNames, usableStnIps, usableStnTypes);
}

void InstrumentList::saveFile()
{
    QFile file(config->getWorkFolder() + "/stationdata.csv");
    QTextStream ts(&file);

    if (!file.open(QIODevice::WriteOnly)) {
        qDebug() << "Couldn't save station data to file" << file.fileName();
    } else {
        for (int i = 0; i < usableStnIps.size(); i++) {
            ts << usableStnNames[i] << ";" << usableStnIps[i] << ";" << usableStnTypes[i]
               << Qt::endl;
        }
        qDebug() << "Saved station data to" << file.fileName();
    }
    file.close();
}

void InstrumentList::tmNetworkTimeoutHandler()
{
    tmNetworkTimeout->stop();
    qWarning() << "Network request timed out";
    state = FAILED;
    emit instrumentListFailed("Network request timed out");
}

void InstrumentList::networkAccessManagerFinished(QNetworkReply *reply)
{
    tmNetworkTimeout->stop();
    fetchDataHandler(reply->readAll());
}

void InstrumentList::parseLoginReply(const QByteArray &reply)
{
    if (!networkAccessManager->cookieJar()
             ->cookiesForUrl(config->getIpAddressServer())
             .isEmpty()) {
        qDebug() << "Cookie received, proceeding";
        state = LOGGEDIN;
    } else {
        qWarning() << "Couldn't login, aborting";
        state = FAILED;
    }
    fetchDataHandler();
}

void InstrumentList::loginRequest()
{
    QNetworkRequest networkRequest;
    networkRequest.setUrl(config->getSdefAuthAddress());
    networkRequest.setHeader(QNetworkRequest::ContentTypeHeader,
                             "application/x-www-form-urlencoded");
    QUrlQuery urlQuery;
    urlQuery.addQueryItem("brukernavn", config->getSdefUsername());
    urlQuery.addQueryItem("passord", config->getSdefPassword());
    tmNetworkTimeout->start(10000);
    networkReply = networkAccessManager->post(networkRequest,
                                              urlQuery.toString(QUrl::FullyEncoded).toUtf8());
}

void InstrumentList::stationListRequest()
{
    QNetworkRequest networkRequest;
    networkRequest.setUrl(config->getIpAddressServer() + "?action=ListStations");
    networkRequest.setHeader(QNetworkRequest::ContentTypeHeader,
                             "application/x-www-form-urlencoded");

    tmNetworkTimeout->start(10000);
    networkReply = networkAccessManager->get(networkRequest);
    state = ASKEDFORSTATIONS;
}

void InstrumentList::parseStationList(const QByteArray &reply)
{
    QJsonDocument jsonDoc = QJsonDocument::fromJson(reply);
    QJsonObject jsonObject = jsonDoc.object();
    QJsonObject metaObject = jsonObject.value("metadata").toObject();

    int nrOfElements = metaObject.value("totaltAntallTreff").toInt(0);
    if (nrOfElements > 0) {
        //nrOfStations = nrOfElements;
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
                stn.type = (StationType) array.toObject().value("Type").toString().toInt();
                stn.mmsi = array.toObject().value("MMSI").toString().toULong();
                int active = array.toObject().value("Aktiv").toString().toInt();
                if (active == 0)
                    stn.active = false;
                else
                    stn.active = true;

                stationInfo.append(stn);
            }
        }
        // We should have a list of stations here, time to ask for what they can do
        stationIndex = 0;
        state = RECEIVEDSTATIONS;
        fetchDataHandler();
    }
    else { // Failed request
        tmNetworkTimeout->stop();
        state = FAILED;
        fetchDataHandler(); // Will deal with error msgs
    }
}

void InstrumentList::instrumentListRequest()
{
    QNetworkRequest networkRequest;
    networkRequest.setUrl(QUrl(config->getIpAddressServer() + "?action=ListInstruments&stnId="
                               + QString::number(stationInfo[stationIndex].StationIndex)));
    networkReply = networkAccessManager->get(networkRequest);
    state = ASKEDFORINSTRUMENTS;
}

void InstrumentList::parseInstrumentList(const QByteArray &reply)
{
    QJsonDocument jsonDoc = QJsonDocument::fromJson(reply);
    QJsonObject jsonObject = jsonDoc.object();
    QJsonObject metaObject = jsonObject.value("metadata").toObject();

    int nrOfElements = metaObject.value("totaltAntallTreff").toInt(0);
    if (nrOfElements > 0) {
        QJsonArray data = jsonObject.value("data").toArray();
        if (data.size() == nrOfElements) { // Super simple fault check, improve! TODO
            for (auto &&array : data) {
                InstrumentInfo info;
                info.index = array.toObject().value("Ind").toString().toInt();
                info.equipmentIndex = array.toObject().value("Utstyr_ind").toString().toInt();
                QString category = array.toObject().value("Kategori").toString();
                if (category.contains("Mottaker"))
                    info.category = InstrumentCategory::RECEIVER;
                else if (category.contains("PC"))
                    info.category = InstrumentCategory::PC;
                else
                    info.category = InstrumentCategory::UNKNOWN;
                info.frequencyRange = array.toObject().value("Frekvensområde").toString();
                info.ipAddress = array.toObject().value("IP").toString();
                info.producer = array.toObject().value("Produsent").toString();
                info.type = array.toObject().value("Type").toString();
                info.serialNumber = array.toObject().value("Serienummer").toString();

                stationInfo[stationIndex].instrumentInfo.append(info);
            }
        }
    }
    else { // Failed request
        tmNetworkTimeout->stop();
        state = FAILED;
        fetchDataHandler(); // Will deal with error msgs
    }
    stationIndex++;
    if (stationIndex == stationInfo.size()) {
        state = RECEIVEDINSTRUMENTS;
        fetchDataHandler();
    } else {
        instrumentListRequest(); // Keep asking until all stns queried
    }
}

void InstrumentList::equipmentListRequest()
{
    QNetworkRequest networkRequest;
    networkRequest.setUrl(QUrl(config->getIpAddressServer() + "?action=ListUtstyr"));
    if (!tmNetworkTimeout->isActive())
        tmNetworkTimeout->start(30000);
    networkReply = networkAccessManager->get(networkRequest);
    state = ASKEDFOREQUIPMENT;
}

void InstrumentList::parseEquipmentList(const QByteArray &reply)
{
    QJsonDocument jsonDoc = QJsonDocument::fromJson(reply);
    QJsonObject jsonObject = jsonDoc.object();
    QJsonObject metaObject = jsonObject.value("metadata").toObject();

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
    state = RECEIVEDEQUIPMENT;
    fetchDataHandler();
}

void InstrumentList::updStationsWithEquipmentList()
{
    for (auto &&station : stationInfo) {
        for (auto &&instrument : station.instrumentInfo) {
            for (auto &&type : equipmentInfo) {
                if (instrument.equipmentIndex == type.index) {
                    instrument.type = type.type;
                    break;
                }
            }
        }
    }
}

void InstrumentList::fetchDataHandler(const QByteArray &reply)
{
    if (state == BEGIN) {
        loginRequest();
        state = ASKEDFORLOGIN;
        qDebug() << ("Logging in");
        emit instrumentListStarted("Logging in");
    } else if (state == ASKEDFORLOGIN) {
        parseLoginReply(reply);
    } else if (state == LOGGEDIN) {
        qDebug() << ("Logged in, asking for station data");
        emit instrumentListStarted("Logged in, asking for station data");
        stationListRequest();
        state = ASKEDFORSTATIONS;
    } else if (state == ASKEDFORSTATIONS) {
        parseStationList(reply);
    } else if (state == RECEIVEDSTATIONS) {
        qDebug() << ("Received station data, now asking for instruments");
        emit instrumentListStarted("Received station data, now asking for instruments");
        instrumentListRequest();
    } else if (state == ASKEDFORINSTRUMENTS) {
        parseInstrumentList(reply);
    } else if (state == RECEIVEDINSTRUMENTS) {
        qDebug() << ("Received instruments, now asking for equipment list");
        emit instrumentListStarted("Received instruments, now asking for equipment list");
        equipmentListRequest();
    } else if (state == ASKEDFOREQUIPMENT) {
        parseEquipmentList(reply);
    } else if (state == RECEIVEDEQUIPMENT) {
        updStationsWithEquipmentList();
        state = DONE;
        qDebug() << ("Station list updated");
        generateLists();
        sortList();
        saveFile();
        emit listReady(usableStnNames, usableStnIps, usableStnTypes);
        emit instrumentListDownloaded("Updated instrument list, " +
                                      QString::number(usableStnNames.size()) +
                                      " instruments total");
    } else if (state == FAILED) {
        qWarning() << "Failed retrieving station list, giving up";
        emit instrumentListFailed("Failed retrieving station list");
    }
}

void InstrumentList::generateLists()
{
    usableStnIps.clear();
    usableStnNames.clear();
    usableStnTypes.clear();

    for (auto &&station : stationInfo) {
        for (auto &&instrument : station.instrumentInfo) {
            if (instrument.type.contains("EM200") || instrument.type.contains("EM100") || instrument.type.contains("PR200") ||
                instrument.type.contains("PR100") || instrument.type.contains("EB500") || instrument.type.contains("ESMW") ||
                instrument.type.contains("ESMB") || instrument.type.contains("USRP") || instrument.type.contains("ESMD"))
            {
                usableStnIps.append(instrument.ipAddress);
                usableStnNames.append(station.officialName);
                usableStnTypes.append(instrument.type);
            }
        }
    }
}

void InstrumentList::sortList()
{
    std::locale no("nb_NO.UTF-8");   // Sort norwegian letters as they should be
    QStringList sort;
    for (int i=0; i < usableStnNames.size(); i++)
        sort.append(usableStnNames[i] + ";" + usableStnIps[i] + ";" + usableStnTypes[i]);

    std::sort(sort.begin(), sort.end(),
              [](const QString &a, const QString &b) {
                  return QString::localeAwareCompare(a, b) < 0;
              });

    usableStnNames.clear();
    usableStnIps.clear();
    usableStnTypes.clear();

    for (auto && line : sort) {
        QStringList split = line.split(';');
        if (split.size() == 3) {
            usableStnNames.append(split[0]);
            usableStnIps.append(split[1]);
            usableStnTypes.append(split[2]);
        }
    }
}
