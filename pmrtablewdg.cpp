#include "pmrtablewdg.h"

PmrTableWdg::PmrTableWdg(QSharedPointer<Config> c)
{
    config = c;

    //wdg->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    //wdg->setWindowFlag(Qt::WindowStaysOnTopHint);
    btnOpenXmlFile->setText("&Open XML file...");
    btnBox = new QDialogButtonBox;
    btnBox->addButton(btnOpenXmlFile, QDialogButtonBox::ResetRole);
    btnBox->addButton(QDialogButtonBox::Save);
    btnBox->addButton(QDialogButtonBox::Cancel);

    connect(btnOpenXmlFile, &QPushButton::clicked, this, &PmrTableWdg::openXmlFile);
    connect(btnBox, &QDialogButtonBox::accepted, this, &PmrTableWdg::saveCurrentSettings);
    connect(btnBox, &QDialogButtonBox::rejected, wdg, &QWidget::close);

    wdg->setLayout(mainLayout);
    mainLayout->addWidget(tableWidget, 0, 0, 1, 4);
    mainLayout->addWidget(selectGreens, 1, 0);
    mainLayout->addWidget(selectYellows, 1, 1);
    mainLayout->addWidget(selectReds, 1, 2);
    mainLayout->addWidget(selectWhites, 1, 3);
    mainLayout->addWidget(btnBox, 2, 0, 1, 4);
    wdg->resize(1000, 600);
    wdg->setAttribute(Qt::WA_QuitOnClose);
}

PmrTableWdg::~PmrTableWdg()
{
    delete tableWidget;
    delete wdg;
    delete btnBox;
}

void PmrTableWdg::start()
{
    loadTable();
    createTableWdg();

    selectGreens->setChecked(config->getPmrUseGreens());
    selectYellows->setChecked(config->getPmrUseYellows());
    selectReds->setChecked(config->getPmrUseReds());
    selectWhites->setChecked(config->getPmrUseWhites());

    connect(selectGreens, &QCheckBox::stateChanged, this, [this](int state) {
        config->setPmrUseGreens(state);
        updTableWdg();
    });
    connect(selectYellows, &QCheckBox::stateChanged, this, [this](int state) {
        config->setPmrUseYellows(state);
        updTableWdg();
    });
    connect(selectReds, &QCheckBox::stateChanged, this, [this](int state) {
        config->setPmrUseReds(state);
        updTableWdg();
    });
    connect(selectWhites, &QCheckBox::stateChanged, this, [this](int state) {
        config->setPmrUseWhites(state);
        updTableWdg();
    });
    wdg->show();
}

void PmrTableWdg::saveCurrentSettings()
{
    saveTable();
    close();
}

void PmrTableWdg::fillPmrChannels()
{
    QFile xmlfile(config->getWorkFolder() + "/vhf.csv");
    if (!xmlfile.open(QIODevice::ReadOnly)) {
        qDebug() << "Cannot open VHF csv file:" << xmlfile.fileName();
    }
    else {
        while (!xmlfile.atEnd()) {
            Pmr pmr;
            QList<QByteArray> list = xmlfile.readLine().split(';');
            unsigned long val = list[2].simplified().replace(" ", "").toULong();
            if (val > 0 && val >= config->getInstrStartFreq() * 1e6 && val <= config->getInstrStopFreq() * 1e6 && \
                    (pmrTable.isEmpty() || (pmrTable.last().centerFrequency != val))) {

                pmr.centerFrequency = val;
                pmr.channelSpacing = list[6].toUInt() * 1e3;
                pmr.comment = list[1];
                if (val < 137e6) pmr.demod = "AM";
                else pmr.demod = "FM";
                pmrTable.append(pmr);
            }
        }
    }
    xmlfile.close();

    xmlfile.setFileName(config->getWorkFolder() + "/uhf.csv");
    if (!xmlfile.open(QIODevice::ReadOnly)) {
        qDebug() << "Cannot open UHF csv file:" << xmlfile.fileName();
    }
    else {
        while (!xmlfile.atEnd()) {
            Pmr pmr;
            QList<QByteArray> list = xmlfile.readLine().split(';');
            unsigned long val = list[2].simplified().replace(" ", "").toULong();
            if (val > 0 && val >= config->getInstrStartFreq() * 1e6 && val <= config->getInstrStopFreq() * 1e6 && \
                    (pmrTable.isEmpty() || (pmrTable.last().centerFrequency != val))) {

                pmr.centerFrequency = val;
                pmr.channelSpacing = list[6].toUInt() * 1e3;
                pmr.comment = list[1];
                pmr.demod = "FM";
                pmrTable.append(pmr);
            }
        }
    }
    xmlfile.close();

}

