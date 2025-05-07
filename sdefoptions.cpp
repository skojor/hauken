#include "sdefoptions.h"

SdefOptions::SdefOptions(QSharedPointer<Config> c)
    : OptionsBaseClass {}
{
    config = c;
    setWindowTitle(tr("1809 options"));
    mainLayout->addRow(new QLabel(tr("1809 options")));
    mainLayout->addRow(cbOpt1);
    cbOpt1->setText("Save trace data to 1809 (SDeF 2.0) format on incidents");
    cbOpt1->setToolTip("If checked all trace data will be saved to a log file for a given period of time");

    mainLayout->addRow(cbOpt2);
    cbOpt2->setText("Upload 1809 data when finished");
    cbOpt2->setToolTip("If checked the file will be uploaded to the given server. Remember to set username/password below");

    mainLayout->addRow(cbOpt3);
    cbOpt3->setText("Add position data from GNSS (mobile setting)");
    cbOpt3->setToolTip("If checked the current position will be appended to the trace data. For mobile usage.");

    mainLayout->addRow(cbOpt4);
    cbOpt4->setText("Compress files in zip format");
    cbOpt4->setToolTip("If checked the measurement file will be zipped after recording. In general this is a good thing");

    mainLayout->addRow(new QLabel("Position source"), comboOpt1);
    comboOpt1->setToolTip("Select which device should be used for SDEF file position updates.\nGNSS device 1 and 2 are the ones selected in the GNSS options dialog");
    comboOpt1->addItems(QStringList() << "InstrumentGnss" << "GNSS device 1" << "GNSS device 2");

    mainLayout->addRow(new QLabel("Server http(s) address"), leOpt3);
    leOpt3->setToolTip("Server address to send 1809 measurement data");
    leOpt3->setText(config->getSdefServer());

    mainLayout->addRow(new QLabel("Server authentication address"), leOpt4);
    leOpt4->setToolTip("Authentication server address (if needed)");
    leOpt4->setText(config->getSdefAuthAddress());

    mainLayout->addRow(new QLabel("Username (Støygolvet login)"), leOpt1);
    leOpt1->setToolTip("Username required to enable Casper uploads");
    leOpt1->setText(config->getSdefUsername());

    mainLayout->addRow(new QLabel("Password (Støygolvet login)"), leOpt2);
    leOpt2->setToolTip("Password required to enable Casper uploads");
    leOpt2->setEchoMode(QLineEdit::Password);

    mainLayout->addRow(new QLabel("Recording time after incident ended (min)"), sbOpt1);
    sbOpt1->setToolTip("Duration of recording in minutes after last incident triggered");
    sbOpt1->setRange(1, 86400);

    mainLayout->addRow(new QLabel("Split files when recording lasts longer than (min)"), sbOpt2);
    sbOpt2->setToolTip("Limit the file size by setting this value. Uploads the file and starts a new recording immediately.");
    sbOpt2->setRange(1, 86400);

    mainLayout->addRow(new QLabel("Record time before incident (seconds)"), sbOpt3);
    sbOpt3->setToolTip("Record traces this many seconds before incident was triggered. Maximum 120 seconds.");
    sbOpt3->setRange(0, 120);

    mainLayout->addRow(cbOpt5);
    cbOpt5->setText("Use new ms format in 1809 file");
    cbOpt5->setToolTip("Enable this option to add date and millisecond timestamp in 1809 file");


    connect(btnBox, &QDialogButtonBox::accepted, this, &SdefOptions::saveCurrentSettings);
    connect(btnBox, &QDialogButtonBox::rejected, dialog, &QDialog::close);
    connect(cbOpt3, &QCheckBox::toggled, this, [this] (bool b) {
        comboOpt1->setEnabled(b);
    });

    mainLayout->addWidget(btnBox);
}

void SdefOptions::start()
{
    cbOpt1->setChecked(config->getSdefSaveToFile());
    cbOpt2->setChecked(config->getSdefUploadFile());
    leOpt2->setText(config->getSdefPassword());
    leOpt3->setText(config->getSdefServer());
    leOpt4->setText(config->getSdefAuthAddress());
    cbOpt3->setChecked(config->getSdefAddPosition());
    cbOpt4->setChecked(config->getSdefZipFiles());
    comboOpt1->setEnabled(cbOpt3->isChecked());
    sbOpt1->setValue(config->getSdefRecordTime());
    sbOpt2->setValue(config->getSdefMaxRecordTime());
    comboOpt1->setCurrentText(config->getSdefGpsSource());
    sbOpt3->setValue(config->getSdefPreRecordTime());
    cbOpt5->setChecked(config->getSdefNewMsFormat());


    dialog->exec();
}

void SdefOptions::saveCurrentSettings()
{
    config->setSdefSaveToFile(cbOpt1->isChecked());
    config->setSdefUploadFile(cbOpt2->isChecked());
    config->setSdefGpsSource(comboOpt1->currentText());
    config->setSdefUsername(leOpt1->text().trimmed());
    config->setSdefPassword(leOpt2->text());
    config->setSdefServer(leOpt3->text().trimmed());
    config->setSdefAuthAddress(leOpt4->text().trimmed());
    config->setSdefAddPosition(cbOpt3->isChecked());
    config->setSdefZipFiles(cbOpt4->isChecked());
    config->setSdefRecordTime(sbOpt1->value());
    config->setSdefMaxRecordTime(sbOpt2->value());
    config->setSdefPreRecordTime(sbOpt3->value());
    config->setSdefNewMsFormat(cbOpt5->isChecked());

    dialog->close();
}
