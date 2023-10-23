
#ifndef ESPENAI_H
#define ESPENAI_H


#include <QObject>
//#include <torch/script.h> // https://pytorch.org/cppdocs/installing.html
#include <QString>
#include <QList>

class EspenAI
{
public:
    EspenAI();
    QString GnssAI(QList< QList<float>>);
};

#endif // ESPENAI_H
