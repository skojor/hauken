#include "mqttoptions.h"

#include <algorithm>
#include <QHeaderView>
#include <QHBoxLayout>
#include <QMessageBox>
#include <QUuid>
#include <QVBoxLayout>

MqttOptions::MqttOptions(QSharedPointer<Config> c)
    : OptionsBaseClass{}
{
    config = c;
    setWindowTitle(tr("MQTT and webswitch configuration"));

    auto pageLayout = new QHBoxLayout(this);
    auto profilePane = new QVBoxLayout;
    profilePane->addWidget(new QLabel(tr("MQTT brokers")));
    profilePane->addWidget(profileList, 1);

    auto profileButtons = new QHBoxLayout;
    profileButtons->addWidget(addProfileButton);
    profileButtons->addWidget(removeProfileButton);
    profilePane->addLayout(profileButtons);
    pageLayout->addLayout(profilePane, 1);

    auto editorLayout = new QVBoxLayout;
    auto connectionGroup = new QGroupBox(tr("Broker connection"));
    auto connectionLayout = new QFormLayout(connectionGroup);
    connectionLayout->addRow(profileEnabled);
    connectionLayout->addRow(tr("Profile name"), profileName);
    connectionLayout->addRow(tr("Server IP/address"), server);
    connectionLayout->addRow(tr("Username"), username);
    connectionLayout->addRow(tr("Password"), password);
    connectionLayout->addRow(tr("Port"), port);
    connectionLayout->addRow(tr("Keepalive topic"), keepaliveTopic);
    editorLayout->addWidget(connectionGroup);

    password->setEchoMode(QLineEdit::Password);
    port->setRange(1, 65535);
    port->setValue(1883);

    auto subscriptionGroup = new QGroupBox(tr("Sensor subscriptions"));
    auto subscriptionLayout = new QVBoxLayout(subscriptionGroup);
    subscriptions->setColumnCount(3);
    subscriptions->setHorizontalHeaderLabels({tr("Name"), tr("Topic"), tr("Incident log")});
    subscriptions->horizontalHeader()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
    subscriptions->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Stretch);
    subscriptions->horizontalHeader()->setSectionResizeMode(2, QHeaderView::ResizeToContents);
    subscriptions->setSelectionBehavior(QAbstractItemView::SelectRows);
    subscriptions->setSelectionMode(QAbstractItemView::SingleSelection);
    subscriptionLayout->addWidget(subscriptions);

    auto subscriptionButtons = new QHBoxLayout;
    subscriptionButtons->addWidget(addSubscriptionButton);
    subscriptionButtons->addWidget(removeSubscriptionButton);
    subscriptionButtons->addStretch();
    subscriptionLayout->addLayout(subscriptionButtons);
    editorLayout->addWidget(subscriptionGroup, 1);

    auto primaryLayout = new QFormLayout(primaryOptions);
    primaryLayout->addRow(testTriggersRecording);
    primaryLayout->addRow(tr("Site filter"), siteFilter);
    primaryLayout->addRow(tr("Temperature HTTP(s) address"), webswitchAddress);
    siteFilter->setRange(0, 3);
    editorLayout->addWidget(primaryOptions);
    pageLayout->addLayout(editorLayout, 3);

    connect(addProfileButton, &QPushButton::clicked, this, &MqttOptions::addProfile);
    connect(removeProfileButton, &QPushButton::clicked, this, &MqttOptions::removeProfile);
    connect(profileList, &QListWidget::currentRowChanged, this, &MqttOptions::profileSelectionChanged);
    connect(addSubscriptionButton, &QPushButton::clicked, this, &MqttOptions::addSubscription);
    connect(removeSubscriptionButton, &QPushButton::clicked, this, &MqttOptions::removeSubscription);
    connect(profileName, &QLineEdit::textChanged, this, [this] {
        if (currentProfileIndex >= 0) updateProfileListItem(currentProfileIndex);
    });

    profiles = config->getMqttBrokerProfiles();
    for (const MqttBrokerProfile &profile : profiles)
        profileList->addItem(profile.name);
    for (int index = 0; index < profiles.size(); ++index)
        updateProfileListItem(index);

    if (!profiles.isEmpty()) profileList->setCurrentRow(0);
}

