#include "generaloptions.h"

GeneralOptions::GeneralOptions(QSharedPointer<Config> c)
    : OptionsBaseClass{}
{
    config = c;
    setWindowTitle(tr("General options"));

    QGroupBox *groupBox1 = new QGroupBox("Station data");
    QFormLayout *layout1 = new QFormLayout;
    groupBox1->setLayout(layout1);

    layout1->addRow(new QLabel("Station name"), leOpt1);
    leOpt1->setToolTip("Station name is used in email notifications");

    layout1->addRow(new QLabel("Station initials"), leOpt7);
    leOpt7->setToolTip("The initials used in the 1809 file header, your own initals or a designated initial for this station. Also used in http reports.");

    layout1->addRow(new QLabel("Latitude"), leOpt2);
    leOpt2->setToolTip("Own station location, used in posisiton offset and SDeF files. Format DD.ddddd");
    QDoubleValidator doubleVal1;
    doubleVal1.setNotation(QDoubleValidator::StandardNotation);
    doubleVal1.setLocale(QLocale::English);
    doubleVal1.setRange(-89, 89, 8);
    leOpt2->setValidator(&doubleVal1);

    layout1->addRow(new QLabel("Longitude"), leOpt3);
    leOpt3->setToolTip("Own station location, used in posisiton offset and SDeF files. Format DDD.ddddd");
    QDoubleValidator doubleVal2(-179, 179, 8);
    doubleVal2.setNotation(QDoubleValidator::StandardNotation);
    doubleVal2.setLocale(QLocale::English);
    leOpt3->setValidator(&doubleVal2);

    layout1->addRow(new QLabel("Altitude (asl)"), leOpt4);
    leOpt4->setToolTip("Own station height above mean sea level, used in altitude offset calculation");
    QDoubleValidator doubleVal3;
    doubleVal3.setNotation(QDoubleValidator::StandardNotation);
    doubleVal3.setLocale(QLocale::English);
    doubleVal3.setRange(0, 8848, 1);
    leOpt4->setValidator(&doubleVal3);

    QGroupBox *groupBox2 = new QGroupBox("Other options");
    QFormLayout *layout2 = new QFormLayout;
    groupBox2->setLayout(layout2);

    layout2->addRow(new QLabel("Configuration folder"), leOpt5);
    leOpt5->setToolTip("Standard folder for configuration files");

    layout2->addRow(cbOpt2);
    cbOpt2->setText("PMR mode");
    cbOpt2->setToolTip("Enabling this will put Hauken in PMR mode. This will change a number of options to optimize for PMR detection");
    cbOpt2->setDisabled(true);

    layout2->addRow(cbOpt3);
    cbOpt3->setText("Use dBm instead of dBμV");
    cbOpt3->setToolTip("Change scale to dBm");
    //cbOpt3->setDisabled(true);

    layout2->addRow(cbOpt4);
    cbOpt4->setText("Dark mode");
    cbOpt4->setToolTip("Force program to choose dark mode colors");

    layout2->addRow(cbOpt5);
    cbOpt5->setText("Show RF spectrum and incident log in separate windows");
    cbOpt5->setToolTip("Jammertest mode, restart after changing!");

    layout2->addRow(new QLabel("Signal level correction value (dB)"), leOpt9);
    leOpt9->setToolTip("Value will be added to the trace values from the receiver");

    layout2->addRow(new QLabel(tr("Overlay text font size")), sbOpt1);
    sbOpt1->setToolTip(tr("Adjust overlay band text font size here"));
    sbOpt1->setRange(6, 36);

    QGroupBox *groupBox3 = new QGroupBox("Geographic blocking");
    QFormLayout *layout3 = new QFormLayout;
    groupBox3->setLayout(layout3);

    layout3->addRow(cbOpt6);
    cbOpt6->setText("Enable geographic blocking");
    cbOpt6->setToolTip("Enabling this option will pause the monitoring and stop all recording if the " \
                       "station is moving outside a given polygon. Monitoring will resume when the" \
                       "station goes back inside the polygon");

    filename = config->getGeoLimitFilename();

    layout3->addRow(selectFileBtn);
    selectFileBtn->setText("Select KML file...( " + filename + " )");
    selectFileBtn->setToolTip("A KML file containing a polygon of the area where usage is allowed");

    connect(selectFileBtn, &QPushButton::clicked, this, [this] {
        filename = QFileDialog::getOpenFileName(this, "Select file", QDir(QCoreApplication::applicationDirPath()).absolutePath(), "KML (*.kml)");
        file->setText(filename);
    });

    QGroupBox *groupBox4 = new QGroupBox("Camera");
    QFormLayout *layout4 = new QFormLayout;
    groupBox4->setLayout(layout4);

    layout4->addRow(cbOpt1);
    cbOpt1->setText("Activate camera when recording is triggered");
    cbOpt1->setToolTip("A triggered recording will also start camera recording");

    layout4->addRow(new QLabel("Camera source"), leOpt10);
    //fillCameraCombo();
    leOpt10->setToolTip("Name of USB (first or second) or IP camera device ip to use, or none to disable");

    layout4->addRow(new QLabel("Record time (seconds)"), sbOpt2);
    sbOpt2->setToolTip("Time in seconds to record a video after the incident ended");
    sbOpt2->setRange(1, 86400);

    connect(btnBox, &QDialogButtonBox::accepted, this, &GeneralOptions::saveCurrentSettings);
    connect(btnBox, &QDialogButtonBox::rejected, dialog, &QDialog::close);

    mainLayout->addWidget(groupBox1);
    mainLayout->addWidget(groupBox3);
    mainLayout->addWidget(groupBox4);
    mainLayout->addWidget(groupBox2);
    mainLayout->addWidget(btnBox);
}