void PmrTableWdg::loadFile(QString filename)
{
    int ctr = 0;
    bool ok;
    QFile xmlfile(filename);
    if (xmlfile.open(QIODevice::ReadOnly | QFile::Text)) {
        QXmlStreamReader xml(&xmlfile);
        if (xml.readNextStartElement() && xml.name() == "FrequencyStoreList") {
            while (xml.readNextStartElement()) {
                ctr++;
                if (xml.name() == "FrequencyStoreEntry") { // iterate every frequencystore entry
                    Pmr pmr;
                    while (xml.readNextStartElement()) {
                        if (xml.name() == "Type") {
                            QString type = xml.readElementText();
                            pmr.type = type.at(0);
                        }
                        else if (xml.name() == "Comment") {
                            pmr.casperComment = xml.readElementText();
                            pmr.casperComment.append(xml.text().string());
                            pmr.casperComment.replace("aring;", "å");
                            pmr.casperComment.replace("quest;", "?");
                            pmr.casperComment.replace("period;", ".");
                            pmr.casperComment.replace("oslash;", "ø");
                            pmr.casperComment.replace("aelig;", "æ");
                            pmr.casperComment.replace("comma;", ",");
                        }
                        else if (xml.name() == "FFM_Settings") { // iterate this group
                            while (xml.readNextStartElement()) {
                                if (xml.name() == "Frequency") {
                                    pmr.centerFrequency = xml.readElementText().toULong(&ok);
                                    if (!ok) qDebug() << "Error reading frequency from xml file" << xml.readElementText();
                                }
                                else if (xml.name() == "IFBandwidth") {
                                    pmr.ifBandwidth = xml.readElementText().toUInt(&ok);
                                    if (!ok) qDebug() << "Error reading IF BW" << xml.readElementText();
                                }
                                else if (xml.name() == "AnalogDemod") {
                                    pmr.demod = xml.readElementText();
                                }
                                else if (xml.name() == "Detail") { // yet another iteration
                                    while (xml.readNextStartElement()) {
                                        if (xml.name() == "Attenuation") {
                                            if (xml.readElementText() == "ON") pmr.attenuation = true;
                                            else pmr.attenuation = false;
                                        }
                                        else if (xml.name() == "Active") {
                                            if (xml.readElementText() == "true") pmr.active = true;
                                            else pmr.active = false;
                                        }
                                        else if (xml.name() == "SquelchState") {
                                            if (xml.readElementText() == "ON") pmr.squelchState = true;
                                            else pmr.squelchState = false;
                                        }
                                        else if (xml.name() == "SquelchValue") {
                                            pmr.squelchLevel = xml.readElementText().toUInt(&ok);
                                            if (!ok) qDebug() << "Error reading squelch level" << xml.readElementText();
                                        }
                                        else xml.skipCurrentElement();
                                    }
                                }
                                else xml.skipCurrentElement();
                            }
                        }
                        else xml.skipCurrentElement();
                    }
                    for (auto &row : pmrTable) {
                        if (row.centerFrequency == pmr.centerFrequency) {
                            row.ifBandwidth = pmr.ifBandwidth;
                            row.type = pmr.type;
                            row.active = pmr.active;
                            row.casperComment = pmr.casperComment;
                            row.demod = pmr.demod;
                            row.squelchLevel = pmr.squelchLevel;
                            row.squelchState = pmr.squelchState;

                            //qDebug() << "Found it!" << row.centerFrequency;
                            break;
                        }
                    }
                }
                else xml.skipCurrentElement();
            }
        }
        qDebug() << "XML entries" << ctr;
    }
}

