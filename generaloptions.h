#ifndef GENERALOPTIONS_H
#define GENERALOPTIONS_H

#include <QPushButton>
#include <QFileDialog>
#include <QCoreApplication>
#include <QDir>
#include "optionsbaseclass.h"

class GeneralOptions : public OptionsBaseClass
{
public:
    GeneralOptions(QSharedPointer<Config> c);

public slots:
    void start();
    void saveCurrentSettings();

private:
    QPushButton *selectFileBtn = new QPushButton;
    QString filename;
    QLabel *file = new QLabel();

};

#endif // GENERALOPTIONS_H
