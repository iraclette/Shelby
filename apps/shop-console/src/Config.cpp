#include "Config.h"

#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QProcessEnvironment>
#include <QTextStream>

static QMap<QString, QString> parseDotEnv(const QString &path) {
    QMap<QString, QString> values;
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return values;
    }

    QTextStream in(&file);
    while (!in.atEnd()) {
        const QString line = in.readLine().trimmed();
        if (line.isEmpty() || line.startsWith('#')) continue;

        const int eq = line.indexOf('=');
        if (eq < 0) continue;

        values.insert(line.left(eq).trimmed(), line.mid(eq + 1).trimmed());
    }
    return values;
}

// Common on-disk locations a phone's photo sync client downloads into,
// checked only when PHOTOS_SYNC_DIR isn't set explicitly. First one that
// actually exists wins; if none do, the photo picker just opens wherever
// the OS remembers last.
static QString guessPhotosSyncDir() {
    const QString home = QDir::homePath();
    const QStringList candidates{
        home + "/Pictures/iCloud Photos/Downloads", // iCloud for Windows
        home + "/Pictures/Phone Link",              // Windows Phone Link (Android)
        home + "/Google Drive/Photos",              // Google Drive desktop sync
    };
    for (const QString &candidate : candidates) {
        if (QDir(candidate).exists()) return candidate;
    }
    return {};
}

Config Config::load() {
    const QString dotEnvPath = QDir(QCoreApplication::applicationDirPath()).filePath(".env");
    const QMap<QString, QString> fileValues = parseDotEnv(dotEnvPath);
    const QProcessEnvironment env = QProcessEnvironment::systemEnvironment();

    Config config;
    config.supabaseUrl = fileValues.value("SUPABASE_URL", env.value("SUPABASE_URL"));
    config.supabaseAnonKey = fileValues.value("SUPABASE_ANON_KEY", env.value("SUPABASE_ANON_KEY"));
    config.photosSyncDir = fileValues.value("PHOTOS_SYNC_DIR", env.value("PHOTOS_SYNC_DIR"));
    if (config.photosSyncDir.isEmpty()) config.photosSyncDir = guessPhotosSyncDir();
    return config;
}
