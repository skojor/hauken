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
#include <QGuiApplication>
#include <QScreen>
#include <QScrollArea>

SettingsDialog::SettingsDialog(QWidget *parent, QSharedPointer<Config> c)
    : QDialog{parent}
{
    config = c;
    setWindowTitle("Options");
    QScreen *targetScreen = parent ? parent->screen() : screen();
    if (!targetScreen)
        targetScreen = QGuiApplication::primaryScreen();

    const QSize screenMargin(32, 32);
    const QSize availableSize = targetScreen
        ? targetScreen->availableGeometry().size()
        : QSize(600, 600);
    const QSize maximumDialogSize = (availableSize - screenMargin).expandedTo(QSize(1, 1));
    setMaximumSize(maximumDialogSize);
    resize(QSize(600, 600).boundedTo(maximumDialogSize));

    auto mainLayout = new QGridLayout(this);
    navList = new QListWidget(this);
    navList->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Expanding);
    navList->setMinimumWidth(120);
    navList->setMaximumWidth(180);
    navList->setSelectionMode(QAbstractItemView::SingleSelection);

    pages = new QStackedWidget(this);

    mainLayout->addWidget(navList, 0, 0);
    mainLayout->addWidget(pages, 0, 1);
    mainLayout->setColumnStretch(1, 1);
    mainLayout->setRowStretch(0, 1);

    auto addPage = [&](OptionsBaseClass *page, const QString &title, const QIcon &icon = {}) {
        auto scrollArea = new QScrollArea(pages);
        scrollArea->setWidget(page);
        scrollArea->setWidgetResizable(true);
        scrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
        scrollArea->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);

        optionPages.append(page);
        pages->addWidget(scrollArea);
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
    mainLayout->addWidget(buttons, 1, 0, 1, 2);

    connect(buttons, &QDialogButtonBox::accepted, this, &SettingsDialog::save);
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);
}

void SettingsDialog::save()
{
    for (auto *page : optionPages)
        page->saveCurrentSettings();

    config->settingsUpdated();
    accept();
}
