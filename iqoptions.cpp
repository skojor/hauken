#include "iqoptions.h"

IqOptions::IqOptions(QSharedPointer<Config> c)
    : OptionsBaseClass {}
{
    config = c;
    setWindowTitle(tr("IQ data and plot options"));

    mainLayout->addRow(cbOpt1);
    cbOpt1->setText("Save IQ data to file");
    cbOpt1->setToolTip("Enable this option to save all captured IQ data to a raw datafile.");

    mainLayout->addRow(cbOpt5);
    cbOpt5->setText("Create IQ high res. plot when triggered");
    cbOpt5->setToolTip("If enabled, the receiver will be switched to FFM mode and collect a few ms of IQ data "\
                       "when triggered. The data will be used to create a high resolution FFT plot and sent"\
                       "via email.");

    mainLayout->addRow(new QLabel(tr("IQ data plot length in microseconds")), sbOpt2);
    sbOpt2->setToolTip("This value determines the time range of the FFT plot. Value range is 40 - 400000 microseconds.");
    sbOpt2->setRange(40, 400000);

    mainLayout->addRow(new QLabel(tr("IQ data bandwidth")), comboOpt1);
    comboOpt1->setToolTip("Which bandwidth to use for the receiver. Receiver dependent, "\
                          "must be set according to receiver specs! This is NOT checked in software!");
    comboOpt1->addItems(QStringList() << "500" << "1000" << "5000" << "10000" << "20000" << "40000");

    mainLayout->addRow(new QLabel(tr("IQ prerecord time (seconds)")), leOpt1);
    leOpt1->setToolTip(tr("Decides how long Hauken will gather IQ data before analyzing." \
                          "Settings this value too small will increase the chance of not spotting an intermittent signal." \
                          "Too large, and the network buffers will saturate. 0.5 seconds is reasonable for a locally connected receiver." \
                          "All data gathered in this time will be saved to disk if enabled above."));

    mainLayout->addRow(cbOpt2);
    cbOpt2->setText(tr("Use dB scale for FFT values"));
    cbOpt2->setToolTip(tr("Use dB if lower level signals should become more visible. Default off, useful for CW/chirp signals."));

    connect(btnBox, &QDialogButtonBox::accepted, this, &IqOptions::saveCurrentSettings);
    connect(btnBox, &QDialogButtonBox::rejected, dialog, &QDialog::close);

    mainLayout->addWidget(btnBox);

}

void IqOptions::start()
{

    cbOpt1->setChecked(config->getIqSaveToFile());
    cbOpt5->setChecked(config->getIqCreateFftPlot());
    sbOpt2->setValue(config->getIqFftPlotLength());
    comboOpt1->setCurrentText(QString::number(config->getIqFftPlotBw()));
    leOpt1->setText(QString::number(config->getIqLogTime(), 'f', 3));
    cbOpt2->setChecked(config->getIqUseDB());

    dialog->exec();
}

void IqOptions::saveCurrentSettings()
{
    config->setIqSaveToFile(cbOpt1->isChecked());
    config->setIqCreateFftPlot(cbOpt5->isChecked());
    config->setIqFftPlotLength(sbOpt2->value());
    config->setIqFftPlotBw(comboOpt1->currentText().toInt());
    config->setIqLogTime(leOpt1->text().toDouble());
    config->setIqUseDB(cbOpt2->isChecked());

    emit updSettings();
    dialog->close();
}
