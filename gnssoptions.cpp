#include "gnssoptions.h"

#include <QHBoxLayout>
#include <QMessageBox>
#include <QSignalBlocker>
#include <QUuid>
#include <QVBoxLayout>

GnssOptions::GnssOptions(QSharedPointer<Config> c)
    : OptionsBaseClass{}
{
    config = c;
    setWindowTitle("GNSS receiver configuration");

    auto pageLayout = new QVBoxLayout(this);
    auto receiversLayout = new QHBoxLayout;

    auto listPane = new QVBoxLayout;
    listPane->addWidget(new QLabel(tr("GNSS receivers")));
    listPane->addWidget(profileList, 1);
    auto listButtons = new QHBoxLayout;
    listButtons->addWidget(addProfileButton);
    listButtons->addWidget(removeProfileButton);
    listPane->addLayout(listButtons);
    receiversLayout->addLayout(listPane, 1);

    QFormLayout *receiverLayout = new QFormLayout(receiverGroupBox);
    receiverLayout->addRow(receiverEnabled);
    receiverEnabled->setToolTip("Check this box to activate the GNSS surveillance for this receiver");
    receiverLayout->addRow(tr("Receiver name"), receiverName);
    receiverName->setToolTip("Name shown in the GNSS status/data windows");

    receiverLayout->addRow(portLabel, receiverPort);
    receiverPort->setEditable(true);
    receiverPort->addItems(getAvailablePorts());
    receiverPort->setToolTip("GNSS device serial port, or TCP/IP server sending raw NMEA and/or uBlox binary data");

    receiverLayout->addRow(baudrateLabel, receiverBaudrate);
    receiverBaudrate->setEditable(true);
    receiverBaudrate->addItems(QStringList() << "1200" << "2400" << "4800" << "9600" << "19200" << "38400" << "57600" << "115200");
    receiverBaudrate->setToolTip("GNSS device baudrate, or TCP port number if IP address is used above");

    receiverLayout->addRow(receiverLogToFile);
    receiverLogToFile->setText("Log NMEA to file");
    receiverLogToFile->setToolTip("When checked all NMEA sentences will be logged to a logfile.\nA new logfile is started every day");

    receiverLayout->addRow(receiverUblox);
    receiverUblox->setText("U-blox M8/M9/M10/X20P and Septentrio support");
    receiverUblox->setToolTip("If checked the GPS will be configured as a uBlox receiver with enhanced binary protocol. \n"\
                       "Will enable several cool features, for now AGC and jamming indicator levels.\n"\
                       "Of course this will only work if the connected GNSS device is a uBlox or Septentrio. Use at your own risk!\n" \
                       "Septentrio support reads out AGC per band for the main antenna, labelled by frequency band. \n" \
                       "Bands with no actively tracked signal are hidden if the MeasEpoch SBF block is also enabled, \n" \
                       "although all binary and NMEA messages will be logged if logging is enabled.");

    receiverLayout->addRow(receiverTriggerRecording);
    receiverTriggerRecording->setText("Detected incidents triggers spectrum recording");
    receiverTriggerRecording->setToolTip("Any incidents detected will cause a recording to be started, and uploaded if configured");

    receiverLayout->addRow(receiverShowSpectrumWindow);
    receiverShowSpectrumWindow->setText("View spectrum in own window");
    receiverShowSpectrumWindow->setToolTip("Opens a separate window showing this receiver's spectrum trace.\n"\
                       "Only available for u-blox and Septentrio receivers that support sending spectrum data: "\
                       "u-blox X20P (MON-SPAN), or Septentrio receivers with the BBSamples (4040) SBF block "\
                       "enabled on the connected port (configured on the receiver itself, e.g. via RxTools or its "\
                       "web interface - Hauken only decodes what the receiver is already configured to send).");

    receiversLayout->addWidget(receiverGroupBox, 3);
    pageLayout->addLayout(receiversLayout);

    QGroupBox *gnss4GroupBox = new QGroupBox("GNSS data display");
    QFormLayout *gnss4Layout = new QFormLayout;
    gnss4GroupBox->setLayout(gnss4Layout);
    gnss4Layout->addRow(cbOpt13);
    cbOpt13->setText("Show GNSS data in a separate window");
    cbOpt13->setToolTip("Enabling this option opens a new window showing vital GNSS information for the configured GNSS receivers");

    cbOpt14->setText("GNSS messages to incident log");
    cbOpt14->setToolTip("Enable this option to show GNSS messages in the incident log");
    gnss4Layout->addRow(cbOpt14);

    gnss4Layout->addRow(new QLabel(tr("Incident filter time (sec)")), sbOpt1);
    sbOpt1->setToolTip("Any incidents will be filtered until the set filter time has passed. If incident is gone before the set time it will not appear in the log.");
    sbOpt1->setRange(1, 86400);

    auto ppsGroupBox = new QGroupBox(tr("PPS timing display"));
    auto ppsLayout = new QFormLayout(ppsGroupBox);
    cbOpt15->setText(tr("Show PPS timing in the GNSS window"));
    cbOpt15->setToolTip(tr("Display reference, GPS, and Galileo PPS status and offsets in the separate GNSS window"));
    ppsLayout->addRow(cbOpt15);
    cbOpt16->setText(tr("Plot PPS difference in a separate window"));
    cbOpt16->setToolTip(tr("Plot reference minus GPS and reference minus Galileo PPS offsets"));
    ppsLayout->addRow(cbOpt16);

    ppsLayout->addRow(tr("MQTT broker"), comboOpt5);
    ppsLayout->addRow(tr("Status topic"), leOpt3);
    ppsLayout->addRow(tr("Availability topic"), leOpt4);
    sbOpt2->setRange(1, 300);
    sbOpt2->setSuffix(tr(" sec"));
    ppsLayout->addRow(tr("Stale timeout"), sbOpt2);

    pageLayout->addWidget(gnss4GroupBox);
    pageLayout->addWidget(ppsGroupBox);
    pageLayout->addWidget(new QLabel("<span style='font-size:normal;color:red;'>Restart the program after changing serial ports!</span>"));

    connect(addProfileButton, &QPushButton::clicked, this, &GnssOptions::addProfile);
    connect(removeProfileButton, &QPushButton::clicked, this, &GnssOptions::removeProfile);
    connect(profileList, &QListWidget::currentRowChanged, this, &GnssOptions::profileSelectionChanged);
    connect(receiverName, &QLineEdit::textChanged, this, [this] {
        if (currentProfileIndex >= 0) updateProfileListItem(currentProfileIndex);
    });

    profiles = config->getGnssReceiverProfiles();
    for (const GnssReceiverProfile &profile : profiles)
        profileList->addItem(profile.name);
    for (int index = 0; index < profiles.size(); ++index)
        updateProfileListItem(index);
    if (!profiles.isEmpty()) profileList->setCurrentRow(0);

    cbOpt13->setChecked(config->getGnssDisplayWidget());
    cbOpt14->setChecked(config->getGnssShowNotifications());
    sbOpt1->setValue(config->getGnssTimeFilter());
    cbOpt15->setChecked(config->getGnssPpsDisplayEnabled());
    cbOpt16->setChecked(config->getGnssPpsPlotEnabled());
    refreshPpsProfiles();
    leOpt3->setText(config->getGnssPpsStatusTopic());
    leOpt4->setText(config->getGnssPpsAvailabilityTopic());
    sbOpt2->setValue(config->getGnssPpsStaleTimeoutSec());
    connect(config.data(), &Config::settingsUpdated, this, &GnssOptions::refreshPpsProfiles);
}

