#ifndef PMRTABLEWDG_H
#define PMRTABLEWDG_H

#include <QWidget>
#include <QDebug>
#include <QDateTime>
#include <QTimer>
#include <QElapsedTimer>
#include <QTextStream>
#include <QFile>
#include <QDir>
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QLineEdit>
#include <QCheckBox>
#include <QLabel>
#include <QDialogButtonBox>
#include <QPushButton>
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QHeaderView>
#include <QXmlStreamReader>
#include <QFileDialog>
#include "config.h"
#include "typedefs.h"

#if QT_VERSION >= QT_VERSION_CHECK(5,14,0)
#define ENDL Qt::endl
#else
#define ENDL endl
#endif

class PmrTableWdg : public Config
{
public:
    explicit PmrTableWdg(QSharedPointer<Config> c);
    ~PmrTableWdg();

    void start();
    void close() {
        wdg->close();
    }
    void saveCurrentSettings();
    void fillPmrChannels();
    void loadFile(QString filename);

    QWidget *wdg = new QWidget;
    QDialogButtonBox *btnBox;
    QList<Pmr> pmrTable;
    QTableWidget *tableWidget = new QTableWidget;
private:
    QSharedPointer<Config> config;
    QPushButton *btnOpenXmlFile = new QPushButton;
    QGridLayout *mainLayout = new QGridLayout;
    QCheckBox *selectGreens = new QCheckBox("Activate all green frequencies");
    QCheckBox *selectYellows = new QCheckBox("Activate all yellow frequencies");
    QCheckBox *selectReds = new QCheckBox("Activate all red frequencies");
    QCheckBox *selectWhites = new QCheckBox("Activate all white frequencies");

private slots:
    bool readXmlEntry(QXmlStreamReader &xml, Pmr &pmr);
    void openXmlFile();
    void saveTable();
    void loadTable();
    void createTableWdg();
    void updTableWdg();
    void clearLayout(QLayout *layout);
};

#endif // PMRTABLEWDG_H