void MqttOptions::start()
{
}

void MqttOptions::saveCurrentSettings()
{
    storeCurrentProfile();

    const QStringList existingIds = config->getMqttBrokerProfileIds();
    QStringList retainedIds;
    for (const MqttBrokerProfile &profile : profiles) retainedIds.append(profile.id);
    for (const QString &id : existingIds) {
        if (!retainedIds.contains(id)) config->removeMqttBrokerProfile(id);
    }

    for (const MqttBrokerProfile &profile : profiles)
        config->setMqttBrokerProfile(profile);

    const auto primaryIt = std::find_if(profiles.cbegin(), profiles.cend(), [](const MqttBrokerProfile &profile) {
        return profile.primary;
    });
    if (primaryIt == profiles.cend()) return;

    config->setMqttActivate(primaryIt->enabled);
    config->setMqttServer(primaryIt->server);
    config->setMqttUsername(primaryIt->username);
    config->setMqttPassword(primaryIt->password);
    config->setMqttPort(primaryIt->port);
    config->setMqttKeepaliveTopic(primaryIt->keepaliveTopic);
    config->setMqttSubNames(primaryIt->subNames);
    config->setMqttSubTopics(primaryIt->subTopics);
    config->setMqttSubToIncidentlog(primaryIt->subToIncidentlog);
    config->setMqttTestTriggersRecording(testTriggersRecording->isChecked());
    config->setMqttSiteFilter(siteFilter->value());
    config->setMqttWebswitchAddress(webswitchAddress->text());
}

void MqttOptions::addProfile()
{
    storeCurrentProfile();

    MqttBrokerProfile profile;
    profile.id = QUuid::createUuid().toString(QUuid::WithoutBraces);
    profile.name = tr("New broker");
    profiles.append(profile);
    profileList->addItem(profile.name);
    profileList->setCurrentRow(profiles.size() - 1);
    profileName->setFocus();
    profileName->selectAll();
}

void MqttOptions::removeProfile()
{
    if (currentProfileIndex < 0 || currentProfileIndex >= profiles.size()) return;
    if (profiles.at(currentProfileIndex).primary) return;

    const QString name = profiles.at(currentProfileIndex).name;
    if (QMessageBox::question(this,
                              tr("Remove MQTT broker"),
                              tr("Remove broker profile '%1'?").arg(name)) != QMessageBox::Yes)
        return;

    const int removedIndex = currentProfileIndex;
    currentProfileIndex = -1;
    profiles.removeAt(removedIndex);
    delete profileList->takeItem(removedIndex);
    if (!profiles.isEmpty()) profileList->setCurrentRow(qMin(removedIndex, profiles.size() - 1));
    else loadProfile(-1);
}

void MqttOptions::profileSelectionChanged(int row)
{
    if (row == currentProfileIndex) return;
    storeCurrentProfile();
    currentProfileIndex = row;
    loadProfile(row);
}

void MqttOptions::addSubscription()
{
    const int row = subscriptions->rowCount();
    subscriptions->insertRow(row);
    subscriptions->setItem(row, 0, new QTableWidgetItem);
    subscriptions->setItem(row, 1, new QTableWidgetItem);
    auto incidentItem = new QTableWidgetItem;
    incidentItem->setFlags(incidentItem->flags() | Qt::ItemIsUserCheckable);
    incidentItem->setCheckState(Qt::Unchecked);
    subscriptions->setItem(row, 2, incidentItem);
    subscriptions->setCurrentCell(row, 0);
}

void MqttOptions::removeSubscription()
{
    if (subscriptions->currentRow() >= 0)
        subscriptions->removeRow(subscriptions->currentRow());
}

