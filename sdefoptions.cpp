#include "sdefoptions.h"

SdefOptions::SdefOptions(QSharedPointer<Config> c)
    : OptionsBaseClass {}
{
    config = c;
    setWindowTitle(tr("SDeF 2.0 (1809 format) options"));
    mainLayout->addRow(new QLabel(tr("SDeF 2.0 (1809 format) options")));
    mainLayout->addRow(cbOpt1);
    cbOpt1->setText("Save trace data to SDeF 2.0 format on incidents");
    cbOpt1->setToolTip("If checked all trace data will be saved to a log file for a given period of time");
    cbOpt1->setChecked(config->getSdefSaveToFile());

    mainLayout->addRow(cbOpt2);
    cbOpt2->setText("Upload SDeF data to Casper when finished");
    cbOpt2->setToolTip("If checked the file will be uploaded to Støygolvet server. Remember to set username/password below");
    cbOpt2->setChecked(config->getSdefUploadFile());

    mainLayout->addRow(cbOpt3);
    cbOpt3->setText("Add position data from GNSS (mobile setting)");
    cbOpt3->setToolTip("If checked the current position will be appended to the trace data. For mobile usage.");
    cbOpt3->setChecked(config->getSdefAddPosition());

    mainLayout->addRow(new QLabel("Position source"), comboOpt1);
    comboOpt1->setToolTip("Select which device should be used for SDEF file position updates.\nGNSS device 1 and 2 are the ones selected in the GNSS options dialog");
    comboOpt1->addItems(QStringList() << "InstrumentGnss" << "GNSS device 1" << "GNSS device 2");
    comboOpt1->setCurrentText(config->getSdefGpsSource());
    comboOpt1->setEnabled(cbOpt3->isChecked());

    mainLayout->addRow(new QLabel("Username (Støygolvet login)"), leOpt1);
    leOpt1->setToolTip("Username required to enable Casper uploads");
    leOpt1->setText(config->getSdefUsername());

    mainLayout->addRow(new QLabel("Password (Støygolvet login)"), leOpt2);
    leOpt2->setToolTip("Password required to enable Casper uploads");
    leOpt2->setEchoMode(QLineEdit::Password);
    leOpt2->setText(config->getSdefPassword());

    mainLayout->addRow(new QLabel("Recording time after incident ended (min)"), sbOpt1);
    sbOpt1->setToolTip("Duration of recording in minutes after last incident triggered");
    sbOpt1->setValue(config->getSdefRecordTime());
    sbOpt1->setRange(1, 86400);

    mainLayout->addRow(new QLabel("Split files when recording lasts longer than (min)"), sbOpt2);
    sbOpt2->setToolTip("Limit the file size by setting this value. Uploads the file and starts a new recording immediately.");
    sbOpt2->setValue(config->getSdefMaxRecordTime());
    sbOpt2->setRange(1, 86400);

    mainLayout->addRow(new QLabel("Record time before incident (seconds)"), sbOpt3);
    sbOpt3->setToolTip("Record traces this many seconds before incident was triggered. Maximum 120 seconds.");
    sbOpt3->setValue(config->getSdefPreRecordTime());
    sbOpt3->setRange(0, 120);

    mainLayout->addRow(new QLabel("Username for uploads (Station initials)"), leOpt3);
    leOpt3->setToolTip("The initials used in the 1809 file header, your own initals or a designated initial for this station");
    leOpt3->setText(config->getSdefStationInitals());

    connect(btnBox, &QDialogButtonBox::accepted, this, &SdefOptions::saveCurrentSettings);
    connect(btnBox, &QDialogButtonBox::rejected, dialog, &QDialog::close);
    connect(cbOpt3, &QCheckBox::toggled, this, [this] (bool b) {
        comboOpt1->setEnabled(b);
    });

    mainLayout->addWidget(btnBox);
}

void SdefOptions::start()
{
    dialog->exec();
}

void SdefOptions::saveCurrentSettings()
{
    config->setSdefSaveToFile(cbOpt1->isChecked());
    config->setSdefUploadFile(cbOpt2->isChecked());
    config->setSdefGpsSource(comboOpt1->currentText());
    config->setSdefUsername(leOpt1->text());
    config->setSdefPassword(leOpt2->text());
    config->setSdefAddPosition(cbOpt3->isChecked());
    config->setSdefRecordTime(sbOpt1->value());
    config->setSdefMaxRecordTime(sbOpt2->value());
    config->setSdefStationInitials(leOpt3->text());
    emit updSettings();
    dialog->close();
}
