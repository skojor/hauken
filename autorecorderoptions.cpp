#include "autorecorderoptions.h"

AutoRecorderOptions::AutoRecorderOptions(QSharedPointer<Config> c)
{
    config = c;
    setWindowTitle("Auto recorder options");

    mainLayout->addRow(cbOpt1);
    cbOpt1->setText("Enable auto recorder on startup");
    cbOpt1->setToolTip("Enabling this options will set up Hauken to start recording at program startup. Enabling this option also enables connect instrument on startup, and auto reconnect.");

    connect(btnBox, &QDialogButtonBox::accepted, this, &AutoRecorderOptions::saveCurrentSettings);
    connect(btnBox, &QDialogButtonBox::rejected, dialog, &QDialog::close);
    mainLayout->addWidget(btnBox);
}

void AutoRecorderOptions::start()
{
    cbOpt1->setChecked(config->getAutoRecorderActivate());
    dialog->exec();
}

void AutoRecorderOptions::saveCurrentSettings()
{
    config->setAutoRecorderActivate(cbOpt1->isChecked());
    if (cbOpt1->isChecked()) {
        config->setInstrAutoReconnect(true);
        config->setInstrConnectOnStartup(true);
    }
    dialog->close();
}
