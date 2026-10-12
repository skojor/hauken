#include <QApplication>
#include <QDateTime>
#include <QDir>
#include <QDebug>
#include <QFile>
#include <QMutex>
#include <QMutexLocker>
#include <QString>
#include <QStringList>
#include <QTextStream>
#include <QTimer>
#include <QStyleHints>
#include "mainwindow.h"
#include "version.h"

QString debugLogFolder;
QMutex logMutex;
bool logToFile = false;

QString dailyLogFilePath()
{
    const QString fileName = "haukenDebug_" + QDate::currentDate().toString("yyyyMMdd") + ".log";
    return QDir(debugLogFolder).filePath(fileName);
}

bool ensureDailyLogFile()
{
    QMutexLocker locker(&logMutex);
    QFile logFile(dailyLogFilePath());
    return logFile.open(QIODevice::WriteOnly | QIODevice::Append);
}

int millisecondsUntilNextMidnight()
{
    const QDateTime now = QDateTime::currentDateTime();
    const QDateTime midnight(now.date().addDays(1), QTime(0, 0));
    return qMax(1, static_cast<int>(now.msecsTo(midnight)));
}

void customMessageOutput(QtMsgType type, const QMessageLogContext &context, const QString &msg)
{
   (void)context;

    QHash<QtMsgType, QString> msgLevelHash({{QtDebugMsg, "Debug"}, {QtInfoMsg, "Info"}, {QtWarningMsg, "Warning"}, {QtCriticalMsg, "Critical"}, {QtFatalMsg, "Fatal"}});
    QByteArray localMsg = msg.toLocal8Bit();
    //QTime time = QTime::currentTime();
    QString formattedTime = QDateTime::currentDateTime().toString("dd.MM hh:mm:ss:zzz"); //= time.toString("hh:mm:ss.zzz");
    QByteArray formattedTimeMsg = formattedTime.toLocal8Bit();
    QString logLevelName = msgLevelHash[type];
    QByteArray logLevelMsg = logLevelName.toLocal8Bit();

    if (logToFile) {
        QString txt = QString("%1 %2: %3").arg(formattedTime, logLevelName, msg);
        QMutexLocker locker(&logMutex);
        QFile outFile(dailyLogFilePath());
        if (!outFile.open(QIODevice::WriteOnly | QIODevice::Append)) {
            fprintf(stderr, "%s\n", txt.toLocal8Bit().constData());
            fflush(stderr);
            return;
        }
        QTextStream ts(&outFile);
        ts << txt << "\n";
    } else {
        fprintf(stderr, "%s %s: %s\n", formattedTimeMsg.constData(), logLevelMsg.constData(), localMsg.constData());
        fflush(stderr);
    }

    /*if (type == QtFatalMsg)
        abort();*/
}

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    a.setApplicationName("Hauken");
    a.setOrganizationName("JSK");
    a.setOrganizationDomain("nkom.no");

    QString ver = QString(PROJECT_VERSION);

    qRegisterMetaType<QVector<qint16> >("QVector<qint16>");
    qRegisterMetaType<QList<QVector<qint16>> >("QList<QVector<qint16>>");
    qRegisterMetaType<QList<QList<float>> >("QList<QList<float>>");
    qRegisterMetaType<QList<QDateTime> >("QList<QDateTime>");
    qRegisterMetaType<NOTIFY::TYPE>("NOTIFY::TYPE");
    qRegisterMetaType<QVector<double> >("QVector<double>");
    qRegisterMetaType<QList<qint16> >("QList<qint16>");
    qRegisterMetaType<IqMetadata>("IqMetaData");

    if (qEnvironmentVariableIsEmpty("QTDIR"))   //  check if the app is ran in Qt Creator
         logToFile = true;

    QSharedPointer<Config> config(new Config, &QObject::deleteLater);
    if (logToFile) {
        debugLogFolder = QDir(config->getWorkFolder()).filePath("debug");
        if (!QDir().mkpath(debugLogFolder) || !ensureDailyLogFile()) {
            fprintf(stderr, "Unable to create debug log in %s\n", debugLogFolder.toLocal8Bit().constData());
            logToFile = false;
        }
    }

    qInstallMessageHandler(customMessageOutput); // custom message handler for debugging
    a.setApplicationVersion(ver);
    //a.setStyle(QLatin1String("windowsvista"));
    a.setStyle("fusion");
    QTimer debugRolloverTimer;
    MainWindow w(nullptr, config);
    //w.setAttribute(Qt::WA_DeleteOnClose);

    w.show();
    if (logToFile) {
        debugRolloverTimer.setSingleShot(true);
        QObject::connect(&debugRolloverTimer, &QTimer::timeout, [&debugRolloverTimer]
        {
            if (!ensureDailyLogFile()) {
                fprintf(stderr, "Unable to create daily debug log file\n");
            }
            debugRolloverTimer.start(millisecondsUntilNextMidnight());
        });
        debugRolloverTimer.start(millisecondsUntilNextMidnight());
    }
    return a.exec();
}
