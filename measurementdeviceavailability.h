#ifndef MEASUREMENTDEVICEAVAILABILITY_H
#define MEASUREMENTDEVICEAVAILABILITY_H

#include <QByteArray>

class MeasurementDeviceAvailability
{
public:
    enum class Result {
        Inconclusive,
        Clear,
        Occupied
    };

    static constexpr int channelCount = 10;
    static constexpr int requiredClearSweeps = 2;

    static bool hasCompleteReply(const QByteArray &reply)
    {
        return reply.count('\n') >= channelCount;
    }

    bool recordSweep(Result result)
    {
        if (result != Result::Clear) {
            clearSweepCount = 0;
            return false;
        }

        return ++clearSweepCount >= requiredClearSweeps;
    }

    void reset()
    {
        clearSweepCount = 0;
    }

    int consecutiveClearSweeps() const
    {
        return clearSweepCount;
    }

private:
    int clearSweepCount = 0;
};

#endif // MEASUREMENTDEVICEAVAILABILITY_H