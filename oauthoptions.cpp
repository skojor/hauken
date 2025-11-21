#include "oauthoptions.h"

OAuthOptions::OAuthOptions(QSharedPointer<Config> c)
{
    config = c;

    setWindowTitle("OAuth configuration");

    mainLayout->addRow(cbOpt1);
    cbOpt1->setText("Use OAuth2 authentication for file uploads");
    cbOpt1->setToolTip("Enable this option to prefer OAuth2 authentication. Ask your IT provider for these values.\n"\
                       "All fields are mandatory.");

    mainLayout->addRow(new QLabel("OAuth2 authority"), leOpt1);
    leOpt1->setToolTip("Set the URL to the login authority.");

    /*mainLayout->addRow(new QLabel("OAuth2 access token URL"), leOpt2);
    leOpt2->setToolTip("Set the URL to use for receiving the access token URL. Mandatory");*/

    mainLayout->addRow(new QLabel("OAuth2 application ID"), leOpt3);
    leOpt3->setToolTip("Application/client ID to use for authentication.");

    mainLayout->addRow(new QLabel("OAuth2 scope"), leOpt4);
    leOpt4->setToolTip("Set the scope which this authentication should cover.");

    mainLayout->addRow(new QLabel("File upload address"), leOpt5);
    leOpt5->setToolTip("The address to use for uploading SDEF measurement files.");

    /*mainLayout->addRow(new QLabel("Address to set operator"), leOpt6);
    leOpt6->setToolTip("Address used to set owner/operator of the file uploaded. Leave blank to disable.");*/

    mainLayout->addWidget(btnBox);
    mainLayout->setSizeConstraint(QLayout::SetNoConstraint);
    dialog->setMinimumWidth(800);
    connect(btnBox, &QDialogButtonBox::accepted, this, &OAuthOptions::saveCurrentSettings);
    connect(btnBox, &QDialogButtonBox::rejected, dialog, &QDialog::close);
}

void OAuthOptions::start()
{
    cbOpt1->setChecked(config->getOauth2Enable());
    leOpt1->setText(config->getOAuth2AuthUrl());
    //leOpt2->setText(config->getOAuth2AccessTokenUrl());
    leOpt4->setText(config->getOAuth2Scope());
    leOpt5->setText(config->getOauth2UploadAddress());
    leOpt3->setText(config->getOAuth2ClientId());
    //leOpt6->setText(config->getOauth2OperatorAddress());

    dialog->exec();
}

void OAuthOptions::saveCurrentSettings()
{
    config->setOAuth2Enable(cbOpt1->isChecked());
    config->setOAuth2AuthUrl(leOpt1->text().trimmed());
    //config->setOAuth2AccessTokenUrl(leOpt2->text().trimmed());
    config->setOAuth2Scope(leOpt4->text().trimmed());
    config->setOAuth2UploadAddress(leOpt5->text().trimmed());
    config->setOAuth2ClientId(leOpt3->text().trimmed());
    //config->setOAuth2OperatorAddress(leOpt6->text().trimmed());

    dialog->close();
}
