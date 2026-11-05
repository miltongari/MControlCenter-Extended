#include "batteryhealth.h"
#include <QtCore/QDir>
#include <QtCore/QFile>
#include <QtCore/QTextStream>

static QString readSysfsLine(const QString &path) {
    QFile file(path);
    if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QTextStream in(&file);
        QString line = in.readLine().trimmed();
        file.close();
        return line;
    }
    return QString();
}

static qint64 readSysfsInt(const QString &path) {
    QString val = readSysfsLine(path);
    if (!val.isEmpty()) {
        bool ok = false;
        qint64 num = val.toLongLong(&ok);
        if (ok) return num;
    }
    return 0;
}

BatteryHealthInfo BatteryHealth::getBatteryHealthInfo() {
    BatteryHealthInfo info;

    QDir powerSupplyDir("/sys/class/power_supply");
    QStringList entries = powerSupplyDir.entryList(QStringList() << "BAT*", QDir::Dirs | QDir::NoDotAndDotDot);
    if (entries.isEmpty()) {
        return info;
    }

    QString batPath = "/sys/class/power_supply/" + entries.first();

    // Check charge_full / charge_full_design (microampere-hours, uAh)
    qint64 chargeFull = readSysfsInt(batPath + "/charge_full");
    qint64 chargeFullDesign = readSysfsInt(batPath + "/charge_full_design");
    qint64 chargeNow = readSysfsInt(batPath + "/charge_now");

    if (chargeFullDesign > 0) {
        info.unit = "mAh";
        info.fullCapacity = chargeFull / 1000;
        info.designCapacity = chargeFullDesign / 1000;
        info.currentCharge = chargeNow / 1000;
        info.healthPercentage = (static_cast<double>(chargeFull) / static_cast<double>(chargeFullDesign)) * 100.0;
        info.available = true;
    } else {
        // Fallback to energy_full / energy_full_design (microwatt-hours, uWh)
        qint64 energyFull = readSysfsInt(batPath + "/energy_full");
        qint64 energyFullDesign = readSysfsInt(batPath + "/energy_full_design");
        qint64 energyNow = readSysfsInt(batPath + "/energy_now");

        if (energyFullDesign > 0) {
            info.unit = "mWh";
            info.fullCapacity = energyFull / 1000;
            info.designCapacity = energyFullDesign / 1000;
            info.currentCharge = energyNow / 1000;
            info.healthPercentage = (static_cast<double>(energyFull) / static_cast<double>(energyFullDesign)) * 100.0;
            info.available = true;
        }
    }

    if (info.healthPercentage > 100.0) {
        info.healthPercentage = 100.0;
    }

    info.cycleCount = static_cast<int>(readSysfsInt(batPath + "/cycle_count"));

    QString mfr = readSysfsLine(batPath + "/manufacturer");
    if (!mfr.isEmpty()) info.manufacturer = mfr;

    QString model = readSysfsLine(batPath + "/model_name");
    if (!model.isEmpty()) info.modelName = model;

    QString tech = readSysfsLine(batPath + "/technology");
    if (!tech.isEmpty()) info.technology = tech;

    return info;
}
