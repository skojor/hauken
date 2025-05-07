#ifndef MQTTOPTIONS_H
#define MQTTOPTIONS_H

#include "config.h"
#include "optionsbaseclass.h"
#include <QScrollArea>
#include <QRect>

class MqttOptions : public OptionsBaseClass
{
public:
    MqttOptions(QSharedPointer<Config> c);

public slots:
    void start();
    void saveCurrentSettings();

private slots:
    void updSubs();
    void addSub();
    void setupWindow();

private:
    QList<QLineEdit *> subNames, subTopics;
    QList<QCheckBox *> subIncidentlog;
    QList<QGroupBox *> subGroupBoxes;
    QList<QFormLayout *> subLayouts;
};

#endif // MQTTOPTIONS_H
