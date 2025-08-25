#include "autorecorderoptions.h"

AutoRecorderOptions::AutoRecorderOptions(QSharedPointer<Config> c)
{
    config = c;
    setWindowTitle("Auto recorder options");

    mainLayout->addRow(cbOpt1);
    cbOpt1->setText("Enable auto recorder on startup");
    cbOpt1->setToolTip("Enabling this options will set up Hauken to start recording at program startup. Enabling this option also enables connect instrument on startup, and auto reconnect.");

    mainLayout->addRow(cbOpt2);
    cbOpt2->setText("Save CEF data to temp file and forward on TCP port 5569");
    cbOpt2->setToolTip(
        tr("CEF data will be forwarded continuously on localhost, port 5569.\n"
           "The log file \"feed.cef\" will be updated with headers."));

    mainLayout->addRow(new QLabel(tr("Maxhold record time (seconds)")), sbOpt1);
    sbOpt1->setToolTip("How often to save to temporary file. Set to 0 for AFAP mode.");
    sbOpt1->setRange(0, 3600);

    connect(btnBox, &QDialogButtonBox::accepted, this, &AutoRecorderOptions::saveCurrentSettings);
    connect(btnBox, &QDialogButtonBox::rejected, dialog, &QDialog::close);
    mainLayout->addWidget(btnBox);
}

void AutoRecorderOptions::start()
{
    cbOpt1->setChecked(config->getAutoRecorderActivate());
    cbOpt2->setChecked(config->getSaveToTempFile());
    sbOpt1->setValue(config->getSaveToTempFileMaxhold());
    dialog->exec();
}

void AutoRecorderOptions::saveCurrentSettings()
{
    config->setAutoRecorderActivate(cbOpt1->isChecked());
    if (cbOpt1->isChecked()) {
        config->setInstrAutoReconnect(true);
        config->setInstrConnectOnStartup(true);
    }
    config->setSaveToTempFile(cbOpt2->isChecked());
    config->setSaveToTempFileMaxhold(sbOpt1->value());
    dialog->close();
}
