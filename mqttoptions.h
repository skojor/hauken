#ifndef MQTTOPTIONS_H
#define MQTTOPTIONS_H

#include "config.h"
#include "optionsbaseclass.h"
#include <QListWidget>
#include <QTableWidget>

class MqttOptions : public OptionsBaseClass
{
    Q_OBJECT
public:
    MqttOptions(QSharedPointer<Config> c);

public slots:
    void start();
    void saveCurrentSettings();

private slots:
    void addProfile();
    void removeProfile();
    void profileSelectionChanged(int row);
    void addSubscription();
    void removeSubscription();

private:
    void loadProfile(int index);
    void storeCurrentProfile();
    void updateProfileListItem(int index);

    QList<MqttBrokerProfile> profiles;
    int currentProfileIndex = -1;

    QListWidget *profileList = new QListWidget;
    QPushButton *addProfileButton = new QPushButton(tr("Add"));
    QPushButton *removeProfileButton = new QPushButton(tr("Remove"));
    QLineEdit *profileName = new QLineEdit;
    QCheckBox *profileEnabled = new QCheckBox(tr("Enable this broker"));
    QLineEdit *server = new QLineEdit;
    QLineEdit *username = new QLineEdit;
    QLineEdit *password = new QLineEdit;
    QSpinBox *port = new QSpinBox;
    QLineEdit *keepaliveTopic = new QLineEdit;
    QTableWidget *subscriptions = new QTableWidget;
    QPushButton *addSubscriptionButton = new QPushButton(tr("Add subscription"));
    QPushButton *removeSubscriptionButton = new QPushButton(tr("Remove subscription"));

    QGroupBox *primaryOptions = new QGroupBox(tr("Primary broker integrations"));
    QCheckBox *testTriggersRecording = new QCheckBox(tr("MQTT test start triggers recording"));
    QSpinBox *siteFilter = new QSpinBox;
    QLineEdit *webswitchAddress = new QLineEdit;
};

#endif // MQTTOPTIONS_H
