#include "emailoptions.h"

EmailOptions::EmailOptions(QSharedPointer<Config> c)
{
    config = c;
    setWindowTitle("Email notifications configuration");

    QGroupBox *groupBox1 = new QGroupBox("SMTP server setup");
    QFormLayout *layout1 = new QFormLayout;
    groupBox1->setLayout(layout1);

    layout1->addRow(new QLabel("SMTP server name"), leOpt1);
    leOpt1->setToolTip("Which SMTP server to send mails through");
    leOpt1->setText(config->getEmailSmtpServer());

    layout1->addRow(new QLabel("SMTP server port"), leOpt2);
    leOpt2->setToolTip("Port used to connect to SMTP server");
    leOpt2->setText(config->getEmailSmtpPort());

    layout1->addRow(new QLabel("Email recipients"), leOpt3);
    leOpt3->setToolTip("Email recipient(s), separate multiple recipients with ;");
    leOpt3->setText(config->getEmailRecipients());

    mainLayout->addWidget(groupBox1);

    QGroupBox *groupBox2 = new QGroupBox("Notifications configuration");
    QFormLayout *layout2 = new QFormLayout;
    groupBox2->setLayout(layout2);

    layout2->addRow(cbOpt1);
    cbOpt1->setText("Notify on measurement device triggers");
    cbOpt1->setToolTip("Will notify when the measurement device causes a trigger incident by high signal levels");
    cbOpt1->setChecked(config->getEmailNotifyMeasurementDeviceHighLevel());

    layout2->addRow(cbOpt2);
    cbOpt2->setText("Notify on measurement device disconnects");
    cbOpt2->setToolTip("Will notify when the measurement device disconnects, and auto reconnect is disabled.\nOr if auto reconnect is not able to reconnect in a few minutes");
    cbOpt2->setChecked(config->getEmailNotifyMeasurementDeviceDisconnected());

    layout2->addRow(cbOpt3);
    cbOpt3->setText("Notify on GNSS device incidents");
    cbOpt3->setToolTip("Will notify on all GNSS incidents triggered");
    cbOpt3->setChecked(config->getEmailNotifyGnssIncidents());

    mainLayout->addWidget(groupBox2);

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

    dialog->close();
}
