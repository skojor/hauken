#ifndef AI_H
#define AI_H

#undef slots
//#include "torch/script.h"
#define slots Q_SLOTS

#include <QObject>
#include <iostream>
#include <memory>
#include <string>
#include <vector>
#include <QDebug>



using namespace std;



class AI
{
public:
    AI();
    QString gnssAI(float data[90][1200]);
};

#endif // AI_H
