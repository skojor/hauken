#include "gnssanalyzer.h"

GnssAnalyzer::GnssAnalyzer(QObject *parent)
    : Config{parent}
{

}

void GnssAnalyzer::getData(GnssData &data)
{

}

void GnssAnalyzer::updSettings()
{
    cnoLimit = getGnssCnoDeviation();
    agcLimit = getGnssAgcDeviation();
    posOffsetLimit = getGnssPosOffset();
    altOffsetLimit = getGnssAltOffset();
    timeOffsetLimit = getGnssTimeOffset();
}
