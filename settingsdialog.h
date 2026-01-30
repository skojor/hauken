#ifndef SETTINGSDIALOG_H
#define SETTINGSDIALOG_H

#include <QWidget>
#include <QObject>
#include <QTabWidget>
#include <QDialogButtonBox>
#include <QDialog>
#include <QVBoxLayout>
#include <QListWidget>
#include <QStackedWidget>
#include "config.h"

class SettingsDialog : public QDialog
{
    Q_OBJECT
public:
    explicit SettingsDialog(QWidget *parent = nullptr, QSharedPointer<Config> c = nullptr);
    void save();

signals:

private:
    //QTabWidget *tabs = new QTabWidget;
    QSharedPointer<Config> config;
    QListWidget    *navList;
    QStackedWidget *pages;
};

#endif // SETTINGSDIALOG_H
