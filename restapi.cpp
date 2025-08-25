#include "restapi.h"

RestApi::RestApi(QSharedPointer<Config> c)
{
    config = c;
    tcpServer->listen(QHostAddress::Any, LISTEN_PORT);
    httpServer->bind(tcpServer);

    httpServer->route("/api",
                      QHttpServerRequest::Method::Post,
                      [this](const QHttpServerRequest &request) {
                          if (!authorize(request))
                              return QHttpServerResponse(
                                  QHttpServerResponder::StatusCode::Unauthorized);
                          const QJsonObject newObject = byteArrayToJsonObject(request.body());
                          parseJson(newObject);
                          return QHttpServerResponse(newObject,
                                                     QHttpServerResponder::StatusCode::Accepted);
                      });
}

RestApi::~RestApi()
{
    httpServer->deleteLater();
    tcpServer->close();
    tcpServer->deleteLater();
}

bool RestApi::authorize(const QHttpServerRequest &request)
{
#ifdef Q_OS_WIN
    //qDebug() << config->getRestKey() << config->getRestSecret();
    for (auto &&[key, value] : request.headers().toListOfPairs()) {
        if (!config->getRestKey().isEmpty() && !config->getRestSecret().isEmpty()
            && key == config->getRestKey() && value == config->getRestSecret()) {
            return true;
        }
    }
#endif
    return false;
}

QJsonObject RestApi::byteArrayToJsonObject(const QByteArray &arr)
{
#ifdef Q_OS_WIN
    QJsonParseError parseError;
    QJsonDocument json = QJsonDocument::fromJson(arr, &parseError);
    if (!json.isObject()) {
        qDebug() << "RestApi: Could not parse POST data:" << parseError.errorString();
    }

    return json.object();
#endif
}

void RestApi::parseJson(QJsonObject object)
{
    QJsonObject::const_iterator iterator = object.constBegin();
    while (iterator < object.constEnd()) {
        if (iterator.key().contains("pscanStartFreq", Qt::CaseInsensitive)) {
            double f = iterator.value().toDouble(-1);
            if (f > 0 && f < 10e3)
                emit pscanStartFreq(f);
        } else if (iterator.key().contains("pscanStopFreq", Qt::CaseInsensitive)) {
            double f = (double) iterator.value().toDouble(-1);
            if (f > 0 && f < 10e3)
                emit pscanStopFreq(f);
        } else if (iterator.key().contains("pscanResolution", Qt::CaseInsensitive)) {
            emit pscanResolution(iterator.value().toString());
        } else if (iterator.key().contains("measurementtime", Qt::CaseInsensitive)) {
            int f = iterator.value().toDouble(-1);
            if (f > 0 && f < 10000)
                emit measurementTime(f);
        } else if (iterator.key().contains("manualatt", Qt::CaseInsensitive)) {
            int f = iterator.value().toDouble(-1);
            if (f >= 0 && f < 40)
                emit manualAtt(f);
        } else if (iterator.key().contains("autoatt", Qt::CaseInsensitive)) {
            emit autoAtt(iterator.value().toBool(false));
        } else if (iterator.key().contains("antport", Qt::CaseInsensitive)) {
            int f = iterator.value().toDouble(-1);
            if (f == 1 || f == 2)
                emit antport(f);
        } else if (iterator.key().contains("instrmode", Qt::CaseInsensitive)) {
            if (iterator.value().toString().contains("pscan", Qt::CaseInsensitive))
                emit mode("pscan");
            else if (iterator.value().toString().contains("ffm", Qt::CaseInsensitive))
                emit mode("ffm");
        } else if (iterator.key().contains("fftmode", Qt::CaseInsensitive)) {
            if (iterator.value().toString().contains("off", Qt::CaseInsensitive))
                emit fftmode("Off");
            else if (iterator.value().toString().contains("min", Qt::CaseInsensitive))
                emit fftmode("Min");
            else if (iterator.value().toString().contains("max", Qt::CaseInsensitive))
                emit fftmode("Max");
            else if (iterator.value().toString().contains("scalar", Qt::CaseInsensitive))
                emit fftmode("Scalar");
            else if (iterator.value().toString().contains("apeak", Qt::CaseInsensitive))
                emit fftmode("APeak");
        } else if (iterator.key().contains("gaincontrol", Qt::CaseInsensitive)) {
            if (iterator.value().toString().contains("normal", Qt::CaseInsensitive))
                emit gaincontrol("Normal");
            else if (iterator.value().toString().contains("lownoise", Qt::CaseInsensitive))
                emit gaincontrol("Low noise");
            else if (iterator.value().toString().contains("lowdistortion", Qt::CaseInsensitive))
                emit gaincontrol("Low distortion");

            // Config settings changes happen here. WARNING, no error checks here, quite possible to destroy the whole config file...
        } else if (iterator.key().contains("settings", Qt::CaseInsensitive)) {
            QStringList split = iterator.value().toString().split(' ');
            if (split.size() == 2) { // We should have a valid key/value pair now
                config->settings->setValue(split[0], split[1]);
            }
        } else if (iterator.key().contains("restart", Qt::CaseInsensitive)) {
            if (iterator.value().toBool() == true) {
                QApplication::quit();
                QProcess::startDetached(QApplication::arguments()[0], QApplication::arguments());
            }
        }

        iterator++;
    }
}
