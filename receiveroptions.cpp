#include "receiveroptions.h"

ReceiverOptions::ReceiverOptions(QSharedPointer<Config> c)
    : OptionsBaseClass {}
{
    config = c;
    setWindowTitle(tr("Measurement receiver options"));

    mainLayout->addRow(cbOpt1);
    cbOpt1->setText("Connect to receiver when program starts");
    cbOpt1->setToolTip("The receiver will be connected upon startup");

    mainLayout->addRow(cbOpt2);
    cbOpt2->setText("Use TCP datastream instead of UDP");
    cbOpt2->setToolTip("Stream data from device with TCP protocol. Only supported on newer receivers.");

    mainLayout->addRow(cbOpt3);
    cbOpt3->setText("Reconnect automagically");
    cbOpt3->setToolTip("If the datastream times out we will check periodically if the receiver is available,"\
                              " and reconnect when possible. Allows other users to temporarily use the device"\
                              " for other purposes. It will also try to reconnect if network is down shortly");

    mainLayout->addRow(cbOpt4);
    cbOpt4->setText("Normalize RF spectrum");
    cbOpt4->setToolTip("The spectrum will be \"corrected\" for any unlinear responses in the frequency range, "\
                                  "and centered around 0 dBuV. Can be used to remove steady noise signals and uneven "\
                                  "amplifier response");

    mainLayout->addRow(new QLabel(tr("Minimum number of traces used for average calculation")), sbOpt1);
    sbOpt1->setToolTip("When started, and whenever any relevant settings are changed, "\
                       "the software will gather traces for average calculation before trig line is \"armed\". "\
                       "Default value is 250. The average calculation will slow down gradually when enough traces are gathered");
    sbOpt1->setRange(0, 1e9);

    mainLayout->addRow(cbOpt5);
    cbOpt5->setText("Create IQ high res. plot when triggered");
    cbOpt5->setToolTip("If enabled, the receiver will be switched to FFM mode and collect a few ms of IQ data "\
                       "when triggered. The data will be used to create a high resolution FFT plot and sent"\
                       "via email.");

    mainLayout->addRow(new QLabel(tr("IQ data plot length in microseconds")), sbOpt2);
    sbOpt2->setToolTip("This value determines the time range of the FFT plot. Value range is 50 - 4000 microsconds.");
    sbOpt2->setRange(50, 4000);

    mainLayout->addRow(new QLabel(tr("IQ data bandwidth")), comboOpt1);
    comboOpt1->setToolTip("Which bandwidth to use for the receiver. Receiver dependent, "\
                          "must be set according to receiver specs!");
    comboOpt1->addItems(QStringList() << "500" << "1000" << "5000" << "10000" << "20000" << "40000");

    connect(btnBox, &QDialogButtonBox::accepted, this, &ReceiverOptions::saveCurrentSettings);
    connect(btnBox, &QDialogButtonBox::rejected, dialog, &QDialog::close);

    mainLayout->addWidget(btnBox);
}

void ReceiverOptions::start()
{
    cbOpt1->setChecked(config->getInstrConnectOnStartup());
    cbOpt2->setChecked(config->getInstrUseTcpDatastream());
    cbOpt3->setChecked(config->getInstrAutoReconnect());
    cbOpt4->setChecked(config->getInstrNormalizeSpectrum());
    sbOpt1->setValue(config->getInstrTracesNeededForAverage());
    cbOpt5->setChecked(config->getInstrCreateFftPlot());
    sbOpt2->setValue(config->getInstrFftPlotLength());
    comboOpt1->setCurrentText(QString::number(config->getInstrFftPlotBw()));
    dialog->exec();
}

void ReceiverOptions::saveCurrentSettings()
{
    config->setInstrConnectOnStartup(cbOpt1->isChecked());
    config->setInstrUseTcpDatastream(cbOpt2->isChecked());
    config->setInstrAutoReconnect(cbOpt3->isChecked());
    config->setInstrNormalizeSpectrum(cbOpt4->isChecked());
    config->setInstrTracesNeededForAverage(sbOpt1->value());
    config->setInstrCreateFftPlot(cbOpt5->isChecked());
    config->setInstrFftPlotLength(sbOpt2->value());
    config->setInstrFftPlotBw(comboOpt1->currentText().toInt());

    emit updSettings();

    dialog->close();
}
