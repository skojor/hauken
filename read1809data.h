#ifndef READ1809DATA_H
#define READ1809DATA_H

#include <QObject>
#include <QDir>
#include <QFile>
#include <QDateTime>
#include <QStringList>
#include <QString>
#include <QTimer>
#include <QDebug>
#include <QBuffer>
#include "config.h"
#include "typedefs.h"
#include "JlCompress.h"

class Read1809Data : public Config
{
    Q_OBJECT
public:
    explicit Read1809Data(QObject *parent = nullptr);
    void readFolder(QString folder);
    void readFile(QString);
    void openFile();
    bool isRunning() { if (file.isOpen()) return true; else return false; }

signals:
    void playbackRunning(bool);
    void popup(QString);
    void status(QString);
    void instrId(QString);
    void toIncidentLog(const NOTIFY::TYPE,const QString,const QString);
    void bytesPerSec(int);
    void tracesPerSec(double);
    void newTrace(const QVector<qint16> &);
    void resetBuffers();
    void positionUpdate(bool b, double lat, double lng);
    void displayGnssData(QString, int, bool);
    void updGnssData(GnssData &);
    void modeUsed(QString);
    void freqRangeUsed(unsigned long, unsigned long);
    void resUsed(int);


private:
    QTimer *timer = new QTimer;
    QFile file;
    bool isMobile;
    QString locationName;
    QString latitude = 0, longitude = 0;
    unsigned long freqStart = 0, freqStop = 0; // kHz
    float filterBW = 0; // kHz
    bool isDBuV = true;
    QDate date;
    unsigned long datapoints = 0;
    float scanTime = 0;
    float maxHoldTime = 0;
    bool isHeaderRead = false;
    QTime traceTime;
    QString tempfile;
    QBuffer *buffer = new QBuffer;
    QByteArray bufferArray;

private slots:
    void readDataline();
};

#endif // READ1809DATA_H
