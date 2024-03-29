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

    layout1->addRow(new QLabel("SMTP server port"), leOpt2);
    leOpt2->setToolTip("Port used to connect to SMTP server");

    layout1->addRow(new QLabel("SMTP username (if needed)"), leOpt5);
    leOpt5->setToolTip("If the SMTP server requires you to login, insert username here");

    layout1->addRow(new QLabel("SMTP password (if needed)"), leOpt6);
    leOpt6->setToolTip("If required by the SMTP server, set the password here");
    leOpt6->setEchoMode(QLineEdit::Password);

    layout1->addRow(new QLabel("AI jammer probability filter"), sbOpt4);
    sbOpt4->setRange(0, 100);
    sbOpt4->setToolTip("Send email to recipients below only if signal is classified as a jammer, and the probability is higher than this value");

    layout1->addRow(new QLabel("AI filtered email recipients"), leOpt10);
    leOpt10->setToolTip("Email recipient(s), separate multiple recipients with ;\n"
                        "Recipients in this list will only receive a notification if signal is classified as a jammer, and probability is higher\n"
                        "than the above set value");

    layout1->addRow(new QLabel("General email recipients"), leOpt3);
    leOpt3->setToolTip("Email recipient(s), separate multiple recipients with ;\n"
                       "All notifications will be sent to the recipients added here");

    layout1->addRow(new QLabel("Own email address"), leOpt4);
    leOpt4->setToolTip("Set the address the email will show as sent from. Should be a valid address");

    layout1->addRow(new QLabel("Minimum time between emails (seconds)"), sbOpt1);
    sbOpt1->setToolTip("Used to reduce number of emails, all incidents happening within this time will be sent at once\nA value of 0 means emails will be sent without delay");
    sbOpt1->setRange(0, 86400);

    layout1->addRow(new QLabel("Microsoft Graph application id"), leOpt7);
    leOpt7->setToolTip("Application id used to identify with Microsoft Graph REST API. If set MS Graph API will be used instead of SMTP");

    layout1->addRow(new QLabel("Microsoft Graph tenant id"), leOpt8);
    leOpt8->setToolTip("Identification used to send mail with Microsoft Graph REST API");

    layout1->addRow(new QLabel("Microsoft Graph secret"), leOpt9);
    leOpt9->setToolTip("Secret used to identify with Microsoft Graph REST API");
    leOpt9->setEchoMode(QLineEdit::Password);

    QGroupBox *groupBox2 = new QGroupBox("Email notifications configuration");
    QFormLayout *layout2 = new QFormLayout;
    groupBox2->setLayout(layout2);

    layout2->addRow(cbOpt1);
    cbOpt1->setText("Send email when measurement device triggers on high level");
    cbOpt1->setToolTip("Will notify when the measurement device causes a trigger incident by high signal levels");

    layout2->addRow(cbOpt2);
    cbOpt2->setText("Send email when measurement device disconnects");
    cbOpt2->setToolTip("Will notify when the measurement device disconnects, and auto reconnect is disabled,\nor if auto reconnect is not able to reconnect in a few minutes");

    layout2->addRow(cbOpt3);
    cbOpt3->setText("Send email when GNSS device triggers incidents");
    cbOpt3->setToolTip("Will notify on all GNSS incidents triggered. Warning! Can generate a lot of emails!");

    layout2->addRow(cbOpt4);
    cbOpt4->setText("Add image of traceplot and waterfall to email");
    cbOpt4->setToolTip("Includes images of the trace and waterfall in the notification email");

    layout2->addRow(new QLabel("Delay before taking image snapshots"), sbOpt3);
    sbOpt3->setToolTip("Wait this many seconds before making a snapshot of the trace and the waterfall. Useful to see more waterfall before mail is sent.\nThis value cannot be higher than the double of incident truncation time (because then the email is sent, and it's a little late to include attachments...");
    sbOpt3->setRange(0, 86400);

    QGroupBox *groupBox3 = new QGroupBox("General notifications configuration");
    QFormLayout *layout3 = new QFormLayout;
    groupBox3->setLayout(layout3);

    layout3->addRow(new QLabel("Incident truncation time (seconds)"), sbOpt2);
    sbOpt2->setToolTip("This function awaits eventual further incidents triggered from the same device, and considers an incident ended only after the time set here.\nIf for instance a GNSS is triggered on/off several times, only one incident will be reported during the time set here.");
    sbOpt2->setRange(0, 86400);

    layout3->addRow(cbOpt5);
    cbOpt5->setText("Play notification sound on incidents");
    cbOpt5->setToolTip("If checked a short message will play through default speakers when incident is triggered");

    mainLayout->addWidget(groupBox1);
    mainLayout->addWidget(groupBox2);
    mainLayout->addWidget(groupBox3);

    connect(btnBox, &QDialogButtonBox::accepted, this, &EmailOptions::saveCurrentSettings);
    connect(btnBox, &QDialogButtonBox::rejected, dialog, &QDialog::close);

    mainLayout->addWidget(btnBox);
}

void EmailOptions::start()
{
    leOpt1->setText(config->getEmailSmtpServer());
    leOpt2->setText(config->getEmailSmtpPort());
    leOpt5->setText(config->getEmailSmtpUser());
    leOpt6->setText(config->getEmailSmtpPassword());
    leOpt3->setText(config->getEmailRecipients());
    leOpt4->setText(config->getEmailFromAddress());
    leOpt8->setText(config->getEmailGraphTenantId());
    leOpt9->setText(config->getEmailGraphSecret());
    leOpt7->setText(config->getEmailGraphApplicationId());
    leOpt10->setText(config->getEmailFilteredRecipients());

    sbOpt1->setValue(config->getEmailMinTimeBetweenEmails());
    cbOpt1->setChecked(config->getEmailNotifyMeasurementDeviceHighLevel());
    cbOpt2->setChecked(config->getEmailNotifyMeasurementDeviceDisconnected());
    cbOpt3->setChecked(config->getEmailNotifyGnssIncidents());
    cbOpt4->setChecked(config->getEmailAddImages());
    cbOpt5->setChecked(config->getSoundNotification());
    sbOpt3->setValue(config->getEmailDelayBeforeAddingImages());
    sbOpt2->setValue(config->getNotifyTruncateTime());
    sbOpt4->setValue(config->getEmailJammerProbabilityFilter());
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
    config->setEmailAddImages(cbOpt4->isChecked());
    config->setSoundNotification(cbOpt5->isChecked());
    config->setEmailDelayBeforeAddingImages(sbOpt3->value());
    config->setNotifyTruncateTime(sbOpt2->value());
    config->setEmailGraphTenantId(leOpt8->text());
    config->setEmailGraphSecret(leOpt9->text());
    config->setEmailGraphApplicationId(leOpt7->text());
    config->setEmailFilteredRecipients(leOpt10->text());
    config->setEmailJammerProbabilityFilter(sbOpt4->value());

    dialog->close();
}
