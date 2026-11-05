#ifndef BATTERYHEALTH_H
#define BATTERYHEALTH_H

#include <QtCore/QString>
#include <QtCore/QFileInfo>

struct BatteryHealthInfo {
    bool available = false;
    double healthPercentage = 0.0;
    qint64 currentCharge = 0;
    qint64 fullCapacity = 0;
    qint64 designCapacity = 0;
    QString unit = "mAh";
    int cycleCount = -1;
    QString manufacturer = "Unknown";
    QString modelName = "Unknown";
    QString technology = "Unknown";
};

class BatteryHealth {
public:
    static BatteryHealthInfo getBatteryHealthInfo();
};

#endif // BATTERYHEALTH_H
