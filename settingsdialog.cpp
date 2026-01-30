#include "settingsdialog.h"
#include "generaloptions.h"
#include "gnssoptions.h"
#include "receiveroptions.h"
#include "sdefoptions.h"
#include "positionreportoptions.h"
#include "emailoptions.h"
#include "arduinooptions.h"
#include "mqttoptions.h"
#include "iqoptions.h"

SettingsDialog::SettingsDialog(QWidget *parent, QSharedPointer<Config> c)
    : QDialog{parent}
{
    config = c;
    setWindowTitle("Options");
    resize(600, 600);
    auto mainLayout = new QGridLayout(this);
    navList = new QListWidget(this);
    navList->setFixedWidth(180);
    navList->setSelectionMode(QAbstractItemView::SingleSelection);

    pages = new QStackedWidget(this);

    mainLayout->addWidget(navList, 0, 0);
    mainLayout->addWidget(pages, 0, 1);

    auto addPage = [&](QWidget *w, const QString &title, const QIcon &icon = {}) {
        pages->addWidget(w);
        navList->addItem(new QListWidgetItem(icon, title));
    };

    addPage(new GeneralOptions(c), "General");
    addPage(new GnssOptions(c), "GNSS");
    addPage(new ReceiverOptions(c), "Receiver");
    addPage(new SdefOptions(c), "Data recording and uploads");
    addPage(new PositionReportOptions(c), "Instrument list and status reports");
    addPage(new EmailOptions(c), "Notifications");
    addPage(new ArduinoOptions(c), "Arduino");
    addPage(new MqttOptions(c), "MQTT and webswitch");
    addPage(new IqOptions(c), "I/Q data and plot");

    connect(navList, &QListWidget::currentRowChanged, pages, &QStackedWidget::setCurrentIndex);

    navList->setCurrentRow(0);

    auto buttons = new QDialogButtonBox(
        QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    mainLayout->addWidget(buttons, 2, 0);

    connect(buttons, &QDialogButtonBox::accepted, this, &SettingsDialog::save);
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);
}

void SettingsDialog::save()
{
    for (int i = 0; i < pages->count(); ++i) {
        auto page = qobject_cast<OptionsBaseClass*>(pages->widget(i));
        if (page)
            page->saveCurrentSettings();
    }

    config->settingsUpdated();
    accept();
}
