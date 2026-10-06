#ifndef GNSSOPTIONS_H
#define GNSSOPTIONS_H

#include <QComboBox>
#include <QGroupBox>
#include <QLineEdit>
#include <QListWidget>
#include <QSerialPortInfo>
#include "config.h"
#include "gnssreceiverprofile.h"
#include "optionsbaseclass.h"

class GnssOptions : public OptionsBaseClass
{
    Q_OBJECT
public:
    GnssOptions(QSharedPointer<Config> c);

public slots:
    void start();
    void saveCurrentSettings();

private slots:
    QStringList getAvailablePorts();
    void refreshPpsProfiles();
    void addProfile();
    void removeProfile();
    void profileSelectionChanged(int row);

private:
    void loadProfile(int index);
    void storeCurrentProfile();
    void updateProfileListItem(int index);

    QList<GnssReceiverProfile> profiles;
    int currentProfileIndex = -1;

    QListWidget *profileList = new QListWidget;
    QPushButton *addProfileButton = new QPushButton(tr("Add GNSS receiver"));
    QPushButton *removeProfileButton = new QPushButton(tr("Remove"));

    QGroupBox *receiverGroupBox = new QGroupBox;
    QCheckBox *receiverEnabled = new QCheckBox;
    QLineEdit *receiverName = new QLineEdit;
    QLabel *portLabel = new QLabel(tr("Serial port name or IP address"));
    QComboBox *receiverPort = new QComboBox;
    QLabel *baudrateLabel = new QLabel(tr("Baudrate or port number"));
    QComboBox *receiverBaudrate = new QComboBox;
    QCheckBox *receiverLogToFile = new QCheckBox;
    QCheckBox *receiverUblox = new QCheckBox;
    QCheckBox *receiverShowSpectrumWindow = new QCheckBox;
    QCheckBox *receiverTriggerRecording = new QCheckBox;
};

#endif // GNSSOPTIONS_H
