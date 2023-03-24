#ifndef GEOLIMIT_H
#define GEOLIMIT_H

#include <QWidget>
#include <QFile>
#include <QDir>
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QLineEdit>
#include <QCheckBox>
#include <QLabel>
#include <QDialogButtonBox>
#include <QPushButton>
#include <QXmlStreamReader>
#include "config.h"
#include "typedefs.h"

class GeoLimit : public Config
{
    Q_OBJECT
public:
    explicit GeoLimit(QObject *parent = nullptr);
    void updSettings();

private:
    QString filename;
    bool activated = false;

private slots:
    void activate();
    void readKmlFile();

signals:
    void warning(QString);
    void toIncidentLog(const NOTIFY::TYPE, const QString, const QString);
};

#endif // GEOLIMIT_H
