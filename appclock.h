#ifndef APPCLOCK_H
#define APPCLOCK_H

#include <QDateTime>
#include <atomic>

// Shiftable wall clock, lets replayed recordings produce their original timestamps
class AppClock
{
public:
    static QDateTime now() { return QDateTime::currentDateTime().addMSecs(offsetMs.load()); }
    static QDateTime nowUtc() { return QDateTime::currentDateTimeUtc().addMSecs(offsetMs.load()); }
    static qint64 currentMSecsSinceEpoch() { return QDateTime::currentMSecsSinceEpoch() + offsetMs.load(); }
    static void setOffsetMs(qint64 ms) { offsetMs.store(ms); }
    static qint64 offsetMsValue() { return offsetMs.load(); }

private:
    static inline std::atomic<qint64> offsetMs{0};
};

#endif // APPCLOCK_H
