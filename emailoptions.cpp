#include "emailoptions.h"

EmailOptions::EmailOptions(QSharedPointer<Config> c)
{
    config = c;
    setWindowTitle("Notifications configuration");

    QGroupBox *groupBox1 = new QGroupBox("SMTP server setup");
    QFormLayout *layout1 = new QFormLayout;
    groupBox1->setLayout(layout1);

    layout1->addRow(new QLabel("SMTP server name"), leOpt1);
    leOpt1->setToolTip("Which SMTP server to send mails through");
    leOpt1->setText(config->getEmailSmtpServer());

    layout1->addRow(new QLabel("SMTP server port"), leOpt2);
    leOpt2->setToolTip("Port used to connect to SMTP server");
    leOpt2->setText(config->getEmailSmtpPort());

    layout1->addRow(new QLabel("SMTP username (if needed)"), leOpt5);
    leOpt5->setToolTip("If the SMTP server requires you to login, insert username here");
    leOpt5->setText(config->getEmailSmtpUser());

    layout1->addRow(new QLabel("SMTP password (if needed)"), leOpt6);
    leOpt6->setToolTip("If required by the SMTP server, set the password here");
    leOpt6->setText(config->getEmailSmtpPassword());
    leOpt6->setEchoMode(QLineEdit::Password);

    layout1->addRow(new QLabel("Email recipients"), leOpt3);
    leOpt3->setToolTip("Email recipient(s), separate multiple recipients with ;");
    leOpt3->setText(config->getEmailRecipients());

    layout1->addRow(new QLabel("Own email address"), leOpt4);
    leOpt4->setToolTip("Set the address the email will show as sent from. Should be a valid address");
    leOpt4->setText(config->getEmailFromAddress());

    layout1->addRow(new QLabel("Minimum time between emails (seconds)"), sbOpt1);
    sbOpt1->setToolTip("Used to reduce number of emails, all incidents happening within this time will be sent at once\nA value of 0 means emails will be sent without delay");
    sbOpt1->setRange(0, 86400);
    sbOpt1->setValue(config->getEmailMinTimeBetweenEmails());


    QGroupBox *groupBox2 = new QGroupBox("Email notifications configuration");
    QFormLayout *layout2 = new QFormLayout;
    groupBox2->setLayout(layout2);

    layout2->addRow(cbOpt1);
    cbOpt1->setText("Send email when measurement device triggers on high level");
    cbOpt1->setToolTip("Will notify when the measurement device causes a trigger incident by high signal levels");
    cbOpt1->setChecked(config->getEmailNotifyMeasurementDeviceHighLevel());

    layout2->addRow(cbOpt2);
    cbOpt2->setText("Send email when measurement device disconnects");
    cbOpt2->setToolTip("Will notify when the measurement device disconnects, and auto reconnect is disabled,\nor if auto reconnect is not able to reconnect in a few minutes");
    cbOpt2->setChecked(config->getEmailNotifyMeasurementDeviceDisconnected());

    layout2->addRow(cbOpt3);
    cbOpt3->setText("Send email when GNSS device triggers incidents");
    cbOpt3->setToolTip("Will notify on all GNSS incidents triggered. Warning! Can generate a lot of emails!");
    cbOpt3->setChecked(config->getEmailNotifyGnssIncidents());

    QGroupBox *groupBox3 = new QGroupBox("General notifications configuration");
    QFormLayout *layout3 = new QFormLayout;
    groupBox3->setLayout(layout3);

    layout3->addRow(new QLabel("Incident truncation time (seconds)"), sbOpt2);
    sbOpt2->setToolTip("This function awaits eventual further incidents triggered from the same device, and considers an incident ended only after the time set here.\nIf for instance a GNSS is triggered on/off several times, only one incident will be reported during the time set here.");
    sbOpt2->setValue(config->getNotifyTruncateTime());
    sbOpt2->setRange(0, 86400);

    mainLayout->addWidget(groupBox1);
    mainLayout->addWidget(groupBox2);
    mainLayout->addWidget(groupBox3);

    connect(btnBox, &QDialogButtonBox::accepted, this, &EmailOptions::saveCurrentSettings);
    connect(btnBox, &QDialogButtonBox::rejected, dialog, &QDialog::close);

    mainLayout->addWidget(btnBox);
}

void EmailOptions::start()
{
    dialog->exec();
}

void EmailOptions::saveCurrentSettings()
{
    config->setEmailSmtpServer(leOpt1->text());
    config->setEmailSmtpPort(leOpt2->text());
    config->setEmailSmtpUser(leOpt5->text());
    config->setEmailSmtpPassword(leOpt6->text());
    config->setEmailRecipients(leOpt3->text());
    config->setEmailFromAddress(leOpt4->text());
    config->setEmailMinTimeBetweenEmails(sbOpt1->value());
    config->setEmailNotifyMeasurementDeviceHighLevel(cbOpt1->isChecked());
    config->setEmailNotifyMeasurementDeviceDisconnected(cbOpt2->isChecked());
    config->setEmailNotifyGnssIncidents(cbOpt3->isChecked());
    config->setNotifyTruncateTime(sbOpt2->value());

    dialog->close();
}
