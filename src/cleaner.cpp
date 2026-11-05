#include "cleaner.h"
#include <QtCore/QDir>
#include <QtCore/QDirIterator>
#include <QtCore/QFile>
#include <QtCore/QFileInfo>
#include <QtCore/QProcess>
#include <QtCore/QTextStream>
#include <QtCore/QRegularExpression>

Cleaner::Cleaner(QObject *parent) : QObject(parent) {}

QString Cleaner::formatBytes(qint64 bytes) {
    if (bytes <= 0) return "0 B";
    const double b = static_cast<double>(bytes);
    if (b < 1024.0) return QString::number(bytes) + " B";
    if (b < 1024.0 * 1024.0) return QString::number(b / 1024.0, 'f', 1) + " KB";
    if (b < 1024.0 * 1024.0 * 1024.0) return QString::number(b / (1024.0 * 1024.0), 'f', 1) + " MB";
    return QString::number(b / (1024.0 * 1024.0 * 1024.0), 'f', 2) + " GB";
}

qint64 Cleaner::calculateDirSize(const QString &dirPath) {
    qint64 totalSize = 0;
    QDir dir(dirPath);
    if (!dir.exists()) return 0;

    QDirIterator it(dirPath, QDir::Files | QDir::Hidden | QDir::NoSymLinks, QDirIterator::Subdirectories);
    while (it.hasNext()) {
        it.next();
        totalSize += it.fileInfo().size();
    }
    return totalSize;
}

qint64 Cleaner::cleanDirContents(const QString &dirPath) {
    qint64 freed = 0;
    QDir dir(dirPath);
    if (!dir.exists()) return 0;

    const QFileInfoList entries = dir.entryInfoList(QDir::Files | QDir::Dirs | QDir::NoDotAndDotDot | QDir::Hidden);
    for (const QFileInfo &entry : entries) {
        if (entry.isDir()) {
            freed += calculateDirSize(entry.absoluteFilePath());
            QDir subDir(entry.absoluteFilePath());
            subDir.removeRecursively();
        } else {
            freed += entry.size();
            QFile::remove(entry.absoluteFilePath());
        }
    }
    return freed;
}

QStringList Cleaner::getAppCacheDirs() {
    const QString home = QDir::homePath();
    QStringList dirs;

    dirs << home + "/.cache/google-chrome/Default/Cache";
    dirs << home + "/.cache/google-chrome/Default/Code Cache";
    dirs << home + "/.cache/chromium/Default/Cache";
    dirs << home + "/.cache/chromium/Default/Code Cache";
    dirs << home + "/.cache/mesa_shader_cache";
    dirs << home + "/.cache/pip";
    dirs << home + "/.cache/yarn";
    dirs << home + "/.cache/npm";
    dirs << home + "/.cache/flatpak";

    // Firefox profile caches
    QDir ffDir(home + "/.cache/mozilla/firefox");
    if (ffDir.exists()) {
        const QStringList profiles = ffDir.entryList(QDir::Dirs | QDir::NoDotAndDotDot);
        for (const QString &prof : profiles) {
            dirs << home + "/.cache/mozilla/firefox/" + prof + "/cache2";
        }
    }

    return dirs;
}

