#include "mqttoptions.h"

MqttOptions::MqttOptions(QSharedPointer<Config> c)
{
    config = c;

    setWindowTitle("MQTT/webswitch configuration");
    connect(btnBox, &QDialogButtonBox::accepted, this, &MqttOptions::saveCurrentSettings);
    connect(btnBox, &QDialogButtonBox::rejected, dialog, &QDialog::close);
    setupWindow();
}

void MqttOptions::start()
{
    cbOpt1->setChecked(config->getMqttActivate());
    leOpt1->setText(config->getMqttServer());
    leOpt13->setText(config->getMqttUsername());
    leOpt14->setText(config->getMqttPassword());
    sbOpt1->setValue(config->getMqttPort());
    leOpt12->setText(config->getMqttKeepaliveTopic());
    leOpt15->setText(config->getMqttWebswitchAddress());

    dialog->exec();
}

void MqttOptions::saveCurrentSettings()
{
    config->setMqttActivate(cbOpt1->isChecked());
    config->setMqttServer(leOpt1->text());

    QStringList names, topics;
    for (auto &val : subNames) if (!val->text().isEmpty()) names.append(val->text());
    for (auto &val : subTopics) if (!val->text().isEmpty()) topics.append(val->text());
    config->setMqttSubNames(names);
    config->setMqttSubTopics(topics);

    config->setMqttKeepaliveTopic(leOpt12->text());
    config->setMqttUsername(leOpt13->text());
    config->setMqttPassword(leOpt14->text());
    config->setMqttPort(sbOpt1->value());
    config->setMqttWebswitchAddress(leOpt15->text());

    dialog->close();
}

void MqttOptions::updSubs()
{
    QStringList names = config->getMqttSubNames();
    QStringList topics = config->getMqttSubTopics();

    if (subGroupBoxes.isEmpty()) {
        //qDebug() << names;
        for (int i=0; i < names.size() + 1; i++) {
            addSub();
        }
    }
    for (int i=0; i<subNames.size(); i++) {
        if (names.size() >= i+1) subNames[i]->setText(config->getMqttSubNames()[i]);
        if (topics.size() >= i+1) subTopics[i]->setText(config->getMqttSubTopics()[i]);
    }
}

void MqttOptions::addSub()
{
    if (!subNames.isEmpty()) disconnect(subNames.last(), &QLineEdit::textChanged, this, &MqttOptions::addSub);

    subGroupBoxes.append(new QGroupBox("Sensor " + QString::number(subGroupBoxes.size()+1)));
    subLayouts.append(new QFormLayout);
    subGroupBoxes.last()->setLayout(subLayouts.last());
    subNames.append(new QLineEdit);
    subTopics.append(new QLineEdit);
    subLayouts.last()->addRow(new QLabel(tr("Name")), subNames.last());
    subNames.last()->setToolTip(tr("Sensor name to be published in the HTTP report"));
    subLayouts.last()->addRow(new QLabel(tr("Subscription topic")), subTopics.last());
    subTopics.last()->setToolTip(tr("Subscription topic to subscribe to (blank to disable)"));
    mainLayout->insertRow(mainLayout->rowCount()-1, subGroupBoxes.last());
    connect(subNames.last(), &QLineEdit::textChanged, this, &MqttOptions::addSub);
}

void MqttOptions::setupWindow()
{
    QGroupBox *subServerGroupBox = new QGroupBox("MQTT server options");
    QFormLayout *subServerLayout = new QFormLayout;
    subServerGroupBox->setLayout(subServerLayout);

    subServerLayout->addRow(cbOpt1);
    cbOpt1->setText("Enable MQTT sensor data");
    cbOpt1->setToolTip("Enabling this option will subscribe for sensor data from an MQTT server. " \
                       "If set the sensor name will be included in the HTTP report together with" \
                       "the value reported from the MQTT server.");

    subServerLayout->addRow(new QLabel(tr("MQTT server IP/address")), leOpt1);
    leOpt1->setToolTip(tr("MQTT server to query for data"));
    subServerLayout->addRow(new QLabel(tr("Server username")), leOpt13);
    leOpt13->setToolTip(tr("Username for login to server (can be blank)"));
    subServerLayout->addRow(new QLabel(tr("Server password")), leOpt14);
    leOpt14->setToolTip(tr("Password for login to server (can be blank)"));
    leOpt14->setEchoMode(QLineEdit::Password);
    subServerLayout->addRow(new QLabel(tr("Server port")), sbOpt1);
    sbOpt1->setToolTip(tr("Port number used to connect to server (default 1883)"));
    sbOpt1->setRange(1,65536);
    QGroupBox *keepAliveGroupBox = new QGroupBox("Keepalive topic");
    QFormLayout *keepAliveLayout = new QFormLayout;
    keepAliveGroupBox->setLayout(keepAliveLayout);
    keepAliveLayout->addRow(new QLabel(tr("Topic")), leOpt12);
    leOpt12->setToolTip(tr("If set this topic will be sent to the MQTT server periodically "\
                           "to keep the connection alive (needed for some MQTT servers). " \
                           "Only topic will be sent, no data"));

    QGroupBox *webswitchGroupBox = new QGroupBox("HTTP webswitch options");
    QFormLayout *webswitchLayout = new QFormLayout;
    webswitchGroupBox->setLayout(webswitchLayout);
    webswitchLayout->addRow(new QLabel(tr("Temperature HTTP(s) address")), leOpt15);
    leOpt15->setToolTip(tr("If a valid address is provided, and a value is returned, " \
                           "the temperature will be read in 60 second intervals and reported via position report."));

    updSubs();

    dialog->setGeometry(100, 100, 450, 800);
    QScrollArea *scrollArea = new QScrollArea(dialog);
    scrollArea->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    scrollArea->setWidgetResizable(true);
    scrollArea->setGeometry(0, 0, 450, 800);
    QWidget *widget = new QWidget();
    widget->setLayout(mainLayout);

    scrollArea->setWidget(widget);
    mainLayout->addWidget(webswitchGroupBox);
    mainLayout->addWidget(subServerGroupBox);
    mainLayout->addWidget(keepAliveGroupBox);
    for (auto &val : subGroupBoxes) mainLayout->addWidget(val);
    mainLayout->addWidget(btnBox);
}
