#ifndef CAMERAOPTIONS_H
#define CAMERAOPTIONS_H

#include "optionsbaseclass.h"
#include <qcamera.h>
#include <qcamerainfo.h>

class CameraOptions : public OptionsBaseClass
{
public:
    CameraOptions(QSharedPointer<Config> c);

public slots:
    void start();
    void saveCurrentSettings();

private slots:
    void fillCameraCombo();

private:
    QList<QCameraInfo> cameras;

};

#endif // CAMERAOPTIONS_H