void GeneralOptions::start()
{
    leOpt1->setText(config->getStationName());
    leOpt2->setText(config->getStnLatitude());
    leOpt3->setText(config->getStnLongitude());
    leOpt4->setText(config->getStnAltitude());
    leOpt5->setText(config->getWorkFolder());
    cbOpt2->setChecked(config->getPmrMode());
    leOpt7->setText(config->getSdefStationInitals());
    cbOpt3->setChecked(config->getUseDbm());
    cbOpt4->setChecked(config->getDarkMode());
    leOpt9->setText(QString::number(config->getCorrValue(), 'f', 1));
    cbOpt5->setChecked(config->getSeparatedWindows());
    sbOpt1->setValue(config->getOverlayFontSize());

    cbOpt6->setChecked(config->getGeoLimitActive());
    file->setText(config->getGeoLimitFilename());

    leOpt10->setText(config->getCameraStreamAddress());
    cbOpt1->setChecked(config->getCameraDeviceTrigger());
    sbOpt2->setValue(config->getCameraRecordTime());

    dialog->exec();
}

void GeneralOptions::saveCurrentSettings()
{
    config->setStationName(leOpt1->text());
    config->setStnLatitude(leOpt2->text());
    config->setStnLongitude(leOpt3->text());
    config->setStnAltitude(leOpt4->text());
    config->setWorkFolder(leOpt5->text());

    if (!QDir(leOpt5->text()).exists())
        if (!QDir().mkpath(leOpt5->text()))
            QMessageBox::warning(this, "File error", "Couldn't create folder " + leOpt5->text());
    if (!QDir(leOpt6->text()).exists())
        if (!QDir().mkpath(leOpt6->text()))
            QMessageBox::warning(this, "File error", "Couldn't create folder " + leOpt6->text());

    config->setPmrMode(cbOpt2->isChecked());
    config->setSdefStationInitials(leOpt7->text());
    config->setUseDbm(cbOpt3->isChecked());
    config->setDarkMode(cbOpt4->isChecked());
    config->setCorrValue(leOpt9->text().toDouble());
    config->setSeparatedWindows(cbOpt5->isChecked());
    config->setOverlayFontSize(sbOpt1->value());

    config->setGeoLimitActive(cbOpt6->isChecked());
    config->setGeoLimitFilename(file->text());

    config->setCameraStreamAddress(leOpt10->text());
    config->setCameraDeviceTrigger(cbOpt1->isChecked());
    config->setCameraRecordTime(sbOpt2->value());

    dialog->close();
}
