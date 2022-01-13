#include "mainwindow.h"
#include <QApplication>
#include <QDebug>
#include <QString>
#include <QStringList>

const QString logFilePath = "debug.log";
bool logToFile = false;

void customMessageOutput(QtMsgType type, const QMessageLogContext &context, const QString &msg)
{
   (void)context;

    QHash<QtMsgType, QString> msgLevelHash({{QtDebugMsg, "Debug"}, {QtInfoMsg, "Info"}, {QtWarningMsg, "Warning"}, {QtCriticalMsg, "Critical"}, {QtFatalMsg, "Fatal"}});
    QByteArray localMsg = msg.toLocal8Bit();
    QTime time = QTime::currentTime();
    QString formattedTime = time.toString("hh:mm:ss.zzz");
    QByteArray formattedTimeMsg = formattedTime.toLocal8Bit();
    QString logLevelName = msgLevelHash[type];
    QByteArray logLevelMsg = logLevelName.toLocal8Bit();

    if (logToFile) {
        QString txt = QString("%1 %2: %3").arg(formattedTime, logLevelName, msg);
        QFile outFile(logFilePath);
        outFile.open(QIODevice::WriteOnly | QIODevice::Append);
        QTextStream ts(&outFile);
        ts << txt << Qt::endl;
        outFile.close();
    } else {
        fprintf(stderr, "%s %s: %s\n", formattedTimeMsg.constData(), logLevelMsg.constData(), localMsg.constData());
        fflush(stderr);
    }

    if (type == QtFatalMsg)
        abort();
}

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    a.setApplicationName("Hauken");
    a.setOrganizationName("JSK");
    a.setOrganizationDomain("nkom.no");
    QString ver;
    QStringList split = QString(SW_VERSION).split('-');
    if (split.size() > 1) ver = split[0] + '-' + split[1];

    qRegisterMetaType<QVector<qint16> >("QVector<qint16>");
    qRegisterMetaType<QList<QVector<qint16>> >("QList<QVector<qint16>>");
    qRegisterMetaType<QList<QDateTime> >("QList<QDateTime>");
    qRegisterMetaType<NOTIFY::TYPE>("NOTIFY::TYPE");

    if (qgetenv("QTDIR").isEmpty())   //  check if the app is ran in Qt Creator
         logToFile = true;
    qInstallMessageHandler(customMessageOutput); // custom message handler for debugging

    a.setApplicationVersion(ver);

    MainWindow w;
    w.show();
    return a.exec();
}
