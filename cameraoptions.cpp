#include "cameraoptions.h"

CameraOptions::CameraOptions(QSharedPointer<Config> c)
{
    config = c;

    setWindowTitle("Camera configuration");

    mainLayout->addRow(new QLabel("Camera"), comboOpt1);
    fillCameraCombo();
    comboOpt1->setToolTip("Which camera device to use");

    mainLayout->addRow(cbOpt1);
    cbOpt1->setText("Measurement device high level triggers recording");
    cbOpt1->setToolTip("If checked, the above chosen camera will be used to create a recording\nwhenever signal levels exceeds the limit settings");

    connect(btnBox, &QDialogButtonBox::accepted, this, &CameraOptions::saveCurrentSettings);
    connect(btnBox, &QDialogButtonBox::rejected, dialog, &QDialog::close);

    mainLayout->addWidget(btnBox);

}

void CameraOptions::start()
{
    if (comboOpt1->findText(config->getCameraName()))
        comboOpt1->setCurrentIndex(comboOpt1->findText(config->getCameraName()));
    cbOpt1->setChecked(config->getCameraDeviceTrigger());

    dialog->exec();
}

void CameraOptions::saveCurrentSettings()
{
    config->setCameraName(comboOpt1->currentText());
    config->setCameraDeviceTrigger(cbOpt1->isChecked());
}

void CameraOptions::fillCameraCombo()
{
    comboOpt1->clear();
    cameras = QCameraInfo::availableCameras();
    for (auto &val : cameras)
        comboOpt1->addItem(val.deviceName());
}
