#ifndef OPTIONSBASECLASS_H
#define OPTIONSBASECLASS_H

#include <QWidget>
#include <QString>
#include <QDialog>
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QPushButton>
#include <QDir>
#include <QMessageBox>
#include <QLineEdit>
#include <QCheckBox>
#include <QLabel>
#include <QSpinBox>
#include <QDoubleValidator>
#include "config.h"


class OptionsBaseClass : public QWidget
{
    Q_OBJECT
public:
    explicit OptionsBaseClass();
    void save();

    QSharedPointer<Config> config;
    QDialog *dialog = new QDialog;
    QLineEdit *leOpt1 = new QLineEdit;
    QLineEdit *leOpt2 = new QLineEdit;
    QLineEdit *leOpt3 = new QLineEdit;
    QLineEdit *leOpt4 = new QLineEdit;
    QLineEdit *leOpt5 = new QLineEdit;
    QLineEdit *leOpt6 = new QLineEdit;
    QLineEdit *leOpt7 = new QLineEdit;
    QCheckBox *cbOpt1 = new QCheckBox;
    QCheckBox *cbOpt2 = new QCheckBox;
    QCheckBox *cbOpt3 = new QCheckBox;
    QCheckBox *cbOpt4 = new QCheckBox;
    QCheckBox *cbOpt5 = new QCheckBox;
    QSpinBox *sbOpt1 = new QSpinBox;
    QSpinBox *sbOpt2 = new QSpinBox;
    QSpinBox *sbOpt3 = new QSpinBox;
    QSpinBox *sbOpt4 = new QSpinBox;
    QSpinBox *sbOpt5 = new QSpinBox;


    QDialogButtonBox *btnBox;
    QFormLayout *mainLayout = new QFormLayout;

public slots:
    virtual void start() = 0;
    virtual void saveCurrentSettings() = 0;

signals:
    void updSettings();

private slots:

private:

};

#endif // OPTIONSBASECLASS_H
