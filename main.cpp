#include "mainwindow.h"
#include <QApplication>
#include <QDebug>
#include <QString>
#include <QStringList>

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

    a.setApplicationVersion(ver);

    MainWindow w;
    w.show();
    return a.exec();
}
