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
#include <QWidget>
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QLineEdit>
#include <QCheckBox>
#include <QLabel>
#include <QDialogButtonBox>
#include <QPushButton>
#include <QSlider>
#include <QList>
#include <QVector>
#include "config.h"
#include "typedefs.h"
#include <JlCompress.h>
#include <opencv2/highgui.hpp>
#include <opencv2/imgproc.hpp>
#include <opencv2/imgcodecs.hpp>

class Read1809Data : public QObject
{
    Q_OBJECT
public:
    Read1809Data(QSharedPointer<Config>);
    void readFolder(QString folder);
    void readFile(QString);
    void openFile();
    bool isRunning() { if (file.isOpen()) return true; else return false; }

public slots:
    void close() { wdg->close();}

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
    void freqChanged(double, double);
    void resChanged(double);


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
    QString note;
    bool isHeaderRead = false;
    QTime traceTime;
    QString tempfile;
    QBuffer *buffer = new QBuffer;
    QByteArray bufferArray;
    int playbackStartPosition = 0;

    QWidget *wdg = new QWidget;
    QPushButton *btnStartPause = new QPushButton;
    QPushButton *btnStop = new QPushButton;
    QPushButton *btnRewind = new QPushButton;
    QPushButton *btnClose = new QPushButton;
    QGridLayout *layout = new QGridLayout;
    QSlider *slider = new QSlider(Qt::Horizontal);

    int filenumber;
    QList<qint16> oldTrace;
    std::vector<qint16> oldFolderTrace; // TODO; Why use two totally different type of containers??
    QTime oldTraceTime;
    QSharedPointer<Config> config;

private slots:
    void readDataline();
    void closeFile();
    void play();
    void pause();
    void stop();
    void rewind();
    void updPlaybackPosition();
    void seek(int pos);
    void readAndConvert(QString folder, QString filename);
    void findMinAvgMax(const std::vector<std::vector<qint16> > &buffer, qint16 *min, qint16 *avg, qint16 *max);
};

#endif // READ1809DATA_H
