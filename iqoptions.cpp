#include "iqoptions.h"

IqOptions::IqOptions(QSharedPointer<Config> c)
    : OptionsBaseClass {}
{
    config = c;
    setWindowTitle(tr("I/Q data and plot options"));

    mainLayout->addRow(cbOpt1);
    cbOpt1->setText("Save I/Q data to file");
    cbOpt1->setToolTip("Enable this option to save all captured I/Q data to a raw datafile.");

    mainLayout->addRow(cbOpt5);
    cbOpt5->setText("Create I/Q high res. plot when triggered");
    cbOpt5->setToolTip("If enabled, the receiver will be switched to FFM mode and collect a few ms of I/Q data "\
                       "when triggered. The data will be used to create a high resolution FFT plot and sent"\
                       "via email.");

    mainLayout->addRow(cbOpt2);
    cbOpt2->setText(tr("Use dB scale for FFT values"));
    cbOpt2->setToolTip(tr("Use dB if lower level signals should become more visible. Default off, useful for CW/chirp signals."));

    mainLayout->addRow(cbOpt3);
    cbOpt3->setText(tr("Apply Hann window on FFT plot"));
    cbOpt3->setToolTip(tr("Using a window function on the I/Q data before FFT reduces imaging and "
                          "echos in the plot, "
                          "but will make the plotted signal look \"smeared\"."));

    mainLayout->addRow(new QLabel(tr("I/Q data plot length in microseconds")), sbOpt2);
    sbOpt2->setToolTip("This value determines the time range of the FFT plot. Value range is 40 - 400000 microseconds.");
    sbOpt2->setRange(40, 400000);

    mainLayout->addRow(new QLabel(tr("I/Q data bandwidth")), comboOpt1);
    comboOpt1->setToolTip("Which bandwidth to use for the receiver. Receiver dependent, "\
                          "must be set according to receiver specs! This is NOT checked in software!");
    comboOpt1->addItems(QStringList() << "500" << "1000" << "5000" << "10000" << "20000" << "40000");

    mainLayout->addRow(new QLabel(tr("I/Q prerecord time (seconds)")), leOpt1);
    leOpt1->setToolTip(tr("Decides how long Hauken will gather I/Q data before analyzing." \
                          "Settings this value too small will increase the chance of not spotting an intermittent signal." \
                          "Too large, and the network buffers will saturate. 0.5 seconds is reasonable for a locally connected receiver." \
                          "All data gathered in this time will be saved to disk if enabled above."));

    mainLayout->addRow(cbOpt4);
    cbOpt4->setText(tr("Record multiple bands when triggered (defined in IqGnssBands.csv)"));
    cbOpt4->setToolTip(tr("Enable this option to make a recording on FFM center frequencies on multiple bands when triggered. Cannot be combined with trig area setting below."));

    mainLayout->addRow(cbOpt5);
    cbOpt5->setText(tr("Record I/Q samples over the whole trig area"));
    cbOpt5->setToolTip(tr("If enabled the I/Q center frequencies will be decided by the trig area(s), so that all areas are covered by I/Q recordings. Cannot be combined with multiband rec. set above"));

    connect(btnBox, &QDialogButtonBox::accepted, this, &IqOptions::saveCurrentSettings);
    connect(btnBox, &QDialogButtonBox::rejected, dialog, &QDialog::close);
    connect(cbOpt4, &QCheckBox::clicked, this, [this](bool b) {
        if (b) cbOpt5->setChecked(false);
    });
    connect(cbOpt5, &QCheckBox::clicked, this, [this](bool b) {
        if (b) cbOpt4->setChecked(false);
    });

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
    cbOpt3->setChecked(config->getIqUseWindow());
    cbOpt4->setChecked(config->getIqRecordMultipleBands());
    cbOpt5->setChecked(config->getIqRecordAllTrigArea());

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
    config->setIqUseWindow(cbOpt3->isChecked());
    config->setIqRecordMultipleBands(cbOpt4->isChecked());
    config->setIqRecordAllTrigArea(cbOpt5->isChecked());

    emit updSettings();
    dialog->close();
}