CleanerScanReport Cleaner::scan() {
    CleanerScanReport report;
    const QString home = QDir::homePath();

    // 1. Thumbnails
    QString thumbPath = home + "/.cache/thumbnails";
    report.thumbnails.id = "thumbnails";
    report.thumbnails.displayName = "Thumbnails Cache";
    report.thumbnails.bytes = calculateDirSize(thumbPath);
    report.thumbnails.available = QDir(thumbPath).exists();

    // 2. Trash
    QString trashFiles = home + "/.local/share/Trash/files";
    QString trashInfo = home + "/.local/share/Trash/info";
    report.trash.id = "trash";
    report.trash.displayName = "Trash Can";
    report.trash.bytes = calculateDirSize(trashFiles) + calculateDirSize(trashInfo);
    report.trash.available = QDir(trashFiles).exists() || QDir(trashInfo).exists();

    // 3. Application Caches
    report.appCache.id = "appCache";
    report.appCache.displayName = "Application & Browser Cache";
    qint64 appCacheTotal = 0;
    for (const QString &d : getAppCacheDirs()) {
        if (QDir(d).exists()) {
            appCacheTotal += calculateDirSize(d);
        }
    }
    report.appCache.bytes = appCacheTotal;
    report.appCache.available = (appCacheTotal > 0);

    // 4. Package Cache
    report.packageCache.id = "packageCache";
    report.packageCache.displayName = "Package Manager Cache";
    qint64 pkgCacheSize = 0;
    if (QDir("/var/cache/apt/archives").exists()) {
        pkgCacheSize += calculateDirSize("/var/cache/apt/archives");
    }
    if (QDir("/var/cache/pacman/pkg").exists()) {
        pkgCacheSize += calculateDirSize("/var/cache/pacman/pkg");
    }
    if (QDir("/var/cache/dnf").exists()) {
        pkgCacheSize += calculateDirSize("/var/cache/dnf");
    }
    report.packageCache.bytes = pkgCacheSize;
    report.packageCache.available = (pkgCacheSize > 0);

    // 5. Journal logs
    report.journalLogs.id = "journal";
    report.journalLogs.displayName = "Systemd Journal Logs";
    QProcess journalProc;
    journalProc.start("journalctl", QStringList() << "--disk-usage");
    if (journalProc.waitForFinished(1000)) {
        QString out = QString::fromUtf8(journalProc.readAllStandardOutput());
        QRegularExpression re(R"(([0-9]+(?:\.[0-9]+)?)\s*([KMGT]?)(?:B)?)");
        auto match = re.match(out);
        if (match.hasMatch()) {
            double val = match.captured(1).toDouble();
            QString unit = match.captured(2).toUpper();
            if (unit == "T") report.journalLogs.bytes = static_cast<qint64>(val * 1024.0 * 1024.0 * 1024.0 * 1024.0);
            else if (unit == "G") report.journalLogs.bytes = static_cast<qint64>(val * 1024.0 * 1024.0 * 1024.0);
            else if (unit == "M") report.journalLogs.bytes = static_cast<qint64>(val * 1024.0 * 1024.0);
            else if (unit == "K") report.journalLogs.bytes = static_cast<qint64>(val * 1024.0);
            else report.journalLogs.bytes = static_cast<qint64>(val);
            report.journalLogs.available = (report.journalLogs.bytes > 0);
        }
    }

    // 6. RAM Cache (Buffers + Cached from /proc/meminfo)
    report.ramCache.id = "ram";
    report.ramCache.displayName = "Inactive Memory (Page Cache)";
    QFile meminfo("/proc/meminfo");
    if (meminfo.open(QIODevice::ReadOnly)) {
        QByteArray data = meminfo.readAll();
        qint64 buffers = 0;
        qint64 cached = 0;
        const auto lines = QString::fromUtf8(data).split('\n', Qt::SkipEmptyParts);
        for (const QString &line : lines) {
            if (line.startsWith("Buffers:")) {
                buffers = line.split(QRegularExpression("\\s+"), Qt::SkipEmptyParts).value(1).toLongLong() * 1024;
            } else if (line.startsWith("Cached:")) {
                cached = line.split(QRegularExpression("\\s+"), Qt::SkipEmptyParts).value(1).toLongLong() * 1024;
            }
        }
        report.ramCache.bytes = buffers + cached;
        report.ramCache.available = (report.ramCache.bytes > 0);
    }

    return report;
}

qint64 Cleaner::cleanThumbnails() {
    return cleanDirContents(QDir::homePath() + "/.cache/thumbnails");
}

qint64 Cleaner::cleanTrash() {
    const QString home = QDir::homePath();
    qint64 freed = cleanDirContents(home + "/.local/share/Trash/files");
    freed += cleanDirContents(home + "/.local/share/Trash/info");
    return freed;
}

qint64 Cleaner::cleanAppCache() {
    qint64 freed = 0;
    for (const QString &d : getAppCacheDirs()) {
        if (QDir(d).exists()) {
            freed += cleanDirContents(d);
        }
    }
    return freed;
}

QString Cleaner::cleanPackageCache() {
    QString result;
    if (QFile::exists("/usr/bin/apt-get")) {
        QProcess proc;
        proc.start("pkexec", QStringList() << "apt-get" << "clean");
        if (proc.waitForFinished(5000)) {
            result += "APT cache cleaned. ";
        } else {
            result += "APT clean timed out or cancelled. ";
        }
    }

    if (QFile::exists("/usr/bin/flatpak")) {
        QProcess proc;
        proc.start("flatpak", QStringList() << "uninstall" << "--unused" << "-y");
        if (proc.waitForFinished(5000)) {
            result += "Unused Flatpaks removed. ";
        }
    }

    if (QFile::exists("/usr/bin/pacman")) {
        QProcess proc;
        proc.start("pkexec", QStringList() << "pacman" << "-Sc" << "--noconfirm");
        if (proc.waitForFinished(5000)) {
            result += "Pacman cache cleaned. ";
        }
    }

    return result.trimmed();
}

QString Cleaner::cleanJournalLogs() {
    QProcess proc;
    proc.start("journalctl", QStringList() << "--vacuum-size=50M" << "--user");
    if (proc.waitForFinished(3000)) {
        return "User journal vacuumed down to 50M.";
    }
    return "Failed to vacuum journal.";
}

QString Cleaner::dropRamCaches() {
    QProcess proc;
    proc.start("pkexec", QStringList() << "sh" << "-c" << "sync && echo 3 > /proc/sys/vm/drop_caches");
    if (proc.waitForFinished(4000)) {
        return "Inactive RAM page cache released successfully.";
    }
    return "RAM cache drop cancelled or timed out.";
}
