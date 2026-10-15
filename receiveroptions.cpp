#include "receiveroptions.h"

ReceiverOptions::ReceiverOptions(QSharedPointer<Config> c)
    : OptionsBaseClass {}
{
    config = c;
    auto mainLayout = new QFormLayout(this);

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

    /*
    cbOpt4->setText("Normalize RF spectrum");
    cbOpt4->setToolTip("The spectrum will be \"corrected\" for any unlinear responses in the frequency range, "\
                                  "and centered around 0 dBuV. Can be used to remove steady noise signals and uneven "\
                                  "amplifier response");*/

    mainLayout->addRow(cbOpt5);
    cbOpt5->setText("Remember average noise floor");
    cbOpt5->setToolTip("When enabled the average noise floor will be stored to disk every 2 minutes. Upon reconnect or restart "\
                       "the levels will be reloaded, skipping the whole avg calculation routine. Changes to frequency, mode, "\
                       "resolution, antenna port, FFT mode or gain control will force a new avg calculation, clearing the stored values");

    mainLayout->addRow(new QLabel(tr("Minimum number of traces used for average calculation")), sbOpt1);
    sbOpt1->setToolTip("When started, and whenever any relevant settings are changed, "\
                       "the software will gather traces for average calculation before trig line is \"armed\". "\
                       "Default value is 250. The average calculation will slow down gradually when enough traces are gathered");
    sbOpt1->setRange(0, 1e9);

    mainLayout->addRow(new QLabel("(Normalize RF spectrum option is moved to main window, below spectrum display)"));

    auto gnssLayout = new QFormLayout(gnssIncidentSettingsGroup);
    gnssLayout->addRow(gnssIncidentSettingsEnabled);
    gnssLayout->addRow(tr("PScan start frequency (MHz)"), gnssStartFreq);
    gnssLayout->addRow(tr("PScan stop frequency (MHz)"), gnssStopFreq);
    gnssLayout->addRow(tr("PScan resolution (kHz)"), gnssResolution);
    gnssLayout->addRow(tr("FFM center frequency (MHz)"), gnssFfmCenterFreq);
    gnssLayout->addRow(tr("FFM span (kHz)"), gnssFfmSpan);
    gnssLayout->addRow(tr("Measurement time (ms)"), gnssMeasurementTime);
    gnssLayout->addRow(tr("Manual attenuator"), gnssManualAttenuator);
    gnssLayout->addRow(tr("Auto attenuator"), gnssAutoAttenuator);
    gnssLayout->addRow(tr("Antenna port index"), gnssAntennaPort);
    gnssLayout->addRow(tr("Instrument mode"), gnssMode);
    gnssLayout->addRow(tr("FFT mode"), gnssFftMode);
    gnssLayout->addRow(tr("Gain control"), gnssGainControl);
    mainLayout->addRow(gnssIncidentSettingsGroup);

    gnssStartFreq->setRange(0, 600e3);
    gnssStartFreq->setDecimals(6);
    gnssStopFreq->setRange(0, 600e3);
    gnssStopFreq->setDecimals(6);
    gnssResolution->setMaxLength(32);
    gnssFfmCenterFreq->setRange(1, 600e3);
    gnssFfmCenterFreq->setDecimals(6);
    gnssFfmSpan->setMaxLength(32);
    gnssMeasurementTime->setRange(1, 5000);
    gnssManualAttenuator->setRange(-40, 40);
    gnssAntennaPort->setRange(0, 32);
    gnssMode->addItems({"PScan", "FFM"});
    gnssFftMode->setMaxLength(32);
    gnssGainControl->setRange(0, 2);

    const MeasurementReceiverSettings gnssSettings = config->getGnssIncidentReceiverSettings();
    gnssIncidentSettingsEnabled->setChecked(config->getGnssIncidentReceiverSettingsEnabled());
    gnssStartFreq->setValue(gnssSettings.startFreq / 1e6);
    gnssStopFreq->setValue(gnssSettings.stopFreq / 1e6);
    gnssResolution->setText(gnssSettings.resolution);
    gnssFfmCenterFreq->setValue(gnssSettings.ffmCenterFreq / 1e6);
    gnssFfmSpan->setText(gnssSettings.ffmSpan);
    gnssMeasurementTime->setValue(gnssSettings.measurementTime);
    gnssManualAttenuator->setValue(gnssSettings.manualAttenuator);
    gnssAutoAttenuator->setChecked(gnssSettings.autoAttenuator);
    gnssAntennaPort->setValue(gnssSettings.antennaPort);
    gnssMode->setCurrentText(gnssSettings.mode);
    gnssFftMode->setText(gnssSettings.fftMode);
    gnssGainControl->setValue(gnssSettings.gainControl);


    cbOpt1->setChecked(config->getInstrConnectOnStartup());
    cbOpt2->setChecked(config->getInstrUseTcpDatastream());
    cbOpt3->setChecked(config->getInstrAutoReconnect());
    //cbOpt4->setChecked(config->getInstrNormalizeSpectrum());
    sbOpt1->setValue(config->getInstrTracesNeededForAverage());
    cbOpt5->setChecked(config->getInstrRestoreAvgLevels());

    /*connect(btnBox, &QDialogButtonBox::accepted, this, &ReceiverOptions::saveCurrentSettings);
    connect(btnBox, &QDialogButtonBox::rejected, dialog, &QDialog::close);

    mainLayout->addWidget(btnBox);*/
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
    //config->setInstrNormalizeSpectrum(cbOpt4->isChecked());
    config->setInstrTracesNeededForAverage(sbOpt1->value());
    config->setInstrRestoreAvgLevels(cbOpt5->isChecked());

    MeasurementReceiverSettings gnssSettings;
    gnssSettings.startFreq = static_cast<quint64>(gnssStartFreq->value() * 1e6);
    gnssSettings.stopFreq = static_cast<quint64>(gnssStopFreq->value() * 1e6);
    gnssSettings.resolution = gnssResolution->text().trimmed();
    gnssSettings.ffmCenterFreq = static_cast<quint64>(gnssFfmCenterFreq->value() * 1e6);
    gnssSettings.ffmSpan = gnssFfmSpan->text().trimmed();
    gnssSettings.measurementTime = gnssMeasurementTime->value();
    gnssSettings.manualAttenuator = gnssManualAttenuator->value();
    gnssSettings.autoAttenuator = gnssAutoAttenuator->isChecked();
    gnssSettings.antennaPort = gnssAntennaPort->value();
    gnssSettings.mode = gnssMode->currentText();
    gnssSettings.fftMode = gnssFftMode->text().trimmed();
    gnssSettings.gainControl = gnssGainControl->value();
    config->setGnssIncidentReceiverSettings(gnssSettings);
    config->setGnssIncidentReceiverSettingsEnabled(gnssIncidentSettingsEnabled->isChecked());

    emit updSettings();

    //dialog->close();
}
