#include "cameraoptions.h"

CameraOptions::CameraOptions(QSharedPointer<Config> c)
{
    config = c;

    setWindowTitle("Camera configuration");

    mainLayout->addRow(cbOpt1);
    cbOpt1->setText("Activate camera when recording is triggered");
    cbOpt1->setToolTip("A triggered recording will also start camera recording");

    mainLayout->addRow(new QLabel("Camera source"), leOpt1);
    //fillCameraCombo();
    leOpt1->setToolTip("Name of USB (first or second) or IP camera device ip to use, or none to disable");

    mainLayout->addRow(new QLabel("Record time (seconds)"), sbOpt1);
    sbOpt1->setToolTip("Time in seconds to record a video after the incident ended");
    sbOpt1->setRange(1, 86400);

    connect(btnBox, &QDialogButtonBox::accepted, this, &CameraOptions::saveCurrentSettings);
    connect(btnBox, &QDialogButtonBox::rejected, dialog, &QDialog::close);

    mainLayout->addWidget(btnBox);

}

void CameraOptions::start()
{
    if (comboOpt1->findText(config->getCameraName()))
        comboOpt1->setCurrentIndex(comboOpt1->findText(config->getCameraName()));
    leOpt1->setText(config->getCameraStreamAddress());
    cbOpt1->setChecked(config->getCameraDeviceTrigger());
    sbOpt1->setValue(config->getCameraRecordTime());

    dialog->exec();
}

void CameraOptions::saveCurrentSettings()
{
    config->setCameraName(comboOpt1->currentText());
    config->setCameraStreamAddress(leOpt1->text());
    config->setCameraDeviceTrigger(cbOpt1->isChecked());
    config->setCameraRecordTime(sbOpt1->value());

    dialog->close();
}

void CameraOptions::fillCameraCombo()
{
    comboOpt1->clear();
    comboOpt1->addItem("None");
/*    cameras = QCameraInfo::availableCameras();
    for (auto &val : cameras)
        comboOpt1->addItem(val.description());*/
}
