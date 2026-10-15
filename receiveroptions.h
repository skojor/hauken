#ifndef RECEIVEROPTIONS_H
#define RECEIVEROPTIONS_H

#include <QDoubleSpinBox>
#include "optionsbaseclass.h"
#include "measurementreceiversettings.h"

class ReceiverOptions : public OptionsBaseClass
{
public:
    ReceiverOptions(QSharedPointer<Config> c);

signals:

public slots:
    void start();
    void saveCurrentSettings();

private:
    QGroupBox *gnssIncidentSettingsGroup = new QGroupBox(tr("GNSS incident receiver profile"));
    QCheckBox *gnssIncidentSettingsEnabled = new QCheckBox(tr("Use these settings for GNSS incidents"));
    QDoubleSpinBox *gnssStartFreq = new QDoubleSpinBox;
    QDoubleSpinBox *gnssStopFreq = new QDoubleSpinBox;
    QLineEdit *gnssResolution = new QLineEdit;
    QDoubleSpinBox *gnssFfmCenterFreq = new QDoubleSpinBox;
    QLineEdit *gnssFfmSpan = new QLineEdit;
    QSpinBox *gnssMeasurementTime = new QSpinBox;
    QSpinBox *gnssManualAttenuator = new QSpinBox;
    QCheckBox *gnssAutoAttenuator = new QCheckBox;
    QSpinBox *gnssAntennaPort = new QSpinBox;
    QComboBox *gnssMode = new QComboBox;
    QLineEdit *gnssFftMode = new QLineEdit;
    QSpinBox *gnssGainControl = new QSpinBox;
};

#endif // RECEIVEROPTIONS_H
