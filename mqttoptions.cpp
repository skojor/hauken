#include "mqttoptions.h"

MqttOptions::MqttOptions(QSharedPointer<Config> c)
{
    config = c;
    setWindowTitle("MQTT configuration");
    connect(btnBox, &QDialogButtonBox::accepted, this, &MqttOptions::saveCurrentSettings);
    connect(btnBox, &QDialogButtonBox::rejected, dialog, &QDialog::close);

    QGroupBox *subServerGroupBox = new QGroupBox("MQTT options");
    QFormLayout *subServerLayout = new QFormLayout;
    subServerGroupBox->setLayout(subServerLayout);

    subServerLayout->addRow(cbOpt1);
    cbOpt1->setText("Enable MQTT sensor data");
    cbOpt1->setToolTip("Enabling this option will subscribe for sensor data from an MQTT server. " \
                       "If set the sensor name will be included in the HTTP report together with" \
                       "the value reported from the MQTT server.");

    subServerLayout->addRow(new QLabel(tr("MQTT server IP/address")), leOpt1);
    leOpt1->setToolTip(tr("MQTT server to query for data"));

    QGroupBox *sub1GroupBox = new QGroupBox("Sensor 1");
    QFormLayout *sub1Layout = new QFormLayout;
    sub1GroupBox->setLayout(sub1Layout);
    sub1Layout->addRow(new QLabel(tr("Name")), leOpt2);
    leOpt2->setToolTip(tr("Sensor name to be published in the HTTP report"));
    sub1Layout->addRow(new QLabel(tr("Subscription topic")), leOpt3);
    leOpt3->setToolTip(tr("Subscription topic to subscribe to (blank to disable)"));

    QGroupBox *sub2GroupBox = new QGroupBox("Sensor 2");
    QFormLayout *sub2Layout = new QFormLayout;
    sub2GroupBox->setLayout(sub2Layout);
    sub2Layout->addRow(new QLabel(tr("Name")), leOpt4);
    leOpt4->setToolTip(tr("Sensor name to be published in the HTTP report"));
    sub2Layout->addRow(new QLabel(tr("Subscription topic")), leOpt5);
    leOpt5->setToolTip(tr("Subscription topic to subscribe to (blank to disable)"));

    QGroupBox *sub3GroupBox = new QGroupBox("Sensor 3");
    QFormLayout *sub3Layout = new QFormLayout;
    sub3GroupBox->setLayout(sub3Layout);
    sub3Layout->addRow(new QLabel(tr("Name")), leOpt6);
    leOpt6->setToolTip(tr("Sensor name to be published in the HTTP report"));
    sub3Layout->addRow(new QLabel(tr("Subscription topic")), leOpt7);
    leOpt7->setToolTip(tr("Subscription topic to subscribe to (blank to disable)"));

    QGroupBox *sub4GroupBox = new QGroupBox("Sensor 4");
    QFormLayout *sub4Layout = new QFormLayout;
    sub4GroupBox->setLayout(sub4Layout);
    sub4Layout->addRow(new QLabel(tr("Name")), leOpt8);
    leOpt8->setToolTip(tr("Sensor name to be published in the HTTP report"));
    sub4Layout->addRow(new QLabel(tr("Subscription topic")), leOpt9);
    leOpt9->setToolTip(tr("Subscription topic to subscribe to (blank to disable)"));

    QGroupBox *sub5GroupBox = new QGroupBox("Sensor 5");
    QFormLayout *sub5Layout = new QFormLayout;
    sub5GroupBox->setLayout(sub5Layout);
    sub5Layout->addRow(new QLabel(tr("Name")), leOpt10);
    leOpt10->setToolTip(tr("Sensor name to be published in the HTTP report"));
    sub5Layout->addRow(new QLabel(tr("Subscription topic")), leOpt11);
    leOpt11->setToolTip(tr("Subscription topic to subscribe to (blank to disable)"));

    QGroupBox *keepAliveGroupBox = new QGroupBox("Keepalive topic");
    QFormLayout *keepAliveLayout = new QFormLayout;
    keepAliveGroupBox->setLayout(keepAliveLayout);
    keepAliveLayout->addRow(new QLabel(tr("Topic")), leOpt12);
    leOpt12->setToolTip(tr("If set this topic will be sent to the MQTT server periodically "\
                           "to keep the connection alive (needed for some MQTT servers). " \
                           "Only topic will be sent, no data"));

    mainLayout->addWidget(subServerGroupBox);
    mainLayout->addWidget(sub1GroupBox);
    mainLayout->addWidget(sub2GroupBox);
    mainLayout->addWidget(sub3GroupBox);
    mainLayout->addWidget(sub4GroupBox);
    mainLayout->addWidget(sub5GroupBox);
    mainLayout->addWidget(keepAliveGroupBox);

    mainLayout->addWidget(btnBox);
}

void MqttOptions::start()
{
    cbOpt1->setChecked(config->getMqttActivate());
    leOpt1->setText(config->getMqttServer());
    leOpt2->setText(config->getMqttSub1Name());
    leOpt3->setText(config->getMqttSub1Topic());
    leOpt4->setText(config->getMqttSub2Name());
    leOpt5->setText(config->getMqttSub2Topic());
    leOpt6->setText(config->getMqttSub3Name());
    leOpt7->setText(config->getMqttSub3Topic());
    leOpt8->setText(config->getMqttSub4Name());
    leOpt9->setText(config->getMqttSub4Topic());
    leOpt10->setText(config->getMqttSub5Name());
    leOpt11->setText(config->getMqttSub5Topic());
    leOpt12->setText(config->getMqttKeepaliveTopic());

    dialog->exec();
}

void MqttOptions::saveCurrentSettings()
{
    config->setMqttActivate(cbOpt1->isChecked());
    config->setMqttServer(leOpt1->text());
    config->setMqttSub1Name(leOpt2->text());
    config->setMqttSub1Topic(leOpt3->text());
    config->setMqttSub2Name(leOpt4->text());
    config->setMqttSub2Topic(leOpt5->text());
    config->setMqttSub3Name(leOpt6->text());
    config->setMqttSub3Topic(leOpt7->text());
    config->setMqttSub4Name(leOpt8->text());
    config->setMqttSub4Topic(leOpt9->text());
    config->setMqttSub5Name(leOpt10->text());
    config->setMqttSub5Topic(leOpt11->text());
    config->setMqttKeepaliveTopic(leOpt12->text());

    dialog->close();
}
