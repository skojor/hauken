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
#include <QComboBox>
#include <QFormLayout>
#include <QGroupBox>
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
    QLineEdit *leOpt8 = new QLineEdit;
    QLineEdit *leOpt9 = new QLineEdit;
    QLineEdit *leOpt10 = new QLineEdit;
    QLineEdit *leOpt11 = new QLineEdit;
    QLineEdit *leOpt12 = new QLineEdit;
    QLineEdit *leOpt13 = new QLineEdit;
    QLineEdit *leOpt14 = new QLineEdit;
    QLineEdit *leOpt15 = new QLineEdit;
    QCheckBox *cbOpt1 = new QCheckBox;
    QCheckBox *cbOpt2 = new QCheckBox;
    QCheckBox *cbOpt3 = new QCheckBox;
    QCheckBox *cbOpt4 = new QCheckBox;
    QCheckBox *cbOpt5 = new QCheckBox;
    QCheckBox *cbOpt6 = new QCheckBox;
    QCheckBox *cbOpt7 = new QCheckBox;
    QCheckBox *cbOpt8 = new QCheckBox;
    QCheckBox *cbOpt9 = new QCheckBox;
    QCheckBox *cbOpt10 = new QCheckBox;
    QCheckBox *cbOpt11 = new QCheckBox;
    QCheckBox *cbOpt12 = new QCheckBox;
    QCheckBox *cbOpt13 = new QCheckBox;
    QCheckBox *cbOpt14 = new QCheckBox;
    QCheckBox *cbOpt15 = new QCheckBox;
    QCheckBox *cbOpt16 = new QCheckBox;
    QSpinBox *sbOpt1 = new QSpinBox;
    QSpinBox *sbOpt2 = new QSpinBox;
    QSpinBox *sbOpt3 = new QSpinBox;
    QSpinBox *sbOpt4 = new QSpinBox;
    QSpinBox *sbOpt5 = new QSpinBox;
    QComboBox *comboOpt1 = new QComboBox;
    QComboBox *comboOpt2 = new QComboBox;
    QComboBox *comboOpt3 = new QComboBox;
    QComboBox *comboOpt4 = new QComboBox;
    QComboBox *comboOpt5 = new QComboBox;
    QComboBox *comboOpt6 = new QComboBox;



    QDialogButtonBox *btnBox;
    QFormLayout *mainLayout = new QFormLayout(this);

public slots:
    virtual void start() = 0;
    virtual void saveCurrentSettings() = 0;

signals:
    void updSettings();

private slots:

private:

};

#endif // OPTIONSBASECLASS_H