bool PmrTableWdg::readXmlEntry(QXmlStreamReader &xml, Pmr &pmr) // next in line should be a frequency entry
{
    bool ok = true;

    qDebug() << xml.columnNumber();
    while (!xml.atEnd() && !xml.name().contains("Type")) xml.readNext();

    if (xml.name().contains("Type")) {
        xml.readNext();
        if (xml.text().size() > 0) pmr.type = xml.text().at(0).unicode();
    }
    else qDebug() << xml.columnNumber();

    while (!xml.atEnd() && !xml.name().contains("Comment")) xml.readNext();
    if (!xml.atEnd()) xml.readNext();
    pmr.casperComment.append(xml.text().string());
    pmr.casperComment.replace("aring;", "å");
    pmr.casperComment.replace("quest;", "?");
    pmr.casperComment.replace("period;", ".");
    pmr.casperComment.replace("oslash;", "ø");
    pmr.casperComment.replace("aelig;", "æ");
    pmr.casperComment.replace("comma;", ",");

    while (!xml.atEnd() && !xml.name().contains("Frequency")) xml.readNext();
    if (!xml.atEnd()) xml.readNext();
    pmr.centerFrequency = xml.text().toULong(&ok);
    if (!ok) return ok;

    while (!xml.atEnd() && !xml.name().contains("IFBandwidth")) xml.readNext();
    if (!xml.atEnd()) xml.readNext();
    pmr.ifBandwidth = xml.text().toInt(&ok);
    if (!ok) return ok;

    while (!xml.atEnd() && !xml.name().contains("AnalogDemod")) xml.readNext();
    if (!xml.atEnd()) xml.readNext();
    pmr.demod.append(xml.text().string());

    while (!xml.atEnd() && !xml.name().contains("Active")) xml.readNext();
    if (!xml.atEnd()) xml.readNext();
    if (xml.text().contains("true")) pmr.active = true;
    else pmr.active = false;

    while (!xml.atEnd() && !xml.name().contains("SquelchState")) xml.readNext();
    if (!xml.atEnd()) xml.readNext();
    if (xml.text().contains("true")) pmr.squelchState = true;
    else pmr.squelchState = false;

    while (!xml.atEnd() && !xml.name().contains("SquelchValue")) xml.readNext();
    if (!xml.atEnd()) xml.readNext();
    pmr.squelchLevel = xml.text().toInt(&ok);

    qDebug() << "xml" << pmr.centerFrequency;
    return ok;
}

void PmrTableWdg::openXmlFile()
{
    QString filename = QFileDialog::getOpenFileName(wdg, tr("Open XML frequency file"), config->getWorkFolder(), tr("XML file (*.xml)"));
    loadFile(filename);
    updTableWdg();
}

void PmrTableWdg::saveTable() // QStringList({"Active", "Frequency", "IF BW", "Type", "Demodulator", "Sqlch lvl", "Sqlch on", "TX time", "Max signal lvl", "Casper comment"}));
{
    QFile tableFile(config->getWorkFolder() + "/table.hauken");
    if (!tableFile.open(QIODevice::WriteOnly | QFile::Text)) {
        qDebug() << "Error saving PMR file, check work folder settings";
    }
    else {
        QTextStream ts(&tableFile);

        for (auto &row : pmrTable) {
            ts << row.active << ";" << row.centerFrequency << ";" << row.ifBandwidth << ";" \
               << row.comment << ";" << row.demod << ";" << row.squelchLevel << ";" \
               << row.squelchState << ";" << row.totalDurationInMilliseconds << ";" \
               << row.maxLevel << ";" << row.casperComment << ";" << row.type << Qt::endl;
        }
    }

}

void PmrTableWdg::loadTable()
{
    pmrTable.clear();

    QFile tableFile(config->getWorkFolder() + "/table.hauken");
    if (!tableFile.open(QIODevice::ReadOnly | QFile::Text)) {
        qDebug() << "First run?";
        fillPmrChannels();
        saveTable();
    }
    else {
        while (!tableFile.atEnd()) {
            Pmr pmr;
            QList<QByteArray> list = tableFile.readLine().split(';');
            unsigned long val = list[1].simplified().replace(" ", "").toULong();
            if (val > 0 && val >= config->getInstrStartFreq() * 1e6 && val <= config->getInstrStopFreq() * 1e6 && \
                    (pmrTable.isEmpty() || (pmrTable.last().centerFrequency != val))) {
                pmr.active = list[0].toUInt();
                pmr.centerFrequency = val;
                pmr.channelSpacing = list[2].toUInt() * 1e3;
                pmr.comment = list[3];
                pmr.demod = list[4];
                pmr.squelchLevel = list[5].toUInt();
                pmr.squelchState = list[6].toUInt();
                pmr.totalDurationInMilliseconds = list[7].toULong();
                pmr.maxLevel = list[8].toInt();
                pmr.casperComment = list[9];
                pmr.type = list[10].at(0);
                pmrTable.append(pmr);
            }
        }
        qDebug() << "Imported" << pmrTable.size() << "entries from" << tableFile.fileName();

    }
    tableFile.close();
}

