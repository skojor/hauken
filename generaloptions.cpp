#include "generaloptions.h"

GeneralOptions::GeneralOptions(QSharedPointer<Config> c)
    : OptionsBaseClass{}
{
    config = c;
    setWindowTitle(tr("General options"));

    mainLayout->addRow(new QLabel("Own station data"));

    mainLayout->addRow(new QLabel("Station name"), leOpt1);
    leOpt1->setToolTip("Station name is used in mail warnings");

    mainLayout->addRow(new QLabel("Latitude"), leOpt2);
    leOpt2->setToolTip("Own station location, used in posisiton offset and SDeF files. Format DD.ddddd");
    QDoubleValidator doubleVal1;
    doubleVal1.setNotation(QDoubleValidator::StandardNotation);
    doubleVal1.setLocale(QLocale::English);
    doubleVal1.setRange(-89, 89, 8);
    leOpt2->setValidator(&doubleVal1);

    mainLayout->addRow(new QLabel("Longitude"), leOpt3);
    leOpt3->setToolTip("Own station location, used in posisiton offset and SDeF files. Format DDD.ddddd");
    QDoubleValidator doubleVal2(-179, 179, 8);
    doubleVal2.setNotation(QDoubleValidator::StandardNotation);
    doubleVal2.setLocale(QLocale::English);
    leOpt3->setValidator(&doubleVal2);

    mainLayout->addRow(new QLabel("Altitude (asl)"), leOpt4);
    leOpt4->setToolTip("Own station height above mean sea level, used in altitude offset calculation");
    QDoubleValidator doubleVal3;
    doubleVal3.setNotation(QDoubleValidator::StandardNotation);
    doubleVal3.setLocale(QLocale::English);
    doubleVal3.setRange(0, 8848, 1);
    leOpt4->setValidator(&doubleVal3);

    mainLayout->addRow(new QLabel("Storage folders"));

    mainLayout->addRow(new QLabel("Configuration folder"), leOpt5);
    leOpt5->setToolTip("Standard folder for configuration files");

    mainLayout->addRow(new QLabel("Log folder"), leOpt6);
    leOpt6->setToolTip("Folder where SDeF and GNSS logs are stored");

    mainLayout->addRow(cbOpt1);
    cbOpt1->setText("Create a new folder for every incident");
    cbOpt1->setToolTip("Enabling this will create a folder named after the current date and time for the relevant logfiles");

    mainLayout->addRow(cbOpt2);
    cbOpt2->setText("PMR mode");
    cbOpt2->setToolTip("Enabling this will put Hauken in PMR mode. This will change a number of options to optimize for PMR detection");
    cbOpt2->setDisabled(true);

    connect(btnBox, &QDialogButtonBox::accepted, this, &GeneralOptions::saveCurrentSettings);
    connect(btnBox, &QDialogButtonBox::rejected, dialog, &QDialog::close);

    mainLayout->addWidget(btnBox);
}

void GeneralOptions::start()
{
    leOpt1->setText(config->getStationName());
    leOpt2->setText(config->getStnLatitude());
    leOpt3->setText(config->getStnLongitude());
    leOpt4->setText(config->getStnAltitude());
    leOpt5->setText(config->getWorkFolder());
    leOpt6->setText(config->getLogFolder());
    cbOpt1->setChecked(config->getNewLogFolder());
    cbOpt2->setChecked(config->getPmrMode());
    dialog->exec();
}

void GeneralOptions::saveCurrentSettings()
{
    config->setStationName(leOpt1->text());
    config->setStnLatitude(leOpt2->text());
    config->setStnLongitude(leOpt3->text());
    config->setStnAltitude(leOpt4->text());
    config->setWorkFolder(leOpt5->text());
    config->setLogFolder(leOpt6->text());

    if (!QDir(leOpt5->text()).exists())
        if (!QDir().mkpath(leOpt5->text()))
            QMessageBox::warning(this, "File error", "Couldn't create folder " + leOpt5->text());
    if (!QDir(leOpt6->text()).exists())
        if (!QDir().mkpath(leOpt6->text()))
            QMessageBox::warning(this, "File error", "Couldn't create folder " + leOpt6->text());

    config->setNewLogFolder(cbOpt1->isChecked());
    config->setPmrMode(cbOpt2->isChecked());


    dialog->close();
}
