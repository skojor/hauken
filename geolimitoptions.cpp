#include "geolimitoptions.h"

GeoLimitOptions::GeoLimitOptions(QSharedPointer<Config> c)
    : OptionsBaseClass{}
{
    config = c;
    setWindowTitle(tr("Geographic limiting options"));

    mainLayout->addRow(cbOpt1);
    cbOpt1->setText("Enable geographic limiting");
    cbOpt1->setToolTip("Enabling this option will pause the monitoring and stop all recording if the " \
                       "station is moving outside a given polygon. Monitoring will resume when the" \
                       "station goes back inside the polygon");

    if (!config->getGeoLimitFilename().isEmpty()) file->setText(config->getGeoLimitFilename());
    else file->setText("(No file selected)");
    mainLayout->addRow(new QLabel("KML file containing a polygon of the area where usage is allowed"), file);
    mainLayout->addRow(selectFileBtn);
    selectFileBtn->setText("Select KML file...");

    connect(selectFileBtn, &QPushButton::clicked, this, [this] {
        filename = QFileDialog::getOpenFileName(this, "Select file", config->getWorkFolder(), "KML files (*.kml)");
        file->setText(filename);
    });

    connect(btnBox, &QDialogButtonBox::accepted, this, &GeoLimitOptions::saveCurrentSettings);
    connect(btnBox, &QDialogButtonBox::rejected, dialog, &QDialog::close);

    mainLayout->addWidget(btnBox);
}

void GeoLimitOptions::start()
{
    cbOpt1->setChecked(config->getGeoLimitActive());
    file->setText(config->getGeoLimitFilename());

    dialog->exec();
}

void GeoLimitOptions::saveCurrentSettings()
{
    config->setGeoLimitActive(cbOpt1->isChecked());
    config->setGeoLimitFilename(file->text());

    dialog->close();
}