void GnssOptions::refreshPpsProfiles()
{
    const QString selectedId = comboOpt5->currentData().toString().isEmpty()
                                   ? config->getGnssPpsBrokerProfileId()
                                   : comboOpt5->currentData().toString();
    comboOpt5->clear();
    for (const MqttBrokerProfile &profile : config->getMqttBrokerProfiles()) {
        QString label = profile.name;
        if (!profile.enabled) label += tr(" (disabled)");
        comboOpt5->addItem(label, profile.id);
    }
    comboOpt5->setCurrentIndex(comboOpt5->findData(selectedId));
}

QStringList GnssOptions::getAvailablePorts()
{
    QStringList list;
    for (auto &val : QSerialPortInfo::availablePorts())
        list.append(val.portName());
    return list;
}

void GnssOptions::start()
{
    //dialog->exec();
}

void GnssOptions::saveCurrentSettings()
{
    storeCurrentProfile();

    const QStringList existingIds = config->getGnssReceiverProfileIds();
    QStringList retainedIds;
    for (const GnssReceiverProfile &profile : profiles) retainedIds.append(profile.id);
    for (const QString &id : existingIds) {
        if (!retainedIds.contains(id)) config->removeGnssReceiverProfile(id);
    }
    for (const GnssReceiverProfile &profile : profiles)
        config->setGnssReceiverProfile(profile);

    config->setGnssDisplayWidget(cbOpt13->isChecked());
    config->setGnssShowNotifications(cbOpt14->isChecked());
    config->setGnssTimeFilter(sbOpt1->value());
    config->setGnssPpsDisplayEnabled(cbOpt15->isChecked());
    config->setGnssPpsPlotEnabled(cbOpt16->isChecked());
    config->setGnssPpsBrokerProfileId(comboOpt5->currentData().toString());
    config->setGnssPpsStatusTopic(leOpt3->text());
    config->setGnssPpsAvailabilityTopic(leOpt4->text());
    config->setGnssPpsStaleTimeoutSec(sbOpt2->value());
}

void GnssOptions::addProfile()
{
    storeCurrentProfile();

    GnssReceiverProfile profile;
    profile.id = QUuid::createUuid().toString(QUuid::WithoutBraces);
    profile.name = tr("New GNSS receiver");
    profile.type = GnssReceiverProfile::Type::Serial;
    profiles.append(profile);
    profileList->addItem(profile.name);
    profileList->setCurrentRow(profiles.size() - 1);
    receiverName->setFocus();
    receiverName->selectAll();
}

