#ifndef GEOLIMITOPTIONS_H
#define GEOLIMITOPTIONS_H

#include "optionsbaseclass.h"
#include <QPushButton>
#include <QFileDialog>

class GeoLimitOptions : public OptionsBaseClass
{
    Q_OBJECT
public:
    GeoLimitOptions(QSharedPointer<Config> c);

public slots:
    void start();
    void saveCurrentSettings();

private:
    QPushButton *selectFileBtn = new QPushButton;
    QString filename;
    QLabel *file = new QLabel();
};

#endif // GEOLIMITOPTIONS_H
