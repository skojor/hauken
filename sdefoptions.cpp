#include "sdefoptions.h"

SdefOptions::SdefOptions(QSharedPointer<Config> c)
    : OptionsBaseClass {}
{
    config = c;
    setWindowTitle(tr("Measurement data storage and upload options"));

    QGroupBox *groupBoxFile = new QGroupBox("File creation and storage");
    QFormLayout *fileLayout = new QFormLayout;
    groupBoxFile->setLayout(fileLayout);

    fileLayout->addRow(cbOpt1);
    cbOpt1->setText("Save trace data to 1809 format on incidents");
    cbOpt1->setToolTip("If checked all trace data will be saved to a log file for a given period of time");

    fileLayout->addRow(cbOpt4);
    cbOpt4->setText("Compress files in zip format");
    cbOpt4->setToolTip("If checked the measurement file will be zipped after recording.");

    fileLayout->addRow(cbOpt10);
    cbOpt10->setText("Upload file to server when finished using OAuth");
    cbOpt10->setToolTip("Enable this option to activate OAuth2 authentication and file upload. "\
                        "Ask your IT provider for the configuration values below.\n"\
                        "All fields are mandatory.");

    fileLayout->addRow(cbOpt6);
    cbOpt6->setText("Create a new folder for every incident");
    cbOpt6->setToolTip("Enabling this will create a folder named after the current date and time for the relevant logfiles");

    fileLayout->addRow(cbOpt7);
    cbOpt7->setText("Activate recorder on startup (continuous recording)");
    cbOpt7->setToolTip("Enabling this options will set up Hauken to start recording at program startup. "\
                       "Enabling this option also enables connect instrument on startup, and auto reconnect.");

    fileLayout->addRow(cbOpt3, comboOpt1);
    cbOpt3->setText("Add position data from GNSS (source)");
    cbOpt3->setToolTip("If checked the current position will be appended to the trace data. For mobile usage.");

    fileLayout->addRow(new QLabel("Log folder"), leOpt6);
    leOpt6->setToolTip("Folder where SDeF and GNSS logs are stored");

    comboOpt1->setToolTip("Select which device should be used for SDEF file position updates.\nGNSS device 1 and 2 are the ones selected in the GNSS options dialog");
    comboOpt1->addItems(QStringList() << "InstrumentGnss" << "GNSS device 1" << "GNSS device 2");

    fileLayout->addRow(new QLabel("Recording time after incident ended (min)"), sbOpt1);
    sbOpt1->setToolTip("Duration of recording in minutes after last incident triggered");
    sbOpt1->setRange(1, 86400);

    fileLayout->addRow(new QLabel("Split files when recording lasts longer than (min)"), sbOpt2);
    sbOpt2->setToolTip("Limit the file size by setting this value. Uploads the file and starts a new recording immediately.");
    sbOpt2->setRange(1, 86400);

    fileLayout->addRow(new QLabel("Record time before incident (seconds)"), sbOpt3);
    sbOpt3->setToolTip("Record traces this many seconds before incident was triggered. Maximum 120 seconds.");
    sbOpt3->setRange(0, 120);

    QGroupBox *groupBoxOAuth = new QGroupBox("OAuth");
    QFormLayout *oauthLayout = new QFormLayout;
    groupBoxOAuth->setLayout(oauthLayout);

    oauthLayout->addRow(new QLabel("OAuth2 authority"), leOpt10);
    leOpt10->setToolTip("Set the URL to the login authority.");

    oauthLayout->addRow(new QLabel("OAuth2 application ID"), leOpt11);
    leOpt11->setToolTip("Application/client ID to use for authentication.");

    oauthLayout->addRow(new QLabel("OAuth2 scope"), leOpt12);
    leOpt12->setToolTip("Set the scope which this authentication should cover.");

    oauthLayout->addRow(new QLabel("File upload address"), leOpt13);
    leOpt13->setToolTip("The address to use for uploading measurement files.");

    QGroupBox *groupBoxTemp = new QGroupBox("Temp file/TCP forward");
    QFormLayout *tempLayout = new QFormLayout;
    groupBoxTemp->setLayout(tempLayout);

    tempLayout->addRow(cbOpt8);
    cbOpt8->setText("Save CEF data to temp file and forward on TCP port 5569");
    cbOpt8->setToolTip(
        tr("CEF data will be forwarded continuously on localhost, port 5569.\n"
           "The log file \"feed.cef\" will be updated with headers."));

    tempLayout->addRow(new QLabel(tr("Temp file maxhold record time (seconds)")), sbOpt4);
    sbOpt4->setToolTip("How often to save to temporary file. Set to 0 for AFAP mode.");
    sbOpt4->setRange(0, 3600);

    mainLayout->addWidget(groupBoxFile);
    mainLayout->addWidget(groupBoxOAuth);
    mainLayout->addWidget(groupBoxTemp);

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
    cbOpt3->setChecked(config->getSdefAddPosition());
    cbOpt4->setChecked(config->getSdefZipFiles());
    comboOpt1->setEnabled(cbOpt3->isChecked());
    sbOpt1->setValue(config->getSdefRecordTime());
    sbOpt2->setValue(config->getSdefMaxRecordTime());
    comboOpt1->setCurrentText(config->getSdefGpsSource());
    sbOpt3->setValue(config->getSdefPreRecordTime());
    leOpt6->setText(config->getLogFolder());
    cbOpt6->setChecked(config->getNewLogFolder());
    cbOpt10->setChecked(config->getOauth2Enable());
    leOpt10->setText(config->getOAuth2AuthUrl());
    leOpt11->setText(config->getOAuth2Scope());
    leOpt13->setText(config->getOauth2UploadAddress());
    leOpt12->setText(config->getOAuth2ClientId());
    cbOpt7->setChecked(config->getAutoRecorderActivate());
    cbOpt8->setChecked(config->getSaveToTempFile());
    sbOpt4->setValue(config->getSaveToTempFileMaxhold());

    dialog->setMinimumWidth(600);
    dialog->exec();
}

void SdefOptions::saveCurrentSettings()
{
    config->setSdefSaveToFile(cbOpt1->isChecked());
    config->setSdefUploadFile(cbOpt2->isChecked());
    config->setSdefGpsSource(comboOpt1->currentText());
    config->setSdefAddPosition(cbOpt3->isChecked());
    config->setSdefZipFiles(cbOpt4->isChecked());
    config->setSdefRecordTime(sbOpt1->value());
    config->setSdefMaxRecordTime(sbOpt2->value());
    config->setSdefPreRecordTime(sbOpt3->value());
    config->setLogFolder(leOpt6->text());
    config->setNewLogFolder(cbOpt6->isChecked());
    config->setOAuth2Enable(cbOpt10->isChecked());
    config->setOAuth2AuthUrl(leOpt10->text().trimmed());
    config->setOAuth2Scope(leOpt11->text().trimmed());
    config->setOAuth2UploadAddress(leOpt13->text().trimmed());
    config->setOAuth2ClientId(leOpt12->text().trimmed());
    config->setAutoRecorderActivate(cbOpt7->isChecked());
    if (cbOpt7->isChecked()) {
        config->setInstrAutoReconnect(true);
        config->setInstrConnectOnStartup(true);
    }
    config->setSaveToTempFile(cbOpt8->isChecked());
    config->setSaveToTempFileMaxhold(sbOpt4->value());

    dialog->close();
}