void GnssOptions::removeProfile()
{
    if (currentProfileIndex < 0 || currentProfileIndex >= profiles.size()) return;
    if (profiles.at(currentProfileIndex).nonRemovable) return;

    const QString name = profiles.at(currentProfileIndex).name;
    if (QMessageBox::question(this,
                              tr("Remove GNSS receiver"),
                              tr("Remove GNSS receiver '%1'?").arg(name)) != QMessageBox::Yes)
        return;

    const int removedIndex = currentProfileIndex;
    currentProfileIndex = -1;

    {
        // Removing the current row can make Qt reselect a nearby row synchronously,
        // re-entering profileSelectionChanged() while profiles/profileList are
        // briefly out of sync. Block those signals and drive the update ourselves.
        const QSignalBlocker blocker(profileList);
        profiles.removeAt(removedIndex);
        delete profileList->takeItem(removedIndex);
        if (!profiles.isEmpty())
            profileList->setCurrentRow(qMin(removedIndex, profiles.size() - 1));
    }

    currentProfileIndex = profiles.isEmpty() ? -1 : profileList->currentRow();
    loadProfile(currentProfileIndex);
}

void GnssOptions::profileSelectionChanged(int row)
{
    if (row == currentProfileIndex) return;
    storeCurrentProfile();
    currentProfileIndex = row;
    loadProfile(row);
}

void GnssOptions::loadProfile(int index)
{
    const bool valid = index >= 0 && index < profiles.size();
    receiverEnabled->setEnabled(valid);
    receiverName->setEnabled(valid);
    removeProfileButton->setEnabled(valid && !profiles.at(index).nonRemovable);

    if (!valid) {
        receiverEnabled->setChecked(false);
        receiverName->clear();
        receiverPort->setCurrentIndex(-1);
        receiverBaudrate->setCurrentText("4800");
        receiverLogToFile->setChecked(false);
        receiverUblox->setChecked(false);
        receiverShowSpectrumWindow->setChecked(false);
        receiverTriggerRecording->setChecked(false);
        receiverGroupBox->setEnabled(false);
        return;
    }

    receiverGroupBox->setEnabled(true);
    const GnssReceiverProfile &profile = profiles.at(index);
    const bool isSerial = profile.type == GnssReceiverProfile::Type::Serial;

    receiverGroupBox->setTitle(isSerial ? tr("Serial/network GNSS receiver") : tr("Instrument GNSS"));
    receiverEnabled->setText(isSerial ? tr("Use this GNSS device")
                                       : tr("Use instrument GNSS to validate position/time offset"));
    receiverEnabled->setChecked(profile.enabled);
    receiverName->setText(profile.name);
    receiverTriggerRecording->setChecked(profile.triggerRecording);

    portLabel->setVisible(isSerial);
    receiverPort->setVisible(isSerial);
    baudrateLabel->setVisible(isSerial);
    receiverBaudrate->setVisible(isSerial);
    receiverLogToFile->setVisible(isSerial);
    receiverUblox->setVisible(isSerial);
    receiverShowSpectrumWindow->setVisible(isSerial);

    if (isSerial) {
        if (!profile.portName.isEmpty() && receiverPort->findText(profile.portName) < 0)
            receiverPort->addItem(profile.portName);
        receiverPort->setCurrentText(profile.portName);
        receiverBaudrate->setCurrentText(profile.baudrate);
        receiverLogToFile->setChecked(profile.logToFile);
        receiverUblox->setChecked(profile.ublox);
        receiverShowSpectrumWindow->setChecked(profile.showSpectrumWindow);
    }
}

void GnssOptions::storeCurrentProfile()
{
    if (currentProfileIndex < 0 || currentProfileIndex >= profiles.size()) return;

    GnssReceiverProfile &profile = profiles[currentProfileIndex];
    profile.name = receiverName->text().trimmed();
    if (profile.name.isEmpty()) profile.name = profile.id;
    profile.enabled = receiverEnabled->isChecked();
    profile.triggerRecording = receiverTriggerRecording->isChecked();

    if (profile.type == GnssReceiverProfile::Type::Serial) {
        profile.portName = receiverPort->currentText().trimmed();
        profile.baudrate = receiverBaudrate->currentText().trimmed();
        profile.logToFile = receiverLogToFile->isChecked();
        profile.ublox = receiverUblox->isChecked();
        profile.showSpectrumWindow = receiverShowSpectrumWindow->isChecked();
    }

    updateProfileListItem(currentProfileIndex);
}

void GnssOptions::updateProfileListItem(int index)
{
    if (index < 0 || index >= profiles.size() || index >= profileList->count()) return;
    QString name = index == currentProfileIndex ? receiverName->text().trimmed() : profiles.at(index).name;
    if (name.isEmpty()) name = profiles.at(index).id;
    if (profiles.at(index).type == GnssReceiverProfile::Type::Instrument) name += tr(" (instrument)");
    profileList->item(index)->setText(name);
}