void PmrTableWdg::createTableWdg()
{
    bool useReds = getPmrUseReds();
    bool useYellows = getPmrUseYellows();
    bool useGreens = getPmrUseGreens();
    bool useWhites = getPmrUseWhites();

    tableWidget->clear();
    tableWidget->setRowCount(pmrTable.size());
    tableWidget->setColumnCount(10);

    tableWidget->setHorizontalHeaderLabels(QStringList({"Active", "Frequency", "IF BW", "Type", "Demodulator", "Sqlch lvl", "Sqlch on", "TX time (s)", "Max signal lvl", "Casper comment"}));
    for (int i=0; i<pmrTable.size(); i++) {

        QTableWidgetItem *item1 = new QTableWidgetItem(QString::number(pmrTable.at(i).centerFrequency / 1e6, 'f', 5));
        //QTableWidgetItem *item2 = new QTableWidgetItem(QString::number(pmrTable.at(i).channelSpacing));
        QTableWidgetItem *item2 = new QTableWidgetItem(QString::number(pmrTable.at(i).ifBandwidth));
        QTableWidgetItem *item3 = new QTableWidgetItem(pmrTable.at(i).comment);
        QTableWidgetItem *item4 = new QTableWidgetItem(pmrTable.at(i).demod);
        QTableWidgetItem *item5 = new QTableWidgetItem(QString::number(pmrTable.at(i).squelchLevel));
        QTableWidgetItem *item6 = new QTableWidgetItem(pmrTable.at(i).casperComment);
        QTableWidgetItem *item7 = new QTableWidgetItem(pmrTable.at(i).totalDurationInMilliseconds / 1e3);
        QTableWidgetItem *item8 = new QTableWidgetItem(pmrTable.at(i).maxLevel / 10);


        QCheckBox *activeBox = new QCheckBox;
        connect(activeBox, &QCheckBox::clicked, this, [this, i](bool state) {
            pmrTable[i].active = state;
        });

        QCheckBox *squelchBox = new QCheckBox;
        connect(squelchBox, &QCheckBox::clicked, this, [this, i](bool state) {
            pmrTable[i].squelchState = state;
        });

        tableWidget->setCellWidget(i, 0, activeBox);
        tableWidget->setItem(i, 1, item1);
        tableWidget->setItem(i, 2, item2);
        tableWidget->setItem(i, 3, item3);
        tableWidget->setItem(i, 4, item4);
        tableWidget->setItem(i, 5, item5);
        tableWidget->setCellWidget(i, 6, squelchBox);
        tableWidget->setItem(i, 7, item6);
        tableWidget->setItem(i, 8, item7);
        tableWidget->setItem(i, 9, item8);

        squelchBox->setChecked(pmrTable.at(i).squelchState);

        if (pmrTable.at(i).type == 'e') tableWidget->item(i, 1)->setBackground(Qt::green);
        else if (pmrTable.at(i).type == 'g') tableWidget->item(i, 1)->setBackground(Qt::yellow);
        else if (pmrTable.at(i).type == 'h') tableWidget->item(i, 1)->setBackground(Qt::red);

        if ((pmrTable.at(i).type == 'e' && useGreens) ||
                (pmrTable.at(i).type == 'g' && useYellows) ||
                (pmrTable.at(i).type == 'h' && useReds) ||
                (pmrTable.at(i).type == 0 && useWhites)) {
            activeBox->setChecked(true);
        }
        else
            activeBox->setChecked(false);


    }
    tableWidget->horizontalHeader()->setSectionResizeMode(QHeaderView::Fixed);
    tableWidget->horizontalHeader()->setStretchLastSection(true);

    tableWidget->verticalHeader()->setVisible(false);
    tableWidget->setColumnWidth(0, 40);
    tableWidget->setColumnWidth(1, 80);
    tableWidget->setColumnWidth(2, 60);
    tableWidget->setColumnWidth(3, 120);
    tableWidget->setColumnWidth(4, 80);
    tableWidget->setColumnWidth(5, 65);
    tableWidget->setColumnWidth(6, 80);
    tableWidget->setColumnWidth(7, 80);
    tableWidget->setColumnWidth(8, 80);
}

void PmrTableWdg::updTableWdg()
{
    bool useGreens = getPmrUseGreens();
    bool useYellows = getPmrUseYellows();
    bool useReds = getPmrUseReds();
    bool useWhites = getPmrUseWhites();


    for (int i=0; i<tableWidget->rowCount(); i++) {
        QCheckBox *activeBox = new QCheckBox;
        connect(activeBox, &QCheckBox::clicked, this, [this, i](bool state) {
            pmrTable[i].active = state;
        });

        if ((pmrTable.at(i).type == 'e' && useGreens) ||
                (pmrTable.at(i).type == 'g' && useYellows) ||
                (pmrTable.at(i).type == 'h' && useReds) ||
                (pmrTable.at(i).type == 0 && useWhites)) {
            activeBox->setChecked(true);
        }
        else {
            activeBox->setChecked(false);
        }
        tableWidget->setCellWidget(i, 0, activeBox);
    }
}

void PmrTableWdg::clearLayout(QLayout *layout)
{
    if (layout == NULL)
        return;
    QLayoutItem *item;
    while((item = layout->takeAt(0))) {
        if (item->layout()) {
            clearLayout(item->layout());
            delete item->layout();
        }
        if (item->widget()) {
            delete item->widget();
        }
        delete item;
    }
}
