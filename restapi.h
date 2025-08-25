#ifndef RESTAPI_H
#define RESTAPI_H

#include <QApplication>
#include <QHttpServer>
#include <QJsonArray>
#include <QJsonObject>
#include <QObject>
#include <QProcess>
#include <QSharedPointer>
#include <QTcpServer>
#include "config.h"

#define LISTEN_PORT 5570

class RestApi : public QObject
{
    Q_OBJECT

public:
    RestApi(QSharedPointer<Config> c);
    ~RestApi();

private slots:
    bool authorize(const QHttpServerRequest &request);
    QJsonObject byteArrayToJsonObject(const QByteArray &arr);
    void parseJson(QJsonObject object);

private:
    QHttpServer *httpServer = new QHttpServer;
    QTcpServer *tcpServer = new QTcpServer;
    QJsonObject myData;
    QSharedPointer<Config> config;

signals:
    void pscanStartFreq(double);
    void pscanStopFreq(double);
    void pscanResolution(QString);
    void measurementTime(int);
    void manualAtt(int);
    void autoAtt(bool);
    void antport(int);
    void mode(QString);
    void fftmode(QString);
    void gaincontrol(QString);
};

#endif // RESTAPI_H
