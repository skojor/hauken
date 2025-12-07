#include "audiooptions.h"

AudioOptions::AudioOptions(QSharedPointer<Config> c)
{
    config = c;
    setWindowTitle("Audio options");

    mainLayout->addRow(cbOpt4);
    cbOpt4->setText("Activate demodulation");

    mainLayout->addRow(new QLabel("Modulation type"), comboOpt3);

    mainLayout->addRow(new QLabel("Modulation bandwidth (kHz)"), comboOpt4);

    mainLayout->addRow(cbOpt3, sbOpt1);
    cbOpt3->setText("Squelch level");
    sbOpt1->setRange(-99,99);

    mainLayout->addRow(cbOpt1, comboOpt1);
    cbOpt1->setText("Audio playback");


    for (auto && val : updMediaDevicesList()) {
        comboOpt1->addItem(val.description());
        if (val.isDefault())
            comboOpt1->setCurrentIndex(comboOpt1->count() - 1);
    }

    mainLayout->addRow(cbOpt2);
    cbOpt2->setText("Save audio to file");

    mainLayout->addRow(new QLabel("Audio mode", this), comboOpt2);
    comboOpt2->addItems(updAudioFormatList());

    mainLayout->addWidget(btnBox);
    connect(btnBox, &QDialogButtonBox::accepted, this, &AudioOptions::saveCurrentSettings);
    connect(btnBox, &QDialogButtonBox::rejected, dialog, &QDialog::close);
}

void AudioOptions::start()
{
    cbOpt1->setChecked(config->getAudioLivePlayback());
    cbOpt2->setChecked(config->getAudioRecordToFile());
    comboOpt2->setCurrentIndex(config->getAudioMode());
    cbOpt3->setChecked(config->getAudioSquelch());
    emit askForDemodTypeList();
    emit askForDemodBwList();

    comboOpt3->setCurrentIndex(comboOpt3->findText(config->getAudioModulationType()));
    comboOpt4->setCurrentIndex(comboOpt4->findText(config->getAudioModulationBw()));
    sbOpt1->setValue(config->getAudioSquelchLevel());
    cbOpt4->setChecked(config->getAudioActivate());

    dialog->exec();
}

void AudioOptions::saveCurrentSettings()
{
    config->setAudioLivePlayback(cbOpt1->isChecked());
    config->setAudioRecordToFile(cbOpt2->isChecked());
    config->setPlaybackDevice(comboOpt1->currentIndex());
    config->setAudioMode(comboOpt2->currentIndex());
    config->setAudioModulationType(comboOpt3->currentText());
    config->setAudioModulationBw(comboOpt4->currentText());
    config->setAudioSquelch(cbOpt3->isChecked());
    config->setAudioSquelchLevel(sbOpt1->value());
    config->setAudioActivate(cbOpt4->isChecked());

    report();

    dialog->close();
}

QList<QAudioDevice> AudioOptions::updMediaDevicesList()
{
    QList<QAudioDevice> audioDevices = QMediaDevices::audioOutputs();

    for (auto && val : audioDevices)
        qDebug() << val.id() << val.description() << val.isDefault();

    return audioDevices;
}

QStringList AudioOptions::updAudioFormatList()
{
    QStringList formats;
    formats.append("32 kHz, 16 bit, stereo");
    formats.append("32 kHz, 16 bit, mono");
    formats.append("32 kHz, 8 bit, stereo");
    formats.append("32 kHz, 8 bit, mono");
    formats.append("16 kHz, 16 bit, stereo");
    formats.append("16 kHz, 16 bit, mono");
    formats.append("16 kHz, 8 bit, stereo");
    formats.append("16 kHz, 8 bit, mono");
    formats.append("8 kHz, 16 bit, stereo");
    formats.append("8 kHz, 16 bit, mono");
    formats.append("8 kHz, 8 bit, stereo");
    formats.append("8 kHz, 8 bit, mono");

    return formats;
}

void AudioOptions::getDemodBwList(const QStringList &list)
{
    comboOpt4->clear();
    comboOpt4->addItems(list);
}

void AudioOptions::getDemodTypeList(const QStringList &list)
{
    comboOpt3->clear();
    comboOpt3->addItems(list);
}

void AudioOptions::reportAudioMode()
{
    if (config->getAudioActivate())
        emit audioMode(config->getAudioMode() + 1);
    else
        emit audioMode(0); // Effectively disabling demod
}

void AudioOptions::reportDemodBw()
{
    emit demodBw(config->getAudioModulationBw().toInt());
}

void AudioOptions::reportDemodType()
{
    emit demodType(config->getAudioModulationType());
}

void AudioOptions::reportPlayback()
{
    emit activateAudio(config->getAudioLivePlayback());
    QList<QAudioDevice> audioDevices = QMediaDevices::audioOutputs();
    for (int i=0; i<audioDevices.size(); i++)
        if (audioDevices[i].isDefault()) emit audioDevice(i);
}

void AudioOptions::reportSquelch()
{
    emit squelch(config->getAudioSquelch());
    emit squelchLevel(config->getAudioSquelchLevel());
}

void AudioOptions::reportRecord()
{
    emit record(config->getAudioRecordToFile());
}

void AudioOptions::report()
{
    reportAudioMode();
    reportDemodBw();
    reportDemodType();
    reportPlayback();
    reportSquelch();
    reportRecord();
}
