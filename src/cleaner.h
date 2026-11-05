#ifndef CLEANER_H
#define CLEANER_H

#include <QtCore/QObject>
#include <QtCore/QString>
#include <QtCore/QStringList>

struct CleanItem {
    QString id;
    QString displayName;
    qint64 bytes = 0;
    bool available = false;
};

struct CleanerScanReport {
    CleanItem thumbnails;
    CleanItem trash;
    CleanItem appCache;
    CleanItem packageCache;
    CleanItem journalLogs;
    CleanItem ramCache;

    qint64 totalDiskBytes() const {
        return thumbnails.bytes + trash.bytes + appCache.bytes + packageCache.bytes + journalLogs.bytes;
    }
};

class Cleaner : public QObject {
    Q_OBJECT
public:
    explicit Cleaner(QObject *parent = nullptr);

    static QString formatBytes(qint64 bytes);
    static qint64 calculateDirSize(const QString &dirPath);
    static qint64 cleanDirContents(const QString &dirPath);

    CleanerScanReport scan();

    qint64 cleanThumbnails();
    qint64 cleanTrash();
    qint64 cleanAppCache();
    QString cleanPackageCache();
    QString cleanJournalLogs();
    QString dropRamCaches();

private:
    static QStringList getAppCacheDirs();
};

#endif // CLEANER_H
