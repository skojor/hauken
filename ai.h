#ifndef AI_H
#define AI_H

#undef slots
#include "torch/script.h"
#define slots Q_SLOTS

#include <QString>
//#include <iostream>
//#include <memory>
//#include <string>
#include <vector>
#include <QDebug>
#include <QStringList>
#include <QList>
#include <QElapsedTimer>
#include <QTimer>
#include <QObject>

class AI : public QObject
{
Q_OBJECT

signals:
    void aiResult(QString);

public:
    AI();
    void receiveBuffer(QVector<QVector<float >> buffer);
    QString gnssAI(float traceBuffer[90][1200]);

private:
    QStringList classes;
    torch::jit::script::Module model;
    at::IntArrayRef sizes;
};

#endif // AI_H