void MqttOptions::loadProfile(int index)
{
    const bool valid = index >= 0 && index < profiles.size();
    profileName->setEnabled(valid);
    profileEnabled->setEnabled(valid);
    server->setEnabled(valid);
    username->setEnabled(valid);
    password->setEnabled(valid);
    port->setEnabled(valid);
    keepaliveTopic->setEnabled(valid);
    subscriptions->setEnabled(valid);
    addSubscriptionButton->setEnabled(valid);
    removeSubscriptionButton->setEnabled(valid);
    removeProfileButton->setEnabled(valid && !profiles.at(index).primary);
    primaryOptions->setEnabled(valid && profiles.at(index).primary);

    subscriptions->setRowCount(0);
    if (!valid) {
        profileName->clear();
        profileEnabled->setChecked(false);
        server->clear();
        username->clear();
        password->clear();
        port->setValue(1883);
        keepaliveTopic->clear();
        return;
    }

    const MqttBrokerProfile &profile = profiles.at(index);
    profileName->setText(profile.name);
    profileEnabled->setChecked(profile.enabled);
    server->setText(profile.server);
    username->setText(profile.username);
    password->setText(profile.password);
    port->setValue(profile.port);
    keepaliveTopic->setText(profile.keepaliveTopic);

    const int count = qMax(profile.subNames.size(), profile.subTopics.size());
    for (int row = 0; row < count; ++row) {
        addSubscription();
        if (row < profile.subNames.size()) subscriptions->item(row, 0)->setText(profile.subNames.at(row));
        if (row < profile.subTopics.size()) subscriptions->item(row, 1)->setText(profile.subTopics.at(row));
        if (row < profile.subToIncidentlog.size() && profile.subToIncidentlog.at(row) == "1")
            subscriptions->item(row, 2)->setCheckState(Qt::Checked);
    }

    if (profile.primary) {
        testTriggersRecording->setChecked(config->getMqttTestTriggersRecording());
        siteFilter->setValue(config->getMqttSiteFilter());
        webswitchAddress->setText(config->getMqttWebswitchAddress());
    }
}

void MqttOptions::storeCurrentProfile()
{
    if (currentProfileIndex < 0 || currentProfileIndex >= profiles.size()) return;

    MqttBrokerProfile &profile = profiles[currentProfileIndex];
    profile.name = profileName->text().trimmed();
    if (profile.name.isEmpty()) profile.name = profile.id;
    profile.enabled = profileEnabled->isChecked();
    profile.server = server->text().trimmed();
    profile.username = username->text().trimmed();
    profile.password = password->text();
    profile.port = port->value();
    profile.keepaliveTopic = keepaliveTopic->text().trimmed();
    profile.subNames.clear();
    profile.subTopics.clear();
    profile.subToIncidentlog.clear();

    for (int row = 0; row < subscriptions->rowCount(); ++row) {
        const QString name = subscriptions->item(row, 0) ? subscriptions->item(row, 0)->text().trimmed() : QString();
        const QString topic = subscriptions->item(row, 1) ? subscriptions->item(row, 1)->text().trimmed() : QString();
        if (topic.isEmpty()) continue;
        profile.subNames.append(name.isEmpty() ? topic : name);
        profile.subTopics.append(topic);
        const bool incidentLog = subscriptions->item(row, 2)
                                 && subscriptions->item(row, 2)->checkState() == Qt::Checked;
        profile.subToIncidentlog.append(incidentLog ? "1" : "0");
    }

    updateProfileListItem(currentProfileIndex);
}

void MqttOptions::updateProfileListItem(int index)
{
    if (index < 0 || index >= profileList->count()) return;
    QString name = index == currentProfileIndex ? profileName->text().trimmed() : profiles.at(index).name;
    if (name.isEmpty()) name = profiles.at(index).id;
    if (profiles.at(index).primary) name += tr(" (primary)");
    profileList->item(index)->setText(name);
}