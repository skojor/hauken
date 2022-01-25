#include "cameraoptions.h"

CameraOptions::CameraOptions(QSharedPointer<Config> c)
{
    config = c;

    setWindowTitle("Camera configuration");

    mainLayout->addRow(new QLabel("USB camera source"), comboOpt1);
    fillCameraCombo();
    comboOpt1->setToolTip("Select which USB camera device to use, or none to disable");

    mainLayout->addRow(new QLabel("Network camera address"), leOpt1);
    leOpt1->setToolTip("Input the RTSP stream address to use, or leave blank to disable\nRTSP streams typically looks something like this:\nrtsp://192.168.100.100:554/stream1");

    mainLayout->addRow(cbOpt1);
    cbOpt1->setText("Measurement device high level triggers recording");
    cbOpt1->setToolTip("If checked, the above chosen camera will be used to create a recording\nwhenever signal levels exceeds the limit settings");

    mainLayout->addRow(new QLabel("Record time (seconds)"), sbOpt1);
    sbOpt1->setToolTip("Time in seconds to record a video after the incident was triggered");
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
    cameras = QCameraInfo::availableCameras();
    for (auto &val : cameras)
        comboOpt1->addItem(val.description());
}
