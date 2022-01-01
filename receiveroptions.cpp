#include "receiveroptions.h"

ReceiverOptions::ReceiverOptions(QSharedPointer<Config> c)
    : OptionsBaseClass {}
{
    config = c;
    setWindowTitle(tr("Measurement receiver options"));

    mainLayout->addRow(cbOpt1);
    cbOpt1->setText("Connect to receiver when program starts");
    cbOpt1->setToolTip("The receiver will be connected upon startup");
    cbOpt1->setChecked(config->getInstrConnectOnStartup());

    mainLayout->addRow(cbOpt2);
    cbOpt2->setText("Use TCP datastream instead of UDP");
    cbOpt2->setToolTip("Stream data from device with TCP protocol. Only supported on newer receivers.");
    cbOpt2->setChecked(config->getInstrUseTcpDatastream());

    mainLayout->addRow(cbOpt3);
    cbOpt3->setText("Reconnect automagically");
    cbOpt3->setToolTip("If the datastream times out we will check periodically if the receiver is available,"\
                              " and reconnect when possible. Allows other users to temporarily use the device"\
                              " for other purposes. It will also try to reconnect if network is down shortly");
    cbOpt3->setChecked(config->getInstrAutoReconnect());

    mainLayout->addRow(cbOpt4);
    cbOpt4->setText("Normalize RF spectrum");
    cbOpt4->setToolTip("The spectrum will be \"corrected\" for any unlinear responses in the frequency range,"\
                                  "and centered around 0 dBuV. Can be used to remove steady noise signals and uneven"\
                                  "amplifier response");
    cbOpt4->setChecked(config->getInstrNormalizeSpectrum());

    connect(btnBox, &QDialogButtonBox::accepted, this, saveCurrentSettings);
    connect(btnBox, &QDialogButtonBox::rejected, dialog, close);

    mainLayout->addWidget(btnBox);
}

void ReceiverOptions::start()
{
    dialog->exec();
}

void ReceiverOptions::saveCurrentSettings()
{
    config->setInstrConnectOnStartup(cbOpt1->isChecked());
    config->setInstrUseTcpDatastream(cbOpt2->isChecked());
    config->setInstrAutoReconnect(cbOpt3->isChecked());
    config->setInstrNormalizeSpectrum(cbOpt4->isChecked());
    emit updSettings();

    dialog->close();
}
